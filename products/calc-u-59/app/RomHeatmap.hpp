#pragma once
#include <cstdint>
#include <vector>

// How the CPU tab draws the ROM heatmap: the grid, the steps of
// the counts and their colours. The counts are the core's (the product kind
// c59.rom-heatmap), published by TIDebugPart at about 10 Hz.

namespace cu59 {

/// Display grid: 80 × 64 cells, one per code address (row-major).
inline constexpr int kHeatmapCols = 80;
inline constexpr int kHeatmapRows = 64;
inline constexpr int kHeatmapCodeWords = 0x1400;

/// Fixed absolute thresholds: the first hit is immediately visible (step 1);
/// steps only grow, so a renderer can repaint cells incrementally.
inline constexpr uint8_t heatStep(uint32_t count) {
    return count == 0 ? 0 : count <= 2 ? 1 : count <= 9 ? 2 : count <= 49 ? 3 : count <= 249 ? 4 : 5;
}

struct Rgb { uint8_t r, g, b; };
/// Cold dark gray to bright yellow (HSB hue 0.06–0.14, sat 0.9–1.0, bri 0.35–1.0).
inline constexpr Rgb kHeatColors[6] = {
    {46, 46, 46}, {89, 38, 9}, {140, 67, 0}, {191, 115, 0}, {230, 165, 0}, {255, 214, 0},
};

} // namespace cu59
