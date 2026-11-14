#pragma once
#include "TestMachines.hpp"
#include "pacing/Pacer.hpp"
#include <cstdint>
#include <functional>
#include <vector>

// A stand-in for the machine session (LIB-17 design §6): it runs each slice
// on a machine built for the tests and moves a wall clock supplied by the
// test by the host time a slice takes. Scripts say per slice whether the
// machine waits for input and whether the slice stops early.

namespace calcu::pacing::test {

using machine::Ticks;
using machine::test::TestMachine;

// The wall clock of a test: it moves only when the test or the runner moves
// it. Any epoch will do; it starts at one hour.
struct ScriptedClock {
    WallTime now = std::chrono::hours(1);
    WallClock fn() {
        return [this] { return now; };
    }
};

struct SliceRecord {
    SliceRequest request;
    SliceResult result;
    Ticks tickAfter;              // the machine's tick count after the slice
    std::uint64_t loopPosition;   // the machine's loop position after the slice
    WallTime wallAfter;           // the clock after the slice
};

class TestSliceRunner : public SliceRunner {
public:
    TestSliceRunner(TestMachine& m, ScriptedClock& c) : machine(&m), clock(&c) {}

    SliceResult runSlice(const SliceRequest& r) override {
        const int i = static_cast<int>(slices.size());
        SliceResult res;
        res.ticksRun = machine->advance(r.ticks);
        res.stoppedEarly = stops && stops(i);
        res.waitsForInput = waits && waits(i);
        clock->now += costPerSlice;
        if (hostSlowdown > 0)
            clock->now += WallTime(static_cast<WallTime::rep>(
                static_cast<double>(wallTimeOf(res.ticksRun, machine->descriptor().tickRate).count()) * hostSlowdown));
        slices.push_back(SliceRecord{r, res, machine->tickCount(), machine->loopPosition(), clock->now});
        return res;
    }

    TestMachine* machine;
    ScriptedClock* clock;
    WallTime costPerSlice{0};           // host time of every slice
    double hostSlowdown = 0;            // host time per emulated time of a slice (EC-6)
    std::function<bool(int)> waits;     // by slice index: the machine waits for input
    std::function<bool(int)> stops;     // by slice index: the slice stopped early
    std::vector<SliceRecord> slices;
};

// Profiles of the tests: a fixed slice of 20 ms, and "the call".
inline PacingProfile fixedProfile(bool jitter = false, bool pacedWhileWaiting = false) {
    return PacingProfile{std::chrono::microseconds(20'000), jitter, pacedWhileWaiting};
}
inline PacingProfile callProfile(bool pacedWhileWaiting = false) {
    return PacingProfile{std::nullopt, false, pacedWhileWaiting};
}

// Emulated time minus wall time since a baseline, in ticks.
inline std::int64_t deviation(const TestMachine& m, Ticks ticks0, WallTime wall, WallTime wall0) {
    const auto rate = m.descriptor().tickRate;
    return static_cast<std::int64_t>(m.tickCount() - ticks0) - static_cast<std::int64_t>(ticksIn(wall - wall0, rate));
}

// A small deterministic generator for scripted call jitter (not the pacer's).
struct ScriptRandom {
    std::uint64_t state;
    std::int64_t below(std::int64_t n) {   // 0 .. n - 1
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return static_cast<std::int64_t>((state >> 33) % static_cast<std::uint64_t>(n));
    }
};

} // namespace calcu::pacing::test
