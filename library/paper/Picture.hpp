#pragma once
#include "paper/Paper.hpp"
#include <cstdint>
#include <vector>

// The picture of Copy and of `paper png` (LIB-8 design §3.3; spec AC-16,
// AC-17): the full width of the paper and all it reaches, marks on white,
// each drawn as the description says, at the paper's physical size and the
// resolution the description states, lowered only so that no side exceeds
// 16000 pixels and the whole 64 million. Made below the frontends, so that
// every frontend and the harness get the same pixels for the same paper.

namespace calcu::paper {

inline constexpr int kMaxPictureSide = 16'000;
inline constexpr std::int64_t kMaxPicturePixels = 64'000'000;

struct PictureSize {
    int width = 0, height = 0;
    double dotsPerInch = 0;
    Coord top = 0, bottom = 0;   // the paper y of the first row's top and of the last row's bottom
};

// The extent and size of the picture (AC-16, AC-17); zero for an empty paper.
PictureSize pictureSize(const Paper&);
// The PNG file; empty for an empty paper.
std::vector<std::uint8_t> picturePng(const Paper&);
// The same for a description and marks (the reference pictures of the
// products' printers in the tests are made from data).
PictureSize pictureSize(const PaperDescription&, const std::vector<Mark>&, Reach, bool empty);
std::vector<std::uint8_t> picturePng(const PaperDescription&, const std::vector<Mark>&, Reach, bool empty);

} // namespace calcu::paper
