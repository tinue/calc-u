#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <algorithm>
#include <array>

// LIB-1 AC-19 (design §6, T10).

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
std::vector<std::int16_t> drain(Machine& m) {
    std::vector<std::int16_t> all;
    std::array<std::int16_t, 64> buf{};   // small on purpose: several takes per drain
    for (std::size_t n; (n = m.takeSound(buf)) > 0;) all.insert(all.end(), buf.begin(), buf.begin() + n);
    return all;
}

// Runs [0, end) in slices of `slice` ticks (the last one shorter) and drains
// after every slice.
std::vector<std::int16_t> run(Machine& m, Ticks end, Ticks slice) {
    std::vector<std::int16_t> all;
    while (m.tickCount() < end) {
        m.advance(std::min(slice, end - m.tickCount()));
        auto part = drain(m);
        all.insert(all.end(), part.begin(), part.end());
    }
    return all;
}
} // namespace

TEST(ac19_samples_at_named_rate_independent_of_slicing) {
    const Ticks end = 100'000;   // 0.1 s of test-dot
    for (int rate : {8'000, 11'025}) {
        auto once = buildTestMachine(kDot, std::nullopt, {}, rate);
        auto small = buildTestMachine(kDot, std::nullopt, {}, rate);
        auto odd = buildTestMachine(kDot, std::nullopt, {}, rate);
        CHECK(once && small && odd);
        if (!once || !small || !odd) return;
        const auto a = run(*once, end, end);
        const auto b = run(*small, end, 100);
        const auto c = run(*odd, end, 7);
        CHECK_EQ(once->tickCount(), end);
        CHECK_EQ(odd->tickCount(), end);
        // The samples of the stretch, at the rate named at the build.
        CHECK_EQ(a.size(), std::size_t((end * rate + kDotRate - 1) / kDotRate));   // 800, 1103
        CHECK(a == b);
        CHECK(a == c);
        CHECK(std::any_of(a.begin(), a.end(), [](std::int16_t s) { return s != 0; }));
    }
}

TEST(ac19_machine_without_sound_gives_none) {
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->descriptor().hasSound);
    m->advance(100'000);
    std::array<std::int16_t, 64> buf{};
    CHECK_EQ(m->takeSound(buf), std::size_t{0});
}
