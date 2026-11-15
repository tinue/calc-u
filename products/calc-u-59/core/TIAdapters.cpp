#include "TIAdapters.hpp"
#include "TIMachine.hpp"
#include <utility>

using calcu::machine::Machine;

// ── TICardReaderAdapter (M-55) ────────────────────────────────────────────────
void TICardReaderAdapter::insertCard(const std::uint8_t* data, std::size_t count) { m_.m_hw.insertCard(data, count); }
std::vector<std::uint8_t> TICardReaderAdapter::cardEject() { return m_.m_hw.cardEject(); }
bool TICardReaderAdapter::isCardPresent() const { return m_.m_hw.isCardPresent(); }
int TICardReaderAdapter::cardMode() const { return m_.m_hw.cardMode(); }
bool TICardReaderAdapter::isWaitingForCard() const { return m_.m_hw.isWaitingForCard(); }
bool TICardReaderAdapter::isInCardWait() const { return m_.m_ranges.cardWait.contains(m_.m_hw.pc()); }
std::uint16_t TICardReaderAdapter::libExecPC() const { return m_.m_hw.libExecPC(); }

// ── TIMemoryAdapter (M-54) ────────────────────────────────────────────────────
void TIMemoryAdapter::writeProgram(const std::uint8_t* k, int count) { m_.m_hw.writeProgram(k, count); }
void TIMemoryAdapter::writeProgramStep(int stepAddr, std::uint8_t k) { m_.m_hw.writeProgramStep(stepAddr, k); }
std::uint8_t TIMemoryAdapter::readProgramStep(int stepAddr) const { return m_.m_hw.readProgramStep(stepAddr); }
void TIMemoryAdapter::writeDataRegister(int regNum, const std::uint8_t* n) { m_.m_hw.writeDataRegister(regNum, n); }
double TIMemoryAdapter::readDataReg(int regNum) const { return m_.m_hw.readDataReg(regNum); }
int TIMemoryAdapter::dataRegCount() const { return m_.m_hw.dataRegCount(); }
int TIMemoryAdapter::partitionProgramRegs() const { return m_.m_hw.partitionProgramRegs(); }
void TIMemoryAdapter::setPartitionProgramRegs(int regs) { m_.m_hw.setPartitionProgramRegs(regs); }
void TIMemoryAdapter::copyRAMReg(int reg, std::uint8_t* out16) const { m_.m_hw.copyRAMReg(reg, out16); }
void TIMemoryAdapter::writeRAMReg(int reg, const std::uint8_t* n) { m_.m_hw.writeRAMReg(reg, n); }
int TIMemoryAdapter::ramRegCount() const { return m_.m_hw.ramRegCount(); }
void TIMemoryAdapter::serialiseRAM(std::uint8_t* dst) const { m_.m_hw.serialiseRAM(dst); }
void TIMemoryAdapter::deserialiseRAM(const std::uint8_t* src) { m_.m_hw.deserialiseRAM(src); }
void TIMemoryAdapter::runInstructions(std::uint32_t n) {
    for (std::uint32_t i = 0; i < n; ++i) m_.stepOne();
}

