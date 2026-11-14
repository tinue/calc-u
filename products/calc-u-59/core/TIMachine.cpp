#include "TIMachine.hpp"
#include "TIKeys.hpp"
#include "TISegments.hpp"

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
    const std::uint32_t r = m_hw.step();
    if (r & TI59Machine::kPolledHardware) m_polledHardware = true;
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
    while (tickCount() - start < atLeast) {
        const std::uint32_t r = stepOne();
        if (!m_observer) continue;
        if (r & TI59Machine::kBreakpointHit) {
            m_stop = TIStop::Breakpoint;
            break;
        }
        const std::uint16_t pc = m_hw.pc();
        if (m_observer->stopAfter(pc, (r & TI59Machine::kStepBoundary) != 0, m_ranges.scanLoop.contains(pc))) {
            m_stop = TIStop::Observer;
            break;
        }
    }
    return tickCount() - start;
}

Ticks TIMachine::tickCount() const { return m_ticksBeforeReset + kTicksPerWeight * m_hw.elapsedTicks(); }

void TIMachine::cpuReset() {
    m_ticksBeforeReset = tickCount();
    m_hw.reset();   // clears the key matrix (AC-8), re-presses the card switch (EC-4)
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
