// LIB-6 L9: the steps of this feature (AC-20 to AC-28, AC-63, AC-66, AC-68, EC-7).
#include "PresetRig.hpp"
#include "TestHarness.hpp"
#include <filesystem>

using namespace calcu::presets;
using namespace calcu::presets::test;
using calcu::pacing::Speed;
using calcu::sessions::Hosting;

namespace {
std::string elapsedLine(const std::vector<std::string>& log) {
    for (const auto& l : log)
        if (l.rfind("Elapsed: ", 0) == 0) return l;
    return {};
}
RigOptions withProgram(Ticks program, Hosting h = Hosting::VirtualTime) {
    RigOptions o;
    o.program = program;
    o.hosting = h;
    return o;
}
} // namespace

TEST(ac20_wait_durations_in_emulated_time) {
    for (const auto& [wait, want] : std::vector<std::pair<std::string, std::string>>{
             {"2s", "Elapsed: 0:02.000"}, {"1.5s", "Elapsed: 0:01.500"}, {"500ms", "Elapsed: 0:00.500"}}) {
        Rig r;
        r.logLines();
        CHECK(r.done(r.load(header() + "keys:\n  - elapsed-time: start\n  - wait: " + wait + "\n  - elapsed-time: report\n")));
        CHECK_EQ(elapsedLine(r.logLines()), want);
    }
}

TEST(ac20_wait_without_unit_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n  - wait: 2\n"),
             std::string("t.test-seg, line 4: A duration needs a unit, as in 2s, 1.5s or 500ms: 2."));
}

TEST(ac21_wait_without_value_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n  - wait:\n  - key: 1\n"), std::string("t.test-seg, line 4: wait: needs a value."));
}

