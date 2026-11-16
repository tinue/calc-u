#pragma once
#include "paper/Paper.hpp"
#include "paper/Replies.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The paper tests' data (LIB-8 design §2, §6): the PC-100C's description as
// test data (AC-24; D-15: its printable part is its whole width until M-53
// confirms it, its greys today's inkAlpha of app/PrinterFont.hpp for 0 to 20
// heating cycles; it moves to Calc-U-59's tests with M-53, when Calc-U-59
// has a description of its own), and the fixture files in
// library/paper/tests/fixtures/:
//   pc100c.marks    the converter's reply of diag.ti59 (Calc-U-59, AC-39)
//   *.png           the reference pictures (§3.3, T5), made by Picture.cpp
//                   in M-39; CALCU_PAPER_PICTURES=1 writes them anew.
// The CE-150's and CE-1600P's reference pictures are Calc-U-1600's
// (products/calc-u-1600/tests/plotter_paper_tests.cpp, on its descriptions).

namespace calcu::paper::test {

PaperDescription pc100cDescription();

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
