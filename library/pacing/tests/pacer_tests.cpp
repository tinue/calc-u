#include "TestHarness.hpp"
#include "TestSliceRunner.hpp"
#include <cstdlib>
#include <memory>

// The pacer at regular and full speed, its profiles and starting afresh.
// Every test runs on a wall clock of its own (ScriptedClock), never the
// host's, so no result depends on the speed of the host.

using namespace calcu::pacing;
using namespace calcu::pacing::test;
using calcu::machine::test::buildTimedTestSeg;
using calcu::machine::test::kDot;
using calcu::machine::test::kDotRate;
using calcu::machine::test::kSegRate;

namespace {

const WallTime k60Hz{16'666'667};
const WallTime k120Hz{8'333'333};
const WallTime kMs{1'000'000};
constexpr Ticks kSlice = 2'000;   // 20 ms at test-seg's 100,000 ticks per second
constexpr Ticks kCallSlice = kSegRate / 60;
constexpr std::int64_t kMsTicks = kSegRate / 1000;   // the host time of a slice in some tests

// A test-seg machine with a fixed instruction length, its runner, a pacer
// with a seed and the baseline of the deviation.
struct Rig {
    ScriptedClock clock;
    std::unique_ptr<TestMachine> m;
    TestSliceRunner runner;
    Pacer pacer;
    Ticks t0 = 0;
    WallTime w0{0};

    explicit Rig(const PacingProfile& p, Ticks len = 1, std::uint64_t loop = 1,
                 std::optional<std::uint32_t> seed = 17u)
        : m(buildTimedTestSeg(len, loop)), runner(*m, clock), pacer(clock.fn(), seed) {
        pacer.setMachine(p, kSegRate);
        mark();
    }
    void mark() {
        t0 = m->tickCount();
        w0 = clock.now;
    }
    std::int64_t dev() const { return deviation(*m, t0, clock.now, w0); }
    PaceResult call(WallTime after, Speed s = Speed::Regular) {
        clock.now += after;
        return pacer.pace(s, runner);
    }
};

// How far emulated time may be from wall time at the end of a call: fixed,
// one slice plus the last instruction; the call, the time between two calls
// plus the last instruction.
std::int64_t fixedBound(Ticks len) { return static_cast<std::int64_t>(kSlice + len); }
std::int64_t callBound(WallTime interval, Ticks len) {
    return static_cast<std::int64_t>(ticksIn(interval, kSegRate) + len);
}

} // namespace

// --- regular speed -----------------------------------------------------------

TEST(call_slice_keeps_wall_time_at_60_hz) {
    Rig r(callProfile(), 3);
    int outside = 0, notOne = 0;
    for (int i = 0; i < 600; ++i) {   // 10 s
        r.clock.now += k60Hz;
        if (r.dev() < -callBound(k60Hz, 3)) ++outside;   // just before the call
        const PaceResult res = r.pacer.pace(Speed::Regular, r.runner);
        if (res.slices != 1) ++notOne;                   // one slice: the call
        if (r.dev() < 0 || r.dev() >= 3) ++outside;      // after it: due ticks, plus less than an instruction
    }
    CHECK_EQ(outside, 0);
    CHECK_EQ(notOne, 0);
}

TEST(fixed_slice_keeps_wall_time) {
    for (bool jitter : {false, true}) {
        Rig r(fixedProfile(jitter), 3);
        int outside = 0, wrongLength = 0;
        for (int i = 0; i < 600; ++i) {
            r.call(k60Hz);
            if (std::llabs(r.dev()) > fixedBound(3)) ++outside;
        }
        for (const auto& s : r.runner.slices)   // slices of 20 ms, ± half a slice with jitter
            if (!jitter ? s.request.ticks > kSlice : (s.request.ticks > kSlice + kSlice / 2 || s.request.ticks < kSlice / 2 - 3))
                ++wrongLength;
        CHECK_EQ(outside, 0);
        CHECK_EQ(wrongLength, 0);
        CHECK(r.runner.slices.size() >= 499);   // 10 s of 20 ms slices
    }
}

