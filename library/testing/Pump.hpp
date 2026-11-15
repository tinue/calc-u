#pragma once
#include "pacing/WallClock.hpp"
#include "sessions/AppSession.hpp"
#include <chrono>

// The host call of the tests (library/testing/): an app session called at
// 60 Hz on a wall time the test keeps (`now`; it moves only here and where
// the test moves it). For the library's tests and both products'.

namespace calcu::testing {

inline constexpr pacing::WallTime kFrame = std::chrono::microseconds(16'667);

// One host call a frame after the last.
inline void pumpFrame(sessions::AppSession& s, pacing::WallTime& now, pacing::WallTime frame = kFrame) {
    now += frame;
    s.call(now);
}
// Host calls until `d` of wall time passed.
inline void pumpFor(sessions::AppSession& s, pacing::WallTime& now, pacing::WallTime d,
                    pacing::WallTime frame = kFrame) {
    for (const auto end = now + d; now < end;) pumpFrame(s, now, frame);
}
// Host calls until `cond` holds (checked before each), at most `limit` of wall time.
template <class F>
bool pumpUntil(sessions::AppSession& s, pacing::WallTime& now, F cond, pacing::WallTime limit,
               pacing::WallTime frame = kFrame) {
    for (const auto end = now + limit; now < end;) {
        if (cond()) return true;
        pumpFrame(s, now, frame);
    }
    return cond();
}

} // namespace calcu::testing
