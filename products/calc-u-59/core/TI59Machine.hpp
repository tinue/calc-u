#pragma once
#include "ROM.hpp"
#include "RAM.hpp"
#include "TMC0501.hpp"
#include "MachineVariant.hpp"
#include <cstdint>
#include <vector>

// The core-internal facade over CPU, ROM and RAM: only
// TIMachine, its capabilities and the tests (TIMachineTesting) use it. It holds no lock and
// starts no thread; the caller serialises every call.
class TI59Machine {
public:
    explicit TI59Machine(MachineVariant variant);

    void loadROM(const uint16_t* data, size_t count);
    void loadConstants(const uint8_t* data, size_t count);
    void reset();

    /// Execute one CPU instruction. Returns TMC0501::step()'s weight and bits
    /// (kStepBoundary: the next instruction is the step dispatch), plus
    /// kBreakpointHit when a breakpoint was hit.
    uint32_t step();
    static constexpr uint32_t kWeightMask = TMC0501::kWeightMask;
    static constexpr uint32_t kStepBoundary = TMC0501::kStepBoundary;
    static constexpr uint32_t kPolledHardware = TMC0501::kPolledHardware;
    static constexpr uint32_t kBreakpointHit = 0x8000'0000u;

    /// The CPU state latched at the last step boundary or idle entry (count 0: none since reset).
    TMC0501::StepBoundary lastStepBoundary() const;

    /// Key input.
    void pressKey(int row, int col);
    void releaseKey(int row, int col);
    uint8_t keyLinesForTest(int col) const { return m_cpu.keyLines(col); }
    uint64_t keySamples(int row, int col) const { return m_cpu.keySamples(row, col); }

    /// Display since the previous call; it restarts the integration window:
    /// one consumer per machine (see TMC0501).
    DisplaySnapshot takeDisplay();
    void startDisplayWindow() { m_cpu.startDisplayWindow(); }   ///< see TMC0501
    void joinDisplayWindows() { m_cpu.joinDisplayWindows(); }

    /// TI-58C persistence helpers.
    void serialiseRAM(uint8_t* dst) const;   // 120*16 bytes
    void deserialiseRAM(const uint8_t* src);

    // ── Preset load helpers ─────────────────────────────────────────────────
    /// Write keycodes[0..count-1] to program RAM starting at step 0.
    void writeProgram(const uint8_t* keycodes, int count);
    /// Write one program step; the others stay as they are.
    void writeProgramStep(int stepAddr, uint8_t keycode);

    /// Write 16 nibbles to data register regNum (see dataRegCount()).
    void writeDataRegister(int regNum, const uint8_t* nibbles16);

    /// Current program-register count read live from SCOM[9][0] * 10.
    /// Reflects whatever the ROM last set via OP 17.
    int partitionProgramRegs() const;

    /// Write the partition directly into SCOM (SCOM[9][0] and SCOM[13][8..9]).
    /// programRAMregs must be a multiple of 10 in the range 0–120.
    void setPartitionProgramRegs(int programRAMregs);

    /// Current solid-state module number from SCOM[9] nibbles 3 (units) and 4 (tens).
    /// Returns 0 for no module, 1–30 for ML01–ML30.
    int insertedModuleNumber() const;

    /// Latched solid-state library execution address: module byte address
    /// (0–4999) of the keycode most recently dispatched by the program
    /// interpreter.  0xFFFF until a module keycode executes.
    uint16_t libExecPC() const;

    /// Cumulative weighted instruction-step count since the last reset() — see
    /// TMC0501::elapsedTicks(). Divide by the machine's active-mode instruction rate
    /// to get elapsed wall-clock seconds.
    uint64_t elapsedTicks() const;

    // ── Magnetic card reader ─────────────────────────────────────────────────
    /// Insert a card immediately.  data/count non-zero = read card (feeds IN CRD);
    /// zero = blank write card (OUT CRD captured).
    void insertCard(const uint8_t* data, size_t count);

    /// Eject the card; returns bytes captured by OUT CRD (empty for read swipes).
    std::vector<uint8_t> cardEject();

    bool isCardPresent()    const;  ///< True while card is passing through reader.
    bool isWaitingForCard() const;  ///< True while ROM is polling TST BUSY for card.
    int  cardMode()         const;  ///< 0=none, 1=read, 2=write

    // ── Connectors ───────────────────────────────────────────────────────────
    /// The PC-100C and the module, as TIMachine plugs them.
    void wire(const TiConnectors& c) { m_cpu.wire(c); }

