#pragma once
#include <cstdint>

// ── Trace feature flags ───────────────────────────────────────────────────────
//
// Independently gated cost tiers for the instruction trace / debug API.
// Set by TIMachine alone (TIMachine::applyTraceFlags): on while the debug
// panel, a trace or the run control's checks want the frame ring.

enum : uint32_t {
    TRACE_NONE         = 0x0000,
    TRACE_PC           = 0x0001,  ///< (pc, opcode, digit, cycleWeight, seqno) only; ~10 bytes/event
    TRACE_REGS_LIGHT   = 0x0002,  ///< adds KR, SR, fA, fB, R5, cpuFlags; medium cost
    TRACE_REGS_FULL    = 0x0004,  ///< adds A–E, SCOM, Sout; ~370 bytes/event; debug sessions only
    TRACE_BREAKPOINTS  = 0x0008,  ///< binary-search breakpoint check on each step; stop-the-world
};


// ── Unified CPU frame ─────────────────────────────────────────────────────────
//
// Combines TraceEvent and CPUSnapshot into a single struct.
// Reduces ring buffers from 2 to 1 and eliminates parallel-array mismatch risk.
// ~392 bytes per frame; 1024-deep ring = ~401 KB.

struct CpuFrame {
    // Identity (always captured)
    uint32_t seqno;
    uint16_t pc;
    uint16_t opcode;
    uint8_t  digit;
    uint8_t  postDigit;   // Digit counter in post-operation state (after executed instruction)
    uint8_t  cycleWeight;

    // Light registers (captured when TRACE_REGS_LIGHT is set)
    uint16_t KR, SR, fA, fB, cpuFlags;
    uint8_t  R5;

    // Full snapshot (captured when TRACE_REGS_FULL is set)
    uint8_t  A[16], B[16], C[16], D[16], E[16];
    uint8_t  SCOM[16][16];
    uint8_t  Sout[16];
    uint16_t EXT, PREG, flags, m_libAddr;
    uint8_t  REG_ADDR, RAM_ADDR, RAM_OP, m_libAddrReadPos;
    uint8_t  displayOn;  ///< 1 when a digit or decimal dot was driven in the last digit-counter revolution, else 0.
    uint8_t  maxDigitDecay; ///< Number of positions (0..12) driven in that revolution (the trace
                            ///< format's field of that name).

    /// Its fields for the machine state (TIStateRecords.hpp), one by one.
    template <class Io>
    void stateIo(Io& io) {
        io(seqno)(pc)(opcode)(digit)(postDigit)(cycleWeight)(KR)(SR)(fA)(fB)(cpuFlags)(R5);
        io(A)(B)(C)(D)(E)(SCOM)(Sout)(EXT)(PREG)(flags)(m_libAddr);
        io(REG_ADDR)(RAM_ADDR)(RAM_OP)(m_libAddrReadPos)(displayOn)(maxDigitDecay);
    }
};

// ── Debug event ───────────────────────────────────────────────────────────────
//
// Emitted by the CPU core for observability; drained by the UI at 60 Hz.
// level: 1 = INFO, 2 = DEBUG.

struct DebugEvent {
    uint8_t level;    ///< 1=INFO, 2=DEBUG
    char    msg[80];  ///< Null-terminated message
};

// ── Step boundary ─────────────────────────────────────────────────────────────
//
// The CPU state at the last step boundary or idle entry, with the module fetch
// latch of that moment and a count of boundaries (0 = none since reset). Read
// through the product kind c59.calculator-state.

struct TIStepBoundary {
    CpuFrame cpu;
    uint16_t libExecPC;
    bool atStep;        ///< false: idle entry
    uint64_t count;

    template <class Io>
    void stateIo(Io& io) { io(cpu)(libExecPC)(atStep)(count); }
};
