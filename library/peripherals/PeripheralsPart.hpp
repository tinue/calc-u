#pragma once
#include "peripherals/HostLinks.hpp"
#include "sessions/SessionPart.hpp"
#include <optional>
#include <string>

// The peripherals' session part: the app session's host links and real-time clocks.
// - The host links are pumped before and after every host call, on the
//   session's thread, between two advances, and rebound when the
//   machine changes.
// - The real-time clocks are set from the session's host-time source when a
//   machine is built and at the end of every operation.
// - The command `set-control {peripheral, place?, control, on}`: a switch of a plugged peripheral set, or a key of it pressed
//   (`on: true`) or released, through the controls capability; the
//   harness's `set-control` and the paper panel send it.
// A product without serial ports passes no rules.

namespace calcu::peripherals {

inline const sessions::CommandName kSetControlCommand{"set-control"};

class PeripheralsPart final : public sessions::SessionPart {
public:
    explicit PeripheralsPart(HostLinkRules rules = {}, std::optional<std::string> folder = std::nullopt,
                             HostLinkFactory factory = {});

    HostLinks& hostLinks() { return links_; }
    const HostLinks& hostLinks() const { return links_; }

    void attach(sessions::SessionSetup&) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void machineChanged(sessions::SessionContext&) override;
    void operationEnded(sessions::SessionContext&) override;

private:
    void pump(sessions::SessionContext&);
    void reportLinkProblems(sessions::SessionContext&);
    HostLinks links_;
};

} // namespace calcu::peripherals
