#include "shell/Loading.hpp"

namespace calcu::shell {

void LoadingRule::busy(bool operationRuns, bool fullSpeed, pacing::WallTime now) {
    if (operationRuns && fullSpeed) {
        if (!since_) since_ = now;
    } else {
        since_.reset();
    }
}

bool LoadingRule::shown(pacing::WallTime now) const { return since_ && now - *since_ >= kDelay; }

std::optional<pacing::WallTime> LoadingRule::due() const {
    if (!since_) return std::nullopt;
    return *since_ + std::chrono::duration_cast<pacing::WallTime>(kDelay);
}

} // namespace calcu::shell
