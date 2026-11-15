#pragma once
#include "TraceTypes.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
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
// | TIDebugTargetAdapter  | M-53 (LIB-10 59)                   |
// | TIDebugPanelAdapter   | M-56 (C59-4)                       |
// | TIPacingHints         | M-33 (LIB-17 59)                   |
//
// The core types still reachable here (CpuFrame, DebugEvent, TIStepBoundary)
// go with their adapters (design D-7). TIPeripheralsAdapter went in M-25
// (C59-6): the PC-100C and the module are peripherals in their places. The
// machine holds no lock: the caller serialises (app/MachineGuard.hpp until
// M-33/M-34).

class TIMachine;

/// The run-control hook of TIDebugTargetAdapter::setRunControl: asked after
/// every instruction with the PC of the next one, whether that is a step
/// boundary and whether it is in the keyboard scan loop; true stops the advance.
struct TIStepObserver {
    virtual ~TIStepObserver() = default;
    virtual bool stopAfter(std::uint16_t pc, bool stepBoundary, bool inScanLoop) = 0;
};
enum class TIStop { None, Breakpoint, Observer };

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

/// The debugger's calls until the debug target interface (LIB-10 59, M-53).
class TIDebugTargetAdapter {
public:
    explicit TIDebugTargetAdapter(TIMachine& m) : m_(m) {}
    /// One instruction; returns TMC0501::step()'s weight and bits. For the
    /// headless recorder.
    std::uint32_t step();
    /// One instruction, stepping on through a HOLD (WAIT Dn) of up to 32.
    void stepInstruction();
    /// Up to the next program byte (a step boundary), a breakpoint or
    /// maxCycles weighted cycles; returns the weighted cycles run.
    std::uint32_t stepUntilNextKeycode(std::uint32_t maxCycles = 50000);
    void beginNextStep();
    CpuFrame snapshotCPU() const;
    TIStepBoundary lastStepBoundary() const;
    std::uint16_t pc() const;
    void setTraceFlags(std::uint32_t flags);
    std::uint32_t traceFlags() const;
    std::uint32_t drainCpuFrames(CpuFrame* out, std::uint32_t max, std::uint32_t* outLost);
    std::uint32_t readCpuFrames(CpuFrame* out, std::uint32_t max) const;
    void addBreakpoint(std::uint16_t pc);
    void removeBreakpoint(std::uint16_t pc);
    void clearBreakpoints();
    /// While set, Machine::advance() checks breakpoints and the observer after
    /// each instruction and may return early (design D-6); nullptr clears it.
    void setRunControl(TIStepObserver* observer);
    /// Why the last advance stopped early, and clears it.
    TIStop takeStop();
    std::uint8_t readROMKeycode(int addr) const;
    static std::string disassemble(std::uint16_t pc, std::uint16_t opcode);
    static double decodeBCD(const std::uint8_t* nibbles16);

private:
    TIMachine& m_;
};

/// The debug panel's calls until C59-4 (M-56).
class TIDebugPanelAdapter {
public:
    explicit TIDebugPanelAdapter(TIMachine& m) : m_(m) {}
    bool loadDebugOverlay(const std::uint16_t* data, std::size_t count);
    void clearDebugOverlay();
    bool runDebugOverlay(std::uint16_t startAddr, std::uint32_t maxSteps, std::uint32_t* outSteps);
    void setDebugLevel(std::uint8_t level);
    std::vector<DebugEvent> drainDebugEvents();

private:
    TIMachine& m_;
};

/// What 2.3.0's full speed throttles on, until LIB-17 59 (M-33; design D-2).
class TIPacingHints {
public:
    explicit TIPacingHints(TIMachine& m) : m_(m) {}
    /// An instruction since the last call was a confirmed spin-wait retrying a
    /// keyboard scan or card-switch poll (TMC0501::kPolledHardware); clears it.
    bool takePolledHardware();
    /// The PC is in the full keyboard scan/detect cycle.
    bool inIdleAndKeyDetect() const;
    /// That range of a model: {lo, hi, extraLo, extraHi} (extra empty when lo > hi).
    static std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId&);

private:
    TIMachine& m_;
};

// The accessors (design §3.4): nullptr if `m` is not a TI machine.
TICardReaderAdapter* cardReaderOf(calcu::machine::Machine&);
TIMemoryAdapter* memoryOf(calcu::machine::Machine&);
TIDebugTargetAdapter* debugTargetOf(calcu::machine::Machine&);
TIDebugPanelAdapter* debugPanelOf(calcu::machine::Machine&);
TIPacingHints* pacingHintsOf(calcu::machine::Machine&);
const TICardReaderAdapter* cardReaderOf(const calcu::machine::Machine&);
const TIMemoryAdapter* memoryOf(const calcu::machine::Machine&);
const TIDebugTargetAdapter* debugTargetOf(const calcu::machine::Machine&);
