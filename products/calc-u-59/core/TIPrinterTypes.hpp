#pragma once
#include <array>
#include <cstdint>

// ── Printer output line ───────────────────────────────────────────────────────
//
// Each printed character-line produces one PrinterCodeLine.  rowCount is 7 for
// a complete line; 0–6 when the print was aborted mid-stroke (a new PRT_PRINT
// arrived before the previous line's BUSY window (kPrinterLineCycles) elapsed).
// The head fires a row one dot column at a time, right to left, so a cut can
// also leave the rightmost partialColumns columns of the next row on paper.
//
// Its own header so that the adapters (TIAdapters.hpp) need not include
// TMC0501.hpp. A core type still used above the core: it goes in M-53 (LIB-8
// 59), after C59-6's printer-output kind no longer hands code lines (C59-1
// design D-7).
struct PrinterCodeLine {
    std::array<uint8_t, 20> codes{};
    uint8_t rowCount{7};        // dot rows started in full (0–7); 7 = complete line
    uint8_t partialColumns{0};  // columns of row rowCount started before the cut (0–5, from the right)
    uint8_t partialHeat{0};     // cycles the leftmost of them was heated before the cut (capped at 255)
};
