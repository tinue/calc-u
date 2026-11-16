// LIB-8, the Calc-U-1600 column (design §10.6, P16-1 to P16-4, the ported
// cases of P16-3 that were shared_geometry_tests.cpp): the CE-150 and the
// CE-1600P on the library's paper output kind.

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../core/Connector/CE1600PCard.hpp"
#include "../core/Connector/Ce150Card.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "SharpTestSupport.hpp"
#include "TestHarness.hpp"
#include "paper/Paper.hpp"
#include "paper/PaperPart.hpp"
#include "paper/Picture.hpp"

namespace {

using namespace sharptest;
using calcu::paper::kPerUnit;
using Colour = AlpsPlotterMechanism::PenColor;

double millimetres(const calcu::paper::PaperDescription& d, calcu::paper::Coord c) {
    return static_cast<double>(c) / kPerUnit * d.unit.value();
}

// The ring of the stepper phases (see ce1600p_tests.cpp): one slot forward is
// a half step in X's own direction.
constexpr std::uint8_t kRing[8] = {0x9, 0x8, 0xc, 0x4, 0x6, 0x2, 0x3, 0x1};

struct Mechanism {
    AlpsPlotterMechanism& m;
    int x = 0, y = 0, z = 0;
    explicit Mechanism(AlpsPlotterMechanism& mech) : m(mech) {
        m.writeMotorX(0x9);
        m.writeMotorY(0x9);
        m.writeMotorZ(0x9);
    }
    void moveX(int halfSteps) {
        for (int i = 0; i < std::abs(halfSteps); ++i) {
            x = (x + (halfSteps > 0 ? 1 : 7)) % 8;
            m.writeMotorX(kRing[x]);
        }
    }
    void feed(int halfSteps) {   // the paper's y grows by halfSteps
        for (int i = 0; i < std::abs(halfSteps); ++i) {
            y = (y + (halfSteps > 0 ? 7 : 1)) % 8;
            m.writeMotorY(kRing[y]);
        }
    }
    void z6to0() {   // pen down from the rest position
        for (int i = 0; i < 6; ++i) {
            z = (z + 1) % 8;
            m.writeMotorZ(kRing[z]);
        }
    }
    void z0to6() {   // pen up
        for (int i = 0; i < 6; ++i) {
            z = (z + 7) % 8;
            m.writeMotorZ(kRing[z]);
        }
    }
};

struct Connected {
    calcu::paper::PaperOutput& out;
    calcu::paper::Paper paper;
    explicit Connected(calcu::paper::PaperOutput& o) : out(o), paper(o.description()) { paper.bind(&out); }
    ~Connected() { paper.bind(nullptr); }
    const calcu::paper::Stroke& stroke(std::size_t i) const { return std::get<calcu::paper::Stroke>(paper.marks().at(i)); }
};

void checkPlotterDescription(const calcu::paper::PaperDescription& d, double widthMm, double drawnMm) {
    CHECK(d.form == calcu::paper::MarkForm::PenStrokes);
    CHECK(!d.textLayer);
    CHECK(d.unit == (calcu::paper::Millimetres{190, 1920}));
    CHECK(std::fabs(millimetres(d, d.width) - widthMm) < 0.001);
    CHECK(std::fabs(millimetres(d, d.printableWidth) - drawnMm) < 0.001);
    const calcu::paper::Coord right = d.width - d.printableLeft - d.printableWidth;
    CHECK(std::llabs(right - d.printableLeft) <= 1);   // the margins split evenly (rounded once, D-3)
    CHECK_EQ(d.pictureDotsPerInch, 1200);
}

// ── P16-1, AC-32 ─────────────────────────────────────────────────────────

TEST(lib8_ac32_ce150_states_its_paper_on_every_model) {
    checkPlotterDescription(sharp::ce150PaperDescription(), 56.0, 42.75);
    const Catalogue c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        bool ok = false;
        auto m = buildSharp(c, model, std::nullopt, {request(c, "CE-150", "printer", std::nullopt, &ok)});
        SKIP_UNLESS(ok && m, "ROMs missing");
        const auto problem = calcu::paper::checkPaperOutput(*m, PeripheralId{"CE-150"}, sharp::ce150PaperDescription());
        if (problem) std::fprintf(stderr, "  %s: %s\n", model, problem->c_str());
        CHECK(!problem);
    }
}

