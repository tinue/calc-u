// A paused machine. The pause comes from the test rules' step after the
// build (MachineSession::setPaused, the debugger's place); the debugger
// steps themselves are the debugger's.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
using calcu::sessions::Hosting;
using calcu::sessions::Task;

namespace {
RigOptions opts(Hosting h, Ticks program = 50'000) {
    RigOptions o;
    o.hosting = h;
    o.program = program;
    return o;
}
void pauseAfterBuild(Rig& r) {
    r.rules.seg.after = [](RunContext& run) -> Task<ItemResult> {
        run.session().machineSession().setPaused(true);
        co_return ItemResult{};
    };
}
bool paused(Rig& r) { return r.session->machineSession().paused(); }
} // namespace

TEST(key_waits_until_running_frontend_step_goes_on) {
    Rig r(opts(Hosting::Paced));
    r.rules.panel = true;
    pauseAfterBuild(r);
    const CallId c = r.load(header() + "keys:\n  - test-panel: one\n  - key: 2\n  - test-panel: two\n");
    for (int i = 0; i < 20; ++i) r.call();
    CHECK(r.session->busy());
    CHECK(r.rules.shown == std::vector<std::string>{"one"});   // a frontend step goes on
    CHECK(r.downs("2").empty());                               // a key waits until the machine runs
    r.session->machineSession().setPaused(false);
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK((r.rules.shown == std::vector<std::string>{"one", "two"}));
    CHECK_EQ(r.downs("2").size(), std::size_t(1));
}

TEST(wait_passes_in_host_time_while_paused) {
    Rig r(opts(Hosting::Paced));
    r.rules.panel = true;
    pauseAfterBuild(r);
    const CallId c = r.load(header() + "keys:\n  - wait: 200ms\n  - test-panel: after\n");
    while (r.session->busy() && !paused(r)) r.call();   // built, at key readiness, paused
    const auto start = r.clock.now;
    const Ticks ticks = r.ticks();
    while (r.session->busy() && r.clock.now - start < std::chrono::milliseconds(150)) r.call();
    CHECK(r.rules.shown.empty());
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK(r.rules.shown == std::vector<std::string>{"after"});
    CHECK(r.clock.now - start >= std::chrono::milliseconds(200));
    CHECK_EQ(r.ticks(), ticks);   // no emulated time passed
    CHECK(paused(r));             // it ends with the machine still paused
}

TEST(wait_ends_at_once_in_virtual_time) {
    Rig r(opts(Hosting::VirtualTime));
    r.rules.panel = true;
    pauseAfterBuild(r);
    const CallId c = r.load(header() + "keys:\n  - wait: 5s\n  - test-panel: after\n");
    CHECK(r.done(c));
    CHECK(r.rules.shown == std::vector<std::string>{"after"});
    CHECK(r.ticks() < 10'000);   // not 5 s
    CHECK(paused(r));
}

TEST(implicit_wait_and_timeout_stand_still_while_paused) {
    Rig r(opts(Hosting::Paced, 50'000));   // 0.5 s
    r.rules.panel = true;
    const CallId c = r.load(header() + "timeout: 1\nkeys:\n  - regular-speed:\n  - key: 1\n  - test-panel: x\n");
    while (r.session->busy() && r.downs("1").empty()) r.call();
    for (int i = 0; i < 5; ++i) r.call();
    CHECK(r.machine()->programRuns());
    r.session->machineSession().setPaused(true);
    const Ticks ticks = r.ticks();
    for (int i = 0; i < 200; ++i) r.call();   // more than 3 s of host time
    CHECK(r.session->busy());
    CHECK(r.rules.shown.empty());   // a paused machine is not idle
    CHECK_EQ(r.ticks(), ticks);     // and the timeout stands still
    r.session->machineSession().setPaused(false);
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK(r.rules.shown == std::vector<std::string>{"x"});
}

TEST(preset_that_ends_paused_stays_paused) {
    Rig r(opts(Hosting::VirtualTime));
    r.rules.panel = true;
    pauseAfterBuild(r);
    const CallId c = r.load(header() + "keys:\n  - test-panel: x\n");
    CHECK(r.done(c));
    CHECK(paused(r));
    CHECK(!r.busy()->operationRuns);
}
