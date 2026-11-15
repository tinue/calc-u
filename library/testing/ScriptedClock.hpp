#pragma once
#include "pacing/WallClock.hpp"
#include <chrono>

// The wall clock of the tests (library/testing/): it moves only when the
// test moves it, plus `step` at every reading (0 by default). Any epoch will
// do; it starts at one hour. A test with no slice runner that moves the clock
// sets a step (the session tests: 1 ms), so that the pacer's full speed ends
// when its allowance is used up. Never the host's clock.

namespace calcu::testing {

struct ScriptedClock {
    pacing::WallTime now = std::chrono::hours(1);
    pacing::WallTime step{0};   // per reading
    pacing::WallClock fn() {
        return [this] {
            const pacing::WallTime t = now;
            now += step;
            return t;
        };
    }
};

} // namespace calcu::testing
