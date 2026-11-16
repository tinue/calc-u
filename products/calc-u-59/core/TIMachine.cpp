#include "TIMachine.hpp"
#include "ModuleFile.hpp"
#include "TIKeys.hpp"
#include "TISegments.hpp"
#include "peripherals/Controls.hpp"
#include <algorithm>

using namespace calcu::machine;

namespace {
constexpr Ticks kTicksPerWeight = 32;   // one instruction: ÷ 2 two-phase ÷ 16 digit-serial
constexpr size_t kRamBytes = 120 * 16;
} // namespace

TIMachine::TIMachine(const MachineDescriptor& d, MachineVariant v, const TIRoms& roms,
                     std::shared_ptr<const TIConstantMemory> content)
    : m_descriptor(d), m_variant(v), m_ranges(romRanges(v)), m_hw(v) {
    m_hw.loadROM(roms.words.data(), roms.words.size());
    m_hw.loadConstants(roms.constants.data(), roms.constants.size());
    const std::vector<std::uint8_t> zero(kRamBytes, 0);
    m_hw.deserialiseRAM(content ? content->ram.data() : zero.data());
    m_hw.reset();
}

std::uint32_t TIMachine::stepOne() {
    m_heatmap.count(m_hw.pc());   // C59-4 AC-8: once per instruction, while on
    const std::uint32_t r = m_hw.step();
    ++m_retired;
    if (m_restore && m_ranges.scanLoop.contains(m_hw.pc())) {
        // AC-19: the ROM's start-up is over; the RAM and the partition go back
        // before key readiness is reported (D-5).
        m_hw.deserialiseRAM(m_restore->ram.data());
        m_hw.setPartitionProgramRegs(m_restore->partitionProgramRegs);
        m_restore.reset();
    }
    return r;
}