TEST(lib8_ac32_ce1600p_states_its_paper) {
    checkPlotterDescription(sharp::ce1600pPaperDescription(), 210.0, 190.0);
    const Catalogue c = sharp::sharpCatalogue();
    bool ok = false;
    auto m = buildSharp(c, "PC-1600", std::nullopt, {request(c, "CE-1600P", "printer", std::nullopt, &ok)});
    SKIP_UNLESS(ok && m, "CE-1600P ROMs missing");
    const auto problem = calcu::paper::checkPaperOutput(*m, PeripheralId{"CE-1600P"}, sharp::ce1600pPaperDescription());
    if (problem) std::fprintf(stderr, "  %s\n", problem->c_str());
    CHECK(!problem);
}

// ── P16-2, AC-33 ─────────────────────────────────────────────────────────

TEST(lib8_ac33_pen_down_move_up_one_stroke_in_the_pen_colour) {
    CE1600PCard p;
    Ce150Card c;
    for (int which = 0; which < 2; ++which) {
        AlpsPlotterMechanism& mech = which == 0 ? p.mechanism() : c.mechanism();
        calcu::paper::PaperOutput& out = which == 0 ? static_cast<calcu::paper::PaperOutput&>(p.paperOutput())
                                                    : static_cast<calcu::paper::PaperOutput&>(c.paperOutput());
        Connected con(out);
        Mechanism m(mech);
        if (which == 0) m.z6to0();
        else mech.applyPenSignals(false, true);
        m.moveX(4);
        m.feed(2);
        if (which == 0) m.z0to6();
        else mech.applyPenSignals(true, false);
        CHECK_EQ(con.paper.marks().size(), std::size_t{1});
        const calcu::paper::Stroke& s = con.stroke(0);
        CHECK(s.colour == sharp::kPenColours[0]);
        CHECK_EQ(s.path.size(), std::size_t{7});   // the place it went down, 4 moves, 2 feeds
        const calcu::paper::Coord left = out.description().printableLeft;
        CHECK(s.path.front() == (calcu::paper::Position{left, 0}));
        CHECK(s.path.back() == (calcu::paper::Position{left + 4 * kPerUnit, 2 * kPerUnit}));
    }
}

TEST(lib8_ac33_move_with_pen_up_and_feed_give_no_mark) {
    CE1600PCard p;
    Connected con(p.paperOutput());
    Mechanism m(p.mechanism());
    m.moveX(10);
    m.feed(6);
    m.moveX(-3);
    CHECK(con.paper.marks().empty());
}

TEST(lib8_ac33_feed_without_mark_grows_the_reach) {
    Ce150Card c;
    Connected con(c.paperOutput());
    Mechanism m(c.mechanism());
    m.feed(8);
    CHECK(con.paper.marks().empty());
    CHECK(!con.paper.empty());
    // The reach runs from the first place fed to (one half step) to the last
    // (the library's rule, AC-4: what a feed touched).
    CHECK(con.paper.reach() == (calcu::paper::Reach{1 * kPerUnit, 8 * kPerUnit}));
    CHECK_EQ(c.paperOutput().standing(), 8 * kPerUnit);
}

// ── P16-4, AC-34 ─────────────────────────────────────────────────────────

