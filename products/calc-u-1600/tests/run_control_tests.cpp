// LIB-10 AC-44 (design §10.10, D16-10): today's run-control cases without
// source lines on the library's run control -- step over a call, a data
// breakpoint, a pause -- driven with the memtest fixture through the DAP's
// layer (MemtestRig.hpp). The cases with source lines, conditions, hit
// counts and log messages are dap_layer_tests.cpp's (C1600-3 AC-23).

#include <cstdint>
#include <cstdio>
#include <vector>

#include "MemtestRig.hpp"
#include "TestHarness.hpp"

namespace {

using Reason = calcu::debugger::RunState::Reason;
using shared::dap::StepKind;

TEST(lib10_ac44_step_over_a_call) {
    memtest::Rig r;
    if (!r.ok) { std::printf("  SKIP: memtest fixture missing\n"); return; }
    // At the harness: step over `sjp 0x40C5` runs all of memtest.
    r.step(StepKind::Over, false);
    CHECK(r.runUntilStopped(20000));
    CHECK(r.reasonIs(Reason::Step) && r.pc() == 0x4603);

    // A breakpoint inside the call still stops the step over.
    r.targets().writeRegister(1, "p", 0x4600);
    r.targets().writeRegister(1, "x", 1);
    r.lineBreakpoint(90);
    r.step(StepKind::Over, false);
    CHECK(r.runUntilStopped(20000));
    CHECK(r.reasonIs(Reason::Breakpoint) && r.pc() == 0x40CE);
}

TEST(lib10_ac44_data_breakpoint_and_pause) {
    memtest::Rig r;
    if (!r.ok) return;
    shared::dap::DataBreakpointSpec w;
    w.addr = 0x40C7; // ERR_FLAG: first written by `sta (ERR_FLAG)` at 40CE
    w.access = shared::dap::DataAccess::Write;
    auto st = r.breakpoints.setData({w});
    r.apply();
    r.resume();
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Watch) && r.stopIds() == std::vector<int>{st[0].id});
    CHECK(r.pc() == 0x40D1); // after the storing instruction

    r.breakpoints.setData({});
    r.apply();
    r.resume();
    r.rig.advance(1000);
    CHECK(!r.rig.paused());
    r.rig.rc().pause(calcu::debugger::kCpu);
    CHECK(r.rig.paused() && r.reasonIs(Reason::Pause));
}

} // namespace
