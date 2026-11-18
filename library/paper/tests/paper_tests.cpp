// The paper: marks, text, reach, cut, with a printer the test drives call by call and with the test
// printers in a session.
#include "PaperTestKit.hpp"
#include "TestHarness.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace mt = calcu::machine::test;

namespace {
const Rgb kBlue{0, 0, 255};
}

TEST(marks_in_order_never_changed) {
    FakePrinter pr(mt::tp4Description());
    Paper p(mt::tp4Description());
    p.bind(&pr);
    pr.dot(u(1), u(2), 1);
    pr.down(kBlue, u(10), u(0));
    pr.to(u(11), u(1));
    const Stroke first = std::get<Stroke>(p.marks()[1]);
    pr.to(u(12), u(2));   // the stroke grows ...
    pr.up();
    pr.dot(u(3), u(4), 0);
    CHECK_EQ(p.marks().size(), std::size_t(3));   // in the order made
    CHECK(std::get<Dot>(p.marks()[0]) == (Dot{Position{u(1), u(2)}, 1}));   // a dot: position, darkness
    const Stroke& s = std::get<Stroke>(p.marks()[1]);
    CHECK(s.colour == kBlue);   // a stroke: its colour and its positions
    CHECK_EQ(s.path.size(), std::size_t(3));
    // ... and its earlier positions stay as they were given.
    CHECK(std::equal(first.path.begin(), first.path.end(), s.path.begin()));
    CHECK(std::get<Dot>(p.marks()[2]) == (Dot{Position{u(3), u(4)}, 0}));
    // A move with the pen up gives no mark.
    pr.to(u(50), u(50));
    CHECK_EQ(p.marks().size(), std::size_t(3));
}

TEST(each_line_and_each_feed_in_the_text_layer) {
    PaperRun run;
    if (!run.server) return;
    CHECK_EQ(run.line("attach printer test-dotter"), std::string("done\n"));
    run.online();
    run.press("1");
    run.line("set-control test-dotter feed on");
    run.line("set-control test-dotter feed off");
    run.press("2");
    const Paper& p = run.paperOf("TD-20");
    CHECK_EQ(p.text().size(), std::size_t(3));
    if (p.text().size() != 3) return;
    CHECK(p.text()[0] == (TextLine{"KEY 1   ", Position{u(2), 0}}));   // its characters and position
    CHECK(p.text()[1] == (TextLine{"", Position{u(2), u(7)}}));        // a feed: an empty line
    CHECK(p.text()[2] == (TextLine{"KEY 2   ", Position{u(2), u(11)}}));
    // Unicode text, as given.
    FakePrinter pr(mt::td20Description());
    Paper q(mt::td20Description());
    q.bind(&pr);
    pr.text("x̄ ∑ π", 0);
    CHECK_EQ(q.text().at(0).text, std::string("x̄ ∑ π"));
}

TEST(reach_from_least_to_greatest_mark_or_feed) {
    FakePrinter pr(mt::tp4Description());
    Paper p(mt::tp4Description());
    p.bind(&pr);
    CHECK(p.reach() == Reach{});
    pr.feedTo(u(20));   // a feed without a mark makes the paper longer
    CHECK(!p.empty());
    CHECK(p.reach() == (Reach{u(20), u(20)}));
    pr.down(kBlue, u(100), u(25));
    pr.to(u(110), u(30));
    pr.up();
    CHECK(p.reach() == (Reach{u(20), u(30)}));
    pr.feedTo(u(60));
    CHECK(p.reach() == (Reach{u(20), u(60)}));
    pr.feedTo(u(-10));   // a feed back below zero
    CHECK(p.reach() == (Reach{u(-10), u(60)}));
    // The test plotter: `feed` 20 units, `back` 30 back.
    PaperRun run;
    if (!run.server) return;
    run.line("attach aux test-plotter");
    run.line("set-control test-plotter feed on");
    run.line("set-control test-plotter back on");
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach -10 20"}));
}

TEST(cut_empties_and_next_print_starts_a_new_paper) {
    FakePrinter pr(mt::td20Description());
    Paper p(mt::td20Description());
    p.bind(&pr);
    pr.text("A", 0);
    pr.dot(u(3), u(1));
    pr.feedTo(u(7));
    p.cut();
    CHECK(p.empty());
    CHECK(p.marks().empty() && p.text().empty());
    CHECK(p.reach() == Reach{});
    pr.text("B", u(7));   // the printer prints on: y counts from where the paper stood at the cut
    pr.dot(u(3), u(8));
    CHECK(p.text().at(0) == (TextLine{"B", Position{0, 0}}));
    CHECK(std::get<Dot>(p.marks().at(0)).at == (Position{u(3), u(1)}));
}

TEST(cut_while_printing_puts_later_marks_on_the_new_paper) {
    // TD-20 in a line: rows after the cut are on the new paper.
    {
        PaperRun run;
        if (!run.server) return;
        run.line("attach printer test-dotter");
        run.online();
        CHECK_EQ(run.line("key-down 3"), std::string("done\n"));   // the line starts; a row every 20 ticks
        CHECK_EQ(run.paperOf("TD-20").text().size(), std::size_t(1));
        CHECK(run.paperOf("TD-20").marks().empty());
        CHECK_EQ(run.line("cut-paper"), std::string("done\n"));
        run.line("wait 100ms");
        run.line("key-up 3");
        const Paper& p = run.paperOf("TD-20");
        CHECK(p.text().empty());     // the line's text was on the old paper
        CHECK(!p.marks().empty());   // its rows are on the new one, from y 0 on
        if (!p.marks().empty()) CHECK_EQ(std::get<Dot>(p.marks().front()).at.y, Coord(5'000));
    }
    // TP-4 in a stroke: the new paper's stroke starts where the pen is.
    FakePrinter pr(mt::tp4Description());
    Paper p(mt::tp4Description());
    p.bind(&pr);
    pr.down(kBlue, u(100), 0);
    pr.to(u(110), u(5));
    p.cut();
    pr.to(u(120), 0);
    pr.up();
    CHECK_EQ(p.marks().size(), std::size_t(1));
    const Stroke& s = std::get<Stroke>(p.marks().at(0));
    CHECK(s.colour == kBlue);
    CHECK(s.path == (std::vector<Position>{{u(110), u(5)}, {u(120), 0}}));
}