TEST(uneven_calls_do_not_add_up) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        ScriptRandom rnd{2};
        int outside = 0, dropped = 0;
        const WallTime longest = k60Hz + 4 * kMs + 30 * kMs;
        for (int i = 0; i < 6'000; ++i) {   // about 100 s
            WallTime d = k60Hz + WallTime(rnd.below(8'000'001) - 4'000'000);   // timer jitter ± 4 ms
            if (i % 50 == 49) d += 30 * kMs;                                     // an oversleep
            const PaceResult res = r.call(d);
            if (res.lagDropped) ++dropped;
            const std::int64_t bound = fixed ? fixedBound(3) : callBound(longest, 3);
            if (std::llabs(r.dev()) > bound) ++outside;
        }
        CHECK_EQ(dropped, 0);
        CHECK_EQ(outside, 0);
        CHECK(std::llabs(r.dev()) <= (fixed ? fixedBound(3) : 3));   // at the end too: nothing added up
    }
}

TEST(overshoot_does_not_add_up) {
    for (bool fixed : {true, false})
        for (Ticks len : {Ticks{7}, Ticks{37}}) {   // neither divides a slice: every slice overshoots
            Rig r(fixed ? fixedProfile(true) : callProfile(), len);
            int outside = 0;
            for (int i = 0; i < 6'000; ++i) {
                r.call(k60Hz);
                if (std::llabs(r.dev()) > (fixed ? fixedBound(len) : callBound(k60Hz, len))) ++outside;
            }
            Ticks overshoot = 0;
            for (const auto& s : r.runner.slices) overshoot += s.result.ticksRun - s.request.ticks;
            CHECK(overshoot > 5 * kSlice);   // dropped, the overshoots would add up to far beyond the bound
            CHECK_EQ(outside, 0);
            CHECK(r.dev() >= 0 ? r.dev() < static_cast<std::int64_t>(fixed ? kSlice + len : len)
                               : r.dev() > -static_cast<std::int64_t>(kSlice));
        }
}

// --- lag ---------------------------------------------------------------------

TEST(lag_over_limit_dropped_whole) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        for (int i = 0; i < 60; ++i) CHECK(!r.call(k60Hz).lagDropped);
        // 90 ms late: under the limit, caught up.
        PaceResult res = r.call(90 * kMs);
        CHECK(!res.lagDropped);
        CHECK(std::llabs(r.dev()) <= (fixed ? fixedBound(3) : 3));
        // 500 ms late: over the limit, dropped whole.
        r.clock.now += 500 * kMs;
        const Ticks before = r.m->tickCount();
        const WallTime at = r.clock.now;
        res = r.pacer.pace(Speed::Regular, r.runner);
        CHECK(res.lagDropped);
        CHECK(res.ticksRun <= (fixed ? kSlice : kCallSlice) + 3);   // one slice at most, nothing caught up
        CHECK(res.ticksRun > 0);
        // From the moment of that call, pacing goes on.
        r.t0 = before;
        r.w0 = at;
        int outside = 0, dropped = 0;
        for (int i = 0; i < 120; ++i) {
            if (r.call(k60Hz).lagDropped) ++dropped;
            if (std::llabs(r.dev()) > (fixed ? fixedBound(3) : callBound(k60Hz, 3))) ++outside;
        }
        CHECK_EQ(dropped, 0);
        CHECK_EQ(outside, 0);
    }
}

// --- full speed --------------------------------------------------------------

TEST(full_speed_stops_at_14_ms_of_the_supplied_clock) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        r.runner.costPerSlice = kMs;   // the host takes 1 ms per slice
        const WallTime start = r.clock.now;
        const PaceResult res = r.pacer.pace(Speed::Full, r.runner);
        CHECK_EQ(res.slices, 14);
        CHECK(res.ranUnpaced);
        CHECK(r.clock.now - start >= Pacer::kFullSpeedAllowance);
        CHECK(r.clock.now - start < Pacer::kFullSpeedAllowance + kMs);
        CHECK(res.ticksRun >= 14 * (fixed ? kSlice / 2 : kCallSlice));   // far beyond the 14 ms due
        if (!fixed)
            for (const auto& s : r.runner.slices) CHECK_EQ(s.request.ticks, kCallSlice);   // 1/60 s
        // A host slower than the allowance still runs one slice per call.
        r.runner.costPerSlice = 40 * kMs;
        CHECK_EQ(r.pacer.pace(Speed::Full, r.runner).slices, 1);
    }
    CHECK_EQ(Pacer::kFullSpeedAllowance.count(), 14);
}

