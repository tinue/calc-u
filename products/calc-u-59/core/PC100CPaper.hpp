#pragma once
#include "paper/PaperOutput.hpp"
#include <array>
#include <cstdint>
#include <vector>

// The PC-100C's paper: its description, the 5 × 7 font of 64 characters, the character of
// each 6-bit code, the dot rules of a line cut short and the placement of a
// line's dots on the paper. The core gives marks (the machine draws nothing),
// so it holds them.
//
// Geometry, from a measured print head, in dot pitches (the unit): total
// span (first → last dot centre) 55 mm; a 5-dot character 1.9 mm (4 units);
// the gap between characters 1.2 mm (1 + kCharGap units). Scaling the raw
// pitch (0.475 mm) to the 55 mm span gives 0.430 mm with the gap ratio
// kept. Vertical: a 10-line measurement (42.3 mm) gives a 4.377 mm line
// pitch = 10.18 units: 7 dot rows plus a 3.18-unit stepper gap. A feed (one
// PRT.FEED; ADV issues two) is half a line plus 2 units, as measured on a
// laser-printed copy: 7.09 units.

namespace pc100cpaper {

using calcu::paper::Coord;

inline constexpr double kCharGap = 1.53;
inline constexpr double kDotPitchMM = 55.0 / (99.0 + 19.0 * kCharGap);   ///< ≈ 0.430 mm
inline constexpr double kWidthUnits = 100.0 + 19.0 * kCharGap;           ///< 129.07: the line with ½ unit at each side
inline constexpr double kLineUnits = 10.18;                              ///< a complete line
inline constexpr double kFeedUnits = kLineUnits / 2.0 + 2.0;             ///< 7.09: one PRT.FEED
inline constexpr double kDotRadiusFraction = 0.38;                       ///< of the dot pitch
inline constexpr double kCharStepUnits = 5.0 + kCharGap;                 ///< 6.53

// The same in the paper's Coord (ten-thousandths of a unit).
inline constexpr Coord kWidth = 1'290'700;
// The roll is 2.5 inch wide, wider than the head's line: 63.5 mm in the
// paper's unit of 0.43 mm, the line centred on it.
inline constexpr Coord kPaperWidth = 1'476'744;
inline constexpr Coord kMargin = (kPaperWidth - kWidth) / 2;
inline constexpr Coord kLine = 101'800;
inline constexpr Coord kFeed = 70'900;
inline constexpr Coord kCharStep = 65'300;

/// Heating cycles after which a dot is fully dark: the darkness of a full dot.
inline constexpr int kFullInkCycles = 20;

/// The character of each 6-bit code (KR bits 9:4), UTF-8; `x̄` is two code points.
extern const char* const kCharacters[64];

/// The 5 × 7 font, by code: each row a mask, bit 4 the leftmost column.
extern const std::uint8_t kFont[64][7];

/// The unit 430/1000 mm, the 2.5 inch roll (147.67 units) with the head's
/// 129.07 units printable in its middle,
/// dots 0.76 units across, the greys of 0 to 20 heating cycles,
/// pictures at 600 dpi, a text layer.
const calcu::paper::PaperDescription& description();

/// Dot rows a printed line takes on paper: 7 for a complete line; for one
/// cut short (the printer interrupt), its complete rows plus the row in
/// progress if any of its columns fired (at least 1, so the line is visible).
constexpr int drawnRows(int rowCount, int partialColumns) {
    int rows = rowCount + (partialColumns > 0 ? 1 : 0);
    return rows < 1 ? 1 : rows > 7 ? 7 : rows;
}

/// Columns of dot row `row` that got ink, as a font row mask (bit 4 =
/// leftmost column): all of a complete row, the rightmost partialColumns of
/// the row in progress (the head fires right to left), none after it.
constexpr std::uint8_t inkedColumns(int rowCount, int partialColumns, int row) {
    if (row < rowCount) return 0x1F;
    if (row == rowCount && partialColumns > 0)
        return static_cast<std::uint8_t>((1u << (partialColumns > 5 ? 5 : partialColumns)) - 1);
    return 0;
}

/// The column of dot row `row` whose heating was cut early (partialHeat
/// below kFullInkCycles): a faint dot, of darkness partialHeat.
constexpr std::uint8_t faintColumns(int rowCount, int partialColumns, int partialHeat, int row) {
    if (row != rowCount || partialHeat >= kFullInkCycles || partialColumns < 1 || partialColumns > 5) return 0;
    return static_cast<std::uint8_t>(1u << (partialColumns - 1));
}

/// Ink of a dot heated `heat` cycles, relative to a normal one. Fitted to a
/// PC-100A's cut-short dots (docs/developer/Core-Architecture.md, "Dot timing
/// within a line"): barely visible at 0–4 cycles, ≈30% at 12, weak to dark
/// at 15–19, full from 20.
double inkAlpha(int heat);

/// The dots of a printed line whose top stands at `top`, in
/// the order of the paper replies' converter (character by character, then row,
/// then column): the dot of column c, row r of character i at
/// (i × 6.53 + c + 0.5, top + r + 0.5); the columns `inkedColumns` gives
/// at darkness 20, the faint one at darkness min(partialHeat, 20).
/// rowCount 7: a complete line.
std::vector<calcu::paper::Dot> lineDots(const std::array<std::uint8_t, 20>& codes, int rowCount, int partialColumns,
                                        int partialHeat, Coord top);

/// The paper a printed line takes: 10.18 units when complete, else its
/// drawn rows (no advance).
constexpr Coord lineHeight(int rowCount, int partialColumns) {
    return rowCount >= 7 ? kLine : static_cast<Coord>(drawnRows(rowCount, partialColumns)) * calcu::paper::kPerUnit;
}

} // namespace pc100cpaper
