#pragma once
#include "TraceTypes.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "typing/TypingSupport.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// The adapters of C59-1 (design §5): the old calls of the app layer on the
// core that the machine interface does not have yet, each until the step that
// takes it behind the seam. They expose nothing for the model, the advances
// and tick count, the three operations, the keys or the display: those go
// through calcu::machine::Machine only (C59-1 AC-17). They are not
// capabilities and never appear in Machine::capabilities().
//
// | Adapter               | Removed in                         |
// |-----------------------|------------------------------------|
// | TICardReaderAdapter   | M-55 (C59-7)                       |
// | TIMemoryAdapter       | M-54 (C59-2, C59-3)                |
// | TIDebugPanelAdapter   | M-56 (C59-4)                       |
//
// The core types still reachable here (CpuFrame, DebugEvent, TIStepBoundary)
// go with their adapters (design D-7). TIDebugTargetAdapter went in M-53
// (LIB-10 59): the debugger reaches the core through the debug target
// (core/TIDebugging.hpp) and the panel's reads moved to TIDebugPanelAdapter
// (adapter L-E3, until M-56). TIPeripheralsAdapter went in M-25
// (C59-6): the PC-100C and the module are peripherals in their places. The
// machine holds no lock: its one caller is the app session's thread
// (ADR 0009; since M-34 on every frontend).

class TIMachine;

/// The card reader until it is a media place (C59-7, M-55).
class TICardReaderAdapter {
public:
    explicit TICardReaderAdapter(TIMachine& m) : m_(m) {}
    /// data/count non-zero: a read card; zero: a blank card for writing.
    void insertCard(const std::uint8_t* data, std::size_t count);
    std::vector<std::uint8_t> cardEject();
    bool isCardPresent() const;
    int cardMode() const;              ///< 0=none, 1=read, 2=write
    bool isWaitingForCard() const;     ///< the ROM polls the card switch with no card in
    bool isInCardWait() const;         ///< the PC is in the card wait of Write / INV Write
    /// The latched module byte address of the running module program (cue card).
    std::uint16_t libExecPC() const;

private:
    TIMachine& m_;
};

/// Program steps, registers and the partition until the keystroke program
/// memory kind and the constant memory (C59-2, C59-3; M-54).
class TIMemoryAdapter {
public:
    explicit TIMemoryAdapter(TIMachine& m) : m_(m) {}
    void writeProgram(const std::uint8_t* keycodes, int count);
    void writeProgramStep(int stepAddr, std::uint8_t keycode);
    std::uint8_t readProgramStep(int stepAddr) const;
    void writeDataRegister(int regNum, const std::uint8_t* nibbles16);
    double readDataReg(int regNum) const;
    int dataRegCount() const;
    int partitionProgramRegs() const;
    void setPartitionProgramRegs(int programRAMregs);
    void copyRAMReg(int reg, std::uint8_t* out16) const;
    void writeRAMReg(int reg, const std::uint8_t* nibbles16);
    int ramRegCount() const;
    void serialiseRAM(std::uint8_t* dst) const;     ///< 120 × 16 bytes
    void deserialiseRAM(const std::uint8_t* src);
    /// n instructions (app/StateApplier.cpp's start-up of 2.3.0), no run control.
    void runInstructions(std::uint32_t n);

private:
    TIMachine& m_;
};

/// The debug panel's calls until C59-4 (M-56). Since M-53 also what the
/// CALCULATOR and CPU tabs, the dumps and the overlay's entry read that the
/// debug target does not give (LIB-10 adapter L-E3).
class TIDebugPanelAdapter {
public:
    explicit TIDebugPanelAdapter(TIMachine& m) : m_(m) {}
    std::uint16_t pc() const;
    CpuFrame snapshotCPU() const;
    TIStepBoundary lastStepBoundary() const;
    /// The newest `max` frames of the ring, oldest first; not consumed.
    std::uint32_t readCpuFrames(CpuFrame* out, std::uint32_t max) const;
    std::uint8_t readROMKeycode(int addr) const;
    /// The CPU tab shows the frame ring (while it shows, while paused, while a start is armed).
    void setPanelTrace(bool on);
    /// The overlay's entry (AsmOverlay.cpp): to the next program byte, at most 50000 weighted cycles.
    std::uint32_t stepUntilNextKeycode(std::uint32_t maxCycles = 50000);
    void beginNextStep();
    static std::string disassemble(std::uint16_t pc, std::uint16_t opcode);
    static double decodeBCD(const std::uint8_t* nibbles16);
    bool loadDebugOverlay(const std::uint16_t* data, std::size_t count);
    void clearDebugOverlay();
    bool runDebugOverlay(std::uint16_t startAddr, std::uint32_t maxSteps, std::uint32_t* outSteps);
    void setDebugLevel(std::uint8_t level);
    std::vector<DebugEvent> drainDebugEvents();

private:
    TIMachine& m_;
};

/// Typing support of the TI machines (LIB-5 design §3.4, D-3): a capability
/// of the generic kind `typing-support`, `TMC0501/typing-support`, owner
/// component TMC0501 (not an adapter). How often the ROM's keyboard-scan
/// instruction found a key pressed; no prompt readiness, no latches.
class TITypingSupport final : public calcu::typing::TypingSupport {
public:
    explicit TITypingSupport(TIMachine& m) : m_(m) {}
    std::optional<std::uint64_t> samples(const calcu::machine::KeyId&) const override;

private:
    TIMachine& m_;
};

/// The full keyboard scan/detect cycle of a model: {lo, hi, extraLo, extraHi}
/// (extra empty when lo > hi), for tools/headless/ and the tests.
std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId&);

// The accessors (design §3.4): nullptr if `m` is not a TI machine.
TICardReaderAdapter* cardReaderOf(calcu::machine::Machine&);
TIMemoryAdapter* memoryOf(calcu::machine::Machine&);
TIDebugPanelAdapter* debugPanelOf(calcu::machine::Machine&);
const TICardReaderAdapter* cardReaderOf(const calcu::machine::Machine&);
const TIMemoryAdapter* memoryOf(const calcu::machine::Machine&);
const TIDebugPanelAdapter* debugPanelOf(const calcu::machine::Machine&);
