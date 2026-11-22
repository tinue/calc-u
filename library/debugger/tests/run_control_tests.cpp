// The run control in an app session on the test processor
// (TestDebugParts.hpp): the gate, pause, resume and the run state; steps
// and the level that owns the pause; step over and out; operations;
// restarts and a new machine.
#include "DebugRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::debugger;
using namespace calcu::debugger::test;
using calcu::sessions::Hosting;
namespace cpu = calcu::machine::test::cpu;

namespace {

Value lv(const char* level) { return Value::object({{"level", level}}); }
Value stepArgs(const char* level, std::int64_t count, const char* kind = "in", const char* target = nullptr) {
    Value::Object o{{"level", level}, {"count", count}, {"kind", kind}};
    if (target) o.push_back({"target", target});
    return Value(std::move(o));
}

// Paused by `pause: cpu` at `address` of target 0.
void pauseAt(DebugRig& r, std::uint64_t address) {
    CHECK(r.runTo(address));
    const CallId c = r.send("pause", lv("cpu"));
    CHECK(r.done(c));
    CHECK(r.paused());
}

// ld x, #5; wait; nop; jmp 0
calcu::machine::test::TestProgram waitProgram() {
    return calcu::machine::test::TestProgram({cpu::kLdX, 5, cpu::kWait, cpu::kNop, cpu::kJmp, 0x00});
}

} // namespace

// ── The gate, pause, resume and the run state ────────────────────────────

TEST(session_has_one_run_control_unused) {
    DebugRig r;
    const RunState* s = r.runState();
    CHECK(s && !s->paused && !s->armed && !s->pauseAtStart);   // the topic is there before anything uses it
    CHECK(r.session->machineSession().runs());
    const Ticks t0 = r.ticks();
    CHECK_EQ(r.advance(1000), r.ticks() - t0);   // the gate is a plain advance
    CHECK(r.ticks() - t0 >= 1000);
}

