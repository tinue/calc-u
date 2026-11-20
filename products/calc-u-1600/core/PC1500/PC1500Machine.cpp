#include "PC1500Machine.hpp"

#include "../Connector/CardState.hpp"
#include "../Connector/SoftwareDefinedCard.hpp"
#include "../Debug/Inspect/PC1500Inspector.hpp"
#include "../Machine/SharpCatalogue.hpp"
#include "../Machine/SharpDisplayOutput.hpp"
#include "../Machine/SharpLoaderMemory.hpp"
#include "../Machine/SharpProductKinds.hpp"
#include "PC1500TypedInput.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "typing/TypingSupport.hpp"

using namespace calcu::machine;

namespace {

PC1500Variant modelOf(const MachineDescriptor& d) {
    return d.model.value == sharp::kPC1500A ? PC1500Variant::PC1500A : PC1500Variant::PC1500;
}

const MachineDescriptor& descriptorOf(PC1500Variant model) {
    return *sharp::sharpCatalogue().model(ModelId{model == PC1500Variant::PC1500A ? sharp::kPC1500A : sharp::kPC1500});
}

} // namespace

// The capability objects, each over the core.
struct PC1500Machine::Kinds {
    explicit Kinds(PC1500Machine& m) : memory(m), inspector(m), support(m), trace(m), tape(m), cassette(m.tapeDeck()), clock(m), printerPort(m), call(m), program(m) {}

    sharp::PC1500LoaderMemory memory;
    struct Inspector final : sharp::InspectorViews {
        explicit Inspector(PC1500Machine& m) : m(m) {}
        std::vector<std::string> view(inspect::View v) const override { return inspect::pc1500View(m, v); }
        std::vector<inspect::MenuEntry> menu(bool dumps) const override { return inspect::pc1500Menu(m, dumps); }
        std::vector<std::uint8_t> slotImage(int slot, int* bankCount) const override {
            const ExpansionCard* card = slot == 1 ? m.memory().expansionConnector().attachedCard() : nullptr;
            *bankCount = card ? card->debugBankCount() : -1;
            return card ? card->debugImage() : std::vector<std::uint8_t>{};
        }
        PC1500Machine& m;
    } inspector;
    // Typing support, prompt readiness (no latches, no sample count).
    struct Support final : calcu::typing::TypingSupport {
        explicit Support(PC1500Machine& m) : m(m) {}
        std::optional<bool> promptReady() const override { return pc1500AtBasicPrompt(m); }
        PC1500Machine& m;
    } support;
    struct Trace final : sharp::TraceRecords {
        explicit Trace(PC1500Machine& m) : m(m) {}
        bool begin() override { return m.beginTraceRecords(TRACE_FULL); }
        bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) override { return m.takeTraceRecords(out, budget); }
        void endRecord() override { m.endTraceRecord(); }
        void end() override { m.endTraceRecords(); }
        PC1500Machine& m;
    } trace;
    struct Tape final : sharp::TapeDeckAccess {
        explicit Tape(PC1500Machine& m) : m(m) {}
        TapeDeck::Status status() const override { return m.tapeStatus(); }
        PC1500Machine& m;
    } tape;
    sharp::CassettePlace cassette;
    struct Clock final : calcu::peripherals::RealTimeClock {
        explicit Clock(PC1500Machine& m) : m(m) {}
        calcu::peripherals::ClockFields read() const override {
            const Upd1990ac::Fields f = m.memory().rtc().fields();   // the uPD1990AC keeps no year
            return {std::nullopt, f.month, f.day, f.dow, f.hour, f.minute, f.second};
        }
        void set(const calcu::peripherals::ClockFields& f, int ms) override {
            const auto now = read();
            m.seedClock(f.year.value_or(2000), f.month.value_or(*now.month), f.day.value_or(*now.day),
                        f.hour.value_or(*now.hour), f.minute.value_or(*now.minute), f.second.value_or(*now.second), ms);
        }
        PC1500Machine& m;
    } clock;
    struct PrinterPort final : sharp::Ce158PrinterPort {
        explicit PrinterPort(PC1500Machine& m) : m(m) {}
        std::vector<std::uint8_t> drain() override { return m.drainCE158ParallelOutput(); }
        PC1500Machine& m;
    } printerPort;
    // A routine call: SJP on the LH5801.
    struct Call final : sharp::RoutineCall {
        explicit Call(PC1500Machine& m) : m(m) {}
        bool call(const std::string& target, std::uint16_t address) override {
            if (target != "LH5801") return false;
            m.cpu().callRoutine(address);
            return true;
        }
        PC1500Machine& m;
    } call;
    struct Program final : sharp::ProgramState {
        explicit Program(PC1500Machine& m) : m(m) {}
        std::vector<std::string> busy() const override {
            if (m.toneSounding()) return {"the buzzer"};
            return {};
        }
        bool programEnded() const override { return m.programEnded(); }
        // The tape, or the CE-158's port with nothing on its way.
        bool awaitsMedium() const override {
            return m.tapeAwaited() || (m.ce158ReceiveWaitSeen() && !port->port().incoming());
        }
        PC1500Machine& m;
        SerialPortCapability* port = nullptr;   // the machine's CE-158 peer
    } program;
};

