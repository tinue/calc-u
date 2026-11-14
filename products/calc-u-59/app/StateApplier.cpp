#include "StateApplier.hpp"
#include <algorithm>
#include <vector>

namespace cu59 {

namespace {

/// The ROM's power-on routine (master clear, display init) reaches idle well
/// within this; skipping it leaves SCOM uninitialised, which confuses the AOS
/// stack and display driver once program/data are written.
constexpr uint32_t kStartupSteps = 300'000;

/// No run control cuts this short: the routine's RAM clear on the TI-59/58
/// would then still be ahead, and take what is written into the RAM next.
void runStartup(calcu::machine::Machine& m) { memoryOf(m)->runInstructions(kStartupSteps); }

/// Program steps zero-padded to `steps` (at least up to the last listed step).
/// A full load pads to the model's physical capacity — not the PARTITION
/// boundary: some listings deliberately load registers as programs and
/// programs as registers (examples/calendar/calendar-05-vanderburgh.ti59), so PROGRAM:
/// and REGISTERS: are trusted as written.
void writeProgram(calcu::machine::Machine& m, const std::vector<ProgramStep>& programSteps, Model model, int steps) {
    int physicalMaxStep = info(model).maxStep();
    for (const auto& s : programSteps)
        if (s.stepAddr >= steps && s.stepAddr <= physicalMaxStep) steps = s.stepAddr + 1;
    std::vector<uint8_t> program(static_cast<size_t>(std::min(steps, physicalMaxStep + 1)), 0);
    for (const auto& s : programSteps)
        if (s.stepAddr >= 0 && static_cast<size_t>(s.stepAddr) < program.size())
            program[static_cast<size_t>(s.stepAddr)] = s.keycode;
    memoryOf(m)->writeProgram(program.data(), static_cast<int>(program.size()));
}

/// Extra (H) registers go straight to raw RAM 60–63; branch on isHidden, not
/// the number's range — normal TI-59 registers go up to 99.
void writeRegisters(calcu::machine::Machine& m, const std::vector<RegisterValue>& registers) {
    for (const auto& g : registers) {
        if (g.isHidden) memoryOf(m)->writeRAMReg(kExtraRegisterBase + g.regNum, g.nibbles.data());
        else memoryOf(m)->writeDataRegister(g.regNum, g.nibbles.data());
    }
}

/// SCOM holds program registers = (max step + 1) / 8.
void setPartition(calcu::machine::Machine& m, int partitionMaxStep) { memoryOf(m)->setPartitionProgramRegs((partitionMaxStep + 1) / 8); }

} // namespace

std::optional<std::string> stateFilePartitionError(const LoadStateResult& r, Model model) {
    const ModelInfo& mi = info(model);
    if (r.partitionWasExplicit && r.partitionMaxStep > mi.maxStep()) {
        return "State file partition (" + std::to_string(r.partitionMaxStep) + ") exceeds " +
               mi.displayName + " maximum (" + std::to_string(mi.maxStep()) + ") — load aborted.";
    }
    return std::nullopt;
}

void applyState(calcu::machine::Machine& m, const LoadStateResult& r, Model model) {
    const ModelInfo& mi = info(model);
    int partitionMaxStep = (!mi.hasLargeMemory && !r.partitionWasExplicit) ? 239 : r.partitionMaxStep;

    m.powerCycle();   // 2.3.0's core reset; the start-up runs here, not at key readiness
    runStartup(m);
    setPartition(m, partitionMaxStep);

    // Clear RAM, but keep the TI-58C's extra registers (raw RAM 60–63,
    // E000–E003 / H00–H03): RAM[60] holds SCOM reconstruction data and
    // clearing it triggers the ROM's memory-clear routine. See
    // reference/CoreArchitecture.md § "TI-58C Extra (Constant Memory) Registers".
    const uint8_t zero[16] = {};
    int clearUpTo = mi.hasConstantMemory ? kExtraRegisterBase : 120;
    for (int reg = 0; reg < clearUpTo; ++reg) memoryOf(m)->writeRAMReg(reg, zero);

    writeProgram(m, r.programSteps, model, mi.maxStep() + 1);
    writeRegisters(m, r.registers);
}

// A reload is the user keying in changes: the listed steps and registers
// change, nothing else (a card read, which replaces a whole bank, is CARD:).
void applyReload(calcu::machine::Machine& m, const ReloadStage& stage, Model model) {
    if (stage.partitionMaxStep) setPartition(m, *stage.partitionMaxStep);
    int physicalMaxStep = info(model).maxStep();
    for (const auto& s : stage.programSteps)
        if (s.stepAddr <= physicalMaxStep) memoryOf(m)->writeProgramStep(s.stepAddr, s.keycode);
    writeRegisters(m, stage.registers);
}

void powerOn(calcu::machine::Machine& m, Model model, const std::vector<uint8_t>* savedRam) {
    if (!info(model).hasConstantMemory) return;
    runStartup(m);
    if (savedRam && savedRam->size() >= 120 * 16) memoryOf(m)->deserialiseRAM(savedRam->data());
}

} // namespace cu59
