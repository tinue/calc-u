#pragma once
#include "TIMachine.hpp"
#include <cstdint>

// Test-only reads inside the core (C59-1, Technical Requirements: "read by
// the test inside the core"): the key lines, the raw RAM and the PC of a
// TIMachine. Only tests/ include it (the seam check, CalcU59CoreSeam).

struct TIMachineTesting {
    /// The key matrix byte of digit slot `col` (bit = K-line).
    static std::uint8_t keyLines(const TIMachine& m, int col) { return m.m_hw.keyLinesForTest(col); }
    static std::uint16_t pc(const TIMachine& m) { return m.m_hw.pc(); }
    static bool restorePending(const TIMachine& m) { return m.m_restore.has_value(); }
    static TI59Machine& hardware(TIMachine& m) { return m.m_hw; }
};
