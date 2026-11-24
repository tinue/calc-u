// The steps `- cut-paper:` and `- copy-paper:`.
#include "PaperTestKit.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestHarness.hpp"
#include "sessions/AppSession.hpp"
#include <algorithm>

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace s = calcu::sessions;

namespace {
const std::string kSeg = "format-version: 2\nmodel: test-seg\n";

// A frontend that offers the clipboard, and takes it from a step or not.
struct StepClipRun {
    PaperRun run;
    s::test::TestFrontend fe;
    explicit StepClipRun(bool inSteps) {
        if (!run.server) return;
        s::HostServices services;
        services.offered = {s::HostService::ClipboardImage, s::HostService::ClipboardText};
        services.clipboardInSteps = inSteps;
        run.server->host().session().listen(fe, services);
    }
    std::size_t asks() const {
        return static_cast<std::size_t>(std::count_if(fe.log.begin(), fe.log.end(), [](const auto& m) {
            const auto* a = std::get_if<s::ServiceAsk>(&m);
            return a && a->service == s::HostService::ClipboardImage;
        }));
    }
};

// The TP-4 plugged in by the preset, with a stroke drawn.
const std::string kDraw = "aux: test-plotter\nkeys:\n  - key: 1\n  - wait: 10ms\n";
} // namespace

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

TEST(cut_paper_copies_then_cuts_where_the_host_takes_it) {
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("cut.test-seg", kSeg + kDraw + "  - cut-paper:\n");
    StepClipRun c(true);
    if (!c.run.server) return;
    CHECK_EQ(c.run.line("preset " + (t / "cut.test-seg")), std::string("done\n"));
    CHECK_EQ(c.asks(), std::size_t(1));
    CHECK(c.run.paperOf("TP-4").empty());
}

TEST(cut_paper_only_cuts_where_the_host_does_not_copy_in_steps) {
    // The Web: the clipboard is offered, but not to a step.
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("cut.test-seg", kSeg + kDraw + "  - cut-paper:\n");
    StepClipRun c(false);
    if (!c.run.server) return;
    CHECK_EQ(c.run.line("preset " + (t / "cut.test-seg")), std::string("done\n"));
    CHECK_EQ(c.asks(), std::size_t(0));
    CHECK(c.run.paperOf("TP-4").empty());
}

TEST(copy_paper_copies_and_keeps_the_paper) {
    calcu::testing::TempFolder t("calcu-paper-step");
    t.write("copy.test-seg", kSeg + kDraw + "  - copy-paper:\n");
    StepClipRun c(true);
    if (!c.run.server) return;
    CHECK_EQ(c.run.line("preset " + (t / "copy.test-seg")), std::string("done\n"));
    CHECK_EQ(c.asks(), std::size_t(1));
    CHECK(!c.run.paperOf("TP-4").empty());
    // The step takes no value.
    t.write("value.test-seg", kSeg + "keys:\n  - copy-paper: TP-4\n");
    CHECK(c.run.line("preset " + (t / "value.test-seg")).rfind("refused: ", 0) == 0);
}

TEST(copy_paper_does_nothing_elsewhere_and_never_fails) {
    calcu::testing::TempFolder t("calcu-paper-step");
    // Not taken from a step (the Web).
    t.write("drawn.test-seg", kSeg + kDraw + "  - copy-paper:\n");
    StepClipRun web(false);
    if (!web.run.server) return;
    CHECK_EQ(web.run.line("preset " + (t / "drawn.test-seg")), std::string("done\n"));
    CHECK_EQ(web.asks(), std::size_t(0));
    CHECK(!web.run.paperOf("TP-4").empty());
    // An empty paper, and no printer: nothing asked, the steps after it run.
    StepClipRun c(true);
    if (!c.run.server) return;
    t.write("empty.test-seg", kSeg + "aux: test-plotter\nkeys:\n  - copy-paper:\n  - key: 1\n");
    CHECK_EQ(c.run.line("preset " + (t / "empty.test-seg")), std::string("done\n"));
    t.write("none.test-seg", kSeg + "keys:\n  - copy-paper:\n  - key: 1\n");
    CHECK_EQ(c.run.line("preset " + (t / "none.test-seg")), std::string("done\n"));
    CHECK_EQ(c.asks(), std::size_t(0));
}
