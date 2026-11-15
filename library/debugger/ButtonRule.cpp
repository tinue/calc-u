#include "debugger/ButtonRule.hpp"

namespace calcu::debugger {

ButtonUse buttonUse(const LevelName& level, const RunState* s) {
    ButtonUse u;
    if (!s) return u;
    u.pause = !s->paused;
    const bool owns = s->paused && s->owner && *s->owner == level;
    u.step = owns;
    u.resume = owns;
    return u;
}

bool StepRepeat::press(std::chrono::milliseconds now) {
    held_ = now + kInterval;
    return true;
}

bool StepRepeat::tick(std::chrono::milliseconds now, bool stepUsable) {
    if (!held_) return false;
    if (!stepUsable) {   // until step cannot be used
        held_.reset();
        return false;
    }
    if (now < *held_) return false;
    *held_ += kInterval;
    return true;
}

} // namespace calcu::debugger
