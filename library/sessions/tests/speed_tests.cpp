// LIB-3 L14: speed and the hand-offs to the pacer (AC-21, AC-22, AC-23).
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

Task<OperationResult> notWaiting(OperationContext& c) {
    c.setReportsWaiting(false);   // a preset `wait:` at full speed (LIB-17 AC-16)
    co_await c.run(1'000'000);
    co_return OperationResult{};
}

struct Probe final : WaitProbe {
    bool waits = false;
    bool waitsForMedium(const calcu::machine::Machine&) const override { return waits; }
};

PacingProfile pacedWhileWaiting() { return PacingProfile{std::nullopt, false, true}; }
} // namespace

TEST(ac21_speed_from_next_call) {
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

TEST(ac21_gesture_changes_nothing_while_operation_sets_speed) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return regularThenFull(c); };
    r.send("tp.op");
    CHECK(r.fe.busy()->speed == Speed::Regular);
    const CallId hold = r.send("speed", Value::object({{"full", true}}));   // the display held
    CHECK(r.fe.reply(hold)->done);                                          // D-6: done, nothing changes
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

TEST(ac22_waits_for_input_from_key_readiness) {
    ScriptedClock clock;
    MachineSession ms(clock.fn(), 1);
    ms.setMachine(mt::buildTestMachine(mt::kSeg), {}, 0);
    auto s = ms.runSlice(SliceRequest{10, WallTime{0}});
    CHECK(s.ticksRun >= 10);
    CHECK(!s.waitsForInput);   // the start-up of 50 ticks runs
    s = ms.runSlice(SliceRequest{100, WallTime{0}});
    CHECK(s.waitsForInput);    // key readiness (LIB-1 AC-42)
}

TEST(ac22_waits_for_input_from_wait_probe) {
    ScriptedClock clock;
    MachineSession ms(clock.fn(), 1);
    Probe probe;
    ms.addWaitProbe(probe);
    ms.setMachine(mt::buildTestMachine(mt::kSeg), {}, 0);
    CHECK(!ms.runSlice(SliceRequest{10, WallTime{0}}).waitsForInput);
    probe.waits = true;   // a media place waits for a medium (LIB-7)
    CHECK(ms.runSlice(SliceRequest{10, WallTime{0}}).waitsForInput);
}

TEST(ac22_operation_reports_not_waiting) {
    // Full speed paced while the machine waits (LIB-17 AC-12): an operation
    // that reports "not waiting" runs unpaced although the machine is ready.
    Rig waiting, unpaced;
    for (Rig* r : {&waiting, &unpaced}) {
        r->build(mt::kSeg, 0, pacedWhileWaiting());
        r->part.operation = runningFor(1'000'000);
    }
    unpaced.part.operation = [](OperationContext& c) { return notWaiting(c); };
    waiting.send("tp.op");
    unpaced.send("tp.op");
    CHECK(waiting.session->machineSession().waitsForInput() == false);   // not ready before the start-up
    CHECK(!unpaced.session->machineSession().waitsForInput());
    Ticks w = 0, u = 0;
    for (int i = 0; i < 4; ++i) {
        const Ticks w0 = waiting.ticks(), u0 = unpaced.ticks();
        waiting.call();
        unpaced.call();
        w = waiting.ticks() - w0;
        u = unpaced.ticks() - u0;
    }
    CHECK(waiting.session->machineSession().waitsForInput());
    CHECK(!unpaced.session->machineSession().waitsForInput());
    CHECK(u > 3 * w);
    // At its end the machine session reports waiting again.
    unpaced.send("tp.stop");
    CHECK(unpaced.session->machineSession().waitsForInput());
}

TEST(ac23_start_afresh_on_create_continue_end_new_machine_resume) {
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
    ms.setPaused(true);                             // the debugger pauses (LIB-10)
    r.call();
    ms.setPaused(false);
    CHECK_EQ(ms.freshStarts(), std::uint64_t(5));   // the debugger resumes
    // Observed: no lag after a long pause; the next call runs one frame's worth.
    ms.setPaused(true);
    r.clock.now += std::chrono::milliseconds(80);   // under the lag limit
    ms.setPaused(false);
    const Ticks t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 < 2'500);
}
