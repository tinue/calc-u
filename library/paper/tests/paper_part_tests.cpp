// The topics and the part, and what the paper lives through, on the
// harness's test product (TD-20 and TP-4, the startup machine control, the
// presets) with a test frontend listening where messages are checked.
#include "PaperTestKit.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestHarness.hpp"
#include "paper/PaperTopics.hpp"
#include "sessions/AppSession.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace mt = calcu::machine::test;
namespace s = calcu::sessions;
namespace machine = calcu::machine;
namespace harness = calcu::harness;

namespace {

const s::TopicName kTd{"paper TD-20"};
const s::TopicName kTp{"paper TP-4"};

// The stream messages of a topic in the log from `mark` on.
std::vector<s::StreamMessage> streams(const s::test::TestFrontend& fe, const s::TopicName& t, std::size_t mark = 0) {
    std::vector<s::StreamMessage> out;
    for (const auto& m : fe.since(mark))
        if (const auto* sm = std::get_if<s::StreamMessage>(&m); sm && sm->topic == t) out.push_back(*sm);
    return out;
}

std::size_t marksIn(const std::vector<s::StreamMessage>& ms) {
    std::size_t n = 0;
    for (const auto& m : ms)
        for (const auto& p : m.appended)
            if (const auto* a = s::payloadAs<PaperAppended>(p))
                for (const Mark& k : a->marks)
                    if (!(a->continuesStroke && &k == &a->marks.front())) ++n;
    return n;
}

s::ListenerId listen(PaperRun& run, s::test::TestFrontend& fe) { return run.server->host().session().listen(fe); }

} // namespace

TEST(one_stream_per_printer_type_plugged_or_not) {
    PaperRun run;
    if (!run.server) return;
    s::test::TestFrontend fe;
    listen(run, fe);
    // Nothing plugged in: one stream for each printer type, and the list.
    CHECK(fe.mirror.state(kPapersTopic) != nullptr);
    const auto* list = fe.state<PaperList>(kPapersTopic);
    CHECK(list && list->papers.size() == 2);
    if (list && list->papers.size() == 2) {
        CHECK(list->papers[0].owner == "TD-20" && list->papers[0].printer == mt::kDotter);
        CHECK(list->papers[1].owner == "TP-4" && list->papers[1].description == mt::tp4Description());
    }
    CHECK(run.line("state paper TD-20").rfind("done", 0) == 0);
    CHECK(run.line("state paper TP-4").rfind("done", 0) == 0);
    CHECK_EQ(run.line("state paper XY-1"), std::string("refused: There is no topic paper XY-1.\n"));
}

TEST(message_carries_appended_marks_lines_and_reach) {
    PaperRun run;
    if (!run.server) return;
    s::test::TestFrontend fe;
    listen(run, fe);
    run.line("attach printer test-dotter");
    run.online();
    const std::size_t mark = fe.log.size();
    run.press("1");
    const auto ms = streams(fe, kTd, mark);
    CHECK(!ms.empty());
    std::vector<TextLine> lines;
    std::size_t dots = 0;
    Reach reach;
    for (const auto& m : ms) {
        CHECK(!m.reset);
        for (const auto& p : m.appended)
            if (const auto* a = s::payloadAs<PaperAppended>(p)) {
                lines.insert(lines.end(), a->lines.begin(), a->lines.end());
                dots += a->marks.size();
                reach = a->reach;
            }
    }
    CHECK_EQ(lines.size(), std::size_t(1));
    if (!lines.empty()) CHECK_EQ(lines[0].text, std::string("KEY 1   "));
    CHECK_EQ(dots, run.paperOf("TD-20").marks().size());
    CHECK(reach == (Reach{0, units(7)}));   // how far the paper reaches now
    // Its value form: dots as one flat array, the reach as two integers.
    if (!ms.empty() && !ms.back().appended.empty()) {
        const s::Value v = ms.front().appended.front()->toValue();
        CHECK(v.find("marks") && v.find("lines") && v.find("reach") && v.find("continuesStroke"));
    }
}

TEST(cut_sends_the_reset) {
    PaperRun run;
    if (!run.server) return;
    s::test::TestFrontend fe;
    listen(run, fe);
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const std::size_t mark = fe.log.size();
    CHECK_EQ(run.line("cut-paper"), std::string("done\n"));
    const auto ms = streams(fe, kTd, mark);
    CHECK_EQ(ms.size(), std::size_t(1));
    if (!ms.empty()) CHECK(ms[0].reset && ms[0].appended.empty());   // the paper was emptied
}

