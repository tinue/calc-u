#pragma once

// Calculator canvas geometry shared by every frontend: the base image
// (2064×4075: display panel, card slot, keyboard), where the LED and cue-card
// overlays sit, and the 9×5 key rectangles with the widened hit test.
// Rects are normalized: display/card to the whole canvas, keys to the
// keyboard section (canvas y from kKeyboardYStart, height kKeyboardYScale).
// Key rects are the caps of the generated faceplate (faceplate/build.sh writes them).

#include <optional>
#include <utility>

namespace cu59 {

struct NRect {
    double x, y, w, h;
    constexpr bool contains(double px, double py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

inline constexpr double kCanvasAspect = 2064.0 / 4075.0;  ///< width / height

/// LED display window (canvas-space), above the card to avoid overlap.
inline constexpr NRect kDisplayRect{0.0, 0.052, 1.0, 0.05};

/// Cue-card slot (canvas-space). The card images are 440 px tall: 440 / 4075 ≈ 0.108.
inline constexpr NRect kCardRect{0.0, 0.11, 1.0, 0.1080};

/// The fitted card's two corners (fractions of the card): the top corners of the
/// artwork's black band, where a press feeds that side and which light up while
/// it is read. The art's arrow (cueCardGeometry) stays black on the lit colour.
inline constexpr double kCardCornerWidth = 0.20;
inline constexpr double kCardCornerHeight = 0.277;
inline constexpr unsigned kCardCornerLitRGB = 0xFFF2B8;

/// A side's corner (fractions of the card): 0 top-left, 1 top-right.
inline constexpr NRect cardCornerRect(int side) {
    return {side == 0 ? 0.0 : 1 - kCardCornerWidth, 0.0, kCardCornerWidth, kCardCornerHeight};
}

/// The side whose corner holds canvas position (nx, ny).
inline std::optional<int> cardCornerAt(double nx, double ny) {
    if (!kCardRect.contains(nx, ny)) return std::nullopt;
    const double x = (nx - kCardRect.x) / kCardRect.w, y = (ny - kCardRect.y) / kCardRect.h;
    for (int side : {0, 1})
        if (cardCornerRect(side).contains(x, y)) return side;
    return std::nullopt;
}

/// Keyboard section within the canvas.
inline constexpr double kKeyboardYStart = 0.2324;
inline constexpr double kKeyboardYScale = 0.7676;

/// Key rects in keyboard-image space, [row 0–8][col 0–4]. Matrix code = (row+1)×10 + (col+1).
inline constexpr NRect kKeyRects[9][5] = {
    // row 0
    {{0.013566, 0.036755, 0.129845, 0.053389}, {0.223837, 0.036755, 0.129845, 0.053389}, {0.435078, 0.036755, 0.129845, 0.053389}, {0.645833, 0.036755, 0.129845, 0.053389}, {0.855620, 0.036755, 0.129845, 0.053389}},
    // row 1
    {{0.013566, 0.148969, 0.129845, 0.053389}, {0.223837, 0.148969, 0.129845, 0.053389}, {0.435078, 0.148969, 0.129845, 0.053389}, {0.645833, 0.148969, 0.129845, 0.053389}, {0.855620, 0.148969, 0.129845, 0.053389}},
    // row 2
    {{0.013566, 0.260862, 0.129845, 0.053389}, {0.223837, 0.260862, 0.129845, 0.053389}, {0.435078, 0.260862, 0.129845, 0.053389}, {0.645833, 0.260862, 0.129845, 0.053389}, {0.855620, 0.260862, 0.129845, 0.053389}},
    // row 3
    {{0.013566, 0.372756, 0.129845, 0.053389}, {0.223837, 0.372756, 0.129845, 0.053389}, {0.435078, 0.372756, 0.129845, 0.053389}, {0.645833, 0.372756, 0.129845, 0.053389}, {0.855620, 0.372756, 0.129845, 0.053389}},
    // row 4
    {{0.013566, 0.484330, 0.129845, 0.053389}, {0.223837, 0.484330, 0.129845, 0.053389}, {0.435078, 0.484330, 0.129845, 0.053389}, {0.645833, 0.484330, 0.129845, 0.053389}, {0.855620, 0.484330, 0.129845, 0.053389}},
    // row 5
    {{0.013566, 0.596224, 0.129845, 0.054668}, {0.200581, 0.596224, 0.164244, 0.054668}, {0.418605, 0.596224, 0.164244, 0.054668}, {0.636628, 0.596224, 0.164244, 0.054668}, {0.855620, 0.596224, 0.129845, 0.054668}},
    // row 6
    {{0.013566, 0.709396, 0.129845, 0.054668}, {0.200581, 0.709396, 0.164244, 0.054668}, {0.418605, 0.709396, 0.164244, 0.054668}, {0.636628, 0.709396, 0.164244, 0.054668}, {0.855620, 0.709396, 0.129845, 0.054668}},
    // row 7
    {{0.013566, 0.821929, 0.129845, 0.054668}, {0.200581, 0.821929, 0.164244, 0.054668}, {0.418605, 0.821929, 0.164244, 0.054668}, {0.636628, 0.821929, 0.164244, 0.054668}, {0.855620, 0.821929, 0.129845, 0.054668}},
    // row 8
    {{0.013566, 0.934143, 0.129845, 0.054668}, {0.200581, 0.934143, 0.164244, 0.054668}, {0.418605, 0.934143, 0.164244, 0.054668}, {0.636628, 0.934143, 0.164244, 0.054668}, {0.855620, 0.934143, 0.129845, 0.054668}},
};

/// Hit-test expansion: each rect grows by this fraction of its width left
/// AND right, and of its height top AND bottom.
inline constexpr double kHitExpandHorizontal = 0.15;
inline constexpr double kHitExpandVertical   = 0.35;

constexpr NRect expandedKeyRect(int row, int col) {
    const NRect& r = kKeyRects[row][col];
    double ew = r.w * kHitExpandHorizontal, eh = r.h * kHitExpandVertical;
    return {r.x - ew, r.y - eh, r.w + ew * 2, r.h + eh * 2};
}

struct KeyHit { int row, col; };  ///< 0-based

/// First key whose expanded rect contains the keyboard-space point; false if none.
inline bool keyAt(double nx, double ny, KeyHit& out) {
    for (int row = 0; row < 9; ++row)
        for (int col = 0; col < 5; ++col)
            if (expandedKeyRect(row, col).contains(nx, ny)) { out = {row, col}; return true; }
    return false;
}

} // namespace cu59
