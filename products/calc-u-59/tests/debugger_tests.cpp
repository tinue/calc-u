// LIB-10's Calc-U-59 column (design §12.5, §12.8: E10-2 to E10-5, E10-8):
// every pause, step and pause-on-start of a TI machine goes through the
// library's run control with the levels `cpu` and `program`
// (app/TILevelRules); a breakpoint set through the session pauses the app
// session (AC-31); the KEYSTROKES directives of a 2.3.0 preset are the run
// control's commands (adapter L-E1, AC-32). Ports 2.3.0's freeze tests
// (shared_runtime_tests.cpp: FreezeController, the EmulatorSession's queued
// steps) onto the run control.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include "../app/TILevelRules.hpp"
#include "../core/TIDebugging.hpp"
#include "../core/TIRomRanges.hpp"

using namespace cu59;
using namespace std::chrono_literals;
using calcu::debugger::kCpu;
using calcu::debugger::kProgram;
using calcu::debugger::RunState;
using calcu::machine::InstructionEvent;
using calcu::sessions::Value;
using testroms::SessionRig;

namespace {

calcu::debugger::RunControl& rc(SessionRig& r) { return r.part().debuggerPart()->runControl(); }
bool paused(SessionRig& r) { return r.part().isFrozen(); }
bool ready(SessionRig& r) { return r.until([&] { return r.machine().isKeyReady(); }); }
cu59::CalculatorState& panel(SessionRig& r) { return testroms::dbg(r.machine()); }

// A program in program memory from step 000 (keycodes).
void program(SessionRig& r, std::vector<uint8_t> codes) {
    testroms::mem(r.machine()).writeProgram(codes.data(), static_cast<int>(codes.size()));
}
const std::vector<uint8_t> kLoop = {76, 11, 85, 1, 95, 61, 0, 0};   // Lbl A + 1 = GTO 000

InstructionEvent event(std::uint32_t marks, std::uint64_t at = 0x100, std::uint64_t next = 0x101) {
    InstructionEvent e;
    e.address = at;
    e.nextAddress = next;
    e.marks = marks;
    return e;
}
constexpr std::uint32_t kIdle = 4u << timarks::kMarkWeightShift;   // an IDLE instruction, weight 4
constexpr std::uint32_t kActive = 1u << timarks::kMarkWeightShift;

} // namespace

// ── E10-2: levels (AC-29) ─────────────────────────────────────────────────

TEST(lib10_ac29_pause_lands_at_the_next_boundary) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.tap(11);   // A: the program runs
    const auto res = rc(r).pause(kProgram);
    CHECK(res.ok && res.deferred);   // it lands at the next program step boundary
    CHECK(r.until([&] { return paused(r); }, 2s));
    CHECK(r.part().runState().owner == kProgram && r.part().runState().reason == RunState::Reason::Pause);
    CHECK(panel(r).lastStepBoundary().count > 0 && panel(r).lastStepBoundary().cpu.pc == panel(r).pc());
}

TEST(lib10_ac29_waiting_for_a_key_lands_after_50000) {
    SessionRig r;
    CHECK(ready(r));
    r.runFor(200ms);
    const auto before = r.ticks();
    rc(r).pause(kCpu);
    CHECK(r.until([&] { return paused(r); }, 20s));
    const auto ran = (r.ticks() - before) / 32;   // weights: IDLE instructions count 4
    CHECK(ran >= TILevelRules::kMaxWeights && ran < TILevelRules::kMaxWeights + 4);
    CHECK(r.machine().isKeyReady());
}

TEST(lib10_ac29_in_an_overlay_lands_after_50000) {
    // An ASM overlay has no program step boundaries: only the count lands it.
    TILevelRules rules;
    auto m = testroms::makeMachine(Model::TI59);
    rules.pauseAsked(kCpu, *m);
    int n = 0;
    while (!rules.pauseLandsAfter(kCpu, event(kActive))) ++n;
    CHECK_EQ(n + 1, int(TILevelRules::kMaxWeights));
    rules.pauseAsked(kProgram, *m);
    n = 0;
    while (!rules.pauseLandsAfter(kProgram, event(kIdle))) ++n;
    CHECK_EQ(n + 1, int(TILevelRules::kMaxWeights / 4));
    CHECK(rules.pauseLandsAfter(kProgram, event(timarks::kMarkBoundary | kActive)));   // a boundary lands at once
}

