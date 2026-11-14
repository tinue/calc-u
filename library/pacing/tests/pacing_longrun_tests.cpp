#include "TestHarness.hpp"
#include "TestSliceRunner.hpp"
#include <cstdlib>
#include <memory>

// LIB-17 AC-2, AC-3 over hours (design §6, L9): five hours of wall time on a
// scripted clock, calls at about 60 Hz with timer jitter of ± 4 ms, an
// oversleep of 50 ms every 9,000 calls and a stall of 3 s every 250,000
// calls. The bound of AC-1 is asserted at the end of every call, from the
// start or from the last stall (AC-4: the lag is dropped there). Both kinds
// of profile; instructions of 97 ticks, so every slice overshoots.

using namespace calcu::pacing;
using namespace calcu::pacing::test;
using calcu::machine::test::buildTimedTestSeg;
using calcu::machine::test::kSegRate;

namespace {

constexpr Ticks kLen = 97;
constexpr Ticks kSlice = 2'000;
constexpr int kCalls = 5 * 3'600 * 60;   // five hours at 60 Hz

struct LongRun {
    int outside = 0, drops = 0, stalls = 0;
    std::int64_t worst = 0;
    double hours = 0;
};

LongRun longRun(const PacingProfile& profile) {
    ScriptedClock clock;
    auto m = buildTimedTestSeg(kLen);
    TestSliceRunner runner(*m, clock);
    Pacer pacer(clock.fn(), 2026u);
    pacer.setMachine(profile, kSegRate);
    const WallTime start = clock.now;
    const WallTime base{16'666'667};
    const WallTime longest = base + WallTime(4'000'000) + WallTime(50'000'000);
    const std::int64_t bound = profile.slice ? static_cast<std::int64_t>(kSlice + kLen)
                                             : static_cast<std::int64_t>(ticksIn(longest, kSegRate) + kLen);
    ScriptRandom rnd{7};
    LongRun out;
    Ticks t0 = m->tickCount();
    WallTime w0 = clock.now;
    for (int i = 0; i < kCalls; ++i) {
        WallTime d = base + WallTime(rnd.below(8'000'001) - 4'000'000);
        if (i % 9'000 == 8'999) d += WallTime(50'000'000);
        if (i % 250'000 == 249'999) {
            d += std::chrono::seconds(3);
            ++out.stalls;
        }
        clock.now += d;
        const Ticks before = m->tickCount();
        const PaceResult res = pacer.pace(Speed::Regular, runner);
        if (res.lagDropped) {   // AC-4: pacing goes on from the moment of this call
            ++out.drops;
            t0 = before;
            w0 = clock.now;
        }
        const std::int64_t dev = deviation(*m, t0, clock.now, w0);
        const std::int64_t absDev = dev < 0 ? -dev : dev;   // int64_t is long on Linux, long long on macOS
        out.worst = std::max(out.worst, absDev);
        if (absDev > bound) ++out.outside;
        runner.slices.clear();   // the records of five hours are not needed
    }
    out.hours = std::chrono::duration<double>(clock.now - start).count() / 3'600;
    return out;
}

} // namespace

TEST(ac2_ac3_library_longrun_scripted_fixed) {
    const LongRun r = longRun(fixedProfile(true));
    CHECK(r.hours > 4.99);
    CHECK_EQ(r.drops, r.stalls);   // only the stalls are over the limit
    CHECK_EQ(r.outside, 0);
    CHECK(r.worst <= static_cast<std::int64_t>(kSlice + kLen));
}

TEST(ac2_ac3_library_longrun_scripted_call) {
    const LongRun r = longRun(callProfile());
    CHECK(r.hours > 4.99);
    CHECK_EQ(r.drops, r.stalls);
    CHECK_EQ(r.outside, 0);
}