TEST(paused_no_emulated_time_display_kept) {
    DebugRig r;
    pauseAt(r, 0x07);
    const Ticks t0 = r.ticks();
    const std::uint64_t retired = r.target().retired();
    const int displays = r.fe.stateMessages(calcu::sessions::kDisplayTopic);
    CHECK_EQ(r.advance(10'000), Ticks(0));
    CHECK_EQ(r.ticks(), t0);
    CHECK_EQ(r.target().retired(), retired);
    CHECK_EQ(r.fe.stateMessages(calcu::sessions::kDisplayTopic), displays);   // the display keeps its last picture
    bool stopped = false;   // a paused machine has every target between two instructions
    CHECK_EQ(r.session->machineSession().advance(100, stopped), Ticks(0));
    CHECK(stopped);
}

TEST(stopped_advance_gives_ticks_run) {
    DebugRig r;
    CHECK(r.runTo(0x00));
    r.breakAt(0x10);
    const Ticks t0 = r.ticks();
    const Ticks ran = r.advance(100'000);
    CHECK(ran < 100'000);   // fewer than were asked for
    CHECK_EQ(r.ticks() - t0, ran);
    CHECK(r.paused());
    CHECK_EQ(r.pc(), std::uint64_t(0x10));
}

TEST(pause_lands_where_rules_say_reply_then) {
    DebugRig r;
    CHECK(r.runTo(0x01));   // just past a boundary
    const CallId c = r.send("pause", lv("program"));
    CHECK(!r.replied(c));            // the reply comes when it has landed
    CHECK(!r.runState()->paused);    // running until then: no third state
    CHECK_EQ(r.rules.pausesAsked, 1);
    r.advance(10'000);
    CHECK(r.done(c));
    const RunState* s = r.replyState(c);
    CHECK(s && s->paused && s->owner == kProgram && s->reason == RunState::Reason::Pause);
    CHECK_EQ(r.pc(), std::uint64_t(0x08));   // after the next program step boundary (the nop at 0x07)
    // cpu lands at once.
    r.send("resume");
    const CallId d = r.send("pause", lv("cpu"));
    CHECK(r.done(d) && r.replyState(d)->owner == kCpu);
}

TEST(resume_whichever_level_owned) {
    DebugRig r;
    for (const char* level : {"program", "cpu"}) {
        const CallId c = r.send("pause", lv(level));
        r.advance(10'000);
        CHECK(r.done(c));
        CHECK(r.paused());
        const CallId d = r.send("resume");
        CHECK(r.done(d));
        CHECK(!r.paused() && !r.runState()->paused);
        CHECK(r.advance(100) >= 100);
    }
}

TEST(unknown_level_refused) {
    DebugRig r;
    const CallId c = r.send("pause", lv("basic"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("The machine has no level basic."));
    r.rules.program = false;   // a product without the level program
    const CallId d = r.send("pause", lv("program"));
    CHECK_EQ(r.reason(d), std::string("The machine has no level program."));
    CHECK(!r.paused());
}

TEST(pause_stops_all_targets_names_current) {
    DebugOptions o;
    o.model = "test-dot";
    DebugRig r(o);
    r.advance(3);   // main has run, inner is next
    CHECK(r.send("pause", lv("cpu")));
    CHECK(r.paused());
    const std::uint64_t mainRetired = r.target(0).retired(), innerRetired = r.target(1).retired();
    r.advance(10'000);
    CHECK_EQ(r.target(0).retired(), mainRetired);
    CHECK_EQ(r.target(1).retired(), innerRetired);
    const auto* d = r.machine()->debugging();
    CHECK(r.runState()->target == d->target(d->current()).info().name);
}

TEST(resume_sets_clocks_and_starts_afresh) {
    DebugOptions o;
    o.model = "test-dot";
    o.hosting = Hosting::Paced;
    DebugRig r(o);
    r.send("pause", lv("cpu"));
    CHECK(r.paused());
    for (int i = 0; i < 30; ++i) r.call();   // half a second of host time paused
    const int sets = r.machine()->clock()->setCalls;
    const auto fresh = r.session->machineSession().freshStarts();
    const Ticks t0 = r.ticks();
    r.send("resume");
    CHECK_EQ(r.machine()->clock()->setCalls, sets + 1);   // the host's time
    CHECK(r.session->machineSession().freshStarts() > fresh);   // pacing starts afresh
    r.call();
    r.call();
    // Two host calls of 16 ms at 1 MHz, not the half second of the pause made up.
    CHECK(r.ticks() - t0 < 100'000);
    CHECK(r.ticks() > t0);
}

TEST(paused_survives_suspend_and_continue) {
    DebugRig r;
    r.send("pause", lv("cpu"));
    r.send("suspend");
    r.send("continue");
    CHECK(r.paused());
    CHECK(r.runState()->paused);
    CHECK_EQ(r.advance(100), Ticks(0));
}

TEST(run_state_fields_for_each_reason) {
    DebugRig r;
    // pause
    r.send("pause", lv("cpu"));
    const RunState* s = r.runState();
    CHECK(s->paused && s->owner == kCpu && s->reason == RunState::Reason::Pause);
    CHECK(s->target == calcu::machine::DebugTargetId{"test-cpu"});
    CHECK(s->hits.empty());
    // step
    r.send("step", stepArgs("cpu", 1));
    r.advance(100);
    CHECK(r.runState()->reason == RunState::Reason::Step);
    // breakpoint, with its ID and owner
    const Value b = r.breakAt(0x10);
    const auto id = static_cast<BreakpointId>(*b.find("id")->integer());
    r.send("resume");
    r.advance(10'000);
    s = r.runState();
    CHECK(s->reason == RunState::Reason::Breakpoint);
    CHECK(s->hits.size() == 1 && s->hits[0].id == id && s->hits[0].setter == "harness");
    r.send("break-off", Value::object({{"setter", "harness"}}));
    // watch
    r.send("watch", Value::object({{"setter", "dap"}, {"target", "test-cpu"}, {"space", "memory"}, {"address", 0x30},
                                   {"count", 1}, {"access", "write"}}));
    r.send("resume");
    r.advance(10'000);
    s = r.runState();
    CHECK(s->reason == RunState::Reason::Watch && s->hits.size() == 1 && s->hits[0].setter == "dap");
    r.send("watch-off", Value::object({{"setter", "dap"}}));
    // armed, pause-on-start
    r.send("pause-on-start", lv("program"));
    CHECK(r.runState()->armed == kProgram);
    r.send("resume");
    r.advance(10'000);
    s = r.runState();
    CHECK(s->reason == RunState::Reason::PauseOnStart && s->owner == kProgram && !s->armed);
    // entry
    r.send("pause-at-start");
    CHECK(r.runState()->pauseAtStart);
    r.send("reset");
    s = r.runState();
    CHECK(s->paused && s->reason == RunState::Reason::Entry && s->owner == kCpu && !s->pauseAtStart);
}

TEST(pause_while_paused_done_nothing_changes) {
    DebugRig r;
    r.send("pause", lv("cpu"));
    const RunState before = *r.runState();
    const std::size_t mark = r.fe.log.size();
    const CallId c = r.send("pause", lv("program"));
    CHECK(r.done(c));
    CHECK(r.runState()->same(before));   // the owner stays cpu
    CHECK_EQ(r.fe.stateMessages(kRunStateTopic, mark), 0);
}

TEST(resume_while_running_done) {
    DebugRig r;
    const std::size_t mark = r.fe.log.size();
    const CallId c = r.send("resume");
    CHECK(r.done(c));
    CHECK(!r.paused());
    CHECK_EQ(r.fe.stateMessages(kRunStateTopic, mark), 0);
}

// ── Steps and the level that owns the pause ──────────────────────────────

TEST(cpu_step_one_instruction_of_named_or_current_target) {
    DebugRig r;
    pauseAt(r, 0x01);
    const CallId c = r.send("step", stepArgs("cpu", 1));
    CHECK(!r.replied(c));
    r.advance(100);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x03));
    const RunState* s = r.replyState(c);
    CHECK(s && s->paused && s->reason == RunState::Reason::Step);
    // Two targets: one step of the named target; the other runs meanwhile.
    DebugOptions o;
    o.model = "test-dot";
    DebugRig d(o);
    d.send("pause", lv("cpu"));
    const std::uint64_t main0 = d.target(0).retired(), inner0 = d.target(1).retired();
    const CallId e = d.send("step", stepArgs("cpu", 2, "in", "inner"));
    d.advance(1000);
    CHECK(d.done(e));
    CHECK_EQ(d.target(1).retired(), inner0 + 2);
    CHECK(d.target(0).retired() >= main0 + 1);
    // Without a target: the current one, the one that carries out the next instruction.
    const std::size_t current = d.machine()->debugging()->current();
    const std::uint64_t before = d.target(current).retired();
    const CallId f = d.send("step", stepArgs("cpu", 1));
    d.advance(1000);
    CHECK(d.done(f));
    CHECK_EQ(d.target(current).retired(), before + 1);
}

TEST(cpu_step_repeats_waiting_instruction_by_rule) {
    DebugOptions o;
    o.program = waitProgram();
    for (int repeats : {32, 2}) {
        DebugRig r(o);
        r.rules.repeats = repeats;
        r.send("reset");   // the program from its start
        CHECK(r.runTo(0x02));
        r.send("pause", lv("cpu"));
        r.target().writeRegister(cpu::kX, 5);
        const std::uint64_t before = r.target().retired();
        const CallId c = r.send("step", stepArgs("cpu", 1));
        r.advance(1000);
        CHECK(r.done(c));
        // x = 5: four waits stay at their address, the fifth moves on.
        if (repeats == 32) {
            CHECK_EQ(r.target().retired(), before + 5);
            CHECK_EQ(r.pc(), std::uint64_t(0x03));
        } else {
            CHECK_EQ(r.target().retired(), before + 3);   // repeated twice, then one more
            CHECK_EQ(r.pc(), std::uint64_t(0x02));
        }
    }
}

TEST(program_step_by_rule) {
    DebugRig r;
    pauseAt(r, 0x03);
    const CallId c = r.send("step", stepArgs("program", 1));
    r.advance(1000);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x08));   // after the next boundary, through the routine
    const CallId d = r.send("step", stepArgs("program", 2));
    r.advance(1000);
    CHECK(r.done(d));
    CHECK_EQ(r.pc(), std::uint64_t(0x08));   // two boundaries: 0x00, then 0x07
    CHECK(r.runState()->owner == kProgram);
}

TEST(running_while_stepping) {
    DebugRig r;
    pauseAt(r, 0x01);
    r.send("step", stepArgs("cpu", 3));
    CHECK(!r.paused());
    CHECK(!r.runState()->paused);   // stepping says running
    CHECK(r.session->machineSession().runs());
    r.advance(1000);
    CHECK(r.runState()->paused);
}

TEST(step_refused_when_not_paused) {
    DebugRig r;
    const CallId c = r.send("step", stepArgs("cpu", 1));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string(kNotPaused));
}

TEST(hit_ends_steps) {
    DebugRig r;
    pauseAt(r, 0x00);
    r.breakAt(0x12);
    const CallId c = r.send("step", stepArgs("cpu", 100));
    r.advance(10'000);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x12));
    CHECK(r.replyState(c)->reason == RunState::Reason::Breakpoint);
}

TEST(other_level_step_takes_the_pause) {
    DebugRig r;
    const CallId c = r.send("pause", lv("program"));
    r.advance(10'000);
    CHECK(r.done(c) && r.runState()->owner == kProgram);
    const CallId d = r.send("step", stepArgs("cpu", 1));
    r.advance(100);
    CHECK(r.done(d));
    CHECK(r.runState()->owner == kCpu);   // the level of the step owns the pause now
}

TEST(step_from_breakpoint_carries_it_out) {
    DebugRig r;
    CHECK(r.runTo(0x00));
    r.breakAt(0x05);
    r.advance(10'000);
    CHECK_EQ(r.pc(), std::uint64_t(0x05));
    CHECK(r.runState()->reason == RunState::Reason::Breakpoint);
    const CallId c = r.send("step", stepArgs("cpu", 1));
    r.advance(100);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x10));   // the call at the breakpoint was carried out
}

TEST(large_count_ended_by_pause_reset_new_machine) {
    DebugRig r;
    r.send("pause", lv("cpu"));
    // pause
    CallId c = r.send("step", stepArgs("cpu", 100'000));
    r.advance(500);
    CHECK(!r.replied(c));
    const CallId p = r.send("pause", lv("cpu"));
    CHECK(r.done(c) && r.done(p));
    CHECK(r.paused() && r.runState()->reason == RunState::Reason::Pause);
    // reset
    c = r.send("step", stepArgs("cpu", 100'000));
    r.advance(500);
    r.send("reset");
    CHECK(r.done(c));
    CHECK(!r.paused());
    // a new machine
    r.send("pause", lv("cpu"));
    c = r.send("step", stepArgs("cpu", 100'000));
    r.advance(500);
    r.send("model", Value::object({{"name", "test-dot"}}));
    CHECK(r.done(c));
    CHECK(!r.paused());
}

// ── Step over and out ────────────────────────────────────────────────────

TEST(step_over_call_runs_routine) {
    DebugRig r;
    pauseAt(r, 0x05);   // call 0x10
    const std::uint64_t before = r.target().retired();
    const CallId c = r.send("step", stepArgs("cpu", 1, "over"));
    r.advance(1000);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x07));
    CHECK_EQ(r.target().retired(), before + 4);   // call, ld, out, ret
    const CallId d = r.send("step", stepArgs("cpu", 1, "over"));   // not a call: one instruction
    r.advance(1000);
    CHECK(r.done(d));
    CHECK_EQ(r.pc(), std::uint64_t(0x08));
}

TEST(step_out_runs_to_return) {
    DebugRig r;
    pauseAt(r, 0x12);   // inside the routine
    const CallId c = r.send("step", stepArgs("cpu", 1, "out"));
    r.advance(1000);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x07));
}

TEST(breakpoint_inside_still_pauses) {
    DebugRig r;
    pauseAt(r, 0x05);
    r.breakAt(0x12);
    const CallId c = r.send("step", stepArgs("cpu", 1, "over"));
    r.advance(1000);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x12));
    CHECK(r.runState()->reason == RunState::Reason::Breakpoint);
}

TEST(step_over_and_out_refused_without_offer) {
    DebugOptions o;
    calcu::machine::test::TestProgram p = defaultProgram();
    p.bare = true;
    o.program = p;
    DebugRig r(o);
    r.send("pause", lv("cpu"));
    for (const char* kind : {"over", "out"}) {
        const CallId c = r.send("step", stepArgs("cpu", 1, kind));
        CHECK_EQ(r.reason(c), std::string("The target test-cpu offers no stepping over and out."));
    }
    CHECK(r.paused());
}

// ── Operations ───────────────────────────────────────────────────────────

TEST(no_breakpoint_or_watch_in_operation) {
    DebugRig r;
    r.breakAt(0x10);
    r.send("watch", Value::object({{"setter", "harness"}, {"target", "test-cpu"}, {"space", "memory"}, {"address", 0x30},
                                   {"count", 1}, {"access", "both"}}));
    // An operation of the preset engine: a wait of 10 ms runs 1000 ticks.
    const CallId l = r.load(pt::header() + "keys:\n  - wait: 10ms\n");
    CHECK(r.done(l));
    CHECK(!r.paused());
    r.advance(10'000);   // outside the operation they pause again
    CHECK(r.paused());
}

TEST(commanded_pause_step_resume_in_operation) {
    DebugRig r;
    r.send("log-level", calcu::sessions::Value::object({{"level", "info"}}));   // the progress lines
    const CallId l = r.load(pt::header() + "keys:\n  - pause: cpu\n  - step: { level: cpu, count: 3 }\n  - resume:\n");
    CHECK(r.done(l));
    const auto log = r.logLines();
    CHECK(pt::hasLine(log, "pause: cpu: done"));
    CHECK(pt::hasLine(log, "step: cpu 3: done"));
    CHECK(pt::hasLine(log, "resume: done"));
    CHECK(!r.paused());
    CHECK(r.runState()->reason == std::nullopt);
}

TEST(armed_start_fires_in_operation) {
    // The library form of test_cdebug.ti59: armed inside the
    // load, fired when key 1 starts the program, inside the key's hold.
    DebugOptions o;
    o.hosting = Hosting::Paced;
    DebugRig r(o);
    const CallId l = r.load(pt::header() + "keys:\n  - pause-on-start: cpu\n  - key: 1\n  - skip-wait:\n  - wait: 100ms\n");
    r.callWhileBusy(10'000);
    CHECK(r.done(l));
    CHECK(r.paused());
    const RunState* s = r.runState();
    CHECK(s->reason == RunState::Reason::PauseOnStart && s->owner == kCpu && !s->armed);
    // The key went down before the pause and came up on the paused machine.
    const auto& ev = r.machine()->keyEvents();
    CHECK(ev.size() >= 2);
    CHECK(ev[ev.size() - 2].down && !ev.back().down);
    CHECK_EQ(ev.back().tick, r.ticks());
    CHECK(ev[ev.size() - 2].tick < r.ticks());
}

TEST(preset_ends_paused_stays_paused) {
    DebugRig r;
    const CallId l = r.load(pt::header() + "keys:\n  - pause: cpu\n");
    CHECK(r.done(l));
    CHECK(r.paused());
    CHECK(!r.session->busy());
}

// ── Restarts and a new machine ───────────────────────────────────────────

TEST(reset_and_cold_boot_while_paused_run) {
    DebugRig r;
    for (const char* cmd : {"reset", "cold-boot"}) {
        r.send("pause", lv("cpu"));
        CHECK(r.paused());
        const CallId c = r.send(cmd);
        CHECK(r.done(c));
        CHECK(!r.paused() && !r.runState()->paused);
        CHECK(r.advance(100) >= 100);
    }
}

TEST(armed_stays_across_reset) {
    DebugRig r;
    r.send("pause-on-start", lv("cpu"));
    r.send("pause", lv("cpu"));   // a pause disarms...
    CHECK(!r.runState()->armed);
    r.send("pause-on-start", lv("cpu"));   // ...armed again while paused
    const CallId c = r.send("reset");
    CHECK(r.done(c));
    CHECK(!r.paused());
    CHECK(r.runState()->armed == kCpu);   // an armed start stays armed
    r.send("key-down", Value::object({{"key", "1"}}));   // the program leaves key readiness
    r.advance(100);
    CHECK(r.paused() && r.runState()->reason == RunState::Reason::PauseOnStart);
}

TEST(new_machine_not_paused_nothing_armed) {
    DebugRig r;
    r.send("pause-on-start", lv("program"));
    r.send("pause", lv("cpu"));
    r.send("pause-on-start", lv("cpu"));
    CHECK(r.paused());
    const CallId c = r.send("model", Value::object({{"name", "test-dot"}}));
    CHECK(r.done(c));
    CHECK(!r.paused());
    const RunState* s = r.runState();
    CHECK(!s->paused && !s->armed);
}

TEST(breakpoints_rebound_by_target_name_rest_dropped) {
    DebugOptions o;
    o.model = "test-dot";
    DebugRig r(o);
    r.breakAt(0x10, "main");
    r.breakAt(0x50, "inner");
    CHECK_EQ(r.debugger.runControl().table().breakpoints().size(), std::size_t(2));
    CHECK(r.done(r.send("variant", Value::object({{"name", "V2"}}))));   // a new machine, the same targets
    CHECK_EQ(r.debugger.runControl().table().breakpoints().size(), std::size_t(2));
    r.advance(10'000);
    CHECK(r.paused() && r.runState()->reason == RunState::Reason::Breakpoint);   // set again on the new targets
    CHECK(r.done(r.send("model", Value::object({{"name", "test-seg"}}))));   // main and inner are gone
    CHECK(r.debugger.runControl().table().breakpoints().empty());
}

// ── A step with a test ───────────────────────────────────────────────────

TEST(step_until_goes_on_while_the_test_says_so) {
    DebugRig r;
    pauseAt(r, 0x00);
    // On while the address is below 0x07, up to 100 steps: ends at the first
    // instruction at or past it.
    int asked = 0;
    const auto res = r.debugger.runControl().step(kCpu, 100, std::nullopt, StepKind::In, 0,
                                                  [&asked](const calcu::machine::DebugTarget& t) {
                                                      ++asked;
                                                      return t.nextAddress() < 0x07;
                                                  });
    CHECK(res.ok && res.deferred);
    r.advance(10'000);
    CHECK(r.paused());
    CHECK(r.runState()->reason == RunState::Reason::Step);
    CHECK(r.pc() >= 0x07);
    CHECK(asked > 1);
    // Without a test the count ends it, as before.
    const auto one = r.debugger.runControl().step(kCpu, 1, std::nullopt, StepKind::In);
    CHECK(one.ok);
    const std::uint64_t before = r.target().retired();
    r.advance(1000);
    CHECK_EQ(r.target().retired(), before + 1);
}

TEST(step_until_ends_at_a_breakpoint) {
    DebugRig r;
    pauseAt(r, 0x00);
    r.breakAt(0x12);
    const auto res = r.debugger.runControl().step(kCpu, 100'000, std::nullopt, StepKind::Over, 0,
                                                  [](const calcu::machine::DebugTarget&) { return true; });
    CHECK(res.ok);
    r.advance(100'000);
    CHECK(r.paused());
    CHECK_EQ(r.pc(), std::uint64_t(0x12));
    CHECK(r.runState()->reason == RunState::Reason::Breakpoint);
}
