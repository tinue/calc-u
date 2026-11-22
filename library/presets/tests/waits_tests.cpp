// Order and waits, failure.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
using calcu::pacing::Speed;
using calcu::sessions::Hosting;

namespace {
// test-seg: the program of key 1 is `program` ticks; idle is 2 ms (200
// ticks) of key readiness, asked every 10 ticks.
constexpr Ticks kStretch = 200;
Rig rigWith(Ticks program, Hosting h = Hosting::VirtualTime) {
    RigOptions o;
    o.program = program;
    o.hosting = h;
    return Rig(o);
}
} // namespace

TEST(sections_in_file_order_header_anywhere) {
    Rig r;
    r.send("log-level", calcu::sessions::Value::object({{"level", "info"}}));   // the progress lines
    const CallId c = r.load(header() +
                            "test-memory: { at: 0, bytes: 01 }\n"
                            "keys:\n  - key: 2\n"
                            "test-memory: { at: 0, bytes: 02 }\n"
                            "keys:\n  - key: 3\n"
                            "printer: test-printer\n");   // a header key at the end
    CHECK(r.done(c));
    CHECK_EQ(int(r.machine()->memory()->bytes[0]), 2);
    bool printer = false;
    for (const auto& e : r.machine()->plugged().byPlace) printer = printer || e.peripheral == mt::kPrinter;
    CHECK(printer);   // built before the first section
    const auto log = r.logLines();
    std::string order;
    for (const auto& l : log)
        if (l.find("  line ") == 0) order += l.substr(7, 2);
    CHECK_EQ(order, std::string("3 5 6 8 "));
}

