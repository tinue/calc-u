#include "control/ControlPart.hpp"
#include <memory>

namespace calcu::control {

void ControlPart::attach(sessions::SessionSetup& s) { topic_ = &s.stateTopic(kControlStateTopic); }

void ControlPart::beforeFlush(sessions::SessionContext& c) {
    const machine::Machine* m = c.machine();
    if (!m || !topic_) return;   // no machine: nothing is set, nothing is sent
    topic_->set(std::make_shared<ControlState>(computeControlState(*m, setup_, c.busy())));
}

} // namespace calcu::control
