#pragma once
#include "pacing/WallClock.hpp"
#include <chrono>

// The wall clock of the session tests (LIB-3 design §6). Like LIB-17's
// ScriptedClock (pacing/tests/TestSliceRunner.hpp) it moves only when the
// test moves it, plus a fixed step at every reading: the session tests have
// no slice runner that moves the clock, and the pacer's full speed runs until
// its allowance is used up on this clock. Never the host's clock.

namespace calcu::sessions::test {

using namespace std::chrono_literals;

struct ScriptedClock {
    pacing::WallTime now = std::chrono::hours(1);
    pacing::WallTime step = 1ms;   // per reading
    pacing::WallClock fn() {
        return [this] {
            const pacing::WallTime t = now;
            now += step;
            return t;
        };
    }
};

} // namespace calcu::sessions::test