TEST(lib10_ac29_program_step_is_one_program_byte_as_sst) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.tap(11);
    rc(r).pause(kProgram);
    CHECK(r.until([&] { return paused(r); }, 2s));
    for (int i = 0; i < 4; ++i) {
        const auto count = panel(r).lastStepBoundary().count;
        CHECK(rc(r).step(kProgram, 1, std::nullopt, calcu::debugger::StepKind::In).deferred);
        CHECK(r.until([&] { return paused(r); }, 2s));
        CHECK_EQ(panel(r).lastStepBoundary().count, count + 1);   // one program byte
        CHECK(r.part().runState().reason == RunState::Reason::Step);
    }
    // A count: so many bytes in one step.
    const auto count = panel(r).lastStepBoundary().count;
    rc(r).step(kProgram, 3, std::nullopt, calcu::debugger::StepKind::In);
    CHECK(r.until([&] { return paused(r); }, 2s));
    CHECK_EQ(panel(r).lastStepBoundary().count, count + 3);
}

TEST(lib10_ac29_program_step_through_rom_and_module_programs) {
    SessionRig r;
    CHECK(ready(r));
    for (int k : {21, 31, 92, 82, 71}) r.tap(k);   // 2nd Pgm 01 SBR: the Master Library's program 01
    r.post(calcu::typing::kScreenKeyDown, Value::object({{"key", "="}, {"pointer", 1}}));
    rc(r).pause(kProgram);
    CHECK(r.until([&] { return paused(r); }, 5s));
    r.post(calcu::typing::kScreenKeyUp, Value::object({{"key", "="}, {"pointer", 1}}));
    for (int i = 0; i < 5; ++i) {
        const auto count = panel(r).lastStepBoundary().count;
        rc(r).step(kProgram, 1, std::nullopt, calcu::debugger::StepKind::In);
        CHECK(r.until([&] { return paused(r); }, 2s));
        CHECK_EQ(panel(r).lastStepBoundary().count, count + 1);
        CHECK_EQ(panel(r).lastStepBoundary().cpu.pc, panel(r).pc());
    }
}

TEST(lib10_ac29_cpu_step_repeats_a_hold_up_to_32) {
    TILevelRules rules;
    CHECK_EQ(rules.cpuStepRepeats(), 32);
    SessionRig r;
    CHECK(ready(r));
    rc(r).pause(kCpu);
    CHECK(r.until([&] { return paused(r); }, 20s));
    const auto& t = r.machine().debugging()->target(0);
    for (int i = 0; i < 20; ++i) {
        const auto before = t.retired();
        const auto at = t.nextAddress();
        rc(r).step(kCpu, 1, std::nullopt, calcu::debugger::StepKind::In);
        CHECK(r.until([&] { return paused(r); }, 2s));
        const auto ran = t.retired() - before;
        CHECK(ran >= 1 && ran <= 33);
        CHECK(t.nextAddress() != at || ran == 33);   // a hold is stepped through
    }
}

// ── E10-3: pause-on-start (AC-30) ─────────────────────────────────────────

TEST(lib10_ac30_program_fires_at_the_next_boundary) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    rc(r).pauseOnStart(kProgram);
    r.runFor(300ms);
    CHECK(!paused(r));   // idle: no program step
    r.tap(11);
    CHECK(r.until([&] { return paused(r); }, 2s));
    const RunState s = r.part().runState();
    CHECK(s.owner == kProgram && s.reason == RunState::Reason::PauseOnStart && !s.armed);
    CHECK_EQ(panel(r).lastStepBoundary().cpu.pc, panel(r).pc());
}

// (was freeze_on_scan_loop_exit_fires_on_key)
TEST(lib10_ac30_cpu_fires_leaving_key_readiness) {
    SessionRig r;
    CHECK(ready(r));
    rc(r).pauseOnStart(kCpu);
    r.runFor(300ms);
    CHECK(!paused(r));   // still scanning
    r.machine().keyDown(calcu::machine::KeyId{"5"});   // raw: the start, not typing
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.machine().keyUp(calcu::machine::KeyId{"5"});
    const RunState s = r.part().runState();
    CHECK(s.owner == kCpu && s.reason == RunState::Reason::PauseOnStart && !s.armed);
    CHECK(!r.machine().isKeyReady());
}

