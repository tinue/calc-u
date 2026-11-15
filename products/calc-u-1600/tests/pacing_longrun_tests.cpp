// The long-run pacing tests of LIB-17 (Technical Requirements; AC-21), on
// 0.8.1's pacer (app/Pacer) before it moves to the library (first step 14 of
// docs/walkthroughs/first-steps.md, M-10 of docs/migration-plan.md).
//
// The path is MachineSession::advance's without a paste or a debugger: each
// call the pacer grants cycles (Pacer::cyclesDue) and the machine runs them
// (runCycles), which may run a few cycles more to finish an instruction.
// Emulated time is what the machine ran, over its clock.
//
// Scripted (Frequent group): a wall clock this test supplies, calls about 60
// times a second with jitter, an oversleep now and then and one stall;
// CALCU1600_PACING_SCRIPTED_SECONDS sets the wall time per model (default
// below). Over every stretch between stalls, emulated time must match wall
// time within the bound below.
//
// Real clock (Checkpoint group): CALCU1600_PACING_LONGRUN=<seconds> runs the
// same path on the real clock (600 for the checkpoint) and checks the bound
// at the end.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "../app/Pacer.hpp"
#include "../core/PC1500/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"

namespace {

// The bound of AC-1 for a profile whose slice is the call: the time between
// two calls (16.7 ms, up to 20.7 ms with the scripted jitter).
constexpr double kBoundSeconds = 0.021;
// 0.8.1's pacer drops the cycles runCycles runs past what it granted (LIB-17
// decision log: the "before" test states this measured drift as the expected
// difference). Measured on 2026-11-14 (first step 14): the PC-1600 runs
// 0.0037 % fast while it is on and 0.21 % fast once it has switched itself
// off (after about 10 minutes); the PC-1500A 12.7 ms over an hour. The
// allowance below covers that drift over a stretch.
constexpr double kOldDriftRate = 0.0025;
constexpr double kDefaultScriptedSeconds = 120;

struct Machines {
    std::unique_ptr<PC1500Machine> m15;
    std::unique_ptr<PC1600Machine> m16;
    double hz = 0;
    const char* name = "";
    uint64_t run(uint64_t n) { return m15 ? m15->runCycles(n) : m16->runCycles(n); }
};

bool build(bool pc1600, Machines* out) {
    std::string err;
    if (pc1600) {
        out->m16 = std::make_unique<PC1600Machine>();
        if (!presetroms::loadPC1600RomSet(*out->m16, testPresetRoms(), "new", &err)) return false;
        out->m16->allReset();
        runBootToPrompt(*out->m16);
        out->hz = PC1600Machine::kTStateHz;
        out->name = "PC-1600";
    } else {
        out->m15 = std::make_unique<PC1500Machine>(PC1500Variant::PC1500A);
        if (!presetroms::loadPC1500Rom(*out->m15, "A04", testPresetRoms(), &err)) return false;
        out->m15->reset();
        runBootToPrompt(*out->m15);
        out->hz = PC1500Machine::kCpuHz;
        out->name = "PC-1500A";
    }
    return true;
}

void test_scripted(bool pc1600) {
    Machines m;
    if (!build(pc1600, &m)) { std::printf("SKIP pacing_longrun_scripted (ROMs missing)\n"); return; }
    const char* env = std::getenv("CALCU1600_PACING_SCRIPTED_SECONDS");
    const double seconds = env ? std::atof(env) : kDefaultScriptedSeconds;
    std::int64_t now = 1'000'000'000'000LL;
    shared::Pacer pacer([&] { return now; });
    pacer.cyclesDue(now, m.hz);  // first sight of the clock rate rebases
    uint32_t lcg = 1600;
    auto rnd = [&] { lcg = lcg * 1664525u + 1013904223u; return (lcg >> 8) / double(1u << 24); };
    const std::int64_t end = now + static_cast<std::int64_t>(seconds * 1e9);
    const long stallAt = static_cast<long>(seconds * 30);  // half way
    std::int64_t begin = now;
    uint64_t ran = 0;
    double worst = 0;
    int stretches = 1;
    for (long call = 1; now < end; ++call) {
        std::int64_t step = 16'667'000 + static_cast<std::int64_t>((rnd() - 0.5) * 8e6);  // ±4 ms jitter
        if (call % 900 == 0) step += 50'000'000;                                        // an oversleep
        if (call == stallAt) {                                                          // a 5 s stall
            now += 5'000'000'000LL;
            ran = 0;
            m.run(pacer.cyclesDue(now, m.hz));  // the capped catch-up, then a new schedule
            begin = now;
            ++stretches;
            continue;
        }
        now += step;
        ran += m.run(pacer.cyclesDue(now, m.hz));
        const double wall = static_cast<double>(now - begin) * 1e-9;
        const double d = std::fabs(wall - static_cast<double>(ran) / m.hz) - kOldDriftRate * wall;
        worst = std::max(worst, d);
        if (std::getenv("CALCU1600_PACING_TRACE") && call % 3600 == 0)
            std::printf("    %s t=%.0f s diff=%+.4f s\n", m.name, static_cast<double>(now - begin) * 1e-9,
                        static_cast<double>(ran) / m.hz - static_cast<double>(now - begin) * 1e-9);
    }
    std::printf("  pacing %s: %.0f s scripted, worst |emulated - wall| beyond 0.8.1's drift %.4f s over %d stretches\n", m.name, seconds,
                worst, stretches);
    CHECK(worst <= kBoundSeconds);
}

void test_real_clock(bool pc1600) {
    const char* env = std::getenv("CALCU1600_PACING_LONGRUN");
    if (!env) return;  // the checkpoint group sets it (600)
    const double seconds = std::atof(env);
    Machines m;
    if (!build(pc1600, &m)) return;
    shared::Pacer pacer;
    pacer.cyclesDue(shared::Pacer::steadyNowNs(), m.hz);
    const std::int64_t begin = shared::Pacer::steadyNowNs();
    uint64_t ran = 0;
    while (shared::Pacer::steadyNowNs() - begin < static_cast<std::int64_t>(seconds * 1e9)) {
        ran += m.run(pacer.cyclesDue(shared::Pacer::steadyNowNs(), m.hz));
        std::this_thread::sleep_for(std::chrono::microseconds(16'667));
    }
    ran += m.run(pacer.cyclesDue(shared::Pacer::steadyNowNs(), m.hz));
    const double wall = static_cast<double>(shared::Pacer::steadyNowNs() - begin) * 1e-9;
    const double emulated = static_cast<double>(ran) / m.hz;
    std::printf("  pacing real clock %s: wall %.3f s, emulated %.3f s, difference %.4f s\n", m.name, wall, emulated,
                emulated - wall);
    CHECK(std::fabs(emulated - wall) <= kBoundSeconds + kOldDriftRate * wall);
}

} // namespace

// Scripted: Frequent group. Real clock: Checkpoint group, only with
// CALCU1600_PACING_LONGRUN set.
TEST(pacing_longrun_scripted_pc1500a) { test_scripted(false); }
TEST(pacing_longrun_scripted_pc1600) { test_scripted(true); }
TEST(pacing_longrun_real_clock_pc1500a) { test_real_clock(false); }
TEST(pacing_longrun_real_clock_pc1600) { test_real_clock(true); }