TEST(back_to_regular_neither_waits_nor_lags) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        r.runner.costPerSlice = kMs / 4;
        for (int i = 0; i < 30; ++i) r.call(k60Hz);
        for (int i = 0; i < 30; ++i) r.call(k60Hz, Speed::Full);   // gains emulated time
        CHECK(r.dev() > static_cast<std::int64_t>(kSlice) * 10);
        r.mark();
        int outside = 0, dropped = 0, idle = 0;
        for (int i = 0; i < 120; ++i) {
            const PaceResult res = r.call(k60Hz);
            if (res.lagDropped) ++dropped;
            if (i == 0 && res.slices == 0) ++idle;   // the gain is not waited out
            if (std::llabs(r.dev()) > (fixed ? fixedBound(3) : callBound(k60Hz, 3)) + kMsTicks) ++outside;
        }
        CHECK_EQ(idle, 0);
        CHECK_EQ(dropped, 0);
        CHECK_EQ(outside, 0);
    }
}

TEST(full_speed_keeps_tick_count) {
    Rig r(fixedProfile(true), 5);
    r.runner.costPerSlice = kMs;
    Ticks paced = 0;
    for (int i = 0; i < 20; ++i) paced += r.call(k60Hz, Speed::Full).ticksRun;
    Ticks reported = 0;
    for (const auto& s : r.runner.slices) reported += s.result.ticksRun;
    CHECK_EQ(paced, reported);
    CHECK_EQ(r.m->tickCount(), reported);                    // what the machine ran, unchanged
    CHECK_EQ(r.m->tickCount(), 5 * r.m->instructionsRun());  // the time the real device would take
    CHECK(r.m->elapsed().seconds().count() > 20 * 0.0167);   // more emulated time than wall time passed
}

// --- pacing profile ----------------------------------------------------------

TEST(profile_slice_fixed_or_the_call_jitter_and_paced_flag) {
    const PacingProfile fixed = fixedProfile(true, true);
    CHECK(fixed.slice.has_value());
    CHECK_EQ(fixed.slice->count(), 20'000);
    CHECK(fixed.jitter);
    CHECK(fixed.fullSpeedPacedWhileWaiting);
    const PacingProfile call{};
    CHECK(!call.slice.has_value());
    CHECK(!call.jitter);
    CHECK(!call.fullSpeedPacedWhileWaiting);
    // The call: one slice per call of what is due. Fixed: slices of the length.
    Rig c(call, 1);
    c.call(50 * kMs);
    CHECK_EQ(c.runner.slices.size(), std::size_t{1});
    CHECK_EQ(c.runner.slices[0].request.ticks, ticksIn(50 * kMs, kSegRate));
    Rig f(fixedProfile(), 1);
    f.call(50 * kMs);
    CHECK_EQ(f.runner.slices.size(), std::size_t{3});   // ends 20, 40, 60 ms: up to one slice ahead
    for (const auto& s : f.runner.slices) CHECK_EQ(s.request.ticks, kSlice);
}

// --- full speed paced while the machine waits --------------------------------

TEST(waiting_at_full_speed_waits_one_slice_after_last_unpaced) {
    for (bool jitter : {false, true}) {
        Rig r(fixedProfile(jitter, true), 1);
        r.runner.costPerSlice = kMs;
        r.runner.waits = [](int i) { return i >= 3; };
        const PaceResult first = r.pacer.pace(Speed::Full, r.runner);
        CHECK_EQ(first.slices, 4);   // three unpaced, the fourth reports waiting and ends the call
        const WallTime lastUnpacedEnd = r.runner.slices[2].wallAfter;
        // Nothing further until one slice length of wall time has passed.
        std::size_t seen = r.runner.slices.size();
        WallTime nextAt{0};
        for (int i = 0; i < 60 && nextAt == WallTime{0}; ++i) {
            const PaceResult res = r.call(kMs, Speed::Full);
            if (r.runner.slices.size() > seen) nextAt = r.clock.now - kMs * static_cast<long>(res.slices);
            CHECK(!res.ranUnpaced);
        }
        CHECK(nextAt - lastUnpacedEnd >= std::chrono::milliseconds(20));
        if (!jitter) CHECK(nextAt - lastUnpacedEnd < std::chrono::milliseconds(21));
        else CHECK(nextAt - lastUnpacedEnd <= std::chrono::milliseconds(31));   // plus at most half a slice
        // While it waits, the machine keeps the pace of regular speed.
        seen = r.runner.slices.size();
        r.mark();
        for (int i = 0; i < 1'000; ++i) r.call(kMs, Speed::Full);   // 1 s
        const std::size_t paced = r.runner.slices.size() - seen;
        CHECK(paced >= 48 && paced <= 52);
        CHECK(std::llabs(r.dev()) <= fixedBound(1) + static_cast<std::int64_t>(ticksIn(52 * kMs, kSegRate)));
    }
}

TEST(not_waiting_returns_to_full_speed) {
    Rig r(fixedProfile(true, true), 1);
    r.runner.costPerSlice = kMs;
    bool waiting = true;
    r.runner.waits = [&](int) { return waiting; };
    r.pacer.pace(Speed::Full, r.runner);
    for (int i = 0; i < 40; ++i) r.call(kMs, Speed::Full);   // paced
    const std::size_t before = r.runner.slices.size();
    waiting = false;
    // The next paced slice reports "not waiting": full speed again, in the same call.
    PaceResult res{};
    for (int i = 0; i < 40 && res.slices == 0; ++i) res = r.call(kMs, Speed::Full);
    CHECK(res.ranUnpaced);
    CHECK(res.slices >= 14);
    CHECK(r.runner.slices.size() - before >= 14);
    const PaceResult next = r.call(k60Hz, Speed::Full);
    CHECK_EQ(next.slices, 14);
    CHECK(next.ranUnpaced);
}

TEST(profile_without_flag_never_paced) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true, false) : callProfile(false), 1);
        r.runner.costPerSlice = kMs;
        r.runner.waits = [](int) { return true; };
        for (int i = 0; i < 5; ++i) {
            const PaceResult res = r.call(k60Hz, Speed::Full);
            CHECK_EQ(res.slices, 14);
            CHECK(res.ranUnpaced);
        }
    }
}

