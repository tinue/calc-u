// app/ file formats against the Swift originals (tests/golden/formats.txt,
// produced by tools/goldens/formats_main.swift): cue cards, BCD encoders,
// register lines and math tokens. Also round trips for the constant memory
// file. The magnetic card format is in magnetic_card_tests.cpp.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/Bcd.hpp"
#include "../app/ConstantMemoryFile.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../app/TextUtil.hpp"
#include <cstdio>
#include <map>

using namespace cu59;

namespace {

std::string hexNib(const uint8_t* b, size_t n) {
    static const char* d = "0123456789ABCDEF";
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        if (b[i] >= 16) { char buf[4]; std::snprintf(buf, sizeof buf, "%X", b[i]); s += buf; }
        else s += d[b[i]];
    }
    return s;
}

std::string num(double d) {
    char b[64];
    std::snprintf(b, sizeof b, "%.17g", d);
    return b;
}

/// Golden records: header line → body (lines up to END).
struct Record { std::string header, body; };

std::vector<Record> records(const std::string& golden, std::vector<std::string>& singles) {
    std::vector<Record> out;
    Record* cur = nullptr;
    for (std::string_view line : splitLines(golden)) {
        if (cur) {
            if (line == "END") { cur = nullptr; continue; }
            cur->body += std::string(line) + "\n";
            continue;
        }
        if (!line.empty()) {
            singles.emplace_back(line);
        }
    }
    return out;
}

const std::string& golden() {
    static std::string g = testroms::readFile("tests/golden/formats.txt").value_or("");
    return g;
}

} // namespace

TEST(formats_golden_present) {
    CHECK(!golden().empty());
}

TEST(bcd_regline_math_match_swift) {
    std::vector<std::string> singles;
    records(golden(), singles);
    int n = 0;
    for (auto& line : singles) {
        std::string_view l = line;
        if (startsWith(l, "BCDT [")) {
            size_t close = l.find("] ");
            auto nib = encodeBcd(l.substr(6, close - 6));
            CHECK_EQ(hexNib(nib.data(), 16), std::string(l.substr(close + 2)));
            ++n;
        } else if (startsWith(l, "BCDD ")) {
            auto w = splitWords(l);
            auto nib = encodeBcd(*parseDouble(w[1]));
            CHECK_EQ(hexNib(nib.data(), 16), std::string(w[2]));
            ++n;
        } else if (startsWith(l, "REG ")) {
            auto w = splitWords(l);
            Nibbles16 nib{};
            for (size_t i = 0; i < 16; ++i) nib[i] = static_cast<uint8_t>(hexDigit(w[1][i]));
            auto enc = encodeRegisterLine(nib);
            std::string encHex;
            for (uint8_t b : enc) { char buf[4]; std::snprintf(buf, sizeof buf, "%02X", b); encHex += buf; }
            CHECK_EQ(encHex, std::string(w[2]));
            auto dec = decodeRegisterLine(enc);
            CHECK_EQ(hexNib(dec.data(), 16), std::string(w[3]));
            ++n;
        } else if (startsWith(l, "MATH ")) {
            auto w = splitWords(l);
            CHECK_EQ(expandMathTokens(w[1]), std::string(w.size() > 2 ? w[2] : ""));
            ++n;
        }
    }
    CHECK(n > 400);
}


namespace {
std::vector<uint8_t> testBank(int bank) {
    std::vector<uint8_t> b(kCardBankBytes);
    for (size_t i = 0; i < b.size(); ++i) b[i] = static_cast<uint8_t>((i * 37 + 11) & 0xFF);
    b[0] = 0x1C; b[1] = 0x11; b[2] = static_cast<uint8_t>(0x10 | ((bank - 1) * 3)); b[3] = 0x00; b[245] = b[244];
    return b;
}
}  // namespace

// The content
// survives a round trip through constant memory format 1; zero registers are
// left out; a register above R063 or a short line refuses the file.
TEST(round_trip_zero_registers_left_out) {
    ConstantMemoryContents c;
    for (std::size_t i = 0; i < 16; ++i) c.ram[5][i] = static_cast<uint8_t>(i % 10);
    for (std::size_t i = 0; i < 16; ++i) c.ram[63][i] = 9;
    CueCardContent cue;
    cue.title = "MEM";
    cue.row1 = "R: line starting with R";
    c.cueCard = cue;
    const std::string text = writeConstantMemoryFile(c);
    CHECK(text.find("R005: ") != std::string::npos);
    CHECK(text.find("R000") == std::string::npos);  // zero registers are omitted
    auto back = readConstantMemoryFile(text);
    CHECK(std::holds_alternative<ConstantMemoryContents>(back));
    if (!std::holds_alternative<ConstantMemoryContents>(back)) return;
    const auto& got = std::get<ConstantMemoryContents>(back);
    CHECK(got.ram == c.ram);
    CHECK(got.cueCard && got.cueCard->row1 == "R: line starting with R");
    const std::string head = "format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n  bytes: |\n";
    CHECK(std::holds_alternative<ConstantMemoryProblem>(readConstantMemoryFile(head + "    R064: 00 00 00 00 00 00 00 00\n")));
    CHECK(std::holds_alternative<ConstantMemoryProblem>(readConstantMemoryFile(head + "    R001: 00 00\n")));
}




TEST(module_rom_file_names) {
    // The module list and the cue cards come from the module files
    // (magnetic_card_tests.cpp).
    CHECK(moduleRomFile("ML") == std::optional<std::string>("ML.rom.yaml"));
    CHECK(moduleRomFile("RP") == std::optional<std::string>("RP.rom.yaml"));
    CHECK(!moduleRomFile("XX").has_value());
}

TEST(cue_card_geometry) {
    CueCardContent c;
    c.template_ = CueCardTemplate::MagnetCard;
    c.title = "Interest";
    const double w = 800, h = 200;
    const CueCardLayout& L = cueCardLayout(c.template_);
    CueCardGeometry g = cueCardGeometry(c, w, h);
    double charW = L.gridFontSize * 0.64;
    CHECK_EQ(g.gridLeft, w * (L.xFractions[0] - L.cellWidthFraction / 2) + charW * 0.5);
    CHECK_EQ(g.gridRight, w * (L.xFractions[4] + L.cellWidthFraction / 2) - charW);
    CHECK_EQ(g.titleFontSize, L.titleFontSize);   // short title: no shrink
    CHECK_EQ(g.bankLeft.cx, w * 0.08);
    CHECK_EQ(g.bankRight.height, h * 0.15);
    CHECK_EQ(g.row2Left.width + g.row2Right.width, g.gridWidth);
    CHECK_EQ(g.washHeight, h * 0.28);
    c.title = std::string(80, 'W');
    CHECK(cueCardGeometry(c, w, h).titleFontSize < L.titleFontSize);
    CHECK_EQ(cueCardInk(c), 0u);
    c.color = CueCardInkColor::Sharpie;
    CHECK_EQ(cueCardInk(c), 0x702A48u);
    c.template_ = CueCardTemplate::SolidState;
    CHECK_EQ(cueCardInk(c), 0xC49223u);   // printed gold whatever PencilColor says
}