TEST(late_listener_gets_whole_paper_once_then_appended) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.line("attach aux test-plotter");
    run.online();
    for (const char* k : {"1", "2", "3", "0"}) run.press(k);
    const std::size_t tdMarks = run.paperOf("TD-20").marks().size();
    const std::size_t tpMarks = run.paperOf("TP-4").marks().size();
    CHECK(tdMarks > 0 && tpMarks == 4);
    s::test::TestFrontend late;
    listen(run, late);
    const auto td = streams(late, kTd), tp = streams(late, kTp);
    CHECK_EQ(td.size(), std::size_t(1));   // once, the whole paper
    CHECK_EQ(tp.size(), std::size_t(1));
    if (td.size() == 1) CHECK(td[0].reset);
    CHECK_EQ(marksIn(td), tdMarks);
    CHECK_EQ(marksIn(tp), tpMarks);
    const std::size_t mark = late.log.size();
    run.press("1");   // from then on only what is appended
    const auto more = streams(late, kTd, mark);
    CHECK(!more.empty() && !more.front().reset);
    CHECK_EQ(marksIn(more), run.paperOf("TD-20").marks().size() - tdMarks);
}

TEST(same_commands_same_ticks_same_marks) {
    std::vector<std::string> replies[2];
    for (auto& r : replies) {
        PaperRun run;
        if (!run.server) return;
        for (const char* l : {"attach printer test-dotter", "attach aux test-plotter", "set-control test-dotter online on",
                              "key 1", "key-down 2", "wait 3ms", "key-up 2", "set-control test-plotter back on",
                              "key 3 0", "wait 100ms"})
            run.line(l);
        for (const char* l : {"paper marks TD-20", "paper marks TP-4", "paper text TD-20"}) r.push_back(run.line(l));
    }
    CHECK(replies[0] == replies[1]);
    CHECK(replies[0].size() == 3 && replies[0][0].size() > 100);
}

TEST(product_without_printers_has_no_paper_topic) {
    s::AppSessionSetup setup;
    setup.clock = [] { return calcu::pacing::WallTime{0}; };
    setup.hostTime = [] { return std::chrono::system_clock::time_point{}; };
    setup.hosting = s::Hosting::VirtualTime;
    s::AppSession session(std::move(setup));
    s::test::TestFrontend fe;
    const s::ListenerId id = session.listen(fe);
    CHECK(fe.mirror.state(kPapersTopic) == nullptr);
    for (const auto& m : fe.log)
        if (const auto* sm = std::get_if<s::StreamMessage>(&m)) CHECK(sm->topic.value.rfind("paper", 0) != 0);
    const s::CallId c = session.send(id, s::Command{kPaperCutCommand, s::Value()});
    CHECK(fe.reply(c) && !fe.reply(c)->done);
}

TEST(full_speed_loses_no_mark_shown_at_end_of_call) {
    PaperRun run;
    if (!run.server) return;
    s::test::TestFrontend fe;
    listen(run, fe);
    run.line("attach printer test-dotter");
    run.online();
    run.line("full-speed");
    // Four lines queued at once; all printed within one advance.
    for (const char* k : {"key-down 0", "key-down 1", "key-down 2", "key-down 3"}) run.line(k);
    const std::size_t mark = fe.log.size();
    run.line("wait 1s");
    const auto ms = streams(fe, kTd, mark);
    CHECK_EQ(ms.size(), std::size_t(1));   // shown at the end of the call in which they came
    const Paper& p = run.paperOf("TD-20");
    CHECK_EQ(p.text().size(), std::size_t(4));
    CHECK_EQ(p.reach().to, units(28));
    std::size_t all = 0;
    for (const auto& m : streams(fe, kTd)) all += marksIn({m});
    CHECK_EQ(all, p.marks().size());   // no mark lost
}

// ── What the paper lives through ────────────────────────────────────────

TEST(reset_cold_boot_power_cycle_keep_the_paper) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const auto before = run.marks("TD-20");
    for (const char* op : {"reset", "cold-boot", "power-cycle"}) {
        CHECK_EQ(run.line(op), std::string("done\n"));
        CHECK(run.marks("TD-20") == before);
    }
    run.online();
    run.press("2");   // printed on, below what was there
    const Paper& p = run.paperOf("TD-20");
    CHECK_EQ(p.text().size(), std::size_t(2));
    if (p.text().size() == 2) CHECK_EQ(p.text()[1].at.y, units(7));
}

TEST(detach_keeps_attach_again_prints_on) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const auto first = run.marks("TD-20");
    CHECK_EQ(run.line("detach printer"), std::string("done\n"));
    CHECK(run.marks("TD-20") == first);   // kept, not shown
    CHECK_EQ(run.line("paper marks"), std::string("refused: No printer is plugged in.\n"));
    run.line("attach printer test-dotter");
    run.online();
    run.press("2");
    // Another model, another place: printed on, from where the paper was left.
    run.line("model test-dot");
    run.line("attach a test-dotter");
    run.online("a");
    run.press("a");
    run.line("detach a");
    run.line("attach a test-dotter:r2");   // another variant
    run.online("a");
    run.press("b");
    const Paper& p = run.paperOf("TD-20");
    CHECK_EQ(p.text().size(), std::size_t(4));
    if (p.text().size() == 4) {
        CHECK_EQ(p.text()[1].at.y, units(7));
        CHECK_EQ(p.text()[2].at.y, units(14));
        CHECK_EQ(p.text()[3].at.y, units(21));
        CHECK_EQ(p.text()[3].text, std::string("KEY b   "));
    }
    CHECK(std::equal(first.begin() + 1, first.end(), run.marks("TD-20").begin() + 1));   // the old marks unchanged
}

