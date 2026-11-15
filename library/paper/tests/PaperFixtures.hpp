#pragma once
#include "paper/Paper.hpp"
#include "paper/Replies.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The paper tests' data (LIB-8 design §2, §6): the descriptions of the three
// products' printers as test data (AC-24, AC-32; D-15: the PC-100C's
// printable part is its whole width until M-53 confirms it, its greys
// today's inkAlpha of app/PrinterFont.hpp for 0 to 20 heating cycles; a
// plotter's stroke is one point wide, as Calc-U-1600 draws it), and the
// fixture files in library/paper/tests/fixtures/:
//   pc100c.marks    the converter's reply of diag.ti59 (Calc-U-59, AC-39)
//   ce150.marks     the converter's reply of plotter_ce150_demo (CE-150)
//   ce1600p.marks   the strokes of plotter_ascii (CE-1600P) that stay within
//                   140 units, so that its picture stays small; reach of those
//   *.png           the reference pictures (§3.3, T5), made by Picture.cpp
//                   in M-39; CALCU_PAPER_PICTURES=1 writes them anew.

namespace calcu::paper::test {

PaperDescription pc100cDescription();
PaperDescription ce150Description();
PaperDescription ce1600pDescription();

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