PC1500Machine::PC1500Machine(const MachineDescriptor& d, const VariantId& v)
    : SharpMachine(d, v), m_memory(modelOf(d)), m_cpu(m_memory), m_kinds(std::make_unique<Kinds>(*this)),
      m_debugging(sharp::makeSharpDebugging(*this)) {
    setCE158SerialLink(&m_ce158Port);
    m_kinds->program.port = &m_ce158Port;
}

PC1500Machine::PC1500Machine(PC1500Variant model) : PC1500Machine(descriptorOf(model), *descriptorOf(model).defaultVariant) {}

PC1500Machine::~PC1500Machine() = default;

void PC1500Machine::coreKey(const std::string& id, bool down) {
    if (down) pressKey(id);
    else releaseKey(id);
}

DisplayOutput PC1500Machine::readDisplay() {
    const PC1500Display disp = display();
    std::vector<std::uint8_t> dots(static_cast<std::size_t>(PC1500Display::kCols) * PC1500Display::kRows, 0);
    for (int row = 0; row < PC1500Display::kRows; ++row)
        for (int col = 0; col < PC1500Display::kCols; ++col)
            dots[static_cast<std::size_t>(row) * PC1500Display::kCols + col] = disp.pixel(col, row) ? 1 : 0;
    return sharp::displayOutput(descriptor(), isDisplayOn(), PC1500Display::kCols, PC1500Display::kRows,
                                std::move(dots), disp.statusLine());
}

ExpansionCard* PC1500Machine::moduleIn(const PlaceName& p) {
    return p.value == sharp::kSlot1 ? m_memory.expansionConnector().attachedCard() : nullptr;
}

void PC1500Machine::corePlug(const PeripheralRequest& req, const PeripheralDescriptor& d) {
    const std::string& place = req.place.steps.at(0).value;
    if (place == sharp::kSlot1) {
        if (auto card = sharp::moduleCardOf(req, d)) attachExpansionCard(std::move(card));
        return;
    }
    if (const auto* b = dynamic_cast<const sharp::BusRomRequest*>(req.content.get())) {
        attachBusRom(std::make_unique<BusRomCard>(b->bytes, b->address, b->me1, b->pv, b->pu, b->name));
        return;
    }
    const std::vector<std::uint8_t>* rom = sharp::romImageOf(req, d, 0);
    if (!rom) return;
    if (req.peripheral.value == sharp::kCE150) attachCE150(rom->data(), rom->size());
    else if (req.peripheral.value == sharp::kCE158) attachCE158(rom->data(), rom->size());
}

void PC1500Machine::coreUnplug(const PlaceName& p, const PeripheralId& id) {
    if (p.value == sharp::kSlot1) m_memory.expansionConnector().detach();
    else if (id.value == sharp::kCE150) { tapeEject(); detachCE150(); }   // the cassette goes with it
    else if (id.value == sharp::kCE158) detachCE158();
    else if (id.value == sharp::kBusRom) detachBusRoms();
}

void PC1500Machine::offer(std::vector<Offered>& out, const std::vector<PlugEntry>&) {
    Kinds& k = *m_kinds;
    out.push_back(byComponent("LH5801", k.memory));
    out.push_back(byComponent("LH5801", k.inspector));
    out.push_back(byComponent("LH5801", k.support));
    out.push_back(byComponent("LH5801", k.trace));
    out.push_back(byComponent("LH5801", k.call));
    out.push_back(byComponent("LH5801", k.program));
    out.push_back(byComponent("LH5801", stateCapability()));
    out.push_back(byComponent(sharp::kRtcComponent, k.clock));
    if (const PlugEntry* e = at(sharp::kPrinter); e && e->peripheral.value == sharp::kCE150) {
        // The plotter's paper output, once its card is on the bus.
        if (calcu::paper::PaperOutput* paper = ce150PaperOutput()) out.push_back(byPeripheral(*e, *paper));
        out.push_back(byPeripheral(*e, k.cassette, "cassette-recorder"));   // the CE-150's cassette
        out.push_back(byPeripheral(*e, k.tape));
    }
    if (const PlugEntry* e = at(sharp::kInterface)) {
        out.push_back(byPeripheral(*e, m_ce158Port.port()));
        out.push_back(byPeripheral(*e, k.printerPort));
    }
}


