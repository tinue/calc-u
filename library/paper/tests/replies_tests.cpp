// LIB-8 T4, T12 (design §6): the replies of `paper text` and `paper marks`
// (AC-23, EC-2) and their reading back (AC-39).
#include "PaperTestKit.hpp"
#include "TestHarness.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace mt = calcu::machine::test;

TEST(ac23_numbers_whole_without_point_else_four_decimals) {
    CHECK_EQ(numberText(units(7)), std::string("7"));
    CHECK_EQ(numberText(65'300), std::string("6.53"));
    CHECK_EQ(numberText(units(-2)), std::string("-2"));
    CHECK_EQ(numberText(669'474), std::string("66.9474"));
    CHECK_EQ(numberText(0), std::string("0"));
    CHECK_EQ(numberText(-5'000), std::string("-0.5"));
    CHECK_EQ(numberText(1), std::string("0.0001"));
    for (Coord c : {Coord(0), Coord(7), Coord(-5'000), Coord(669'474), Coord(-123'456'789)})
        CHECK(parseNumberText(numberText(c)) == std::optional<Coord>(c));
    CHECK(!parseNumberText("1.23456"));
    CHECK(!parseNumberText("1."));
    CHECK(!parseNumberText("abc"));
}

TEST(ac23_marks_reply_reach_then_dots_and_strokes) {
    FakePrinter pr(mt::tp4Description());
    Paper p(mt::tp4Description());
    p.bind(&pr);
    pr.dot(u(1.5), u(2), 1);
    pr.down(Rgb{0, 128, 0}, u(100), u(-3));
    pr.to(u(110.25), u(2));
    pr.up();
    pr.feedTo(u(9));
    CHECK_EQ(marksReply(p), (std::vector<std::string>{"reach -3 9", "dot 1.5 2 1", "stroke #008000 100,-3 110.25,2"}));
}

TEST(ac23_text_reply_one_line_per_text_line) {
    FakePrinter pr(mt::td20Description());
    Paper p(mt::td20Description());
    p.bind(&pr);
    pr.text("  1.5   ", 0);
    pr.text("", u(7));
    pr.text("x̄", u(11));
    CHECK_EQ(textReply(p), (std::vector<std::string>{"  1.5   ", "", "x̄"}));
}

TEST(ec2_empty_paper_replies_reach_0_0_and_no_lines) {
    Paper p(mt::td20Description());
    CHECK(textReply(p).empty());
    CHECK_EQ(marksReply(p), (std::vector<std::string>{"reach 0 0"}));
    PaperRun run;
    if (!run.server) return;
    CHECK_EQ(run.line("paper text TD-20"), std::string("done\n"));
    CHECK_EQ(run.line("paper marks TD-20"), std::string("done 1\nreach 0 0\n"));
}

TEST(ac39_marks_reply_read_and_written_unchanged) {
    for (const char* name : {"pc100c.marks", "ce150.marks", "ce1600p.marks"}) {
        const std::vector<std::string> lines = fixtureLines(name);
        CHECK(lines.size() >= 2);
        std::string error;
        const auto read = readMarksReply(lines, &error);
        CHECK_EQ(error, std::string());
        if (!read) continue;
        CHECK(marksReply(read->reach, read->marks) == lines);   // the form is fixed (OQ-4)
    }
    std::string error;
    CHECK(!readMarksReply({"reach 0 0", "dot 1 2"}, &error));
    CHECK_EQ(error, std::string("line 2: not a dot or a stroke"));
}
