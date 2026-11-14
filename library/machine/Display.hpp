#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

// Display output in display kinds (LIB-1 design §3.6; machine.md M-2). No
// pixel format, glyph, colour or power state (AC-18, AC-43). Brightness is
// the duty the core measures, from 0 to 1; what the eye sees is decided
// above the machine.

namespace calcu::machine {

struct SegmentPosition {
    std::vector<bool> lit;                                  // per SegmentDef of the layout
    std::optional<float> brightness;                        // if the geometry has positionBrightness
    std::vector<std::optional<float>> segmentBrightness;    // per SegmentDef; set for ownBrightness segments
};
struct SegmentFrame   { std::vector<SegmentPosition> positions; };   // order: the product's
struct DotMatrixFrame { int width = 0, height = 0; std::vector<std::uint8_t> dots; };  // row-major, 0 or 1; cells from the descriptor
struct IndicatorOutput {
    std::string_view name;   // views into the machine's descriptor
    bool on;
    std::optional<float> brightness;
};
struct DisplayOutput {
    bool on = true;          // a display that is off shows nothing (AC-43)
    std::variant<SegmentFrame, DotMatrixFrame> frame;
    std::vector<IndicatorOutput> indicators;
};

} // namespace calcu::machine