// --- starting afresh ----------------------------------------------------------

TEST(start_afresh_no_lag_nothing_to_wait) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        for (int i = 0; i < 60; ++i) r.call(k60Hz);
        // Suspended for 5 s, then continued: start afresh.
        r.clock.now += std::chrono::seconds(5);
        r.pacer.startAfresh();
        r.mark();
        PaceResult res = r.call(k60Hz);
        CHECK(!res.lagDropped);
        CHECK(res.slices >= 1);
        CHECK(res.ticksRun <= kSlice + 3);   // nothing caught up
        // Ahead at full speed, then afresh: nothing to wait out.
        r.runner.costPerSlice = kMs;
        r.call(k60Hz, Speed::Full);
        r.runner.costPerSlice = WallTime{0};
        r.pacer.startAfresh();
        r.mark();
        int outside = 0, dropped = 0;
        for (int i = 0; i < 120; ++i) {
            res = r.call(k60Hz);
            if (res.lagDropped) ++dropped;
            if (i == 0) CHECK(res.slices >= 1);
            if (std::llabs(r.dev()) > (fixed ? fixedBound(3) : callBound(k60Hz, 3))) ++outside;
        }
        CHECK_EQ(dropped, 0);
        CHECK_EQ(outside, 0);
    }
}

TEST(new_machine_new_rate_and_profile) {
    Rig r(fixedProfile(true), 3);
    for (int i = 0; i < 60; ++i) r.call(k60Hz);
    const Ticks segTicks = r.m->tickCount();
    // A model switch: test-dot at 1,000,000 ticks per second, slice "the call".
    auto dot = calcu::machine::test::buildTestMachine(kDot);
    CHECK(dot != nullptr);
    if (!dot) return;
    r.clock.now += std::chrono::seconds(2);
    r.runner.machine = dot.get();
    r.pacer.setMachine(callProfile(), kDotRate);
    const std::size_t first = r.runner.slices.size();
    const WallTime w0 = r.clock.now;
    int outside = 0, dropped = 0;
    for (int i = 0; i < 60; ++i) {
        if (r.call(k60Hz).lagDropped) ++dropped;
        const std::int64_t dev = deviation(*dot, 0, r.clock.now, w0);
        if (dev < 0 || dev >= 4) ++outside;   // the new rate, slice "the call", instructions of 4 ticks
    }
    CHECK_EQ(dropped, 0);
    CHECK_EQ(outside, 0);
    CHECK_EQ(r.runner.slices.size() - first, std::size_t{60});
    CHECK(r.runner.slices[first].request.ticks + 4 >= ticksIn(k60Hz, kDotRate));
    CHECK(r.runner.slices[first].request.ticks <= ticksIn(k60Hz, kDotRate));
    CHECK_EQ(r.m->tickCount(), segTicks);   // the old machine ran no more
    // And back, with a fixed slice of the old rate.
    r.runner.machine = r.m.get();
    r.pacer.setMachine(fixedProfile(false), kSegRate);
    r.call(k60Hz);
    CHECK_EQ(r.runner.slices.back().request.ticks, kSlice);
}

