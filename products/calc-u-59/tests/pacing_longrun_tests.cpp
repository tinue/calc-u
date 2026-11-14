// The long-run pacing tests of LIB-17 (Technical Requirements; AC-18), on
// 2.3.0's pacer (EmulationRunner) before it moves to the library (first step
// 14 of docs/walkthroughs/first-steps.md, M-10 of docs/migration-plan.md).
//
// Scripted (Frequent group): the cooperative runner on a wall clock this test
// supplies; an hour per model, calls about 60 times a second with jitter,
// an oversleep now and then and one stall of 5 s. Over every stretch between
// stalls, emulated time must match wall time within the bound below.
//
// Real clock (Checkpoint group): CALCU59_PACING_LONGRUN=<seconds> runs the
// threaded runner, as the Swift and Qt6 apps do, on the real clock for that
// long (600 for the checkpoint) and checks the same bound at the end.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/EmulationRunner.hpp"
#include "../app/FreezeController.hpp"
#include "../app/Pacing.hpp"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <thread>

using namespace cu59;
using Clock = std::chrono::steady_clock;

namespace {

// Emulated time is weighted ticks over the model's instruction rate
// (instructionHz: the resonator / 32; elapsedTicks counts an IDLE step four
// times on the TI-58/59). 2.3.0 runs one batch of int(instructionHz x 20 ms)
// ticks per 20 ms (app/Pacing.hpp:22): the TI-59 at 284 ticks for 284.375,
// the TI-58C at 246 for 246.25, so both run slow against their own clock
// (0.13 % and 0.10 %); the TI-58 divides evenly. That rate is the expected
// difference of this "before" test (LIB-17 AC-18; first step 14): the test
// checks emulated time against wall time x rate.
double ticksPerSecond(Model model) { return info(model).instructionHz; }
double oldRate(Model model) {
    const double exact = info(model).instructionHz * kBatchSeconds;
    return static_cast<int32_t>(exact) / exact;
}

// The bound of AC-1 for 2.3.0, as measured on it (first step 14: the old
// code's own bound): one batch of 20 ms (the profile's slice) plus the
// batch-end jitter of half a batch and the spread of the calls (worst
// measured 0.031 s, TI-58C).
constexpr double kBoundSeconds = 0.035;

struct Stretch { double wall = 0, emulated = 0; };

}  // namespace

TEST(pacing_longrun_scripted_ti) {
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        const double tps = ticksPerSecond(model) * oldRate(model);  // emulated seconds at 2.3.0's rate
        FreezeController freeze(model);
        EmulationRunner::Callbacks cb;
        cb.jitterSeed = 59;
        EmulationRunner runner(*m, model, freeze, cb, Threading::Cooperative);
        runner.start();
        auto t = Clock::time_point{} + std::chrono::hours(2);
        runner.pump(t, nullptr);
        uint32_t lcg = 12345;
        auto rnd = [&] { lcg = lcg * 1664525u + 1013904223u; return (lcg >> 8) / double(1u << 24); };
        std::vector<Stretch> stretches;
        auto begin = t;
        uint64_t ticks0 = m->elapsedTicks();
        double worst = 0;
        const auto end = t + std::chrono::hours(1);
        int calls = 0;
        while (t < end) {
            auto step = std::chrono::microseconds(16'667 + static_cast<int>((rnd() - 0.5) * 8000));  // ±4 ms jitter
            if (++calls % 9000 == 0) step += std::chrono::milliseconds(50);                          // an oversleep
            bool stall = calls == 108'000;                                                           // 30 min in
            if (stall) {
                Stretch s{std::chrono::duration<double>(t - begin).count(), (m->elapsedTicks() - ticks0) / tps};
                stretches.push_back(s);
                t += std::chrono::seconds(5);
                runner.pump(t, nullptr);  // the schedule restarts instead of catching up
                begin = t;
                ticks0 = m->elapsedTicks();
                continue;
            }
            t += step;
            runner.pump(t, nullptr);
            const double d = std::fabs(std::chrono::duration<double>(t - begin).count() - (m->elapsedTicks() - ticks0) / tps);
            worst = std::max(worst, d);
        }
        stretches.push_back({std::chrono::duration<double>(t - begin).count(), (m->elapsedTicks() - ticks0) / tps});
        runner.stop();
        runner.pump(t, nullptr);
        std::printf("  pacing %s: %.2f ticks/s at 2.3.0's rate, worst |emulated - wall| %.4f s over %zu stretches\n",
                    model == Model::TI59 ? "TI-59" : model == Model::TI58 ? "TI-58" : "TI-58C", tps, worst, stretches.size());
        CHECK(worst <= kBoundSeconds);
        for (const auto& s : stretches) CHECK(std::fabs(s.wall - s.emulated) <= kBoundSeconds);
    }
}

TEST(pacing_longrun_real_clock_ti) {
    const char* env = std::getenv("CALCU59_PACING_LONGRUN");
    if (!env) return;  // the checkpoint group sets it (600)
    const double seconds = std::atof(env);
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        const double tps = ticksPerSecond(model) * oldRate(model);
        FreezeController freeze(model);
        EmulationRunner::Callbacks cb;
        EmulationRunner runner(*m, model, freeze, cb, Threading::Threads);
        const uint64_t t0 = m->elapsedTicks();
        const auto w0 = Clock::now();
        runner.start();
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        uint64_t ticks = 0;
        runner.stop();
        runner.sync([&] { ticks = m->elapsedTicks() - t0; });
        const double wall = std::chrono::duration<double>(Clock::now() - w0).count();
        const double emulated = ticks / tps;
        std::printf("  pacing real clock %s: wall %.3f s, emulated %.3f s, difference %.4f s\n",
                    model == Model::TI59 ? "TI-59" : model == Model::TI58 ? "TI-58" : "TI-58C", wall, emulated, emulated - wall);
        CHECK(std::fabs(emulated - wall) <= kBoundSeconds);
    }
}
