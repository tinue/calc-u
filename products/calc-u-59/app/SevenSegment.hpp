#pragma once
#include <cstdint>

// What each cell of the 12-digit LED display draws. The glyphs come from the
// machine's display output (lit segments per position, core/TISegments.hpp
// holds the rules), never from a font: no font reproduces the hardware's
// lit-segment patterns (see reference/NewGUIGuide.md). The "C" annunciator's
// pattern is here. The drawing itself (shear, glow) stays in each frontend;
// see reference/AppLayer.md § "LED Display Rendering".
//
// Segments: bit 0=A(top) 1=B(upper-right) 2=C(lower-right) 3=D(bottom)
//           4=E(lower-left) 5=F(upper-left) 6=G(middle)

namespace cu59 {

inline constexpr uint8_t kSegmentsC = 0b0111001;  // A F E D — "C" annunciator

/// What one cell (0 = rightmost) shows: its glyph (the A–G bits of
/// LedFrame::segments) at its brightness (`level` 0–255). Cell 11 shows the "C" annunciator instead while that is the
/// brighter of the two (`calcOpacity`), so a faint C doesn't hide a lit glyph
/// and a glyph that is off or fading doesn't hide the C. `opacity` is 0–1.
struct LedCell {
    uint8_t segments;
    double opacity;
};
inline constexpr LedCell ledCell(int i, uint8_t glyph, uint8_t level, double calcOpacity) {
    const uint8_t segments = level == 0 ? 0 : glyph;
    const double opacity = level / 255.0;
    if (i == 11 && calcOpacity > 0 && (segments == 0 || calcOpacity > opacity)) return {kSegmentsC, calcOpacity};
    return {segments, opacity};
}

} // namespace cu59
