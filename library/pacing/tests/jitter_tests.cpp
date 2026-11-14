#include "TestHarness.hpp"
#include "TestSliceRunner.hpp"
#include <algorithm>
#include <memory>
#include <set>

// LIB-17 AC-9 to AC-11 (design §6, L1, L2): the jittered slice ends.

using namespace calcu::pacing;
using namespace calcu::pacing::test;
using calcu::machine::test::buildTimedTestSeg;
using calcu::machine::test::kSegRate;

namespace {

constexpr Ticks kSlice = 2'000;   // 20 ms at 100,000 ticks per second
const WallTime k20Ms{20'000'000};
const WallTime k60Hz{16'666'667};

// The ticks of the slices a pacer asks for, on a test-seg machine, over
// `calls` calls `interval` apart.
std::vector<Ticks> requests(std::optional<std::uint32_t> seed, int calls, WallTime interval) {
    ScriptedClock clock;
    auto m = buildTimedTestSeg(1);
    TestSliceRunner runner(*m, clock);
    Pacer pacer(clock.fn(), seed);
    pacer.setMachine(fixedProfile(true), kSegRate);
    for (int i = 0; i < calls; ++i) {
        clock.now += interval;
        pacer.pace(Speed::Regular, runner);
    }
    std::vector<Ticks> out;
    for (const auto& s : runner.slices) out.push_back(s.request.ticks);
    return out;
}

} // namespace

TEST(ac9_jitter_offsets_within_half_a_slice_and_do_not_add_up) {
    // The draw: evenly from 0 to half a slice.
    SliceJitter j(5);
    Ticks lo = kSlice, hi = 0, sum = 0;
    const int n = 20'000;
    for (int i = 0; i < n; ++i) {
        const Ticks r = j.offset(kSlice);
        lo = std::min(lo, r);
        hi = std::max(hi, r);
        sum += r;
    }
    CHECK(lo <= 5);
    CHECK_EQ(hi <= kSlice / 2, true);
    CHECK(hi >= kSlice / 2 - 5);
    const double mean = static_cast<double>(sum) / n;
    CHECK(mean > 485 && mean < 515);   // 500 for an even draw over 0 to 1,000

    // The slice ends of a pacer: each at an offset of 0 to half a slice after
    // its grid point, so after 1,000 slices the machine is still within half
    // a slice of the grid (instructions of 1 tick: no overshoot).
    ScriptedClock clock;
    auto m = buildTimedTestSeg(1);
    TestSliceRunner runner(*m, clock);
    Pacer pacer(clock.fn(), 9u);
    pacer.setMachine(fixedProfile(true), kSegRate);
    for (int i = 0; i < 1'000; ++i) {
        clock.now += k20Ms;
        pacer.pace(Speed::Regular, runner);
    }
    CHECK(runner.slices.size() >= 1'000);
    int outside = 0;
    std::set<Ticks> offsets;
    for (std::size_t k = 0; k < runner.slices.size(); ++k) {
        const Ticks grid = (k + 1) * kSlice;
        const Ticks end = runner.slices[k].tickAfter;
        if (end < grid || end > grid + kSlice / 2) ++outside;
        offsets.insert(end - std::min(end, grid));
    }
    CHECK_EQ(outside, 0);
    CHECK(offsets.size() > 300);   // the offsets vary
    // AC-1 holds as without jitter: emulated time within one slice of wall time.
    const std::int64_t dev = deviation(*m, 0, clock.now, std::chrono::hours(1));
    CHECK(dev <= static_cast<std::int64_t>(kSlice) && dev > -static_cast<std::int64_t>(kSlice / 2));
}

TEST(ac10_jitter_same_seed_same_sequence) {
    const auto a = requests(42u, 300, k60Hz);
    const auto b = requests(42u, 300, k60Hz);
    CHECK(a.size() >= 240);
    CHECK(a == b);
    CHECK(std::set<Ticks>(a.begin(), a.end()).size() > 50);   // jittered: the lengths differ
    // One sequence on every platform: std::minstd_rand as the standard
    // defines it (x = x · 48271 mod 2³¹ − 1), our own draw x mod (S/2 + 1).
    SliceJitter j(42);
    std::uint64_t x = 42;
    int differ = 0;
    for (int i = 0; i < 1'000; ++i) {
        x = x * 48'271 % 2'147'483'647;
        if (j.offset(kSlice) != x % (kSlice / 2 + 1)) ++differ;
    }
    CHECK_EQ(differ, 0);
    CHECK(requests(43u, 300, k60Hz) != a);   // another seed, another sequence
}

TEST(ac10_jitter_unseeded_differs) {
    SliceJitter a, b;
    std::vector<Ticks> sa, sb;
    for (int i = 0; i < 32; ++i) {
        sa.push_back(a.offset(kSlice));
        sb.push_back(b.offset(kSlice));
    }
    CHECK(sa != sb);
    CHECK(requests(std::nullopt, 100, k60Hz) != requests(std::nullopt, 100, k60Hz));
}

TEST(ac11_jittered_slice_ends_hit_more_than_one_loop_position) {
    // A loop of 8 instructions of 5 ticks: 40 ticks, which divide the slice
    // of 2,000.
    for (bool jitter : {false, true}) {
        ScriptedClock clock;
        auto m = buildTimedTestSeg(5, 8);
        TestSliceRunner runner(*m, clock);
        Pacer pacer(clock.fn(), 59u);
        pacer.setMachine(fixedProfile(jitter), kSegRate);
        while (runner.slices.size() < 50) {
            clock.now += k60Hz;
            pacer.pace(Speed::Regular, runner);
        }
        std::set<std::uint64_t> positions;
        for (std::size_t k = 0; k < 50; ++k) positions.insert(runner.slices[k].loopPosition);
        if (jitter) CHECK(positions.size() > 1);
        else CHECK_EQ(positions.size(), std::size_t{1});   // without jitter: always the same position
    }
}
