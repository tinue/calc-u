#pragma once
#include "harness/Reply.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

// The display as text and as the machine gives it.
// - readDotMatrix follows Calc-U-1600's parseLcdText without any model in
//   it. Per cell: all dots off gives a blank, an exact glyph its text, an
//   inverted glyph its text, anything else U+FFFD, counted in notText. The
//   font is the product's: it comes from the machine's ROM.
// - readSegments writes one string per position, from left to right, by the
//   rule the product supplies for its segment layout.
// - A display that is off gives the single line `display off`.

namespace calcu::harness {

struct DisplayText {
    bool on = true;
    std::vector<std::string> rows;
    // The indicators that are on, in descriptor order, with their brightness
    // where the descriptor gives one (shown only with --json).
    std::vector<std::pair<std::string, std::optional<Decimal>>> indicators;
    int notText = 0;
    bool dotMatrix = false;   // notText is reported for a dot matrix only
};

// A cell column-wise, as Calc-U-1600's LcdCell: bit n of a byte is row n.
struct Glyph {
    std::string text;
    std::array<std::uint8_t, 8> columns{};
};
struct GlyphFont {
    int cellWidth = 6, cellHeight = 8;
    std::vector<Glyph> glyphs;   // on a duplicate cell the first wins
};

inline constexpr const char* kNotText = "\xEF\xBF\xBD";   // U+FFFD

// Cells of font.cellWidth × font.cellHeight from the top left; rows of the
// matrix below the last whole cell row are not read.
DisplayText readDotMatrix(const machine::DotMatrixFrame&, const GlyphFont&,
                          std::span<const machine::IndicatorOutput>,
                          const std::vector<machine::IndicatorDef>& defs);

// The rule for one position: its text, "" for a blank.
using SegmentRule = std::function<std::string(const machine::SegmentPosition&, const machine::SegmentLayout&)>;
// `rightmostFirst`: the frame's position 0 is the rightmost (the TI order);
// the rows are written from left to right either way.
DisplayText readSegments(const machine::SegmentFrame&, const machine::SegmentLayout&, const SegmentRule&,
                         bool rightmostFirst, std::span<const machine::IndicatorOutput>,
                         const std::vector<machine::IndicatorDef>& defs);

// The indicators that are on, in descriptor order.
std::vector<std::pair<std::string, std::optional<Decimal>>> indicatorsOn(std::span<const machine::IndicatorOutput>,
                                                                         const std::vector<machine::IndicatorDef>&);

// The lines of `display-text` and its JSON result.
std::vector<std::string> displayTextLines(const DisplayText&);
Value displayTextValue(const DisplayText&);

// The lines of `display` and its JSON result: the raw output.
std::vector<std::string> displayLines(const machine::DisplayOutput&, const machine::DisplayDescriptor&);
Value displayValue(const machine::DisplayOutput&, const machine::DisplayDescriptor&);

} // namespace calcu::harness
