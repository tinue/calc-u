// The step `- cut-paper:`.
#include "PaperTestKit.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;

namespace {
const std::string kSeg = "format-version: 2\nmodel: test-seg\n";
}

TEST(cut_paper_cuts_the_plugged_printers_paper_only) {
    calcu::testing::TempFolder t("calcu-paper-step");
    // The TD-20's paper is kept from before; a preset plugs only the TP-4
    // in, draws, and cuts: the TP-4's paper is cut, the TD-20's stays.
    t.write("cut.test-seg", kSeg + "aux: test-plotter\nkeys:\n  - key: 1\n  - wait: 10ms\n  - cut-paper:\n");
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("2");
    const auto td = run.marks("TD-20");
    CHECK(td.size() > 1);
    run.line("detach printer");
    CHECK_EQ(run.line("preset " + (t / "cut.test-seg")), std::string("done\n"));
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach 0 0"}));
    CHECK(run.marks("TD-20") == td);
    // The step takes no value.
    t.write("value.test-seg", kSeg + "keys:\n  - cut-paper: TD-20\n");
    CHECK(run.line("preset " + (t / "value.test-seg")).rfind("refused: ", 0) == 0);
}

TEST(without_printer_no_effect_no_failure) {
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("cut.test-seg", kSeg + "keys:\n  - key: 1\n  - cut-paper:\n  - key: 2\n");
    PaperRun run;
    if (!run.server) return;
    CHECK_EQ(run.line("preset " + (t / "cut.test-seg")), std::string("done\n"));
    CHECK_EQ(run.machine().presses(), std::uint64_t(2));   // the steps after it ran
    for (const auto& [owner, p] : run.part().papers()) CHECK(p->empty());
}

TEST(while_paused_finishes_at_once) {
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("cut.test-seg", kSeg + "printer: test-dotter\nkeys:\n  - cut-paper:\n");
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    run.server->host().session().machineSession().setPaused(true);
    const auto before = run.machine().tickCount();
    run.part().cutPlugged(run.server->host().session().context());   // what the step carries out
    CHECK(run.paperOf("TD-20").empty());
    CHECK_EQ(run.machine().tickCount(), before);   // no emulated time
    run.server->host().session().machineSession().setPaused(false);
}

TEST(cut_paper_in_the_harness) {
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("prints.test-seg", kSeg + "aux: test-plotter\nkeys:\n  - key: 1\n  - wait: 10ms\n  - cut-paper:\n  - key: 3\n");
    PaperRun run;
    if (!run.server) return;
    CHECK_EQ(run.line("preset " + (t / "prints.test-seg")), std::string("done\n"));
    run.line("wait 100ms");
    // The stroke of key 1 was cut away; the one of key 3 is on the new paper.
    const auto m = run.marks("TP-4");
    CHECK_EQ(m.size(), std::size_t(2));
    if (m.size() == 2) CHECK(m[1].rfind("stroke #ff0000 250,0 ", 0) == 0);
}