bool PC1500Machine::loadROM(const uint8_t* data, std::size_t size) {
    return m_memory.loadROM(data, size);
}

bool PC1500Machine::loadROMFile(const std::string& path) {
    return m_memory.loadROMFile(path);
}

void PC1500Machine::coreReset() {
    m_frame = m_lastFrame = FrameNotes{};   // key readiness is seen again from here (keyReady())
    m_memory.reset();
    m_memory.keyboard().releaseAll();
    m_cpu.reset();
    m_debugStop.clear();
    // A chip reset re-anchors an attached CE-150 (LH5810 latches cleared,
    // steppers/pen re-homed) but does NOT unplug it or wipe its paper --
    // real ink stays on real paper. Mirrors the CE-1600P, whose card is
    // likewise left attached across PC1600Machine::reset().
    if (m_ce150Card) m_ce150Card->reset();
    m_ce158.reset();
}

void PC1500Machine::coreColdBoot() {
    m_memory.clearRam();
    coreReset();
}

bool PC1500Machine::attachCE150(const uint8_t* rom, size_t romSize) {
    if (romSize != Ce150Card::kRomSize) return false;
    auto card = std::make_unique<Ce150Card>();
    if (!card->loadRom(rom, romSize)) return false;
    detachCE150Locked();
    card->reset();
    card->connectRecorder(&m_tapeDeck);
    m_memory.systemBus().attach(card.get());
    m_ce150Card = std::move(card);
    return true;
}

bool PC1500Machine::tapePlay(const std::string& path, std::string& error) {
    return m_tapeDeck.loadForPlay(path, error);
}

void PC1500Machine::tapeRecord(const std::string& path) {
    m_tapeDeck.armRecord(path);
}

bool PC1500Machine::tapeEject(std::string* error) {
    return m_tapeDeck.eject(error);
}

TapeDeck::Status PC1500Machine::tapeStatus() const {
    return m_tapeDeck.status();
}

std::string PC1500Machine::tapeTakeLastError() {
    return m_tapeDeck.takeLastError();
}

void PC1500Machine::detachCE150() {
    detachCE150Locked();
}

void PC1500Machine::detachCE150Locked() {
    if (!m_ce150Card) return;
    m_memory.systemBus().detach(m_ce150Card.get());
    m_ce150Card.reset();
}

bool PC1500Machine::attachCE158(const uint8_t* rom, size_t romSize) {
    auto card = m_ce158.build(rom, romSize, Ce158Card::kCpuHz);
    if (!card) return false;
    detachCE158Locked();
    m_memory.systemBus().attach(card.get());
    m_ce158.install(std::move(card));
    return true;
}

void PC1500Machine::attachBusRom(std::unique_ptr<BusRomCard> card) {
    m_memory.systemBus().attachFirst(card.get());
    m_busRoms.push_back(std::move(card));
}

void PC1500Machine::detachBusRoms() {
    for (auto& c : m_busRoms) m_memory.systemBus().detach(c.get());
    m_busRoms.clear();
}

void PC1500Machine::detachCE158() {
    detachCE158Locked();
}

void PC1500Machine::detachCE158Locked() {
    if (!m_ce158.attached()) return;
    m_memory.systemBus().detach(m_ce158.card());
    m_ce158.remove();
}

void PC1500Machine::setCE158SerialLink(SerialLink* link) {
    m_ce158.setSerialLink(link);
}

std::vector<uint8_t> PC1500Machine::drainCE158ParallelOutput() {
    return m_ce158.drainParallelOutput();
}

sharp::PlotterPaperOutput* PC1500Machine::ce150PaperOutput() {
    return m_ce150Card ? &m_ce150Card->paperOutput() : nullptr;
}

// Reads and clears the plotter mechanism's event log, which step() and
// runCycles() fill on every pen change.
std::vector<std::string> PC1500Machine::drainCE150Events() {
    if (!m_ce150Card) return {};
    return m_ce150Card->mechanism().drainEvents();
}

