#include "debugger/Levels.hpp"

namespace calcu::debugger {

void LevelRules::startArmed(const LevelName&, machine::Machine& m) { seenReady_ = m.isKeyReady(); }

bool LevelRules::startFiresAfter(const LevelName& level, const machine::InstructionEvent&, const machine::Machine& m) {
    if (level != kCpu) return false;
    if (m.isKeyReady()) {
        seenReady_ = true;
        return false;
    }
    return seenReady_;
}

} // namespace calcu::debugger
