// The "after" long-run pacing tests of LIB-17 AC-18 (design §6, P59-4): the
// library's pacer, through the app session as the Swift and Qt6 frontends
// host it (M-33). The rate of emulated time against wall time is compared
// with 2.3.0's, measured by the "before" test (pacing_longrun_tests.cpp, first
// step 14): up to 0.3 % passes, 0.3 % to 1 % is printed for the user, above
// 1 % fails.
//
// Scripted (Frequent): an hour per model on a clock this test supplies, the
// same calls as the "before" test. Real clock (Checkpoint):
// CALCU59_PACING_LONGRUN=<seconds>.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/Pacing.hpp"
#include "../app/TIAppPart.hpp"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <thread>

using namespace cu59;
using Clock = std::chrono::steady_clock;
using calcu::pacing::WallTime;

namespace {

// 2.3.0's rate (pacing_longrun_tests.cpp): whole ticks per 20 ms batch.
double oldRate(Model model) {
    const double exact = testroms::instructionHz(model) * kBatchSeconds;
    return static_cast<int32_t>(exact) / exact;
}

// AC-1's bound: one 20 ms slice, its jitter of half a slice, the spread of
// the calls (as the "before" test's).
constexpr double kBoundSeconds = 0.035;

const char* nameOf(Model m) { return m == Model::TI59 ? "TI-59" : m == Model::TI58 ? "TI-58" : "TI-58C"; }

std::unique_ptr<TIAppSession> session(Model model, calcu::pacing::WallClock clock) {
    TIAppSetup s;
    s.roms = testroms::romSet();
    s.cuecards = testroms::readFile("roms/solid-state/cuecards.txt").value_or("");
    s.startModel = model;
    s.saveFolder = (std::filesystem::temp_directory_path() / "calcu59-longrun").string();
    return makeAppSession(std::move(s), std::move(clock), 59u);
}

} // namespace

TEST(lib17_ac18_pacing_longrun_scripted_ti) {
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        WallTime now{std::chrono::hours(2)};
        auto app = session(model, [&] { return now; });
        auto& ms = app->session->machineSession();
        const double tps = ms.machine()->descriptor().tickRate;
        for (int i = 0; i < 600 && !ms.machine()->isKeyReady(); ++i) app->session->call(now += std::chrono::microseconds(16'667));

        uint32_t lcg = 12345;
        auto rnd = [&] { lcg = lcg * 1664525u + 1013904223u; return (lcg >> 8) / double(1u << 24); };
        auto begin = now;
        auto ticks0 = ms.machine()->tickCount();
        double wallSum = 0, emulatedSum = 0, worst = 0;
        const auto end = now + std::chrono::hours(1);
        for (int call = 1; now < end; ++call) {
            auto d = std::chrono::microseconds(16'667 + static_cast<int>((rnd() - 0.5) * 8000));   // ±4 ms jitter
            if (call % 9000 == 0) d += std::chrono::milliseconds(50);                                // an oversleep
            if (call == 108'000) {                                                                   // a stall of 5 s, 30 min in
                wallSum += std::chrono::duration<double>(now - begin).count();
                emulatedSum += (ms.machine()->tickCount() - ticks0) / tps;
                now += std::chrono::seconds(5);
                app->session->call(now);   // the lag is dropped, not caught up
                begin = now;
                ticks0 = ms.machine()->tickCount();
                continue;
            }
            now += d;
            app->session->call(now);
            const double dev = std::fabs(std::chrono::duration<double>(now - begin).count() -
                                         (ms.machine()->tickCount() - ticks0) / tps);
            worst = std::max(worst, dev);
        }
        wallSum += std::chrono::duration<double>(now - begin).count();
        emulatedSum += (ms.machine()->tickCount() - ticks0) / tps;
        const double rate = emulatedSum / wallSum;
        const double before = oldRate(model);
        const double diff = std::fabs(rate / before - 1.0);
        std::printf("  pacing after %s: rate %.6f (2.3.0: %.6f), difference %.3f %%, worst |emulated - wall| %.4f s\n",
                    nameOf(model), rate, before, diff * 100, worst);
        if (diff > 0.003 && diff <= 0.01)
            std::printf("  NOTICE for the user: %s differs by %.3f %% (0.3 to 1 %%)\n", nameOf(model), diff * 100);
        CHECK(diff <= 0.01);
        CHECK(worst <= kBoundSeconds);
    }
}

TEST(lib17_ac18_pacing_longrun_real_clock_ti) {
    const char* env = std::getenv("CALCU59_PACING_LONGRUN");
    if (!env) return;  // the checkpoint group sets it (600)
    const double seconds = std::atof(env);
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        auto app = session(model, steadyWallTime);
        auto& ms = app->session->machineSession();
        const double tps = ms.machine()->descriptor().tickRate;
        const auto t0 = ms.machine()->tickCount();
        const auto w0 = Clock::now();
        while (Clock::now() - w0 < std::chrono::duration<double>(seconds)) {
            app->session->call(steadyWallTime());
            std::this_thread::sleep_for(std::chrono::microseconds(16'667));
        }
        app->session->call(steadyWallTime());
        const double wall = std::chrono::duration<double>(Clock::now() - w0).count();
        const double emulated = (ms.machine()->tickCount() - t0) / tps;
        std::printf("  pacing after, real clock %s: wall %.3f s, emulated %.3f s, difference %.4f s\n", nameOf(model),
                    wall, emulated, emulated - wall);
        CHECK(std::fabs(emulated - wall) <= kBoundSeconds);
    }
}
