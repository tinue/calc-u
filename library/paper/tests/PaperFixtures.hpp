#pragma once
#include "paper/Paper.hpp"
#include "paper/Replies.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The paper tests' data: the fixture files in library/paper/tests/fixtures/,
// the reference pictures of the test printers, made by Picture.cpp;
// CALCU_PAPER_PICTURES=1 writes them anew. The PC-100C's description, its
// marks and its reference picture are Calc-U-59's (products/calc-u-59/tests/
// pc100c_paper_tests.cpp, on core/PC100CPaper.hpp); the CE-150's and
// CE-1600P's are Calc-U-1600's (products/calc-u-1600/tests/
// plotter_paper_tests.cpp, on its descriptions).

namespace calcu::paper::test {

std::string fixturePath(const std::string& name);
std::vector<std::string> fixtureLines(const std::string& name);
std::vector<std::uint8_t> fixtureBytes(const std::string& name);
bool writePictures();   // CALCU_PAPER_PICTURES=1
void writeFixture(const std::string& name, const std::vector<std::uint8_t>&);

// The PNG's width, height and pixels per metre (IHDR, pHYs); nullopt for
// anything that is not a PNG.
struct PngInfo {
    int width = 0, height = 0;
    std::uint32_t pixelsPerMetre = 0;
    int colourType = -1;
};
std::optional<PngInfo> pngInfo(const std::vector<std::uint8_t>&);

} // namespace calcu::paper::test
