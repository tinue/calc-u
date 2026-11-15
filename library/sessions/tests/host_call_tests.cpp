// LIB-3 L7, L8: the machine session, host calls, delivery modes (AC-1 to
// AC-4).
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;
using calcu::pacing::Speed;

namespace {
// An input queue that presses "1" once at tick 1000 (the machine session's
// SliceHook).
struct PressAt final : SliceHook {
    Ticks at = 1000;
    std::vector<Ticks> acted;
    Ticks nextDue(Ticks) const override { return acted.empty() ? at : kNever; }
    void act(calcu::machine::Machine& m, Ticks now, calcu::pacing::WallTime) override {
        acted.push_back(now);
        m.keyDown(calcu::machine::KeyId{"1"});
    }
};
} // namespace

TEST(ac1_app_session_holds_one_machine_session) {
    Rig r;
    MachineSession& ms = r.session->machineSession();
    CHECK(&ms == &r.session->context().machineSession());
    CHECK(ms.machine() == nullptr);
    r.build();
    CHECK(ms.machine() == r.m);
    CHECK(r.session->context().machine() == r.m);
    // Its input queue: a hook acts at its tick, the advance cut there.
    PressAt hook;
    ms.addHook(hook);
    for (int i = 0; i < 3; ++i) r.call();
    CHECK_EQ(hook.acted.size(), std::size_t(1));
    CHECK(!hook.acted.empty() && hook.acted[0] >= 1000 && hook.acted[0] < 1005);
    CHECK_EQ(r.m->keyEvents().size(), std::size_t(1));
    CHECK(&ms == &r.session->machineSession());
}

TEST(ac1_machine_replaced_inside_session) {
    Rig r;
    r.build(mt::kSeg);
    r.call();
    CHECK_EQ(r.part.machineChanges, 1);
    CHECK(std::holds_alternative<calcu::machine::SegmentFrame>(r.fe.state<DisplayState>(kDisplayTopic)->output.frame));
    r.build(mt::kDot);   // LIB-18 replaces machines inside the session
    r.call();
    CHECK_EQ(r.part.machineChanges, 2);
    CHECK(std::holds_alternative<calcu::machine::DotMatrixFrame>(r.fe.state<DisplayState>(kDisplayTopic)->output.frame));
    CHECK(r.ticks() > 0);
    auto taken = r.session->machineSession().takeMachine();
    r.call();
    CHECK_EQ(r.part.machineChanges, 3);
    CHECK(r.session->context().machine() == nullptr);
}

TEST(ac2_call_runs_machine_and_sends_changes) {
    Rig r;
    r.build();
    const std::size_t mark = r.fe.log.size();
    Ticks last = 0;
    for (int i = 0; i < 60; ++i) {
        r.call();
        CHECK(r.ticks() > last);   // every call runs the machine
        last = r.ticks();
    }
    // About one second of emulated time for 60 calls a frame apart.
    CHECK(r.ticks() > mt::kSegRate * 9 / 10 && r.ticks() < mt::kSegRate * 11 / 10);
    CHECK(r.fe.stateMessages(kDisplayTopic, mark) > 1);   // the digits of the tick count move
    CHECK_EQ(r.part.beforeCalls, 60);
    CHECK_EQ(r.part.afterCalls, 60);
}

TEST(ac2_without_operation_runs_at_last_commanded_speed) {
    Rig r;
    r.build();
    r.call();
    Ticks t0 = r.ticks();
    r.call();
    const Ticks regular = r.ticks() - t0;
    r.send("speed", Value::object({{"full", true}}));
    CHECK(r.fe.busy() && !r.fe.busy()->operationRuns && r.fe.busy()->speed == Speed::Full);
    t0 = r.ticks();
    r.call();
    const Ticks full = r.ticks() - t0;
    CHECK(full > 3 * regular);
    t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 > 3 * regular);   // stays until told otherwise
    r.send("speed", Value::object({{"full", false}}));
    t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 < 2 * regular);
}

TEST(ac3_no_call_no_run) {
    Rig r;
    r.build();
    r.call();
    const Ticks t = r.ticks();
    r.clock.now += std::chrono::seconds(5);   // time passes, no host call
    r.send("tp.set", Value::object({{"value", 1}}));
    r.send("speed", Value::object({{"full", true}}));
    r.ask("ticks");
    CHECK_EQ(r.ticks(), t);
}

namespace {
// One script of commands and host calls (AC-4).
std::vector<std::string> runScript(TestFrontend::Delivery d) {
    RigOptions o;
    o.delivery = d;
    o.services.offered = {HostService::AudioSink};
    Rig r(o);
    r.build(mt::kDot);
    for (int i = 0; i < 3; ++i) r.call();
    r.send("tp.set", Value::object({{"value", 2}}));
    r.send("key-down", Value::object({{"key", "a"}}));
    r.call();
    r.send("speed", Value::object({{"full", true}}));
    r.call();
    r.send("speed", Value::object({{"full", false}}));
    r.part.operation = runningFor(50'000);
    r.send("tp.op");
    r.send("tp.op");   // refused
    r.callWhileBusy();
    r.send("tp.append", Value::object({{"text", "t"}}));
    r.ask("ticks");
    r.call();
    r.fe.pump();
    return r.fe.described();
}
} // namespace

TEST(ac4_at_once_and_queued_give_same_messages_in_same_order) {
    const auto atOnce = runScript(TestFrontend::Delivery::AtOnce);
    const auto queued = runScript(TestFrontend::Delivery::Queued);
    CHECK(atOnce.size() > 20);
    CHECK_EQ(atOnce.size(), queued.size());
    CHECK(atOnce == queued);
}

// LIB-8 A-6 (design T8): a virtual-time host's advance runs the parts'
// per-call duties before its flush, as a host call does, so that what a part
// sends from afterCall (the paper's new marks) goes out with the harness's
// `wait`.
TEST(advance_unpaced_runs_parts_after_call) {
    Rig r(RigOptions{Hosting::VirtualTime});
    r.build();
    const int before = r.part.afterCalls;
    r.session->advanceUnpaced(1'000);
    CHECK_EQ(r.part.afterCalls, before + 1);
    CHECK_EQ(r.part.beforeCalls, 0);   // no host call was made
}