// Row 59-34 (Q-1): a `cpu` start fires at the ROM's start-up after a Reset
// and a Cold boot on every model (the session case is part_flows_tests.cpp's
// lib10_ac30_part_cpu_freeze_on_start_sees_reset_and_clean_reset); here the
// rule: 0000 next while it was not 0000 at arming, and what was seen stays
// when the run control arms again at the restart.
TEST(lib10_ac30_cpu_fires_at_0000_after_reset_and_cold_boot) {
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        SessionRig r(model);
        CHECK(ready(r));
        rc(r).pauseOnStart(kCpu);
        r.runFor(50ms);
        r.post(calcu::startup::kResetCommand);
        CHECK(r.until([&] { return paused(r); }, 3s));
        CHECK(r.part().runState().reason == RunState::Reason::PauseOnStart);
        rc(r).resume();
        CHECK(ready(r));
        rc(r).pauseOnStart(kCpu);
        r.runFor(50ms);
        r.post(calcu::startup::kColdBootCommand);
        CHECK(r.until([&] { return paused(r); }, 3s));
        rc(r).resume();
    }
    TILevelRules rules;
    auto m = testroms::makeMachine(Model::TI59);
    testroms::run(*m, 10);   // not at 0000
    rules.startArmed(kCpu, *m);
    CHECK(!rules.startFiresAfter(kCpu, event(kActive, 0x10, 0x11), *m));
    CHECK(rules.startFiresAfter(kCpu, event(kActive, 0x10, 0x0000), *m));
}

// (was freeze_on_pc_change_and_mutual_exclusion)
TEST(lib10_ac30_arming_one_disarms_the_other) {
    SessionRig r;
    CHECK(ready(r));
    rc(r).pauseOnStart(kCpu);
    rc(r).pauseOnStart(kProgram);
    CHECK(r.part().runState().armed == kProgram);
    rc(r).pauseOnStart(kCpu);
    CHECK(r.part().runState().armed == kCpu);
    rc(r).pauseOnStart(std::nullopt);
    CHECK(!r.part().runState().armed);
    rc(r).pauseOnStart(kProgram);
    rc(r).pause(kCpu);   // a pause disarms it (AC-9)
    CHECK(!r.part().runState().armed);
}

// The TI-58's idle pass detours through the card veto: it is part of the
// scan loop, so a `cpu` start waits for a key (was
// ti58_card_veto_is_part_of_the_scan_loop).
TEST(lib10_ac30_ti58_card_veto_is_part_of_the_scan_loop) {
    auto m = testroms::makeMachine(Model::TI58);
    CHECK(testroms::runToScanLoop(*m, Model::TI58));
    bool sawVeto = false, left = false;
    for (int i = 0; i < 20'000; ++i) {
        m->advance(1);
        sawVeto = sawVeto || testroms::dbg(*m).pc() == kCardVetoLo;
        left = left || !m->isKeyReady();
    }
    CHECK(sawVeto && !left);
    CHECK(!romRanges(MachineVariant::TI59).scanLoop.contains(kCardVetoLo));   // TI-59: card inserted
    SessionRig r(Model::TI58);
    CHECK(ready(r));
    rc(r).pauseOnStart(kCpu);
    r.runFor(1s);
    CHECK(!paused(r));
    r.machine().keyDown(calcu::machine::KeyId{"5"});
    CHECK(r.until([&] { return paused(r); }, 2s));
}

