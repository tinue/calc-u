// The harness's paper commands on its test product (TD-20 and TP-4).
#include "PaperTestKit.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "paper/Picture.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

using namespace calcu::paper;
using namespace calcu::paper::test;

namespace {
bool startsWith(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }
std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
} // namespace

TEST(cut_paper_plugged_or_by_name) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.line("attach aux test-plotter");
    run.online();
    run.press("1");
    CHECK(run.marks("TD-20").size() > 1 && run.marks("TP-4").size() > 1);
    CHECK_EQ(run.line("cut-paper TP-4"), std::string("done\n"));   // by name
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach 0 0"}));
    CHECK(run.marks("TD-20").size() > 1);
    CHECK_EQ(run.line("cut-paper"), std::string("done\n"));   // the printer plugged in (first in place order)
    CHECK(run.marks("TD-20").size() == 1 || run.marks("TP-4").size() == 1);
    CHECK_EQ(run.line("cut-paper td-20"), std::string("done\n"));   // any case
    CHECK_EQ(run.marks("TD-20"), (std::vector<std::string>{"reach 0 0"}));
    CHECK_EQ(run.line("cut-paper XY-1"), std::string("refused: Unknown printer: XY-1. Printers: TD-20, TP-4.\n"));
}

TEST(paper_text_refused_without_text_layer) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    CHECK_EQ(run.line("paper text"), std::string("done 1\nKEY 1   \n"));
    CHECK_EQ(run.line("paper text TP-4"), std::string("refused: The TP-4 has no text layer.\n"));
}

TEST(paper_marks_reply) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach aux test-plotter");
    run.press("0");
    CHECK_EQ(run.line("paper marks"), std::string("done 2\nreach 0 5\nstroke #000000 100,0 110,5 120,0 130,5\n"));
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const auto m = run.marks("TD-20");
    CHECK(!m.empty() && m.front() == "reach 0 7");
    CHECK(m.size() > 1 && startsWith(m[1], "dot "));
}

TEST(paper_png_writes_the_picture) {
    calcu::testing::TempFolder t("calcu-paper-png");
    PaperRun run;
    if (!run.server) return;
    run.line("attach aux test-plotter");
    run.press("2");
    const std::string path = t / "tp4.png";
    CHECK_EQ(run.line("paper png " + path + " TP-4"), std::string("done\n"));
    CHECK(readFile(path) == picturePng(run.paperOf("TP-4")));
    const std::string json = run.server->line("paper png " + path).text;
    CHECK_EQ(json, std::string("done\n"));
    CHECK_EQ(run.line("paper png " + (t / "x.png") + " TD-20"), std::string("refused: The paper is empty.\n"));
}

TEST(paper_copy_and_cut_refused_for_empty_paper) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    CHECK_EQ(run.line("paper copy"), std::string("refused: The paper is empty.\n"));
    CHECK_EQ(run.line("paper cut"), std::string("refused: The paper is empty.\n"));
    run.online();
    run.press("1");
    // The harness offers no clipboard: a copy is refused, a cut is done.
    CHECK_EQ(run.line("paper copy"), std::string("refused: No frontend offers the clipboard.\n"));
    CHECK_EQ(run.line("paper cut TD-20"), std::string("done\n"));
    CHECK_EQ(run.marks("TD-20"), (std::vector<std::string>{"reach 0 0"}));
    CHECK(startsWith(run.line("paper"), "refused: usage: paper"));
    CHECK(startsWith(run.line("paper fold"), "refused: usage: paper"));
}

TEST(state_reads_what_was_appended) {
    PaperRun run;
    if (!run.server) return;
    run.line("state paper TD-20");   // read once: from now on what is appended
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const std::string first = run.line("state paper TD-20");
    CHECK(startsWith(first, "done "));
    CHECK(first.find("\"KEY 1   \"") != std::string::npos);
    CHECK_EQ(run.line("state paper TD-20"), std::string("done\n"));   // nothing new
    run.line("cut-paper");
    CHECK_EQ(run.line("state paper TD-20"), std::string("done 1\nreset\n"));
}

TEST(kept_paper_by_name_without_printer_refused_without_name) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    run.line("detach printer");
    CHECK_EQ(run.line("paper text TD-20"), std::string("done 1\nKEY 1   \n"));
    for (const char* l : {"paper text", "paper marks", "paper copy", "paper cut", "cut-paper"})
        CHECK_EQ(run.line(l), std::string("refused: No printer is plugged in.\n"));
}

TEST(paper_png_unwritable_path_refused_with_reason) {
    calcu::testing::TempFolder t("calcu-paper-png");
    PaperRun run;
    if (!run.server) return;
    run.line("attach aux test-plotter");
    run.press("1");
    const std::string path = t / "missing/folder/p.png";
    const std::string r = run.line("paper png " + path);
    CHECK(startsWith(r, "refused: Cannot write " + path + ": "));
    CHECK(!std::filesystem::exists(path));
}

TEST(commands_on_a_kept_paper_by_name) {
    calcu::testing::TempFolder t("calcu-paper-png");
    PaperRun run;
    if (!run.server) return;
    run.line("attach aux test-plotter");
    run.press("1");
    run.line("detach aux");
    const auto kept = run.marks("TP-4");
    CHECK(kept.size() == 2);
    CHECK_EQ(run.line("paper png " + (t / "kept.png") + " TP-4"), std::string("done\n"));
    CHECK(std::filesystem::exists(t / "kept.png"));
    CHECK_EQ(run.line("paper cut TP-4"), std::string("done\n"));   // no clipboard here: cut all the same
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach 0 0"}));
}