TEST(machine_replaced_without_printer_keeps_paper) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.line("attach aux test-plotter");
    run.online();
    run.press("1");
    const auto td = run.marks("TD-20"), tp = run.marks("TP-4");
    CHECK_EQ(run.line("model test-dot"), std::string("done\n"));   // built bare: no printer
    CHECK(run.marks("TD-20") == td);
    CHECK(run.marks("TP-4") == tp);
    // The command without a name means the printer plugged in: refused.
    CHECK_EQ(run.line("cut-paper"), std::string("refused: No printer is plugged in.\n"));
    CHECK(run.marks("TD-20") == td);
}

TEST(user_preset_naming_printer_starts_empty_other_paper_stays) {
    calcu::testing::TempFolder t("calcu-paper-presets");
    t.write("td.test-seg", "format-version: 2\nmodel: test-seg\nprinter: test-dotter\n");
    t.write("both.test-seg", "format-version: 2\nmodel: test-seg\nprinter: test-dotter\naux: test-plotter\n");
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.line("attach aux test-plotter");
    run.online();
    run.press("1");
    const auto tp = run.marks("TP-4");
    CHECK(run.marks("TD-20").size() > 1);
    CHECK_EQ(run.line("preset " + (t / "td.test-seg")), std::string("done\n"));
    CHECK_EQ(run.marks("TD-20"), (std::vector<std::string>{"reach 0 0"}));   // empty at the start
    CHECK(run.marks("TP-4") == tp);                                           // the other paper stays
    CHECK_EQ(run.line("preset " + (t / "both.test-seg")), std::string("done\n"));
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach 0 0"}));
}

TEST(startup_preset_at_model_switch_keeps_paper) {
    calcu::testing::TempFolder t("calcu-paper-startup");
    t.write("dot.test-dot", "format-version: 2\nmodel: test-dot\na: test-dotter\n");
    t.write("settings.ini", "format-version = 1\n\n[preset]\ntest-dot = " + (t / "dot.test-dot") + "\n");
    harness::RunOptions o;
    o.settingsFile = t / "settings.ini";
    PaperRun run(o);
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const auto td = run.marks("TD-20");
    CHECK_EQ(run.line("model test-dot"), std::string("done\n"));
    CHECK(!run.machine().plugged().byPlace.empty());   // the startup preset plugged the TD-20 in
    CHECK(run.marks("TD-20") == td);                    // and the paper stays
}

TEST(new_session_starts_with_empty_papers) {
    PaperRun run;
    if (!run.server) return;
    for (const auto& [owner, p] : run.part().papers()) CHECK(p->empty());
    CHECK_EQ(run.marks("TD-20"), (std::vector<std::string>{"reach 0 0"}));
    CHECK_EQ(run.marks("TP-4"), (std::vector<std::string>{"reach 0 0"}));
}

TEST(detach_and_attach_send_nothing_for_papers) {
    PaperRun run;
    if (!run.server) return;
    s::test::TestFrontend fe;
    listen(run, fe);
    run.line("attach printer test-dotter");
    run.online();
    run.press("1");
    const std::size_t mark = fe.log.size();
    run.line("detach printer");
    run.line("wait 10ms");
    run.line("attach printer test-dotter");
    run.line("wait 10ms");
    run.line("model test-dot");
    run.line("model test-seg");
    CHECK(streams(fe, kTd, mark).empty());
    CHECK(streams(fe, kTp, mark).empty());
}

TEST(attach_in_place_keeps_marks_made_before_the_part_binds) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    // Printed before the part has seen the printer: kept by PaperOutputBase.
    const machine::PlacePath at{{machine::PlaceName{"printer"}}};
    auto* printer = run.machine().printerAt(at);
    CHECK(printer && printer->connected() == nullptr);
    run.machine().keyDown(machine::KeyId{"1"});
    run.machine().advance(200);
    run.machine().keyUp(machine::KeyId{"1"});
    CHECK(printer && printer->connected() == nullptr);
    CHECK_EQ(run.text("TD-20"), (std::vector<std::string>{"KEY 1   "}));   // handed over at the bind
    CHECK(run.marks("TD-20").size() > 1);
}

TEST(detach_in_a_line_keeps_what_was_printed) {
    PaperRun run;
    if (!run.server) return;
    run.line("attach printer test-dotter");
    run.online();
    run.line("key-down 1");
    run.server->host().advance(45);   // two rows of five
    run.line("detach printer");
    const auto marks = run.marks("TD-20");
    CHECK_EQ(run.text("TD-20"), (std::vector<std::string>{"KEY 1   "}));
    CHECK(!marks.empty() && marks.front() == "reach 0 3");   // the line ends where the head was
    bool faint = false, full = false;
    for (const auto& l : marks) {
        faint = faint || (l.rfind("dot", 0) == 0 && l.substr(l.size() - 2) == " 0");
        full = full || (l.rfind("dot", 0) == 0 && l.substr(l.size() - 2) == " 1");
    }
    CHECK(faint && full);   // the last row faint
}