void PC1500Machine::seedClock(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    // Sakamoto's day-of-week: 0 = Sunday. Valid for any Gregorian date;
    // keeps this Core self-contained (no host <ctime> needed) so headless
    // tests can seed a fixed date and check the dow nibble too.
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = (month < 3) ? year - 1 : year;
    int dow = (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;

    m_memory.seedClock(year, month, day, hour, minute, second, dow, millisecond);
}

int PC1500Machine::step() {
    noteFetch(m_cpu.pc());
    int c = m_cpu.step();
    if (c == 0 && m_cpu.breakpointsEnabled() && m_cpu.consumeBreakpointHit()) m_debugStop.latch(::DebugStop::Breakpoint, 1);
    if (m_watches && m_watches->hitPending()) m_debugStop.latch(::DebugStop::Watch, 1);
    // PU/PV (SPU/RPU/SPV/RPV) never touch the bus themselves, so pushing
    // their post-instruction state here is sufficient for the next bus
    // access to see it -- see PC1500Memory::updatePUPV()'s own doc comment.
    m_memory.updatePUPV(m_cpu.pu(), m_cpu.pv());
    // Real time keeps passing while halted or powered off, budgeted at
    // kHaltTickCycles like runCycles(). A 0 return otherwise means the CPU
    // is parked on a breakpoint: nothing ran, so no time passes either.
    if (c > 0) advancePeripherals(static_cast<uint32_t>(c));
    else if (m_cpu.halted() || m_cpu.poweredOff()) advancePeripherals(LH5801::kHaltTickCycles);
    maybeDrainTrace();
    return c;
}

void PC1500Machine::advancePeripherals(uint32_t cycles) {
    m_elapsedCycles += cycles;  // every cycle step() and runCycles() consume passes here
    m_frameCycles += cycles;
    if (m_frameCycles >= kPC1500CyclesPerFrame) {
        m_frameCycles -= kPC1500CyclesPerFrame;
        const uint64_t edges = m_memory.piezo().edgeCount();
        m_frame.tone = edges != m_frameEdges;
        m_frameEdges = edges;
        m_lastFrame = m_frame;
        m_frame = FrameNotes{};
    }
    // The uPD1990AC RTC's TP output is defined in real time, not CPU
    // cycles (it has its own independent crystal) -- this Core has no
    // other notion of elapsed time to drive it from, so it advances here,
    // once per instruction, the same way LH5801's own internal timer
    // advances via tickTimer().
    m_memory.advanceRtc(cycles);
    // Per-step hook for an attached CE-150 (no-op today -- the plotter is
    // fully reactive; see Ce150Card::tick()).
    if (m_ce150Card) m_ce150Card->tick(cycles);
    m_memory.advanceSerial(cycles); // SDO -> CMTOUT, the buzzer, and the cassette recorder's time
    m_ce158.tick(cycles); // the UART's own clock keeps running
}

uint64_t PC1500Machine::runCycles(uint64_t maxCycles) {
    uint64_t consumed = 0;
    while (consumed < maxCycles) {
        noteFetch(m_cpu.pc());
        int c = m_cpu.step();
        m_memory.updatePUPV(m_cpu.pu(), m_cpu.pv());
        if (c == 0) {
            if (m_cpu.breakpointsEnabled() && m_cpu.consumeBreakpointHit()) {
                m_debugStop.latch(::DebugStop::Breakpoint, 1);
                break;
            }
            if (m_cpu.halted() || m_cpu.poweredOff()) {
                // step() still ticks the timer once per call while halted
                // (see LH5801::step()'s HLT branch) -- keep polling so a
                // loaded timer can eventually wake it, budgeting each poll
                // at kHaltTickCycles the same way step() paces its own
                // internal tick, rather than treating "halted" as "no
                // forward progress possible" (real hardware's HLT-then-
                // timer-interrupt idle loop, confirmed via the boot smoke
                // test, depends on exactly this still advancing). Genuinely
                // powered off (poweredOff()) budgets the same way -- no
                // internal-timer tick happens there (step() returns 0
                // before even reaching it, matching the real cut CPU
                // clock), but the uPD1990AC RTC chip and the host key
                // queue both sit outside the CPU and must keep advancing
                // regardless: the RTC keeps advancing while powered off.
                advancePeripherals(LH5801::kHaltTickCycles);
                consumed += static_cast<uint64_t>(LH5801::kHaltTickCycles);
                continue;
            }
        }
        advancePeripherals(static_cast<uint32_t>(c));
        consumed += static_cast<uint64_t>(c);
        maybeDrainTrace();
        if (m_watches && m_watches->hitPending()) { // the watched access's instruction has completed
            m_debugStop.latch(::DebugStop::Watch, 1);
            break;
        }
    }
    return consumed;
}

bool PC1500Machine::beginTraceRecords(uint32_t flags) {
    if (m_traceFile) return false;
    m_traceFile = std::make_unique<PC1500TraceFile>();
    m_traceDrainCounter = 0;
    m_cpu.setTraceFlags(flags);
    uint32_t staleLost = 0;
    m_cpu.drainTraceEvents(m_traceDrainBuf, kTraceDrainBufFrames, &staleLost);
    return true;
}

void PC1500Machine::endTraceRecord() {
    if (!m_traceFile) return;
    pumpTraceFile();
    m_traceFile->finish();
    m_cpu.setTraceFlags(TRACE_NONE);
}

void PC1500Machine::endTraceRecords() {
    m_traceFile.reset();
    m_cpu.setTraceFlags(TRACE_NONE);
    m_traceDrainCounter = 0;
}

void PC1500Machine::pumpTraceFile() {
    // drainTraceEvents() takes LH5801's own ring mutex.
    uint32_t lost = 0;
    uint32_t n = m_cpu.drainTraceEvents(m_traceDrainBuf, kTraceDrainBufFrames, &lost);
    if (lost) m_traceFile->writeGap(lost);
    for (uint32_t i = 0; i < n; i++) m_traceFile->writeFrame(m_traceDrainBuf[i]);
}

void PC1500Machine::pressKey(const std::string& name) {
    m_memory.keyboard().setKeyState(PC1500Keyboard::keyFromName(name), true);
}
void PC1500Machine::releaseKey(const std::string& name) {
    m_memory.keyboard().setKeyState(PC1500Keyboard::keyFromName(name), false);
}
void PC1500Machine::setOnKeyPressed(bool pressed) {
    bool risingEdge = m_memory.setOnKeyPressed(pressed);
    // m_memory.setOnKeyPressed() above only latches the pollable IF bit 1
    // (for BREAK, which needs the CPU already running to poll it) -- a
    // halted or powered-off CPU never polls it, so also raise a real wake
    // here. Two distinct states can leave the CPU unresponsive to
    // anything else, and each needs its own wake shape:
    //
    // - Genuinely powered off (LH5801::poweredOff(), set by the OFF
    //   instruction/0xFD 0x4C -- the ROM's real OFF-key and AUTO POWER OFF
    //   handler both execute it): real hardware has physically cut the
    //   CPU clock, so the ON key's BFI pin restores power with a cold
    //   reset, not a HLT resume -- powerOn() (reset() under the hood).
    // - Merely HLTed with the timer stopped (TM==0, so
    //   LH5801::tickTimer()'s usual wake-on-timer-interrupt can never fire
    //   again): a ROM code path can reach this shape without going through
    //   OFF at all -- wakeFromHalt() is the unconditional (non-IE-gated)
    //   counterpart to requestMaskableInterrupt(), matching the ON key's
    //   real wiring straight to the BFI pin/power latch, not the ordinary
    //   maskable-IRQ line, but (unlike powerOn()) just resumes fetch/
    //   execute right where HLT left off -- no reset.
    //
    // Same shape as PC1600Machine::setOnKeyPressed()'s wakeFromHalt() call
    // for its LH5801-family LH5803 (which has no OFF instruction of its
    // own to distinguish). Rising edge only, matching the latch and a
    // real PB7 edge.
    if (risingEdge) {
        if (m_cpu.poweredOff()) m_cpu.powerOn();
        else m_cpu.wakeFromHalt();
    }
}

PC1500Display PC1500Machine::display() const {
    return PC1500Display(m_memory);
}

// ── Machine state ────────────────────────────────────────

template <class Io>
void PC1500Machine::stateIo(Io& io) {
    io.begin("5801");
    m_cpu.stateIo(io);
    io.end();
    m_memory.stateIo(io);
    io.begin("TIME");
    io(m_elapsedCycles);
    io.end();
    io.begin("TAPE");
    m_tapeDeck.stateIo(io);
    io.end();
    sharp::cardStateIo(io, "SLT1", m_memory.expansionConnector().attachedCard());
    sharp::cardStateIo(io, "C150", m_ce150Card.get());
    sharp::cardStateIo(io, "C158", m_ce158.card());
    io.begin("LINE");
    m_ce158Port.stateIo(io);
    io.end();
    io.begin("CASS");
    m_kinds->cassette.stateIo(io);
    io.end();
}

void PC1500Machine::coreSaveState(sharp::StateWriter& w) const { const_cast<PC1500Machine*>(this)->stateIo(w); }   // only reads
void PC1500Machine::coreRestoreState(sharp::StateReader& r) { stateIo(r); }
