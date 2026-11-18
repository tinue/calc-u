#pragma once
#include "pacing/WallClock.hpp"
#include <chrono>
#include <optional>

// "Loading…": shown
// when an operation has run at full speed for 500 ms of wall time without a
// break; hidden at once when it ends or goes on at regular speed; the next
// stretch at full speed counts from its start. Wall time comes from the
// caller. Mirrored in shellModel.js (calcuShellLoading).

namespace calcu::shell {

class LoadingRule {
public:
    static constexpr std::chrono::milliseconds kDelay{500};

    // At each change of the busy topic (and at any other time: unchanged
    // input keeps the stretch).
    void busy(bool operationRuns, bool fullSpeed, pacing::WallTime now);
    bool shown(pacing::WallTime now) const;
    // When to look again (a timer), while a stretch has not reached the delay.
    std::optional<pacing::WallTime> due() const;

private:
    std::optional<pacing::WallTime> since_;
};

} // namespace calcu::shell
