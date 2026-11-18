// Pause-on-start and pause-at-start.
#include "DebugRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::debugger;
using namespace calcu::debugger::test;

namespace {
Value lv(const char* level) { return Value::object({{"level", level}}); }
} // namespace

TEST(armed_fires_at_rule_event_once) {
    DebugRig r;
    CHECK(r.runTo(0x01));
    CHECK(r.done(r.send("pause-on-start", lv("program"))));
    CHECK(r.runState()->armed == kProgram);
    r.advance(10'000);
    CHECK(r.paused());
    const RunState* s = r.runState();
    CHECK(s->reason == RunState::Reason::PauseOnStart && s->owner == kProgram && !s->armed);
    CHECK_EQ(r.pc(), std::uint64_t(0x08));   // after the next program step (the nop at 0x07)
    r.send("resume");
    CHECK(r.advance(10'000) >= 10'000);   // it fires once
    CHECK(!r.paused());
}

TEST(arming_one_disarms_other_off_both_pause_both) {
    DebugRig r;
    r.send("pause-on-start", lv("cpu"));
    CHECK(r.runState()->armed == kCpu);
    r.send("pause-on-start", lv("program"));
    CHECK(r.runState()->armed == kProgram);   // arming one disarms the other
    r.send("pause-on-start", Value::object({{"off", true}}));
    CHECK(!r.runState()->armed);
    r.send("pause-on-start", lv("cpu"));
    r.send("pause", lv("cpu"));
    CHECK(!r.runState()->armed);   // a pause disarms both
    const CallId c = r.send("pause-on-start", lv("basic"));
    CHECK(!r.done(c));
}

TEST(default_cpu_rule_leaves_key_readiness) {
    DebugRig r;
    CHECK(r.machine()->isKeyReady());
    r.send("pause-on-start", lv("cpu"));
    CHECK(r.advance(5'000) >= 5'000);   // still key ready: nothing fires
    CHECK(!r.paused());
    r.send("key-down", Value::object({{"key", "1"}}));   // the program starts: the processor leaves key readiness
    r.advance(1'000);
    CHECK(r.paused());
    CHECK(r.runState()->reason == RunState::Reason::PauseOnStart && r.runState()->owner == kCpu);
    CHECK(!r.machine()->isKeyReady());
}

TEST(paused_before_first_instruction_after_reset_cold_boot_build) {
    DebugRig r;
    for (const char* cmd : {"reset", "cold-boot"}) {
        r.send("pause-at-start");
        const CallId c = r.send(cmd);
        CHECK(r.paused());
        const RunState* s = r.runState();
        CHECK(s->reason == RunState::Reason::Entry && s->owner == kCpu && !s->pauseAtStart);
        CHECK_EQ(r.pc(), std::uint64_t(0));
        CHECK(r.target().history(8).empty());   // before its first instruction
        // In the harness's virtual time the restart holds there.
        CHECK_EQ(r.reason(c), std::string("The operation did not finish."));
        r.send("resume");
    }
    r.send("pause-at-start");
    r.send("model", Value::object({{"name", "test-dot"}}));   // a build
    CHECK(r.paused());
    CHECK(r.runState()->reason == RunState::Reason::Entry);
    CHECK_EQ(r.target(0).retired(), std::uint64_t(0));
    CHECK_EQ(r.target(1).retired(), std::uint64_t(0));
}

TEST(once_and_separate_from_pause_on_start) {
    DebugRig r;
    r.send("pause-at-start");
    CHECK(r.runState()->pauseAtStart && !r.runState()->armed);   // not pause-on-start
    r.send("reset");
    CHECK(r.paused());
    r.send("resume");
    CHECK(r.done(r.send("reset")));   // once
    CHECK(!r.paused());
    r.send("pause-on-start", lv("cpu"));
    CHECK(!r.runState()->pauseAtStart);   // and the other way round
}

// Calc-U-59's ASM overlay: an event outside an advance on which
// an armed start fires, with the reason pause-on-start; not when unarmed or
// armed for the other level.
TEST(fire_start_outside_an_advance) {
    DebugRig r;
    CHECK(!r.debugger.runControl().fireStart(kCpu));   // not armed
    r.send("pause-on-start", lv("program"));
    CHECK(!r.debugger.runControl().fireStart(kCpu));   // the other level
    CHECK(!r.paused());
    r.send("pause-on-start", lv("cpu"));
    CHECK(r.debugger.runControl().fireStart(kCpu));
    CHECK(r.paused());
    const RunState s = r.debugger.runControl().state();
    CHECK(s.reason == RunState::Reason::PauseOnStart && s.owner == kCpu && !s.armed);
    CHECK(!r.debugger.runControl().fireStart(kCpu));   // fired once
}
