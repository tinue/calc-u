#include "TestHarness.hpp"
#include "machine/EmulatedTime.hpp"
#include <cmath>
#include <cstdint>
#include <vector>

// EmulatedTime: conversions and comparisons across tick rates.

using namespace calcu::machine;

namespace {
// Every tick rate in the catalogues.
const std::vector<TickRate> kRates{384'000, 394'000, 455'000, 1'300'000, 3'580'000};
constexpr Ticks kDay = 86'400;

std::vector<Ticks> countsFor(TickRate rate) {
    const Ticks year = 366 * kDay * rate;   // one year of emulated time, rounded up
    std::vector<Ticks> c{0, 1, rate - 1ull, rate, 2ull * rate, 59ull * rate, 3'600ull * rate, kDay * rate,
                         Ticks{1} << 40, year};
    std::uint64_t x = 0x9E3779B97F4A7C15ull ^ rate;   // a fixed pseudo-random set below the year
    for (int i = 0; i < 2'000; ++i) {
        x = x * 6364136223846793005ull + 1442695040888963407ull;
        c.push_back((x >> 11) % year);
    }
    return c;
}
} // namespace

TEST(round_trip_every_catalogue_rate) {
    for (TickRate rate : kRates) {
        int wrong = 0;
        for (Ticks t : countsFor(rate)) {
            EmulatedTime e(t, rate);
            if (EmulatedTime::fromSeconds(e.seconds(), rate).ticks() != t) ++wrong;
        }
        CHECK_EQ(wrong, 0);
    }
}

TEST(whole_multiples_give_whole_seconds) {
    for (TickRate rate : kRates) {
        int wrong = 0;
        for (Ticks k : {Ticks{0}, Ticks{1}, Ticks{7}, Ticks{3'600}, kDay, 366 * kDay}) {
            const double s = EmulatedTime(k * rate, rate).seconds().count();
            if (s != static_cast<double>(k) || std::floor(s) != s) ++wrong;
            if (EmulatedTime::fromSeconds(Seconds(static_cast<double>(k)), rate).ticks() != k * rate) ++wrong;
        }
        CHECK_EQ(wrong, 0);
    }
}

TEST(compare_and_add_across_rates) {
    // One second on a TI-59 and one on a PC-1600: equal, though the counts differ.
    const EmulatedTime ti(455'000, 455'000), sharp(3'580'000, 3'580'000);
    CHECK(ti == sharp);
    CHECK((ti <=> sharp) == 0);
    // Equal counts at different rates are different times.
    CHECK(EmulatedTime(1'000, 384'000) > EmulatedTime(1'000, 3'580'000));
    // One tick apart after a year, and after 2^40 seconds, where doubles no
    // longer tell the counts apart: compared exactly.
    for (Ticks secs : {366 * kDay, Ticks{1} << 40}) {
        const EmulatedTime a(secs * 455'000 + 1, 455'000), b(secs * 3'580'000, 3'580'000);
        CHECK(a > b);
        CHECK(b < a);
        CHECK(EmulatedTime(secs * 455'000, 455'000) == b);
    }
    // Adding and subtracting happen in seconds.
    CHECK_EQ((ti + sharp).count(), 2.0);
    CHECK_EQ((EmulatedTime(910'000, 455'000) - sharp).count(), 1.0);
}