TEST(stopped_early_ends_the_call_resume_starts_afresh) {
    Rig r(fixedProfile(true), 3);
    r.runner.costPerSlice = kMs;
    int stopAt = 2;
    r.runner.stops = [&](int i) { return i == stopAt; };
    PaceResult res = r.pacer.pace(Speed::Full, r.runner);
    CHECK_EQ(res.slices, 3);   // the third stopped: the call ends
    // Resumed (afresh); regular: three slices due, the first stops.
    r.pacer.startAfresh();
    stopAt = 3;
    res = r.call(60 * kMs);
    CHECK_EQ(res.slices, 1);
    // Paused by the debugger: the session does not call. It resumes and starts afresh.
    r.clock.now += std::chrono::seconds(3);
    r.pacer.startAfresh();
    r.mark();
    res = r.call(k60Hz);
    CHECK(!res.lagDropped);
    CHECK(res.ticksRun <= kSlice + 3);
    CHECK(std::llabs(r.dev()) <= fixedBound(3));
}

// --- edge cases: repeated times, toggling, call rates, slow hosts ------------

TEST(call_slice_same_time_twice_runs_nothing) {
    for (bool fixed : {false, true}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        for (int i = 0; i < 10; ++i) r.call(k60Hz);
        const Ticks ticks = r.m->tickCount();
        const std::size_t slices = r.runner.slices.size();
        r.call(k60Hz);
        const Ticks after = r.m->tickCount();
        const PaceResult again = r.call(WallTime{0});
        CHECK_EQ(again.slices, 0);
        CHECK_EQ(again.ticksRun, Ticks{0});
        CHECK_EQ(r.m->tickCount(), after);
        CHECK(after >= ticks);
        CHECK(r.runner.slices.size() >= slices);
    }
}

TEST(toggling_full_speed_keeps_regular_stretches) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true, true) : callProfile(), 3);
        r.runner.costPerSlice = kMs / 4;
        int outside = 0, dropped = 0;
        for (int round = 0; round < 200; ++round) {
            r.call(k60Hz, Speed::Full);   // the display tapped
            r.mark();
            for (int i = 0; i < 1 + round % 4; ++i) {
                const PaceResult res = r.call(k60Hz);
                if (res.lagDropped) ++dropped;
                if (std::llabs(r.dev()) > (fixed ? fixedBound(3) : callBound(k60Hz, 3)) + kMsTicks) ++outside;
            }
        }
        CHECK_EQ(dropped, 0);
        CHECK_EQ(outside, 0);
    }
}

TEST(call_slice_at_120_hz) {
    for (bool fixed : {false, true}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        int outside = 0;
        for (int i = 0; i < 1'200; ++i) {   // 10 s
            r.call(k120Hz);
            if (std::llabs(r.dev()) > (fixed ? fixedBound(3) : callBound(k120Hz, 3))) ++outside;
        }
        CHECK_EQ(outside, 0);
        CHECK(std::llabs(r.dev()) <= (fixed ? fixedBound(3) : 3));
    }
}

TEST(throttled_caller_runs_slow_without_failing) {
    for (bool fixed : {true, false})
        for (WallTime interval : {150 * kMs, WallTime(std::chrono::seconds(1))}) {
            Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
            int notDropped = 0, idle = 0;
            for (int i = 0; i < 100; ++i) {
                const PaceResult res = r.call(interval);
                if (!res.lagDropped) ++notDropped;
                if (res.ticksRun == 0) ++idle;   // the machine still runs, one slice per call
            }
            CHECK_EQ(notDropped, 0);
            CHECK_EQ(idle, 0);
            CHECK(r.dev() < 0);   // slower than the real device
        }
}

TEST(slow_host_drops_lag_again_and_again) {
    for (bool fixed : {true, false}) {
        Rig r(fixed ? fixedProfile(true) : callProfile(), 3);
        r.runner.hostSlowdown = 2.0;   // the host needs 2 s of wall time per emulated second
        int dropped = 0;
        for (int i = 0; i < 3'000; ++i)
            if (r.call(k60Hz / 4).lagDropped) ++dropped;
        const double wall = std::chrono::duration<double>(r.clock.now - r.w0).count();
        const double emulated = r.m->elapsed().seconds().count();
        CHECK(dropped > 10);
        CHECK(emulated < wall);
        CHECK(emulated > 0.3 * wall);   // as fast as the host allows, near half
    }
}
