#pragma once
#include "machine/Display.hpp"
#include <cstdint>
#include <vector>

// The picture of a TI display that a preset's `screenshot:` writes: the
// twelve positions and the
// C indicator in the classic segment style (app/LedGeometry.hpp,
// LedStyle::Classic), flat (no glow, which each frontend paints its own
// way), lit dots in the style's active colour on the display's background,
// as a PNG (formats::PngWriter). Deterministic: the same output gives the
// same bytes.

namespace cu59 {

std::vector<std::uint8_t> displayPicture(const calcu::machine::DisplayOutput&);

} // namespace cu59
