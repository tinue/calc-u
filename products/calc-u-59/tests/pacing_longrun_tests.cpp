// The long-run pacing tests: the library's pacer, through the app session as
// every frontend hosts it. The rate of emulated time against wall time is
// compared with the expected rate of whole ticks per 20 ms slice: up to
// 0.3 % passes, 0.3 % to 1 % is printed for the user, above 1 % fails.
//
// Scripted (Frequent): an hour per model on a clock this test supplies, calls
// about 60 times a second with ±4 ms jitter, an oversleep now and then and one
// stall. Real clock (Checkpoint): CALCU59_PACING_LONGRUN=<seconds>.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
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

// The expected rate: whole ticks per 20 ms slice (int(instructionHz x 20 ms))
// against the exact ticks of the slice.
double sliceRate(ModelId model) {
    constexpr double kSliceSeconds = 0.020;
    const double exact = testroms::instructionHz(model) * kSliceSeconds;
    return static_cast<int32_t>(exact) / exact;
}

// The pacer's bound: one 20 ms slice, its jitter of half a slice, the spread of
// the calls.
constexpr double kBoundSeconds = 0.035;

const char* nameOf(const ModelId& m) { return info(m).displayName; }

calcu::sessions::ProductSession<TIAppPart> session(ModelId model, calcu::pacing::WallClock clock) {
    static const calcu::testing::TempFolder folder("calcu59-longrun");
    static calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(settings::settingsDecl());
    static const auto paths = testroms::rigPaths(settings, folder.path().string(), folder.path().string());
    return makeAppSession(testroms::appSetup(settings, model, *paths), std::move(clock), 59u);
}

} // namespace

TEST(pacing_longrun_scripted_ti) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        WallTime now{std::chrono::hours(2)};
        auto app = session(model, [&] { return now; });
        auto& ms = app.session->machineSession();
        const double tps = ms.machine()->descriptor().tickRate;
        for (int i = 0; i < 600 && !ms.machine()->isKeyReady(); ++i) app.session->call(now += std::chrono::microseconds(16'667));

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
                app.session->call(now);   // the lag is dropped, not caught up
                begin = now;
                ticks0 = ms.machine()->tickCount();
                continue;
            }
            now += d;
            app.session->call(now);
            const double dev = std::fabs(std::chrono::duration<double>(now - begin).count() -
                                         (ms.machine()->tickCount() - ticks0) / tps);
            worst = std::max(worst, dev);
        }
        wallSum += std::chrono::duration<double>(now - begin).count();
        emulatedSum += (ms.machine()->tickCount() - ticks0) / tps;
        const double rate = emulatedSum / wallSum;
        const double expected = sliceRate(model);
        const double diff = std::fabs(rate / expected - 1.0);
        std::printf("  pacing %s: rate %.6f (expected %.6f), difference %.3f %%, worst |emulated - wall| %.4f s\n",
                    nameOf(model), rate, expected, diff * 100, worst);
        if (diff > 0.003 && diff <= 0.01)
            std::printf("  NOTICE for the user: %s differs by %.3f %% (0.3 to 1 %%)\n", nameOf(model), diff * 100);
        CHECK(diff <= 0.01);
        CHECK(worst <= kBoundSeconds);
    }
}

TEST(pacing_longrun_real_clock_ti) {
    const char* env = std::getenv("CALCU59_PACING_LONGRUN");
    if (!env) return;  // the checkpoint group sets it (600)
    const double seconds = std::atof(env);
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        auto app = session(model, steadyWallTime);
        auto& ms = app.session->machineSession();
        const double tps = ms.machine()->descriptor().tickRate;
        const auto t0 = ms.machine()->tickCount();
        const auto w0 = Clock::now();
        while (Clock::now() - w0 < std::chrono::duration<double>(seconds)) {
            app.session->call(steadyWallTime());
            std::this_thread::sleep_for(std::chrono::microseconds(16'667));
        }
        app.session->call(steadyWallTime());
        const double wall = std::chrono::duration<double>(Clock::now() - w0).count();
        const double emulated = (ms.machine()->tickCount() - t0) / tps;
        std::printf("  pacing, real clock %s: wall %.3f s, emulated %.3f s, difference %.4f s\n", nameOf(model),
                    wall, emulated, emulated - wall);
        CHECK(std::fabs(emulated - wall) <= kBoundSeconds);
    }
}
