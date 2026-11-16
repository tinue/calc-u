// LIB-10, the Calc-U-1600 column (design §10.10, D16-4, D16-5, D16-6 in
// the session, D16-8, D16-9): the library's run control and trace session on
// the Sharp machines -- the advance with checks (AC-39, AC-5), the level
// `cpu` and pause-on-start (AC-38), the run control without a debugger
// (AC-40), the trace format and its limit (AC-42, AC-43, AC-23).

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../app/SharpTraceRecorder.hpp"
#include "../app/tests/SessionFixture.hpp"
#include "../core/PC1600/PC1600InterfaceMachine.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "SharpDebugRig.hpp"
#include "SharpTestSupport.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "TraceFileReader.hpp"
#include "debugger/RunState.hpp"
#include "debugger/TraceSession.hpp"

namespace {

using namespace sessionfixture;
using calcu::debugger::RunState;
using sessions::Value;
namespace c = shared::commands;
namespace dbg = calcu::debugger;

// The PC-1600 of test_pc1600_lh5803_thread (debug_target_tests.cpp): the
// Z-80 hands the bus to the LH5803 (OUT (38H),A ; HALT), whose ROM loops at
// C000: sta (0x1000) ; nop ; bch C000.
std::unique_ptr<calcu::machine::Machine> handOverMachine() {
    auto m = sharpdebugtest::bareMachine("PC-1600");
    PC1600Machine& core = static_cast<PC1600InterfaceMachine&>(*m).core();
    std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x00), upper(PC1600Memory::kBankSize, 0x00);
    lower[0] = 0xD3; lower[1] = 0x38; lower[2] = 0x76;
    core.loadBank0(lower.data(), lower.size(), upper.data(), upper.size());
    std::vector<uint8_t> rom(16384, 0x00);
    const uint8_t code[] = {0xAE, 0x10, 0x00, 0x38, 0x9E, 0x06};
    for (size_t i = 0; i < sizeof code; i++) rom[i] = code[i];
    rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
    core.loadLH5803Rom(rom.data(), rom.size());
    core.reset();
    return m;
}

bool reasonIs(const RunState& s, RunState::Reason r) { return s.paused && s.reason && *s.reason == r; }

// ── D16-4: the advance with checks (AC-39, AC-5) ─────────────────────────

TEST(lib10_ac39_step_of_one_target_other_runs_with_the_bus) {
    sharpdebugtest::Rig rig(handOverMachine());
    rig.rc().pause(dbg::kCpu);
    calcu::machine::Debugging& d = rig.debugging();
    CHECK_EQ(d.current(), std::size_t(0));   // the Z-80 has the bus
    const std::uint64_t z0 = d.target(0).retired(), l0 = d.target(1).retired();
    const auto r = rig.rc().step(dbg::kCpu, 1, calcu::machine::DebugTargetId{"LH5803"}, dbg::StepKind::In);
    CHECK(r.ok);
    CHECK(rig.runUntilPaused(100, 1000));
    CHECK(reasonIs(rig.rc().state(), RunState::Reason::Step));
    CHECK_EQ(d.target(1).retired(), l0 + 1);   // one instruction of the target stepped
    CHECK(d.target(0).retired() >= z0 + 2);    // the Z-80 ran meanwhile: OUT, HALT
}

TEST(lib10_ac39_only_the_target_about_to_run_passes_its_breakpoint) {
    sharpdebugtest::Rig rig(handOverMachine());
    calcu::machine::Debugging& d = rig.debugging();
    // Both stand on a breakpoint: the Z-80 at 0000 (about to run), the
    // LH5803 at its reset address C000 (off the bus).
    rig.rc().table().addBreakpoint("harness", d, calcu::machine::DebugTargetId{"SC7852"}, 0x0000, {});
    rig.rc().table().addBreakpoint("harness", d, calcu::machine::DebugTargetId{"LH5803"}, 0xC000, {});
    rig.rc().table().apply(d);
    rig.rc().pause(dbg::kCpu);
    const std::uint64_t l0 = d.target(1).retired();
    rig.rc().resume();
    CHECK(rig.runUntilPaused(100, 1000));
    const RunState s = rig.rc().state();
    CHECK(reasonIs(s, RunState::Reason::Breakpoint));
    CHECK(s.target && s.target->value == "LH5803");
    CHECK(d.target(1).nextAddress() == 0xC000 && d.target(1).retired() == l0);   // its first instruction, not a later pass
}