TEST(section_finished_when_written) {
    Rig r = rigWith(50'000);
    // The section after a key starts when the key's program has ended, and
    // has finished when it is written: the next key goes down at once then.
    const CallId c = r.load(header() + "keys:\n  - key: 1\ntest-memory: { at: 5, bytes: 55 }\nkeys:\n  - key: 2\n");
    CHECK(r.done(c));
    CHECK_EQ(int(r.machine()->memory()->bytes[5]), 0x55);
    const auto one = r.downs("1"), two = r.downs("2");
    CHECK(one.size() == 1 && two.size() == 1);
    CHECK(two[0] >= one[0] + 50'000 + kStretch);
    CHECK(two[0] <= one[0] + 50'000 + kStretch + 100);   // no wait after the section
}

TEST(key_finishes_when_the_program_stops) {
    Rig r = rigWith(80'000);
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - key: 2\n")));
    const auto one = r.downs("1"), two = r.downs("2");
    CHECK(one.size() == 1 && two.size() == 1);
    CHECK(two[0] >= one[0] + 80'000 + kStretch);   // the program ran to its end, then idle for the stretch
    CHECK(two[0] <= one[0] + 80'000 + kStretch + 50);
}

TEST(wait_runs_at_the_speed_in_force) {
    // Paced: 300 ms of emulated time at regular speed take about 300 ms of
    // host calls; at full speed far fewer.
    auto calls = [](bool regular) {
        Rig r = rigWith(1'000, Hosting::Paced);
        const CallId c = r.load(header() + "keys:\n" + (regular ? "  - regular-speed:\n" : "  - full-speed:\n") +
                                "  - wait: 300ms\n  - key: 2\n");
        const int n = r.callWhileBusy();
        CHECK(r.done(c));
        return n;
    };
    const int regular = calls(true), full = calls(false);
    CHECK(regular >= 12);   // 300 ms of host calls (16 ms and the clock's own step of 1 ms a reading)
    CHECK(full <= 5);
}

TEST(timeout_fails_the_item_at_both_speeds) {
    for (const char* speed : {"  - full-speed:\n", "  - regular-speed:\n"}) {
        Rig r = rigWith(200'000);   // 2 s
        const CallId c = r.load(header() + "timeout: 1\nkeys:\n" + speed + "  - key: 1\n  - key: 2\n");
        CHECK(!r.done(c));
        CHECK(hasLine({r.reason(c)}, "t.test-seg, line 6: The key: 1 has not finished after 1 s of emulated time"));
        CHECK(r.downs("2").empty());
        CHECK(r.machine()->programRuns());   // the machine as it is
    }
}

TEST(last_item_and_let_it_finish_not_bounded) {
    Rig r = rigWith(200'000);
    CHECK(r.done(r.load(header() + "timeout: 1\nkeys:\n  - key: 1\n")));   // the last item is not waited for
    CHECK(r.done(r.load(header() + "timeout: 1\nkeys:\n  - key: 1\n  - let-it-finish:\n")));
    CHECK(!r.machine()->programRuns());
    CHECK(r.ticks() >= r.downs("1")[0] + 200'000);
}

TEST(skip_wait_next_starts_first_press_at_once) {
    Rig r = rigWith(500'000);
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - skip-wait:\n  - key: 0\n  - key: 2\n")));
    const auto one = r.downs("1"), zero = r.downs("0"), two = r.downs("2");
    CHECK(one.size() == 1 && zero.size() == 1 && two.size() == 1);
    // 0 goes down while the program runs (it waits for no readiness) and stops it.
    CHECK(zero[0] < one[0] + 5'000);
    CHECK(two[0] < one[0] + 10'000);
}

TEST(skip_wait_first_or_last_refused) {
    Reading rd;
    CHECK(hasLine({rd.error(header() + "keys:\n  - skip-wait:\n  - key: 1\n")},
                  "line 4: skip-wait: cannot be the first or the last item."));
    CHECK(hasLine({rd.error(header() + "keys:\n  - key: 1\n  - skip-wait:\n")},
                  "line 5: skip-wait: cannot be the first or the last item."));
    // Across sections it is the first and last item of the preset that count.
    CHECK(rd.error(header() + "keys:\n  - key: 1\n  - skip-wait:\nkeys:\n  - key: 2\n").empty());
}

TEST(let_it_finish_last_waits_until_idle) {
    Rig r = rigWith(150'000);
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - let-it-finish:\n")));
    CHECK(r.machine()->isKeyReady());
    CHECK(r.ticks() >= r.downs("1")[0] + 150'000 + kStretch);
}

TEST(let_it_finish_elsewhere_refused) {
    Reading rd;
    CHECK(hasLine({rd.error(header() + "keys:\n  - let-it-finish:\n  - key: 1\n")},
                  "line 4: let-it-finish: can only be the last item."));
    CHECK(hasLine({rd.error(header() + "keys:\n  - key: 1\n  - let-it-finish:\nkeys:\n  - key: 2\n")},
                  "line 5: let-it-finish: can only be the last item."));
}

TEST(last_item_not_waited_for_regular_speed_after) {
    Rig r = rigWith(150'000);
    r.send("speed", Value::object({{"full", true}}));   // whatever ran before the load
    CHECK(r.busy()->speed == Speed::Full);
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n")));
    CHECK(r.machine()->programRuns());   // what it started runs on
    CHECK(r.ticks() < r.downs("1")[0] + 150'000);
    CHECK(r.busy()->speed == Speed::Regular && !r.busy()->operationRuns);
    // A failure ends at regular speed too.
    r.send("speed", Value::object({{"full", true}}));
    CHECK(!r.done(r.load(header() + "keys:\n  - key: 2\n  - expect: 9\n")));
    CHECK(r.busy()->speed == Speed::Regular);
}

TEST(failure_stops_machine_as_is_reply_notice_line) {
    Rig r;
    r.notices();
    const CallId c = r.load(header() + "keys:\n  - key: 2\n  - expect: /^9/\n  - key: 3\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 5: Expected /^9/; the display shows "));
    const auto n = r.notices();
    CHECK(n.size() == 1 && n[0] == r.reason(c));
    CHECK_EQ(r.downs("2").size(), std::size_t(1));   // the machine as it is at that moment
    CHECK(r.downs("3").empty());                      // the steps after it are not carried out
    CHECK_EQ(r.machine()->presses(), std::uint64_t(1));
}
