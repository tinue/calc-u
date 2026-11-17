#pragma once
#include "TIMachine.hpp"
#include <cstdint>
#include <vector>

// Test-only reads inside the core (C59-1, Technical Requirements: "read by
// the test inside the core"): the key lines, the raw RAM and the PC of a
// TIMachine, and the PC-100C's code lines (P-E3). Only tests/ include it
// (the seam check, CalcU59CoreSeam). The core's own memory calls
// (hardware(): writeProgram, serialiseRAM, …) are the tests' since the memory
// adapter went (M-59).

struct TIMachineTesting {
    /// The key matrix byte of digit slot `col` (bit = K-line).
    static std::uint8_t keyLines(const TIMachine& m, int col) { return m.m_hw.keyLinesForTest(col); }
    static std::uint16_t pc(const TIMachine& m) { return m.m_hw.pc(); }
    static bool restorePending(const TIMachine& m) { return m.m_restore.has_value(); }
    static TI59Machine& hardware(TIMachine& m) { return m.m_hw; }
    /// n instructions, no run control (the golden runs' replay of 2.3.0's start-up).
    static void runInstructions(TIMachine& m, std::uint32_t n) {
        for (std::uint32_t i = 0; i < n; ++i) m.stepOne();
    }
    /// To the next program step boundary through TIMachine's step (at most
    /// 50000 weighted cycles), then the next step's pre-execution phase.
    static void stepToBoundary(TIMachine& m) {
        std::uint32_t done = 0;
        while (done < 50000) {
            const std::uint32_t r = m.stepOne();
            done += r & TI59Machine::kWeightMask;
            if (r & TI59Machine::kStepBoundary) break;
        }
        m.m_hw.beginNextStep();
    }
    /// LIB-8 adapter P-E3 (design §11.5, D-26): the code lines the PC-100C
    /// ended since the last call, for the golden runs' `dots` lines. The first call turns
    /// the record on, for this PC-100C and every one plugged in later; none
    /// without a PC-100C.
    static std::vector<PC100C::CodeLine> printedCodeLines(TIMachine& m) {
        m.m_recordCodeLines = true;
        if (!m.m_printer) return {};
        m.m_printer->recordCodeLines(true);
        return m.m_printer->takeCodeLines();
    }
};