TEST(lib10_checks_off_take_the_old_path) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const calcu::machine::Catalogue cat = sharp::sharpCatalogue();
    auto plain = sharptest::buildSharp(cat, "PC-1600");
    auto gated = sharptest::buildSharp(cat, "PC-1600");
    CHECK(plain && gated);
    if (!plain || !gated) return;
    plain->coldBoot();
    gated->coldBoot();
    calcu::machine::Machine& g = *gated;
    sharpdebugtest::Rig rig(std::move(gated));
    const calcu::machine::Ticks frame = plain->descriptor().tickRate / 60;
    for (int i = 0; i < 60; ++i) {
        plain->advance(frame);
        rig.advance(frame);
    }
    CHECK_EQ(plain->tickCount(), g.tickCount());
    std::uint64_t h1 = 1469598103934665603ull, h2 = h1;
    calcu::machine::DebugTarget& a = plain->debugging()->target(0);
    calcu::machine::DebugTarget& b = g.debugging()->target(0);
    for (std::uint64_t addr = 0; addr < 0x10000; ++addr) {
        h1 = (h1 ^ a.peek(0, addr, {}).value) * 1099511628211ull;
        h2 = (h2 ^ b.peek(0, addr, {}).value) * 1099511628211ull;
    }
    CHECK_EQ(h1, h2);
    CHECK_EQ(a.retired(), b.retired());
}

// ── D16-5: the level `cpu`, pause-on-start (AC-38) ───────────────────────

TEST(lib10_ac38_cpu_only_pause_lands_before_the_next_instruction) {
    sharpdebugtest::Rig rig(handOverMachine());
    const std::uint64_t r0 = rig.debugging().target(0).retired();
    const auto r = rig.rc().pause(dbg::kCpu);
    CHECK(r.ok && !r.deferred);   // at once: before the next instruction
    CHECK(rig.paused());
    rig.advance(10'000);
    CHECK_EQ(rig.debugging().target(0).retired(), r0);
}

TEST(lib10_ac38_program_level_refused) {
    sharpdebugtest::Rig rig(handOverMachine());
    const auto r = rig.rc().pause(dbg::LevelName{"program"});
    CHECK(!r.ok && r.reason == "The machine has no level program.");
    CHECK(!rig.paused());
}

void pauseOnStartFires(const char* model) {
    SessionFixture f;
    CHECK(f.start(model));
    f.runFor(1);   // at the prompt: ready for keys
    CHECK(f.ok(dbg::kPauseOnStartCommand, Value::object({{"level", "cpu"}})));
    const auto* armed = f.state<RunState>(dbg::kRunStateTopic);
    CHECK(armed && armed->armed && !armed->paused);
    // A command that keeps the processor from the keys: a FOR loop on the
    // PC-1500 and PC-1500A, a long BEEP on the PC-1600 (its key readiness:
    // C1600-1 AC-21, D-3).
    f.typeLine(std::string(model) == "PC-1600" ? "BEEP 1,40,500" : "FOR I=1 TO 200:NEXT I");
    for (int i = 0; i < 600; ++i) {
        const auto* s = f.state<RunState>(dbg::kRunStateTopic);
        if (s && s->paused) break;
        f.call();
    }
    const auto* s = f.state<RunState>(dbg::kRunStateTopic);
    CHECK(s && reasonIs(*s, RunState::Reason::PauseOnStart));
    CHECK(f.ok(dbg::kResumeCommand));
}

TEST(lib10_ac38_pause_on_start_fires_leaving_key_readiness_pc1500) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    pauseOnStartFires("PC-1500");
}
TEST(lib10_ac38_pause_on_start_fires_leaving_key_readiness_pc1500a) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    pauseOnStartFires("PC-1500A");
}
TEST(lib10_ac38_pause_on_start_fires_leaving_key_readiness_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    pauseOnStartFires("PC-1600");
}

// ── D16-6: the run control without a debugger (AC-40) ────────────────────

TEST(lib10_ac40_run_control_without_a_debugger) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;   // the Qt6 frontend's session: SharpAppPart, no DAP client
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(dbg::kPauseCommand, Value::object({{"level", "cpu"}})));
    const auto* s = f.state<RunState>(dbg::kRunStateTopic);
    CHECK(s && s->paused && s->target && s->target->value == "SC7852");
    const auto ticks = f.part().machine()->tickCount();
    f.runFor(0.25);
    CHECK_EQ(f.part().machine()->tickCount(), ticks);
    const sessions::CallId step = f.send(dbg::kStepCommand, Value::object({{"level", "cpu"}, {"count", 3}, {"kind", "in"}}));
    for (int i = 0; i < 60 && !f.takeReply(step); ++i) f.call();
    s = f.state<RunState>(dbg::kRunStateTopic);
    CHECK(s && reasonIs(*s, RunState::Reason::Step));
    CHECK(f.ok(dbg::kResumeCommand));
    f.runFor(0.25);
    CHECK(f.part().machine()->tickCount() > ticks);
}

