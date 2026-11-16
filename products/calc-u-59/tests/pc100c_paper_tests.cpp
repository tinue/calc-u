// LIB-8's Calc-U-59 column (design §11.4, §11.6: E8-1 to E8-4): the PC-100C
// gives its paper through the paper output kind. The description (AC-24), a
// printed line and a feed (AC-25), lines cut short (AC-26) and Copy's picture
// (AC-27), on the real ROMs, and the reference picture moved from the
// library in M-53 (tests/fixtures/paper/pc100c.png, pc100c.marks: the
// converter's reply of diag.ti59).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../core/PC100CPaper.hpp"
#include "paper/PaperPart.hpp"
#include "paper/Picture.hpp"
#include "paper/Replies.hpp"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>

using namespace cu59;
using namespace calcu::paper;
using testroms::run;

namespace {

constexpr int k2nd = 21, kPrtKey = 94, kAdvKey = 93, kClr = 25;
constexpr int kDigit[10] = {92, 82, 83, 84, 72, 73, 74, 62, 63, 64};

const std::string kFixtures = "tests/fixtures/paper/";

std::vector<std::string> fixtureLines(const std::string& name) {
    std::ifstream in(kFixtures + name);
    std::vector<std::string> out;
    for (std::string l; std::getline(in, l);) out.push_back(l);
    return out;
}

std::vector<std::uint8_t> fixtureBytes(const std::string& name) {
    std::ifstream in(kFixtures + name, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// A TI-59 with the PC-100C at key readiness, its paper tapped from the start.
std::unique_ptr<calcu::machine::Machine> printing(Model model = Model::TI59) {
    auto m = testroms::makeMachine(model);
    testroms::printedLines(*m);
    testroms::runToScanLoop(*m, model);
    return m;
}

void prt(calcu::machine::Machine& m, Model model, int digit) {
    testroms::tap(m, model, kDigit[digit]);
    testroms::tap(m, model, k2nd);
    testroms::tap(m, model, kPrtKey);
    run(m, static_cast<uint32_t>(testroms::instructionHz(model) * 0.5));
}

const Paper& paperOf(calcu::machine::Machine& m) { return testroms::paperTap(m).paper; }

} // namespace

// E8-1, AC-24: dots, a text layer, 430/1000 mm, 129.07 units, the whole width
// printable, 21 greys; the capability conforms on every model.
TEST(lib8_ac24_pc100c_states_its_paper) {
    const PaperDescription& d = pc100cpaper::description();
    CHECK(d.unit == (Millimetres{430, 1000}));
    CHECK_EQ(d.width, Coord{1'290'700});
    CHECK_EQ(d.printableLeft, Coord{0});
    CHECK_EQ(d.printableWidth, d.width);
    CHECK(d.form == MarkForm::Dots);
    CHECK(d.textLayer);
    CHECK_EQ(d.darknessGreys.size(), std::size_t{21});
    CHECK_EQ(int(d.darknessGreys[20]), 0);   // a full dot is black
    CHECK(d.darknessGreys[0] > 230);          // barely visible
    CHECK_EQ(d.pictureDotsPerInch, 600);
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        auto m = testroms::makeMachine(model);
        const auto problem = checkPaperOutput(*m, kPC100C, d);
        CHECK(!problem.has_value());
        if (problem) std::fprintf(stderr, "  %s\n", problem->c_str());
    }
}

// E8-2, AC-25: a line gives its 20 positions at (0, top) and the font's dots.
TEST(lib8_ac25_line_gives_20_positions_and_font_dots) {
    auto m = printing();
    prt(*m, Model::TI59, 7);
    const Paper& p = paperOf(*m);
    CHECK_EQ(p.text().size(), std::size_t{1});
    if (p.text().empty()) return;
    CHECK_EQ(p.text()[0].text, std::string("          7.        "));
    CHECK(p.text()[0].at == (Position{0, 0}));
    std::array<std::uint8_t, 20> codes{};
    codes[10] = 8;    // 7
    codes[11] = 32;   // .
    std::vector<Mark> want;
    for (const Dot& d : pc100cpaper::lineDots(codes, 7, 0, 0, 0)) want.push_back(d);
    CHECK(p.marks() == want);
}

TEST(lib8_ac25_complete_line_takes_10_18) {
    auto m = printing();
    prt(*m, Model::TI59, 1);
    prt(*m, Model::TI59, 2);
    const Paper& p = paperOf(*m);
    CHECK_EQ(p.text().size(), std::size_t{2});
    if (p.text().size() < 2) return;
    CHECK_EQ(p.text()[1].at.y, Coord{101'800});
    CHECK_EQ(p.reach().to, Coord{2 * 101'800});
}

TEST(lib8_ac25_feed_takes_7_09_and_an_empty_line) {
    auto m = printing();
    // ADV pressed and released: the ROM's Adv issues two PRT.FEEDs.
    testroms::printerControl(*m, PC100C::kAdv, true);
    run(*m, 300);
    testroms::printerControl(*m, PC100C::kAdv, false);
    run(*m, static_cast<uint32_t>(testroms::instructionHz(Model::TI59) * 0.2));
    const Paper& p = paperOf(*m);
    CHECK(p.text().size() >= 2);
    for (const auto& l : p.text()) CHECK(l.text.empty());
    CHECK(p.marks().empty());
    CHECK_EQ(p.reach().to, static_cast<Coord>(p.text().size()) * 70'900);
}

TEST(lib8_ac25_adv_gives_two_feeds) {
    auto m = printing();
    testroms::tap(*m, Model::TI59, k2nd);
    testroms::tap(*m, Model::TI59, kAdvKey);   // 2nd Adv, once
    run(*m, static_cast<uint32_t>(testroms::instructionHz(Model::TI59) * 0.5));
    const Paper& p = paperOf(*m);
    CHECK_EQ(p.text().size(), std::size_t{2});
    CHECK_EQ(p.reach().to, Coord{2 * 70'900});
}

// The characters of every code (all 64, x̄ as two code points) through the
// placement: each code's dots are its font's.
TEST(lib8_ac25_characters_of_today_for_every_code) {
    CHECK_EQ(std::string(pc100cpaper::kCharacters[55]), std::string("x\xCC\x84"));   // x̄
    std::set<std::string> distinct;
    for (int c = 0; c < 64; ++c) {
        distinct.insert(pc100cpaper::kCharacters[c]);
        std::array<std::uint8_t, 20> codes{};
        codes[0] = static_cast<std::uint8_t>(c);
        std::size_t n = 0;
        for (int r = 0; r < 7; ++r)
            for (int b = 0; b < 5; ++b) n += (pc100cpaper::kFont[c][r] >> b) & 1u;
        CHECK_EQ(pc100cpaper::lineDots(codes, 7, 0, 0, 0).size(), n);
    }
    CHECK_EQ(distinct.size(), std::size_t{64});
}

// E8-3, AC-26: a line cut short gives the rows and columns that fired, the
// faint one with its darkness, and takes only the paper of its rows.
TEST(lib8_ac26_cut_short_rows_and_columns_with_their_darkness) {
    std::array<std::uint8_t, 20> codes{};
    codes.fill(11);   // "A": row 3 is 0x1F, every column inked
    const auto dots = pc100cpaper::lineDots(codes, 3, 2, 5, 0);   // 3 rows, 2 columns of row 3 (the last after 5 cycles)
    bool faint = false;
    for (const Dot& d : dots) {
        const int row = static_cast<int>(d.at.y / kPerUnit);
        CHECK(row <= 3);
        if (row == 3) {
            const int col = static_cast<int>((d.at.x % pc100cpaper::kCharStep) / kPerUnit);
            CHECK(col >= 3);   // the 2 rightmost columns fired
            if (col == 3) {
                CHECK_EQ(int(d.darkness), 5);
                faint = true;
            } else {
                CHECK_EQ(int(d.darkness), 20);
            }
        }
    }
    CHECK(faint);
}

TEST(lib8_ac26_takes_only_the_paper_of_its_rows) {
    CHECK_EQ(pc100cpaper::lineHeight(4, 2), Coord{5 * kPerUnit});
    CHECK_EQ(pc100cpaper::lineHeight(4, 0), Coord{4 * kPerUnit});
    CHECK_EQ(pc100cpaper::lineHeight(7, 0), Coord{101'800});
}

// E8-4, AC-27: Copy's picture is at 600 dpi, about 1311 pixels wide.
TEST(lib8_ac27_copy_is_600_dpi_about_1311_pixels_wide) {
    auto m = printing();
    prt(*m, Model::TI59, 3);
    const PictureSize s = pictureSize(paperOf(*m));
    CHECK_EQ(s.width, 1311);
    CHECK_EQ(s.dotsPerInch, 600.0);
}

// The reference picture of the PC-100C (moved from the library in M-53, on
// the product's description; byte for byte).
TEST(lib8_ac16_reference_picture_of_the_pc100c) {
    std::string error;
    const auto read = readMarksReply(fixtureLines("pc100c.marks"), &error);
    CHECK_EQ(error, std::string());
    if (!read) return;
    CHECK(marksReply(read->reach, read->marks) == fixtureLines("pc100c.marks"));   // AC-39: read and written unchanged
    // The marks on a paper of the product's description, as the library's test put them.
    Paper paper(pc100cpaper::description());
    for (const Mark& mk : read->marks) {
        const Dot& d = std::get<Dot>(mk);
        paper.dot(d.at, d.darkness);
    }
    paper.fedTo(read->reach.to);
    CHECK_EQ(pictureSize(paper).width, 1311);
    const auto png = picturePng(paper);
    if (const char* e = std::getenv("CALCU_PAPER_PICTURES"); e && std::string(e) == "1")
        std::ofstream(kFixtures + "pc100c.png", std::ios::binary)
            .write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    const bool same = png == fixtureBytes("pc100c.png");
    CHECK(same);
}
