#pragma once
#include "TIMachine.hpp"
#include <cstdint>
#include <vector>

// Test-only reads inside the core (C59-1, Technical Requirements: "read by
// the test inside the core"): the key lines, the raw RAM and the PC of a
// TIMachine, and the PC-100C's code lines (P-E3). Only tests/ and, until
// M-59, tools/headless/capi.cpp include it (the seam check, CalcU59CoreSeam).

struct TIMachineTesting {
    /// The key matrix byte of digit slot `col` (bit = K-line).
    static std::uint8_t keyLines(const TIMachine& m, int col) { return m.m_hw.keyLinesForTest(col); }
    static std::uint16_t pc(const TIMachine& m) { return m.m_hw.pc(); }
    static bool restorePending(const TIMachine& m) { return m.m_restore.has_value(); }
    static TI59Machine& hardware(TIMachine& m) { return m.m_hw; }
    /// LIB-8 adapter P-E3 (design §11.5, D-26): the code lines the PC-100C
    /// ended since the last call, for the golden runs' `dots` lines and the
    /// headless tool's printer_code_lines (until M-59). The first call turns
    /// the record on, for this PC-100C and every one plugged in later; none
    /// without a PC-100C.
    static std::vector<PC100C::CodeLine> printedCodeLines(TIMachine& m) {
        m.m_recordCodeLines = true;
        if (!m.m_printer) return {};
        m.m_printer->recordCodeLines(true);
        return m.m_printer->takeCodeLines();
    }
};