// ── D16-8: the trace format (AC-42, AC-23) ───────────────────────────────

fs::path writePreset(SessionFixture& f, const std::string& name, const std::string& text) {
    const fs::path p = f.root / name;
    std::ofstream out(p, std::ios::binary);
    out << text;
    return p;
}

tracetest::TraceSummary sessionTrace(const char* model, const std::string& steps, const std::string& file) {
    SessionFixture f;
    CHECK(f.start(model));
    const fs::path preset = writePreset(f, "t.preset", std::string("format-version: 2\nmodel: ") + model + "\nkeys:\n" + steps);
    CHECK(f.ok(calcu::presets::kLoadPresetCommand, Value::object({{"ref", preset.generic_string()}})));
    const auto* t = f.state<dbg::TraceState>(dbg::kTraceTopic);
    CHECK(t && !t->running);   // the operation's end ends a trace left open
    return tracetest::readTrace(f.paths->traceDir() + "/" + file);
}

void checkWellFormed(const tracetest::TraceSummary& s) {
    CHECK(s.headerOk);
    CHECK(s.firstRecordType == 0x01);   // SESSION_START
    CHECK(s.lastRecordType == 0x03);    // SESSION_END
    CHECK(s.sessionStartCount == 1 && s.sessionEndCount == 1);
    CHECK(s.allEventPayloads25 && s.allZ80Payloads29 && !s.truncated);
}

TEST(lib10_ac42_trace_step_produces_wellformed_file) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const auto s = sessionTrace("PC-1500A", "  - trace: t.bin\n  - wait: 0.3s\n  - trace: off\n", "t.bin");
    checkWellFormed(s);
    CHECK(s.traceEventCount > 0);
}

TEST(lib10_ac42_trace_step_left_open_is_closed_at_the_end) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const auto s = sessionTrace("PC-1500A", "  - trace: t2.bin\n  - wait: 0.2s\n", "t2.bin");
    checkWellFormed(s);
    CHECK(s.traceEventCount > 0);
}

TEST(lib10_ac42_z80_frame_shares_one_file_with_lh5801_frame) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    // A PC-1600 with the CE-150 in MODE 1: the LH5803 drives the plotter.
    const auto s = sessionTrace("PC-1600\nprinter: CE-150",
                                "  - trace: t3.bin\n  - type: MODE 1\n  - type: LPRINT 1\n  - wait: 2s\n  - trace: off\n",
                                "t3.bin");
    checkWellFormed(s);
    CHECK(s.z80EventCount > 0);
    CHECK(s.traceEventCount > 0);   // the LH5803's frames in the same file
}

TEST(lib10_ac42_pc1600_trace_step_produces_wellformed_file) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const auto s = sessionTrace("PC-1600", "  - trace: t4.bin\n  - wait: 0.3s\n  - trace: off\n", "t4.bin");
    checkWellFormed(s);
    CHECK(s.z80EventCount > 0);
}

TEST(lib10_ac42_pc1600_left_open_is_closed) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const auto s = sessionTrace("PC-1600", "  - trace: t5.bin\n  - wait: 0.2s\n", "t5.bin");
    checkWellFormed(s);
}

// §10.11: the trace session's bytes equal the reference recorded with
// 0.8.1's file path before the core was touched, but for the time of the
// session start (bytes 19 to 26).
void checkReference(const char* model, int frames, const char* reference) {
    std::ifstream in(reference, std::ios::binary);
    const std::vector<std::uint8_t> want((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(!want.empty());
    const calcu::machine::Catalogue cat = sharp::sharpCatalogue();
    auto m = sharptest::buildSharp(cat, model);
    CHECK(m != nullptr);
    if (!m || want.empty()) return;
    auto rec = shared::makeSharpTraceRecorder(*m);
    CHECK(rec != nullptr);
    if (!rec) return;
    std::vector<std::uint8_t> bytes;
    rec->begin(bytes);
    m->coldBoot();
    for (int i = 0; i < frames; ++i) {
        m->advance(m->descriptor().tickRate / 60);
        CHECK(rec->take(bytes, UINT64_MAX));
    }
    rec->end(bytes);
    CHECK_EQ(bytes.size(), want.size());
    std::size_t diffs = 0, first = 0;
    for (std::size_t i = 0; i < std::min(bytes.size(), want.size()); ++i) {
        if (i >= 19 && i < 27) continue;   // the session start's time
        if (bytes[i] != want[i] && diffs++ == 0) first = i;
    }
    if (diffs) std::fprintf(stderr, "  %s: %zu bytes differ, the first at %zu\n", reference, diffs, first);
    CHECK_EQ(diffs, std::size_t(0));
}

TEST(lib10_ac42_bytes_equal_the_recorded_reference) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    checkReference("PC-1600", 29, "tests/golden-runs/pc1600-basic.trace");
    checkReference("PC-1500A", 120, "tests/golden-runs/pc1500a-basic.trace");
}

TEST(lib10_ac23_button_trace_is_trace_bin) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.ok(dbg::kTraceCommand, Value::object({{"button", true}})));
    const auto* t = f.state<dbg::TraceState>(dbg::kTraceTopic);
    CHECK(t && t->running && t->fileName == "TRACE.bin");
    f.runFor(0.2);
    CHECK(f.ok(dbg::kTraceCommand, Value::object({{"off", true}})));
    t = f.state<dbg::TraceState>(dbg::kTraceTopic);
    CHECK(t && !t->running);
    const auto s = tracetest::readTrace(f.paths->traceDir() + "/TRACE.bin");
    checkWellFormed(s);
    CHECK(s.traceEventCount > 0);
}

