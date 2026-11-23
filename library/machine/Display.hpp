#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Display output in display kinds. No pixel format, glyph, colour or power
// state. Brightness is the duty the core measures, from 0 to 1; what the
// eye sees is worked out above the machine.

namespace calcu::machine {

struct SegmentPosition {
    std::vector<bool> lit;                                  // per SegmentDef of the layout
    std::optional<float> brightness;                        // if the geometry has positionBrightness
    std::vector<std::optional<float>> segmentBrightness;    // per SegmentDef; set for ownBrightness segments
};
struct SegmentFrame   { std::vector<SegmentPosition> positions; };   // order: the product's
struct DotMatrixFrame { int width = 0, height = 0; std::vector<std::uint8_t> dots; };  // row-major, 0 or 1; cells from the descriptor
struct IndicatorOutput {
    std::string name;        // a copy: a payload holds no pointer into a machine
    bool on;
    std::optional<float> brightness;
};
// A part of the display that is switched on and off on its own (one of
// the PC-1600's two display controllers). Its dots show 0 while it is off.
struct DisplayPart {
    std::string name;
    bool on = true;
};
struct DisplayOutput {
    bool on = true;          // a display that is off shows nothing
    std::variant<SegmentFrame, DotMatrixFrame> frame;
    std::vector<IndicatorOutput> indicators;
    std::vector<DisplayPart> parts;   // empty: the display is switched as a whole
};

} // namespace calcu::machine
