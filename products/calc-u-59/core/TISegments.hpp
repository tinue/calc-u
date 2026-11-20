#pragma once
#include "TMC0501.hpp"
#include "machine/Display.hpp"
#include <cstdint>
#include <string_view>

// The glyph rules of the TI display and the turn
// of a DisplaySnapshot into the machine's DisplayOutput. Every glyph comes from
// this table, never from a font: no font reproduces the hardware's lit-segment
// patterns (docs/developer/New-GUI-Guide.md).
//
// Segments: bit 0=A(top) 1=B(upper-right) 2=C(lower-right) 3=D(bottom)
//           4=E(lower-left) 5=F(upper-left) 6=G(middle)

inline constexpr uint8_t kDigitSegments[10] = {
    0b0111111,  // 0: A B C D E F
    0b0000110,  // 1: B C
    0b1011011,  // 2: A B D E G
    0b1001111,  // 3: A B C D G
    0b1100110,  // 4: B C F G
    0b1101101,  // 5: A C D F G
    0b1111101,  // 6: A C D E F G
    0b0000111,  // 7: A B C
    0b1111111,  // 8: all
    0b1101111,  // 9: A B C D F G
};

inline constexpr uint8_t kSegmentsMinus  = 0b1000000;  // G only
inline constexpr uint8_t kSegmentsDegree = 0b1100011;  // A B F G
inline constexpr uint8_t kSegmentsQuote  = 0b0100010;  // B F — double quote
inline constexpr uint8_t kSegmentsApostrophe = 0b0100000;  // F

/// Segments for one display cell from its A-register digit and B-register
/// control nibble (DisplaySnapshot::digits/ctrl), or 0 when the
/// zero-suppression circuit blanks it. Ctrl 0/1/8/9 select the digit,
/// 2 a double quote, 4 an apostrophe, 5 a degree sign, 6 minus; 3, 7 and
/// above are blank. 2, 4 and 5 were checked on a real TI-59; the ROM's normal
/// displays use none of them (LRN SST Del… in source 8 shows all three). The
/// zero suppression darkens 2 and 4 and turns 5 into a minus; the core applies
/// that (TMC0501::postOperation), so a 5 that arrives here is a degree sign.
inline constexpr uint8_t segmentsFor(uint8_t digit, uint8_t ctrl, bool suppressed) {
    if (suppressed) return 0;
    switch (ctrl) {
    case 0: case 1: case 8: case 9: return kDigitSegments[(digit & 0xF) % 10];
    case 2: return kSegmentsQuote;
    case 4: return kSegmentsApostrophe;
    case 5: return kSegmentsDegree;
    case 6: return kSegmentsMinus;
    default: return 0;
    }
}

/// The segment layout of the descriptor: A-G, then DP.
inline constexpr int kTISegmentCount = 8;
inline constexpr int kTIDecimalPoint = 7;

/// 12 positions, index 0 rightmost. Per position: lit A-G from segmentsFor,
/// DP lit when driven; brightness = the position's normalised duty, DP's own
/// brightness = its duty. The indicator `indicator` (C) is
/// on while driven, brightness = calcIndicator / 16. The display is always on.
inline calcu::machine::DisplayOutput toDisplayOutput(const DisplaySnapshot& d, std::string_view indicator) {
    calcu::machine::SegmentFrame frame;
    frame.positions.resize(12);
    for (int i = 0; i < 12; ++i) {
        auto& p = frame.positions[static_cast<size_t>(i)];
        const uint8_t seg = segmentsFor(d.digits[i], d.ctrl[i], (d.suppressedMask >> i) & 1u);
        p.lit.resize(kTISegmentCount);
        for (int s = 0; s < 7; ++s) p.lit[static_cast<size_t>(s)] = (seg >> s) & 1u;
        p.lit[kTIDecimalPoint] = d.dpDuty[i] > 0;
        p.brightness = d.digitDuty[i];
        p.segmentBrightness.resize(kTISegmentCount);
        p.segmentBrightness[kTIDecimalPoint] = d.dpDuty[i];
    }
    calcu::machine::DisplayOutput out;
    out.on = true;
    out.frame = std::move(frame);
    out.indicators.push_back({std::string(indicator), d.calcIndicator > 0, d.calcIndicator / 16.0f});
    return out;
}
