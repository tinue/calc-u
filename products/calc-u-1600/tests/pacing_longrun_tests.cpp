// The long-run pacing tests of LIB-17 (Technical Requirements; AC-21), the
// "after" test: Calc-U-1600 on the library's pacer through the library's app
// session (M-31). The "before" test ran on 0.8.1's pacer in M-10 (first step
// 14 of docs/walkthroughs/first-steps.md); its measured drift -- the PC-1600
// 0.0037 % fast while on and 0.21 % fast once switched off, the PC-1500A
// 12.7 ms over an hour, because 0.8.1 dropped the overshoot of each advance
// -- is the expected difference to this one (LIB-17 decision log).
//
// Scripted (Frequent group): a wall clock this test supplies, calls about 60
// times a second with ±4 ms jitter, an oversleep now and then and one stall;
// the machine is switched off by its OFF key half way (design D-4).
// CALCU1600_PACING_SCRIPTED_SECONDS sets the wall time per model (default
// below). After every call, emulated time (ticks at the machine's nominal
// clock) must match wall time within the bound of AC-1, and over each
// stretch between stalls it may differ by no more than 100 ppm.
//
// Real clock (Checkpoint group): CALCU1600_PACING_LONGRUN=<seconds> runs the
// same session on the real clock (600 for the checkpoint) and checks the
// bound at the end.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "../app/tests/SessionFixture.hpp"
#include "TestHarness.hpp"

namespace {

namespace sessions = calcu::sessions;
using sessionfixture::SessionFixture;

// The bound of AC-1 for a profile whose slice is the call: the last
// instruction's overshoot, well under a millisecond; 2 ms leaves room.
constexpr double kBoundSeconds = 0.002;
constexpr double kPpm = 100e-6;   // AC-21: a crystal's tolerance
constexpr double kDefaultScriptedSeconds = 120;

std::uint64_t ticksNow(SessionFixture& f) { return f.part().machine()->tickCount(); }

void key(SessionFixture& f, const char* name, bool down) {
    f.host->send(down ? sessions::kKeyDownCommand : sessions::kKeyUpCommand, sessions::Value::object({{"key", name}}));
}

void test_scripted(const char* model) {
    SessionFixture f("calcu-pacing-longrun");
    CHECK(f.start(model));
    const double hz = static_cast<double>(f.part().machine()->descriptor().tickRate);
    const char* env = std::getenv("CALCU1600_PACING_SCRIPTED_SECONDS");
    const double seconds = env ? std::atof(env) : kDefaultScriptedSeconds;
    std::uint32_t lcg = 1600;
    auto rnd = [&] {
        lcg = lcg * 1664525u + 1013904223u;
        return (lcg >> 8) / double(1u << 24);
    };
    const long calls = static_cast<long>(seconds * 60);
    const long stallAt = calls / 3, offAt = calls / 2;
    f.call();   // the pacer starts afresh after the start: a first call
    auto begin = f.now;
    std::uint64_t t0 = ticksNow(f);
    double worst = 0, worstRate = 0;
    int stretches = 1;
    auto closeStretch = [&] {
        const double wall = std::chrono::duration<double>(f.now - begin).count();
        const double emulated = static_cast<double>(ticksNow(f) - t0) / hz;
        if (wall > 1) worstRate = std::max(worstRate, std::fabs(emulated - wall) / wall);
    };
    for (long call = 1; call <= calls; ++call) {
        double step = 1000.0 / 60 + (rnd() - 0.5) * 8;   // ±4 ms jitter
        if (call % 900 == 0) step += 50;                 // an oversleep (under the lag limit)
        if (call == stallAt) {                           // a 5 s stall: the lag is dropped whole (AC-4)
            closeStretch();
            t0 = ticksNow(f);   // the call that drops the lag runs one slice from the new origin
            f.call(5000);
            begin = f.now;
            ++stretches;
            continue;
        }
        if (call == offAt) key(f, "off", true);
        if (call == offAt + 6) key(f, "off", false);
        f.call(step);
        const double wall = std::chrono::duration<double>(f.now - begin).count();
        const double d = std::fabs(wall - static_cast<double>(ticksNow(f) - t0) / hz);
        worst = std::max(worst, d);
    }
    closeStretch();
    const bool off = f.host->display() && !f.host->display()->on;
    std::printf("  pacing %s: %.0f s scripted, %d stretches, worst |emulated - wall| %.5f s, worst rate %.1f ppm%s\n",
                model, seconds, stretches, worst, worstRate * 1e6, off ? ", switched off" : "");
    CHECK(worst <= kBoundSeconds);
    CHECK(worstRate <= kPpm);
    CHECK(off);   // the second half ran switched off
}

void test_real_clock(const char* model) {
    const char* env = std::getenv("CALCU1600_PACING_LONGRUN");
    if (!env) return;   // the checkpoint group sets it (600)
    const double seconds = std::atof(env);
    SessionFixture f("calcu-pacing-longrun-real");
    auto steady = [] {
        return std::chrono::duration_cast<calcu::pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
    };
    f.now = steady();
    if (!f.start(model)) return;
    const double hz = static_cast<double>(f.part().machine()->descriptor().tickRate);
    auto realCall = [&] {
        f.now = steady();
        f.session().call(f.now);
    };
    realCall();
    const auto begin = f.now;
    const std::uint64_t t0 = ticksNow(f);
    bool switchedOff = false;
    while (std::chrono::duration<double>(steady() - begin).count() < seconds) {
        std::this_thread::sleep_for(std::chrono::microseconds(16'667));
        if (!switchedOff && std::chrono::duration<double>(steady() - begin).count() > seconds / 2) {
            key(f, "off", true);
            realCall();
            key(f, "off", false);
            switchedOff = true;
            continue;
        }
        realCall();
    }
    realCall();
    const double wall = std::chrono::duration<double>(f.now - begin).count();
    const double emulated = static_cast<double>(ticksNow(f) - t0) / hz;
    std::printf("  pacing real clock %s: wall %.3f s, emulated %.3f s, difference %.4f s\n", model, wall, emulated,
                emulated - wall);
    CHECK(std::fabs(emulated - wall) <= kBoundSeconds + kPpm * wall);
}

} // namespace

// Scripted: Frequent group. Real clock: Checkpoint group, only with
// CALCU1600_PACING_LONGRUN set.
TEST(lib17_ac21_pacing_longrun_scripted_pc1500a) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    test_scripted("PC-1500A");
}
TEST(lib17_ac21_pacing_longrun_scripted_pc1600) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROMs in roms/");
    test_scripted("PC-1600");
}
TEST(lib17_ac21_pacing_longrun_real_clock_pc1500a) { test_real_clock("PC-1500A"); }
TEST(lib17_ac21_pacing_longrun_real_clock_pc1600) { test_real_clock("PC-1600"); }