TEST(lib8_ac34_four_pen_colours) {
    sharp::PlotterPaperOutput out(sharp::ce1600pPaperDescription());
    calcu::paper::Paper paper(out.description());
    paper.bind(&out);
    const Colour colours[4] = {Colour::Black, Colour::Blue, Colour::Green, Colour::Red};
    for (int i = 0; i < 4; ++i) {
        out.penDown(colours[i], i, 0);
        out.penTo(i, 10);
        out.penUp();
    }
    CHECK_EQ(paper.marks().size(), std::size_t{4});
    const calcu::paper::Rgb want[4] = {{0, 0, 0}, {0, 0, 255}, {0, 128, 0}, {255, 0, 0}};
    for (std::size_t i = 0; i < 4; ++i) CHECK(std::get<calcu::paper::Stroke>(paper.marks()[i]).colour == want[i]);
    paper.bind(nullptr);
}

// ── P16-3, AC-37: the cases of shared_geometry_tests.cpp ─────────────────

TEST(lib8_ac37_places_map_to_the_panel) {
    sharp::PlotterPaperOutput out(sharp::ce1600pPaperDescription());
    calcu::paper::Paper paper(out.description());
    paper.bind(&out);
    out.penDown(Colour::Black, 0, 0);
    out.penTo(1920, 1920);   // 1920 raw steps are 190 mm
    out.penUp();
    const auto& s = std::get<calcu::paper::Stroke>(paper.marks().at(0));
    const calcu::paper::PaperDescription& d = out.description();
    CHECK(std::fabs(millimetres(d, s.path[0].x) - 10.0) < 0.001);    // (210 - 190) / 2, the margin
    CHECK(std::fabs(millimetres(d, s.path[1].x) - 200.0) < 0.001);
    CHECK(std::fabs(millimetres(d, s.path[1].y) - 190.0) < 0.001);
    paper.bind(nullptr);
}

TEST(lib8_ac37_strokes_split_by_colour) {
    sharp::PlotterPaperOutput out(sharp::ce150PaperDescription());
    calcu::paper::Paper paper(out.description());
    paper.bind(&out);
    out.penDown(Colour::Black, 0, 0);
    out.penTo(10, 0);
    out.penTo(10, 10);
    out.penUp();
    out.penDown(Colour::Red, 0, 5);
    out.penUp();
    out.penDown(Colour::Black, 20, 20);
    out.penTo(30, 20);
    out.penUp();
    CHECK_EQ(paper.marks().size(), std::size_t{3});
    CHECK(std::get<calcu::paper::Stroke>(paper.marks()[0]).path.size() == 3);
    CHECK(std::get<calcu::paper::Stroke>(paper.marks()[1]).colour == sharp::kPenColours[3]);
    CHECK(std::get<calcu::paper::Stroke>(paper.marks()[1]).path.size() == 1);   // one place is a dot
    CHECK(std::get<calcu::paper::Stroke>(paper.marks()[2]).path.size() == 2);
    CHECK(paper.reach() == (calcu::paper::Reach{0, 20 * kPerUnit}));
    const auto raw = sharp::rawPlotPoints(paper);
    CHECK_EQ(raw.size(), std::size_t{6});
    CHECK(raw[3].color == 3 && raw[3].newStroke == 1 && raw[3].y == 5);
    paper.bind(nullptr);
}

TEST(lib8_ac37_copied_picture_size) {
    sharp::PlotterPaperOutput out(sharp::ce1600pPaperDescription());
    calcu::paper::Paper paper(out.description());
    paper.bind(&out);
    out.penDown(Colour::Black, 0, 0);
    out.penTo(100, 0);
    out.penUp();
    const calcu::paper::PictureSize s = calcu::paper::pictureSize(paper);
    CHECK_EQ(s.width, 9921);   // 210 mm at 1200 dots per inch
    CHECK(std::fabs(s.dotsPerInch - 1200.0) < 1e-9);
    // A very long roll is lowered below 1200 dots per inch (AC-17).
    out.penDown(Colour::Black, 0, 200000);
    out.penUp();
    const calcu::paper::PictureSize big = calcu::paper::pictureSize(paper);
    CHECK(big.dotsPerInch < 1200.0);
    CHECK(big.height <= calcu::paper::kMaxPictureSide);
    CHECK(static_cast<std::int64_t>(big.width) * big.height <= calcu::paper::kMaxPicturePixels);
    paper.bind(nullptr);
}

} // namespace