    // ── Trace / debug API ─────────────────────────────────────────────────────
    void     setTraceFlags(uint32_t flags);
    uint32_t traceFlags() const;

    void setDebugLevel(uint8_t level);
    std::vector<DebugEvent> drainDebugEvents();

    void addBreakpoint(uint16_t pc);
    void removeBreakpoint(uint16_t pc);
    void clearBreakpoints();

    /// Load debug ASM words into overlay region 0x1800+. Returns false on overflow.
    bool loadDebugOverlay(const uint16_t* data, size_t count);

    /// Clear all debug ASM overlay words.
    void clearDebugOverlay();

    /// Force execution entry at startAddr. Returns true when entry succeeded.
    bool runDebugOverlay(uint16_t startAddr, uint32_t maxSteps,
                         uint32_t* outSteps);

    uint32_t drainCpuFrames(CpuFrame* out, uint32_t max, uint32_t* outLost);
    uint32_t readCpuFrames(CpuFrame* out, uint32_t max) const;

    /// Run up to n steps; returns count actually executed.
    /// Stops early if a breakpoint is hit (when TRACE_BREAKPOINTS is set).
    uint32_t stepN(uint32_t n, bool stopOnBreakpoint = true);

    /// Run until the interpreter is about to execute the next program byte (a
    /// step boundary, see step()), a breakpoint, or maxCycles cycle-equivalents
    /// (IDLE steps count as 4, active steps as 1) — e.g. when the program stops.
    /// One call per keycode, as SST: a jump completes inside the byte that
    /// causes it. Returns cycle-equivalents executed.
    uint32_t stepUntilNextKeycode(uint32_t maxCycles = 50000);

    /// Current program counter (for CLI inspection between stepN calls).
    uint16_t pc() const;

    static std::string disassemble(uint16_t pc, uint16_t opcode);

    // ── Calculator-level API ──────────────────────────────────────────────────

    /// Decode a 16-nibble TI-59 BCD register to a double.
    static double decodeBCD(const uint8_t* nibbles16);

    /// Read data register regNum (see dataRegCount()) decoded as double.
    double readDataReg(int regNum) const;

    /// Data registers as the ROM addresses them: Rnn = RAM[dataRegCount() - 1 - nn],
    /// descending to meet the program at the partition. 120 on the TI-59, 60 on
    /// the TI-58/58C, where the TI-58C's 4 extra registers (H00–H03, RAM 60–63)
    /// sit above R00 — unless the TI-58C is partitioned like a TI-59
    /// (examples/historic/repartition.ti58c): then it is 120 and only R56–R59 exist, on
    /// the extra registers. Registers without RAM read 0 and ignore writes.
    int dataRegCount() const;

    /// Read program step stepAddr (0–479) as 2-digit keycode (0–99).
    uint8_t readProgramStep(int stepAddr) const;

    /// Read a ROM keycode at address 0–383.
    uint8_t readROMKeycode(int addr) const;
    /// A 13-bit word of the ROM (with the overlay range) as the CPU reads it.
    uint16_t readROMWord(uint16_t addr) const;

    /// Capture a snapshot of all CPU registers at the current instant.
    CpuFrame snapshotCPU() const;

    /// Pre-execution phase: COND restore, patch previous ring entry, capture snapshot.
    /// Call after stepUntilNextKeycode() or after step() in debugger mode.
    void beginNextStep();

    /// Raw RAM access — copies/writes a complete 16-nibble register.
    /// reg must be in [0, ramRegCount()).
    void copyRAMReg(int reg, uint8_t* out16) const;
    void writeRAMReg(int reg, const uint8_t* nibbles16);
    /// Constant after construction.
    int  ramRegCount() const { return m_ram.size(); }

    MachineVariant variant() const { return m_variant; }

    /// The processor and the RAM (the ROM comes from the ROM set).
    template <class Io>
    void stateIo(Io& io) {
        m_ram.stateIo(io);
        m_cpu.stateIo(io);
    }

private:
    int dataRegCountLocked() const;
    int dataRegRAMIndexLocked(int regNum) const;  // -1: no RAM behind it
    void storeProgramStep(int stepAddr, uint8_t keycode);
    MachineVariant m_variant;
    ROM            m_rom;
    RAM            m_ram;
    TMC0501        m_cpu;

    int cardSwitchCol() const; // Digit-counter column for the card-switch key.
};