TEST(ac22_regular_speed_busy_says_so_last_counts) {
    Rig r(withProgram(30'000, Hosting::Paced));
    const CallId c = r.load(header() + "keys:\n  - full-speed:\n  - regular-speed:\n  - full-speed:\n  - regular-speed:\n"
                                       "  - key: 1\n  - key: 2\n");
    bool regularSeen = false;
    int calls = 0;
    while (r.session->busy() && calls < 10'000) {
        if (!r.downs("1").empty()) regularSeen = regularSeen || r.busy()->speed == Speed::Regular;
        r.call();
        ++calls;
    }
    CHECK(r.done(c));
    CHECK(regularSeen);   // the busy topic says that it runs at regular speed
    CHECK(calls >= 15);   // 0.3 s paced
}

TEST(ac22_ends_at_regular_speed_whatever_before) {
    Rig r;
    r.send("speed", Value::object({{"full", true}}));
    CHECK(r.done(r.load(header() + "keys:\n  - full-speed:\n  - key: 2\n")));
    CHECK(r.busy()->speed == Speed::Regular);
    CHECK(r.session->context().commandedSpeed() == Speed::Regular);
}

TEST(ac23_elapsed_start_report_format_minutes_beyond_59) {
    CHECK_EQ(elapsedText(0, 100'000), std::string("0:00.000"));
    CHECK_EQ(elapsedText(12'345, 100'000), std::string("0:00.123"));
    CHECK_EQ(elapsedText(Ticks{100'000} * 3725, 100'000), std::string("62:05.000"));   // minutes on beyond 59
    Rig r;
    r.logLines();
    CHECK(r.done(r.load(header() + "keys:\n  - wait: 1s\n  - elapsed-time: start\n  - wait: 250ms\n"
                                   "  - elapsed-time: report\n")));
    CHECK_EQ(elapsedLine(r.logLines()), std::string("Elapsed: 0:00.250"));
}

TEST(ac23_report_without_start_counts_from_build) {
    Rig r;
    r.logLines();
    CHECK(r.done(r.load(header() + "keys:\n  - wait: 1s\n  - elapsed-time: report\n")));
    // From tick 0 of the machine built: the start-up to key readiness, then 1 s.
    const std::string l = elapsedLine(r.logLines());
    CHECK(l == "Elapsed: 0:01.016" || l == "Elapsed: 0:01.017");
}

TEST(ac24_expect_after_item_finished_by_product_comparison) {
    Rig r(withProgram(60'000));
    // test-seg shows presses, (ticks / 1000) % 10, keys down: after key 1
    // and its program, one press and no key down.
    const CallId c = r.load(header() + "keys:\n  - key: 1\n  - expect: /^1\\d0$/\n");
    CHECK(r.done(c));
    CHECK(!r.machine()->programRuns());   // checked when the program had ended
}

TEST(ac24_expect_failure_reason_holds_text_and_display) {
    Rig r;
    const CallId c = r.load(header() + "keys:\n  - key: 2\n  - expect: 5\n  - key: 3\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 5: Expected 5; the display shows 1"));
}

TEST(ac66_not_expect_passes_and_fails) {
    Rig r;
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2\n  - not-expect: /^5/\n  - key: 3\n")));
    const CallId c = r.load(header() + "keys:\n  - key: 2\n  - not-expect: /^1/\n  - key: 3\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "line 5: Not expected: /^1/; the display shows 1"));
    CHECK(r.downs("3").empty());
}

TEST(ec7_expect_display_off_fails) {
    Rig r;
    const CallId c = r.load(header("test-dot") + "keys:\n  - key: off\n  - expect: A\n  - key: on\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "line 5: Expected A, but the display is off."));
    // not-expect passes on a display that is off (D-14).
    CHECK(r.done(r.load(header("test-dot") + "keys:\n  - key: off\n  - not-expect: A\n  - key: on\n")));
}

// AC-68 through the hook (Part D simplification P-6): a text the model's
// rules refuse is a read error at its line; the other model takes it.
TEST(ac68_expect_text_refused_by_the_model_rules_when_read) {
    Reading rd;
    rd.rules.seg.expectCheck = [](const ExpectText& e, std::string_view keyword) -> std::optional<std::string> {
        if (!e.regex) return std::nullopt;
        return "The test-seg takes no regular expression in " + std::string(keyword) + ".";
    };
    CHECK_EQ(rd.error(header() + "keys:\n  - key: 1\n  - expect: /^1/\n"),
             std::string("t.test-seg, line 5: The test-seg takes no regular expression in expect:."));
    CHECK(hasLine({rd.error(header() + "keys:\n  - not-expect: /^1/\n")}, "regular expression in not-expect:."));
    CHECK(rd.error(header() + "keys:\n  - expect: 1\n").empty());
    CHECK(rd.error(header("test-dot") + "keys:\n  - expect: /^1/\n").empty());
}

TEST(ac25_screenshot_into_the_folder) {
    Rig r;
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2\n  - screenshot: two.png\n  - key: 3\n")));
    const auto bytes = calcu::testing::readBytes(r.shots.path() / "two.png");
    CHECK(!bytes.empty());
    CHECK_EQ(bytes.size(), std::size_t(1 + 3 * 8));   // the test picture: on, then 3 positions of 8 segments
    CHECK_EQ(r.rules.seg.pictures, 1);
}

TEST(ac25_screenshot_kept_by_host_without_folder) {
    RigOptions o;
    o.screenshots = false;
    o.services.offered.insert(calcu::sessions::HostService::KeepFile);
    Rig r(o);
    const std::size_t mark = r.fe.log.size();
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2\n  - screenshot: two.png\n  - key: 3\n")));
    bool asked = false;
    for (const auto& m : r.fe.since(mark))
        if (const auto* a = std::get_if<calcu::sessions::ServiceAsk>(&m))
            asked = asked || (a->service == calcu::sessions::HostService::KeepFile && a->args.find("name") &&
                              *a->args.find("name")->string() == "two.png" && a->args.find("bytes")->bytes());
    CHECK(asked);
    // Neither a folder nor a host that keeps it: the step fails.
    RigOptions none;
    none.screenshots = false;
    Rig n(none);
    const CallId c = n.load(header() + "keys:\n  - screenshot: two.png\n  - key: 3\n");
    CHECK(!n.done(c));
    CHECK(hasLine({n.reason(c)}, "There is no folder for screenshots."));
}

TEST(ac25_name_empty_or_with_separator_refused) {
    Reading rd;
    for (const char* name : {"", "a/b.png", "a\\\\b.png", "../x.png"})
        CHECK(hasLine({rd.error(header() + "keys:\n  - screenshot: " + name + "\n")}, "line 4: screenshot:"));
}

TEST(ac25_unwritable_folder_fails) {
    Rig r;
    r.shots.remove("");   // nothing there
    std::filesystem::remove_all(r.shots.path());
    const CallId c = r.load(header() + "keys:\n  - screenshot: two.png\n  - key: 3\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "line 4: The screenshot two.png cannot be written to "));
    std::filesystem::create_directories(r.shots.path());   // for the folder's own clean-up
}

TEST(ac26_switch_step_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "printer: test-printer\nkeys:\n  - switch: test-printer trace on\n"),
             std::string("t.test-seg, line 5: Unknown step switch."));
}

TEST(ac27_syncclock_sets_every_clock_and_logs) {
    Rig r;
    r.logLines();
    CHECK(r.done(r.load(header("test-dot") + "keys:\n  - wait: 2s\n  - sync-clock:\n")));
    CHECK(r.machine()->clock()->setCalls >= 2);   // at the build and by the step
    const auto host = calcu::peripherals::hostDateTime(r.session->context().hostTime());
    const auto f = r.machine()->clock()->read();
    CHECK(f.hour == host.hour && f.minute == host.minute);
    CHECK(hasLine(r.logLines(), "Clock set to "));
}

TEST(ac27_syncclock_refused_where_not_applies) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n  - sync-clock:\n"),
             std::string("t.test-seg, line 4: sync-clock is not available on the test-seg."));
    CHECK(hasLine({rd.error(header("test-dot") + "keys:\n  - sync-clock: now\n")}, "sync-clock: takes no value."));
}

TEST(ac28_key_and_type_finish_at_idle) {
    Rig r(withProgram(40'000));
    // type: 12 then ENTER (clr); its 1 starts the program; the next step
    // starts when ENTER is done and the machine is idle.
    CHECK(r.done(r.load(header() + "keys:\n  - type: 12\n  - key: 3\n")));
    const auto one = r.downs("1"), three = r.downs("3"), clr = r.downs("clr");
    CHECK(one.size() == 1 && three.size() == 1 && clr.size() == 1);
    CHECK(clr[0] > one[0]);
    CHECK(three[0] >= one[0] + 40'000 + 200);
}

TEST(ac63_frontend_step_never_fails_passed_over_without_panel) {
    Rig r;
    CHECK(r.done(r.load(header() + "keys:\n  - test-panel: cpu\n  - key: 2\n")));
    CHECK(r.rules.shown.empty());   // nobody has the panel: passed over
    r.rules.panel = true;
    CHECK(r.done(r.load(header() + "keys:\n  - test-panel: cpu\n  - key: 2\n")));
    CHECK(r.rules.shown == std::vector<std::string>{"cpu"});
}