// ── D16-9: the size limit (AC-43, AC-24) ─────────────────────────────────

bool limitNotice(SessionFixture& f) {
    const auto notices = f.notices();
    for (const std::string& n : notices)
        if (n.find("reached the size limit of 1 MB") != std::string::npos) return true;
    return false;
}

TEST(lib10_ac43_preset_trace_ends_at_the_limit_with_a_notice) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.settings.setTraceMaxFileSizeMB(1);
    CHECK(f.start("PC-1600"));
    const fs::path preset = writePreset(f, "long.pc1600", "format-version: 2\nmodel: PC-1600\nkeys:\n  - trace: long.bin\n  - wait: 8s\n");
    CHECK(f.ok(calcu::presets::kLoadPresetCommand, Value::object({{"ref", preset.generic_string()}})));
    const std::string path = f.paths->traceDir() + "/long.bin";
    CHECK(fs::exists(path) && fs::file_size(path) <= 1024 * 1024);
    CHECK(fs::exists(path) && fs::file_size(path) > 1000 * 1000);   // MiB, not 1'000'000 bytes (ledger 1600-14)
    checkWellFormed(tracetest::readTrace(path));
    CHECK(limitNotice(f));
}

TEST(lib10_ac43_button_trace_ends_at_the_limit_with_a_notice) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.settings.setTraceMaxFileSizeMB(1);
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(dbg::kTraceCommand, Value::object({{"button", true}})));
    f.call(500);
    for (int i = 0; i < 300; ++i) {
        const auto* t = f.state<dbg::TraceState>(dbg::kTraceTopic);
        if (t && !t->running) break;
        f.call();
    }
    const auto* t = f.state<dbg::TraceState>(dbg::kTraceTopic);
    CHECK(t && !t->running);
    const std::string path = f.paths->traceDir() + "/TRACE.bin";
    CHECK(fs::exists(path) && fs::file_size(path) <= 1024 * 1024);
    checkWellFormed(tracetest::readTrace(path));
    CHECK(limitNotice(f));
}


// R-1 (LIB-10 design §10.11; Longer, CalcU1600DebuggerSpeed): a PC-1600 at
// full speed with a breakpoint set that is never reached, against the same
// run without one; the numbers are for the qa-report.
TEST(lib10_speed_pc1600_full_speed_with_a_breakpoint) {
    if (!std::getenv("CALCU1600_SPEED")) return;
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    auto measure = [](bool breakpoint) {
        const calcu::machine::Catalogue cat = sharp::sharpCatalogue();
        auto m = sharptest::buildSharp(cat, "PC-1600");
        m->coldBoot();
        calcu::machine::Machine& machine = *m;
        sharpdebugtest::Rig rig(std::move(m));
        if (breakpoint) {
            rig.rc().table().addBreakpoint("harness", rig.debugging(), calcu::machine::DebugTargetId{"SC7852"}, 0x3FFF, {});
            rig.rc().table().apply(rig.debugging());
        }
        const calcu::machine::Ticks second = machine.descriptor().tickRate;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 20 * 60; ++i) rig.advance(second / 60);
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    };
    const double plain = measure(false), withBreakpoint = measure(true);
    std::printf("  PC-1600, 20 s emulated at full speed: %.2f s without, %.2f s with a breakpoint (%.1fx real time, %.1fx)\n",
                plain, withBreakpoint, 20.0 / plain, 20.0 / withBreakpoint);
    CHECK(withBreakpoint < plain * 3);
}

} // namespace