Ticks TIMachine::advance(Ticks atLeast) {
    const Ticks start = tickCount();
    if (!m_debugging.active()) {   // the fast path: no check, no observer (golden runs, a running app)
        while (tickCount() - start < atLeast) stepOne();
        return tickCount() - start;
    }
    // LIB-10 §3.2, §12.4: a breakpoint stops before the instruction at its
    // address (passed once after a pause there, AC-10); the observer is asked
    // after every instruction with the marks of timarks::kMark....
    const Checks& c = m_debugging.m_checks;
    const auto& bps = m_debugging.m_target.breakpoints();
    const bool runOn = c.observer && m_debugging.m_runOn && m_debugging.m_runOn();
    constexpr Ticks kRunOnAtMost = Ticks{50'000} * 4 * kTicksPerWeight;   // TILevelRules lands it before
    while (tickCount() - start < atLeast || (runOn && tickCount() - start < atLeast + kRunOnAtMost)) {
        const std::uint16_t at = m_hw.pc();
        if (c.breakpoints && !m_debugging.m_passOnce && std::binary_search(bps.begin(), bps.end(), at)) {
            DebugStop stop;
            stop.kind = DebugStop::Breakpoint;
            stop.address = at;
            m_debugging.m_stop = stop;
            break;
        }
        m_debugging.m_passOnce = false;
        const std::uint32_t r = stepOne();
        if (!c.observer) continue;
        const std::uint16_t next = m_hw.pc();
        InstructionEvent ev;
        ev.address = at;
        ev.nextAddress = next;
        ev.marks = ((r & TI59Machine::kStepBoundary) ? timarks::kMarkBoundary : 0u) |
                   (m_ranges.scanLoop.contains(next) ? timarks::kMarkScanLoop : 0u) |
                   ((r & TI59Machine::kWeightMask) << timarks::kMarkWeightShift);
        if (c.observer->stopAfter(ev)) {
            m_hw.beginNextStep();   // §12.4: the next step's state is what is read while paused
            m_debugging.m_stop = DebugStop{};
            break;
        }
    }
    return tickCount() - start;
}

void TIMachine::applyTraceFlags() {
    const std::uint32_t want = (m_panelTrace || m_recordTrace || m_checksTrace) ? (TRACE_PC | TRACE_REGS_FULL) : TRACE_NONE;
    if (m_hw.traceFlags() != want) m_hw.setTraceFlags(want);
}

Ticks TIMachine::tickCount() const { return m_ticksBeforeReset + kTicksPerWeight * m_hw.elapsedTicks(); }

void TIMachine::cpuReset() {
    m_ticksBeforeReset = tickCount();
    m_asmOverlay.m_running = false;   // C59-4 AC-18: a Reset ends the overlay's run
    m_hw.reset();   // clears the key matrix (AC-8), re-presses the card switch (EC-4)
    if (m_printer) m_printer->operation();
    if (m_module) m_module->operation();
}

// ── Places (C59-6) ───────────────────────────────────────────────────────────

namespace {
const PlacePath kPrinterPath{{kTIPrinterPlace}};
const PlacePath kSlotPath{{kTIModulePlace}};
const CapabilityId kControlsId{"printer/controls"};
const CapabilityId kPaperId{"printer/paper-output"};
const CapabilityId kTypingId{"TMC0501/typing-support"};
const CapabilityId kTraceRecordsId{"TMC0501/c59.trace-records"};
const CapabilityId kProgramMemoryId{"RAM/c59.program-memory"};
const CapabilityId kConstantMemoryId{"RAM/c59.constant-memory"};
const CapabilityId kAsmOverlayId{"TMC0501/c59.asm-overlay"};
const CapabilityId kHeatmapId{"TMC0501/c59.rom-heatmap"};
const CapabilityId kCalculatorStateId{"TMC0501/c59.calculator-state"};
const CapabilityId kMachineLogId{"TMC0501/c59.machine-log"};
} // namespace

void TIMachine::rewire() {
    const auto d0 = static_cast<std::uint8_t>(hasConstantMemory(m_variant) ? 10 : 0);
    m_hw.wire(TiConnectors{m_printer.get(), d0, m_module.get()});
}

void TIMachine::plug(PeripheralRequest req, const PeripheralDescriptor& d, TickRate) {
    if (req.place == kPrinterPath) {
        m_printer = std::make_unique<PC100C>(req.switches);
        m_printer->recordCodeLines(m_recordCodeLines);
    } else {
        // The module file as bundled (LIB-12 C-1): the attach checked that it
        // is there (rule 5), the lookup that it reads (LIB-12 AC-6).
        std::vector<std::uint8_t> rom;
        for (const auto& image : req.roms) {
            std::string error;
            TIModuleFile f =
                readModuleFile({reinterpret_cast<const char*>(image.bytes.data()), image.bytes.size()}, error);
            if (error.empty()) rom = std::move(f.rom);
        }
        m_module = std::make_unique<SolidStateRomModule>(rom.data(), rom.size());
        m_moduleId = d.id;
    }
    rewire();
}

void TIMachine::unplug(const PlacePath& place, const PeripheralId&) {
    if (place == kPrinterPath) {
        if (m_printer) m_printer->endLineHere();   // LIB-8 EC-7: the rows printed go onto the paper first
        m_printer.reset();
    } else {
        m_module.reset();
        m_moduleId = {};
    }
    rewire();
}

Plugged TIMachine::plugged() const {
    std::vector<PlugEntry> slot, printer;
    if (m_module) slot.push_back({kSlotPath, m_moduleId, std::nullopt});
    if (m_printer) printer.push_back({kPrinterPath, kPC100C, std::nullopt});
    Plugged p;
    p.byPlace = slot;
    p.byPlace.insert(p.byPlace.end(), printer.begin(), printer.end());
    p.byConnector = {{PlacePath{}, ConnectorId{"module-slot"}, slot},
                     {PlacePath{}, ConnectorId{"printer-connector"}, printer}};
    return p;
}

std::vector<CapabilityEntry> TIMachine::capabilities() const {
    const ComponentOwner cpu{ComponentId{"TMC0501"}};
    std::vector<CapabilityEntry> out{{calcu::typing::TypingSupport::kKind, kTypingId, cpu},
                                     {TraceRecords::kKind, kTraceRecordsId, cpu},
                                     {cu59::AsmOverlay::kKind, kAsmOverlayId, cpu},
                                     {cu59::RomHeatmapCounter::kKind, kHeatmapId, cpu},
                                     {cu59::CalculatorState::kKind, kCalculatorStateId, cpu},
                                     {cu59::MachineLog::kKind, kMachineLogId, cpu}};
    const ComponentOwner ram{ComponentId{"RAM"}};
    out.push_back({cu59::ProgramMemory::kKind, kProgramMemoryId, ram});
    if (hasConstantMemory(m_variant)) out.push_back({cu59::ConstantMemory::kKind, kConstantMemoryId, ram});
    if (!m_printer) return out;
    const PeripheralOwner owner{kPC100C, kPrinterPath};
    out.push_back({calcu::peripherals::Controls::kKind, kControlsId, owner});
    out.push_back({calcu::paper::PaperOutput::kKind, kPaperId, owner});
    return out;
}

Capability* TIMachine::capability(const CapabilityId& id) {
    if (id == kTypingId) return &m_typing;
    if (id == kTraceRecordsId) return &m_traceRecords;
    if (id == kAsmOverlayId) return &m_asmOverlay;
    if (id == kHeatmapId) return &m_heatmap;
    if (id == kCalculatorStateId) return &m_calculatorState;
    if (id == kMachineLogId) return &m_machineLog;
    if (id == kProgramMemoryId) return &m_programMemory;
    if (id == kConstantMemoryId) return hasConstantMemory(m_variant) ? &m_constantMemory : nullptr;
    if (!m_printer) return nullptr;
    if (id == kControlsId) return &m_printer->controls();
    if (id == kPaperId) return static_cast<calcu::paper::PaperOutput*>(m_printer.get());
    return nullptr;
}

void TIMachine::reset() {
    if (hasConstantMemory(m_variant)) {   // the TI-58C: the same as a power cycle
        powerCycle();
        return;
    }
    // The ROM's start-up clears the RAM and sets the partition up anew: keep
    // both (a pending restore stays the one to put back).
    if (!m_restore) {
        SavedMemory saved{std::vector<std::uint8_t>(kRamBytes), m_hw.partitionProgramRegs()};
        m_hw.serialiseRAM(saved.ram.data());
        m_restore = std::move(saved);
    }
    cpuReset();
}

void TIMachine::powerCycle() {
    // TI-59/58: the memory is left to the start-up, which clears it. TI-58C:
    // the constant memory stays.
    m_restore.reset();
    cpuReset();
}

void TIMachine::coldBoot() {
    m_restore.reset();
    m_asmOverlay.clear();   // C59-4 AC-26: a Cold boot removes the overlay
    const std::vector<std::uint8_t> zero(kRamBytes, 0);
    m_hw.deserialiseRAM(zero.data());
    cpuReset();
}

bool TIMachine::isKeyReady() const { return !m_restore && m_ranges.scanLoop.contains(m_hw.pc()); }

KeyCall TIMachine::keyDown(const KeyId& id) {
    const TIKey* k = tiKeyById(id.value);
    if (!k) return KeyCall::UnknownKey;
    m_hw.pressKey(kTIKeyLineBits[k->col], k->row);
    return KeyCall::Done;
}

KeyCall TIMachine::keyUp(const KeyId& id) {
    const TIKey* k = tiKeyById(id.value);
    if (!k) return KeyCall::UnknownKey;
    m_hw.releaseKey(kTIKeyLineBits[k->col], k->row);
    return KeyCall::Done;
}

DisplayOutput TIMachine::readDisplay() { return toDisplayOutput(m_hw.takeDisplay(), m_descriptor.display.indicators[0].name); }
