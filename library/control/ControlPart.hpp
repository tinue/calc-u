#pragma once
#include "control/Compute.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <utility>

// The control state in the app session: declares the state topic
// `control-state` and computes it at every flush (SessionPart::beforeFlush),
// so at the end of every host call, command, request and service answer;
// the session sends it only when it differs from what was last sent.
// Without a machine nothing is set: a product that could not start sends no
// control state. It comes
// among the library's parts, before the product's.

namespace calcu::control {

class ControlPart final : public sessions::SessionPart {
public:
    explicit ControlPart(ControlSetup setup) : setup_(std::move(setup)) {}
    void attach(sessions::SessionSetup&) override;
    void beforeFlush(sessions::SessionContext&) override;

private:
    ControlSetup setup_;
    sessions::StateTopic* topic_ = nullptr;
};

} // namespace calcu::control
