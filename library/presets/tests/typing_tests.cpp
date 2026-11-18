// Typing through the preset loader: the first press after `skip-wait:`, key
// names checked when read, and when a key step finishes.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;

TEST(first_press_after_skip_wait_at_once) {
    RigOptions o;
    o.program = 300'000;   // 3 s: the machine is not ready meanwhile
    Rig r(o);
    // The rig's profiles wait for key readiness before a press ...
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - skip-wait:\n  - key: 2\n  - key: 0\n")));
    const auto one = r.downs("1"), two = r.downs("2");
    CHECK(one.size() == 1 && two.size() == 1);
    CHECK(two[0] < one[0] + 5'000);   // ... but right after skip-wait: the first press goes down at once
    // Its later presses wait as usual.
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - skip-wait:\n  - key: 2 3\n  - key: 0\n")));
    const auto one2 = r.downs("1"), three = r.downs("3");
    CHECK(three.size() == 1 && three[0] >= one2[0] + 300'000);
    // Without skip-wait the step waits for the machine to be idle first.
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - key: 2\n")));
    CHECK(r.downs("2")[0] >= r.downs("1")[0] + 300'000);
}

TEST(key_name_refused_when_read_before_build) {
    Rig r;
    mt::TestMachine* before = r.machine();
    const int builds = r.machines.builder_.inner.builds;
    const CallId c = r.load(header() + "printer: test-printer\nkeys:\n  - key: 2\n  - key: 2 enter\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 6: enter is not a key of the test-seg."));
    CHECK(r.machine() == before);   // before anything is built
    CHECK_EQ(r.machines.builder_.inner.builds, builds);
    CHECK(r.downs("2").empty());
    // Other spellings of the descriptor are names too.
    CHECK(r.done(r.load(header() + "keys:\n  - key: CLEAR 2\n")));
    CHECK_EQ(r.downs("clr").size(), std::size_t(1));
}

TEST(key_step_finishes_at_idle_command_at_key_up) {
    RigOptions o;
    o.program = 100'000;
    Rig r(o);
    // The step has finished only when the machine is idle as well.
    CHECK(r.done(r.load(header() + "keys:\n  - key: 1\n  - key: 2\n")));
    CHECK(r.downs("2")[0] >= r.downs("1")[0] + 100'000);
    // The command `key` replies when the last key is up, not when idle.
    Value::Array names{Value("1")};
    const CallId k = r.send("key", Value::object({{"names", Value(std::move(names))}}));
    for (int i = 0; i < 1'000 && !r.replied(k); ++i) r.session->advanceUnpaced(100);
    CHECK(r.done(k));
    CHECK(r.machine()->programRuns());
}