// The start lands on the same instruction as 2.3.0's run loop, which ran one
// instruction at a time and asked FreezeController after each (was
// c59_1_runbatch_stops_at_freeze_inside_advance).
TEST(lib10_ac35_c59_1_runbatch_stops_at_freeze_inside_advance) {
    auto stepped = testroms::makeMachine(Model::TI59), batched = testroms::makeMachine(Model::TI59);
    for (auto* m : {stepped.get(), batched.get()}) testroms::runToScanLoop(*m, Model::TI59);
    const auto loop = romRanges(MachineVariant::TI59).scanLoop;
    // 2.3.0's rule, one instruction at a time.
    bool seen = false;
    const uint16_t armedAt = testroms::dbg(*stepped).pc();
    testroms::press(*stepped, 83);
    for (int i = 0; i < 100'000; ++i) {
        stepped->advance(1);
        const uint16_t pc = testroms::dbg(*stepped).pc();
        if (loop.contains(pc)) { seen = true; continue; }
        if (seen || (pc == 0 && armedAt != 0)) break;
    }
    // Now: TILevelRules as the observer of whole advances.
    TILevelRules rules;
    rules.startArmed(kCpu, *batched);
    struct Observer final : calcu::machine::InstructionObserver {
        TILevelRules& rules;
        calcu::machine::Machine& m;
        Observer(TILevelRules& r, calcu::machine::Machine& mm) : rules(r), m(mm) {}
        bool stopAfter(const InstructionEvent& e) override { return rules.startFiresAfter(kCpu, e, m); }
    } observer(rules, *batched);
    testroms::press(*batched, 83);
    batched->debugging()->setChecks(calcu::machine::Checks{false, false, &observer});
    bool fired = false;
    for (int i = 0; i < 50 && !fired; ++i) {
        batched->advance(batched->descriptor().tickRate / 50);
        fired = batched->debugging()->takeStop().has_value();
    }
    batched->debugging()->setChecks({});
    CHECK(fired);
    CHECK_EQ(batched->tickCount(), stepped->tickCount());
    CHECK_EQ(testroms::dbg(*batched).pc(), testroms::dbg(*stepped).pc());
}

// ── E10-4: breakpoints (AC-31) ────────────────────────────────────────────

TEST(lib10_ac31_app_session_pauses_at_a_breakpoint) {
    SessionRig r;
    CHECK(ready(r));
    const auto at = r.machine().debugging()->target(0).nextAddress();   // in the scan loop: hit soon
    const auto reply = r.send(calcu::debugger::kBreakCommand,
                              Value::object({{"setter", "harness"}, {"target", "TMC0501"}, {"address", static_cast<int64_t>(at)}}));
    CHECK(reply.done);
    CHECK(r.until([&] { return paused(r); }, 2s));
    const RunState s = r.part().runState();
    CHECK(s.reason == RunState::Reason::Breakpoint && s.owner == kCpu);
    CHECK_EQ(r.machine().debugging()->target(0).nextAddress(), at);   // before the instruction there
    CHECK_EQ(s.hits.size(), std::size_t{1});
    // Resume passes it once; it is hit again in the loop.
    rc(r).resume();
    CHECK(r.until([&] { return paused(r); }, 2s));
}

// ── E10-5: the debugger steps of a preset (AC-32; since M-54 LIB-6's
// loader with LIB-10's steps, which replaced the KEYSTROKES directives of
// 2.3.0 and their adapter L-E1) ────────────────────────────────────────────

namespace {
void load(SessionRig& r, const std::string& keys) {
    r.post(calcu::presets::kLoadPresetCommand,
           Value::object({{"content", "format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\nkeys:\n" + keys}, {"name", "d.ti59"}}));
}
} // namespace

TEST(lib10_ac32_freeze_calculator_is_pause_program) {
    SessionRig r;
    CHECK(ready(r));
    load(r, "  - key: 2\n  - pause: program\n");   // idle: it lands after 50000 weights
    CHECK(r.until([&] { return paused(r) && !r.busy(); }, 10s));
    CHECK(r.part().runState().owner == kProgram);
}

TEST(lib10_ac32_freeze_cpu_is_pause_cpu) {
    SessionRig r;
    CHECK(ready(r));
    load(r, "  - pause: cpu\n");
    CHECK(r.until([&] { return paused(r) && !r.busy(); }, 20s));
    CHECK(r.part().runState().owner == kCpu);
}

TEST(lib10_ac32_freeze_on_start_is_pause_on_start) {
    SessionRig r;
    CHECK(ready(r));
    load(r, "  - pause-on-start: cpu\n");
    CHECK(r.until([&] { return r.part().runState().armed == kCpu && !r.busy(); }, 5s));
    load(r, "  - pause-on-start: program\n");
    CHECK(r.until([&] { return r.part().runState().armed == kProgram && !r.busy(); }, 5s));
    load(r, "  - pause-on-start: off\n");
    CHECK(r.until([&] { return !r.part().runState().armed && !r.busy(); }, 5s));
}

