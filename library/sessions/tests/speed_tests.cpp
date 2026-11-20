// Speed and the hand-offs to the pacer.
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;
using calcu::pacing::PacingProfile;
using calcu::pacing::SliceRequest;
using calcu::pacing::Speed;
using calcu::pacing::WallTime;

namespace {
Task<OperationResult> regularThenFull(OperationContext& c) {
    c.setSpeed(Speed::Regular);
    co_await c.run(5'000);
    c.setSpeed(Speed::Full);
    co_await c.run(200'000);
    co_return OperationResult{};
}

} // namespace

TEST(speed_from_next_call) {
    Rig r;
    r.build();
    r.call();
    Ticks t0 = r.ticks();
    r.call();
    const Ticks regular = r.ticks() - t0;
    t0 = r.ticks();
    const CallId id = r.send("speed", Value::object({{"full", true}}));
    CHECK(r.fe.reply(id)->done);
    CHECK_EQ(r.ticks(), t0);   // not in the command
    r.call();
    CHECK(r.ticks() - t0 > 3 * regular);
    CHECK(!r.fe.reply(r.send("speed", Value::object({{"fast", 1}})))->done);   // malformed: refused
    CHECK(r.fe.busy()->speed == Speed::Full);
}

TEST(gesture_changes_nothing_while_operation_sets_speed) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return regularThenFull(c); };
    r.send("tp.op");
    CHECK(r.fe.busy()->speed == Speed::Regular);
    const CallId hold = r.send("speed", Value::object({{"full", true}}));   // the display held
    CHECK(r.fe.reply(hold)->done);                                          // done, nothing changes
    CHECK(r.fe.busy()->speed == Speed::Regular);
    r.call();
    Ticks t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 < 2'500);   // still the preset's regular stretch
    while (r.fe.busy()->speed == Speed::Regular && r.session->busy()) r.call();
    r.send("speed", Value::object({{"full", false}}));   // let go
    CHECK(r.fe.busy()->speed == Speed::Full);
    r.callWhileBusy();
    // After the operation: the speed commanded before it, untouched.
    CHECK(!r.fe.busy()->operationRuns && r.fe.busy()->speed == Speed::Regular);
}

TEST(waits_for_input_from_key_readiness) {
    ScriptedClock clock;
    MachineSession ms(clock.fn(), 1);
    ms.setMachine(mt::buildTestMachine(mt::kSeg), {}, 0);
    auto s = ms.runSlice(SliceRequest{10, WallTime{0}});
    CHECK(s.ticksRun >= 10);
    CHECK(!s.waitsForInput);   // the start-up of 50 ticks runs
    s = ms.runSlice(SliceRequest{100, WallTime{0}});
    CHECK(s.waitsForInput);    // key readiness
}

TEST(waits_for_input_from_an_awaited_medium) {
    ScriptedClock clock;
    MachineSession ms(clock.fn(), 1);
    auto built = mt::buildTestMachine(mt::kSeg);
    built->enableMedia();
    mt::TestMachine& m = *built;
    ms.setMachine(std::move(built), {}, 0);
    CHECK(!ms.runSlice(SliceRequest{10, WallTime{0}}).waitsForInput);
    m.mediaPlace("reader")->waits = true;   // a media place waits for a medium
    CHECK(ms.runSlice(SliceRequest{10, WallTime{0}}).waitsForInput);
}

TEST(busy_machine_does_not_wait) {
    ScriptedClock clock;
    MachineSession ms(clock.fn(), 1);
    auto built = mt::buildTestMachine(mt::kSeg);
    mt::TestMachine& m = *built;
    ms.setMachine(std::move(built), {}, 0);
    CHECK(ms.runSlice(SliceRequest{100, WallTime{0}}).waitsForInput);
    m.busy = {"the test"};   // takes keys, but something keeps it busy
    CHECK(!ms.runSlice(SliceRequest{10, WallTime{0}}).waitsForInput);
}

TEST(an_operation_is_never_paced_while_the_machine_waits) {
    // Full speed is paced while the machine waits for the user, so it does
    // not spin at the prompt; an operation (a preset, a load) runs as fast as
    // the host allows although the machine is ready.
    Rig user, op;
    for (Rig* r : {&user, &op}) {
        r->build(mt::kSeg, 0, PacingProfile{});
        r->waits(true);
    }
    user.send("speed", Value::object({{"full", true}}));
    op.part.operation = runningFor(1'000'000);
    op.send("tp.op");
    Ticks u = 0, o = 0;
    for (int i = 0; i < 4; ++i) {
        const Ticks u0 = user.ticks(), o0 = op.ticks();
        user.call();
        op.call();
        u = user.ticks() - u0;
        o = op.ticks() - o0;
    }
    CHECK(user.session->machineSession().waitsForInput());
    CHECK(op.session->machineSession().waitsForInput());
    CHECK(o > 3 * u);
}

TEST(start_afresh_on_create_continue_end_new_machine_resume) {
    Rig r;
    MachineSession& ms = r.session->machineSession();
    CHECK_EQ(ms.freshStarts(), std::uint64_t(1));   // created
    r.build();
    CHECK_EQ(ms.freshStarts(), std::uint64_t(2));   // a new machine, with its profile
    r.call();
    r.send("suspend");
    CHECK_EQ(ms.freshStarts(), std::uint64_t(2));
    r.send("continue");
    CHECK_EQ(ms.freshStarts(), std::uint64_t(3));   // continued after a suspend
    r.part.operation = runningFor(1'000);
    r.send("tp.op");
    r.callWhileBusy();
    CHECK_EQ(ms.freshStarts(), std::uint64_t(4));   // an operation ended
    ms.setPaused(true);                             // the debugger pauses
    r.call();
    ms.setPaused(false);
    CHECK_EQ(ms.freshStarts(), std::uint64_t(5));   // the debugger resumes
    // Observed: no lag after a long pause; the next call runs one frame's
    // worth, and at most one slice ahead.
    ms.setPaused(true);
    r.clock.now += std::chrono::milliseconds(80);   // under the lag limit
    ms.setPaused(false);
    const Ticks t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 < 3'500);
}
