#pragma once
#include "control/Compute.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <utility>

// The control state in the app session (LIB-4 design §3.5; spec AC-4, AC-6,
// AC-8, AC-9): declares the state topic `control-state` and computes it at
// every flush (SessionPart::beforeFlush), so at the end of every host call,
// command, request and service answer; LIB-3 sends it only when it differs
// from what was last sent (LIB-3 AC-5). Without a machine nothing is set:
// a product that could not start sends no control state (AC-6). It comes
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
