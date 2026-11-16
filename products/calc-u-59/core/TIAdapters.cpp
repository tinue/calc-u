#include "TIAdapters.hpp"
#include "TIKeys.hpp"
#include "TIMachine.hpp"
#include <utility>

using calcu::machine::Machine;

std::optional<std::uint64_t> TITypingSupport::samples(const calcu::machine::KeyId& id) const {
    const TIKey* k = tiKeyById(id.value);
    if (!k) return std::nullopt;
    return m_.m_hw.keySamples(kTIKeyLineBits[k->col], k->row);
}


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

// ── TIDebugPanelAdapter (M-56) ────────────────────────────────────────────────
std::uint16_t TIDebugPanelAdapter::pc() const { return m_.m_hw.pc(); }
std::uint16_t TIDebugPanelAdapter::libExecPC() const { return m_.m_hw.libExecPC(); }
CpuFrame TIDebugPanelAdapter::snapshotCPU() const { return m_.m_hw.snapshotCPU(); }
TIStepBoundary TIDebugPanelAdapter::lastStepBoundary() const { return m_.m_hw.lastStepBoundary(); }
std::uint32_t TIDebugPanelAdapter::readCpuFrames(CpuFrame* out, std::uint32_t max) const {
    return m_.m_hw.readCpuFrames(out, max);
}
std::uint8_t TIDebugPanelAdapter::readROMKeycode(int addr) const { return m_.m_hw.readROMKeycode(addr); }
void TIDebugPanelAdapter::setPanelTrace(bool on) {
    m_.m_panelTrace = on;
    m_.applyTraceFlags();
}
std::uint32_t TIDebugPanelAdapter::stepUntilNextKeycode(std::uint32_t maxCycles) {
    std::uint32_t done = 0;
    while (done < maxCycles) {
        const std::uint32_t r = m_.stepOne();
        done += r & TI59Machine::kWeightMask;   // weight: 1 active, 4 IDLE
        if (r & TI59Machine::kStepBoundary) break;
    }
    return done;
}
void TIDebugPanelAdapter::beginNextStep() { m_.m_hw.beginNextStep(); }
std::string TIDebugPanelAdapter::disassemble(std::uint16_t pc, std::uint16_t op) { return TI59Machine::disassemble(pc, op); }
double TIDebugPanelAdapter::decodeBCD(const std::uint8_t* n) { return TI59Machine::decodeBCD(n); }
bool TIDebugPanelAdapter::loadDebugOverlay(const std::uint16_t* data, std::size_t count) {
    return m_.m_hw.loadDebugOverlay(data, count);
}
void TIDebugPanelAdapter::clearDebugOverlay() { m_.m_hw.clearDebugOverlay(); }
bool TIDebugPanelAdapter::runDebugOverlay(std::uint16_t startAddr, std::uint32_t maxSteps, std::uint32_t* outSteps) {
    return m_.m_hw.runDebugOverlay(startAddr, maxSteps, outSteps);
}
void TIDebugPanelAdapter::setDebugLevel(std::uint8_t level) { m_.m_hw.setDebugLevel(level); }
std::vector<DebugEvent> TIDebugPanelAdapter::drainDebugEvents() { return m_.m_hw.drainDebugEvents(); }

// ── The idle/key-detect range ────────────────────────────────────────────────
std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId& model) {
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
TIMemoryAdapter* memoryOf(Machine& m) { auto* t = ti(m); return t ? &t->m_memory : nullptr; }
TIDebugPanelAdapter* debugPanelOf(Machine& m) { auto* t = ti(m); return t ? &t->m_debugPanel : nullptr; }
const TIMemoryAdapter* memoryOf(const Machine& m) { return memoryOf(const_cast<Machine&>(m)); }
const TIDebugPanelAdapter* debugPanelOf(const Machine& m) { return debugPanelOf(const_cast<Machine&>(m)); }
