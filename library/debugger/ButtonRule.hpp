#pragma once
#include "debugger/RunState.hpp"
#include <chrono>
#include <optional>

// The rule of the three run-control buttons of a level: pause can be used
// while the machine runs; step and resume while it is paused and the
// buttons' level owns the pause. Step fires on press and then every 250 ms while held, until release
// or until step cannot be used (Calc-U-59 HoldRepeatButton.swift:11-12).
// The Qt6 widget (library/qt6/RunControlButtons) and the React component
// (library/web/components/RunControlButtons.jsx) follow the same rule; its
// cases are one table, library/debugger/tests/button_cases.json.

namespace calcu::debugger {

struct ButtonUse {
    bool pause = false, step = false, resume = false;
    bool operator==(const ButtonUse&) const = default;
};

ButtonUse buttonUse(const LevelName& level, const RunState* state);   // no state yet: nothing can be used

// The hold repeat of step: the times at which a held step fires.
class StepRepeat {
public:
    static constexpr std::chrono::milliseconds kInterval{250};   // four times a second
    // Pressed at `now`: fires at once.
    bool press(std::chrono::milliseconds now);
    void release() { held_.reset(); }
    // True when it fires at `now` (once per interval passed; never after a
    // release or while step cannot be used).
    bool tick(std::chrono::milliseconds now, bool stepUsable);
    bool held() const { return held_.has_value(); }

private:
    std::optional<std::chrono::milliseconds> held_;   // when it fires next
};

} // namespace calcu::debugger