// ── TIDebugTargetAdapter (M-53) ───────────────────────────────────────────────
std::uint32_t TIDebugTargetAdapter::step() { return m_.stepOne(); }
void TIDebugTargetAdapter::stepInstruction() {
    const std::uint16_t prior = m_.m_hw.pc();
    m_.stepOne();
    // HOLD (WAIT Dn): the PC didn't advance — step until it does.
    for (int limit = 32; m_.m_hw.pc() == prior && limit > 0; --limit) m_.stepOne();
}
std::uint32_t TIDebugTargetAdapter::stepUntilNextKeycode(std::uint32_t maxCycles) {
    std::uint32_t done = 0;
    while (done < maxCycles) {
        const std::uint32_t r = m_.stepOne();
        done += r & TI59Machine::kWeightMask;   // weight: 1 active, 4 IDLE
        if (r & (TI59Machine::kBreakpointHit | TI59Machine::kStepBoundary)) break;
    }
    return done;
}
void TIDebugTargetAdapter::beginNextStep() { m_.m_hw.beginNextStep(); }
CpuFrame TIDebugTargetAdapter::snapshotCPU() const { return m_.m_hw.snapshotCPU(); }
TIStepBoundary TIDebugTargetAdapter::lastStepBoundary() const { return m_.m_hw.lastStepBoundary(); }
std::uint16_t TIDebugTargetAdapter::pc() const { return m_.m_hw.pc(); }
void TIDebugTargetAdapter::setTraceFlags(std::uint32_t f) { m_.m_hw.setTraceFlags(f); }
std::uint32_t TIDebugTargetAdapter::traceFlags() const { return m_.m_hw.traceFlags(); }
std::uint32_t TIDebugTargetAdapter::drainCpuFrames(CpuFrame* out, std::uint32_t max, std::uint32_t* outLost) {
    return m_.m_hw.drainCpuFrames(out, max, outLost);
}
std::uint32_t TIDebugTargetAdapter::readCpuFrames(CpuFrame* out, std::uint32_t max) const {
    return m_.m_hw.readCpuFrames(out, max);
}
void TIDebugTargetAdapter::addBreakpoint(std::uint16_t pc) { m_.m_hw.addBreakpoint(pc); }
void TIDebugTargetAdapter::removeBreakpoint(std::uint16_t pc) { m_.m_hw.removeBreakpoint(pc); }
void TIDebugTargetAdapter::clearBreakpoints() { m_.m_hw.clearBreakpoints(); }
void TIDebugTargetAdapter::setRunControl(TIStepObserver* observer) { m_.m_observer = observer; }
TIStop TIDebugTargetAdapter::takeStop() { return std::exchange(m_.m_stop, TIStop::None); }
std::uint8_t TIDebugTargetAdapter::readROMKeycode(int addr) const { return m_.m_hw.readROMKeycode(addr); }
std::string TIDebugTargetAdapter::disassemble(std::uint16_t pc, std::uint16_t op) { return TI59Machine::disassemble(pc, op); }
double TIDebugTargetAdapter::decodeBCD(const std::uint8_t* n) { return TI59Machine::decodeBCD(n); }

// ── TIDebugPanelAdapter (M-56) ────────────────────────────────────────────────
bool TIDebugPanelAdapter::loadDebugOverlay(const std::uint16_t* data, std::size_t count) {
    return m_.m_hw.loadDebugOverlay(data, count);
}
void TIDebugPanelAdapter::clearDebugOverlay() { m_.m_hw.clearDebugOverlay(); }
bool TIDebugPanelAdapter::runDebugOverlay(std::uint16_t startAddr, std::uint32_t maxSteps, std::uint32_t* outSteps) {
    return m_.m_hw.runDebugOverlay(startAddr, maxSteps, outSteps);
}
void TIDebugPanelAdapter::setDebugLevel(std::uint8_t level) { m_.m_hw.setDebugLevel(level); }
std::vector<DebugEvent> TIDebugPanelAdapter::drainDebugEvents() { return m_.m_hw.drainDebugEvents(); }

// ── TIPacingHints (M-33) ──────────────────────────────────────────────────────
bool TIPacingHints::takePolledHardware() { return std::exchange(m_.m_polledHardware, false); }
bool TIPacingHints::inIdleAndKeyDetect() const { return m_.m_ranges.idleAndKeyDetect.contains(m_.m_hw.pc()); }
std::array<std::uint16_t, 4> TIPacingHints::idleAndKeyDetectRange(const calcu::machine::ModelId& model) {
    const MachineVariant v = model.value == "TI-58" ? MachineVariant::TI58
                             : model.value == "TI-58C" ? MachineVariant::TI58C
                                                       : MachineVariant::TI59;
    const PcRange r = romRanges(v).idleAndKeyDetect;
    return {r.lo, r.hi, r.extraLo, r.extraHi};
}

// ── Accessors (design §3.4): the family, then a static_cast (no RTTI) ────────
namespace {
TIMachine* ti(Machine& m) { return m.descriptor().family == kTIFamily ? static_cast<TIMachine*>(&m) : nullptr; }
} // namespace
TICardReaderAdapter* cardReaderOf(Machine& m) { auto* t = ti(m); return t ? &t->m_cardReader : nullptr; }
TIMemoryAdapter* memoryOf(Machine& m) { auto* t = ti(m); return t ? &t->m_memory : nullptr; }
TIDebugTargetAdapter* debugTargetOf(Machine& m) { auto* t = ti(m); return t ? &t->m_debugTarget : nullptr; }
TIDebugPanelAdapter* debugPanelOf(Machine& m) { auto* t = ti(m); return t ? &t->m_debugPanel : nullptr; }
TIPacingHints* pacingHintsOf(Machine& m) { auto* t = ti(m); return t ? &t->m_pacing : nullptr; }
const TICardReaderAdapter* cardReaderOf(const Machine& m) { return cardReaderOf(const_cast<Machine&>(m)); }
const TIMemoryAdapter* memoryOf(const Machine& m) { return memoryOf(const_cast<Machine&>(m)); }
const TIDebugTargetAdapter* debugTargetOf(const Machine& m) { return debugTargetOf(const_cast<Machine&>(m)); }