TEST(lib10_ac32_step_and_resume) {
    SessionRig r;
    CHECK(ready(r));
    load(r, "  - key: 2\n  - pause: program\n  - step: { level: program, count: 3 }\n");
    CHECK(r.until([&] { return paused(r) && !r.busy(); }, 10s));
    CHECK(r.part().runState().reason == RunState::Reason::Step);
    r.post(calcu::presets::kRunStepCommand, Value::object({{"verb", "resume"}, {"value", ""}}));
    CHECK(r.until([&] { return !paused(r) && !r.busy(); }, 5s));
    // A step while running is no effect, with a line of the log (LIB-10 OQ-4).
    r.post(calcu::presets::kRunStepCommand, Value::object({{"verb", "step"}, {"value", "{ level: cpu }"}}));
    CHECK(r.until([&] { return !r.busy(); }, 5s));
    r.runFor(100ms);
    CHECK(!paused(r));
}

TEST(lib10_ac32_debug_panel_calculator_cpu_log) {
    SessionRig r;
    CHECK(ready(r));
    load(r, "  - debug-panel: calculator\n  - debug-panel: cpu\n  - debug-panel: log\n");
    CHECK(r.until([&] { return !r.busy(); }, 5s));
    r.runFor(20ms);
    // The product topic debug-view (C59-4 AC-2, M-56): a state topic, so the
    // three tabs asked in one call arrive as the last one, counted three times.
    CHECK(!r.panels().empty() && r.panels().back() == DebugPanel::Log);
    CHECK(r.state<TIDebugView>(topics::kDebugView) && r.state<TIDebugView>(topics::kDebugView)->requests == 3);
    r.debug().showView(r.session().context(), "log");
    r.runFor(20ms);
    CHECK(r.panels().back() == DebugPanel::Log);
}

// ── E10-8: what 2.3.0's tests checked of the freeze (AC-35) ────────────────

// A pause waits for its landing; its reply comes then (was
// session_queued_steps_run_after_the_host_call).
TEST(lib10_ac35_session_queued_steps_run_after_the_host_call) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.tap(11);
    const auto id = r.post(calcu::debugger::kPauseCommand, Value::object({{"level", "program"}}));
    CHECK(r.until([&] { return r.reply(id) != nullptr; }, 2s));
    CHECK(r.reply(id)->done && paused(r));
    const auto step = r.post(calcu::debugger::kStepCommand, Value::object({{"level", "program"}}));
    CHECK(r.until([&] { return r.reply(step) != nullptr; }, 2s));
    CHECK(paused(r));
}

// (was session_freeze_fires_through_listener)
TEST(lib10_ac35_session_freeze_fires_through_listener) {
    SessionRig r;
    CHECK(ready(r));
    r.send(calcu::debugger::kPauseOnStartCommand, Value::object({{"level", "cpu"}}));
    r.frame();
    CHECK(testroms::hw(r.machine()).traceFlags() != TRACE_NONE);   // an armed start: the frame ring
    r.machine().keyDown(calcu::machine::KeyId{"2"});
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.machine().keyUp(calcu::machine::KeyId{"2"});
    const auto* s = r.state<RunState>(calcu::debugger::kRunStateTopic);
    CHECK(s && s->paused && s->owner == kCpu && !s->armed);
    r.send(calcu::debugger::kResumeCommand);
    r.frame();
    CHECK(!paused(r));
    CHECK_EQ(testroms::hw(r.machine()).traceFlags(), static_cast<uint32_t>(TRACE_NONE));
}

// (was session_steps_only_when_frozen_and_arms_exclude)
TEST(lib10_ac35_session_steps_only_when_frozen_and_arms_exclude) {
    SessionRig r;
    CHECK(ready(r));
    const auto refused = rc(r).step(kProgram, 1, std::nullopt, calcu::debugger::StepKind::In);
    CHECK(!refused.ok && refused.reason == calcu::debugger::kNotPaused);
    rc(r).pauseOnStart(kProgram);
    rc(r).pauseOnStart(kCpu);
    CHECK(r.part().runState().armed == kCpu);
    rc(r).pause(kProgram);
    CHECK(r.until([&] { return paused(r); }, 20s));
    CHECK(r.part().runState().owner == kProgram && !r.part().runState().armed);
    rc(r).step(kCpu, 1, std::nullopt, calcu::debugger::StepKind::In);
    CHECK(r.until([&] { return paused(r); }, 2s));
    CHECK(r.part().runState().owner == kCpu);   // the step's level takes the pause over (AC-45)
    rc(r).resume();
    r.frame();
    CHECK(!paused(r));
}
