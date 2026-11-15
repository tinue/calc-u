#pragma once
#include "peripherals/HostLinks.hpp"
#include "sessions/SessionPart.hpp"
#include <optional>
#include <string>

// LIB-2's session part (LIB-2 design §9 A-2, A-4; LIB-3 design A-1, A-7): the
// app session's host links and real-time clocks.
// - The host links are pumped before and after every host call, on the
//   session's thread, between two advances (AC-18), and rebound when the
//   machine changes (AC-20).
// - The real-time clocks are set from the session's host-time source when a
//   machine is built and at the end of every operation (AC-23).
// - The command `set-control {peripheral, place?, control, on}` (LIB-6
//   D-16): a switch of a plugged peripheral set, or a key of it pressed
//   (`on: true`) or released, through the controls capability (AC-16); the
//   harness's `switch` and LIB-8's paper panel send it.
// A product without serial ports passes no rules.

namespace calcu::peripherals {

inline const sessions::CommandName kSetControlCommand{"set-control"};

class PeripheralsPart final : public sessions::SessionPart {
public:
    explicit PeripheralsPart(HostLinkRules rules = {}, std::optional<std::string> folder = std::nullopt,
                             HostLinkFactory factory = {});

    HostLinks& hostLinks() { return links_; }

    void attach(sessions::SessionSetup&) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void machineChanged(sessions::SessionContext&) override;
    void operationEnded(sessions::SessionContext&) override;

private:
    void pump(sessions::SessionContext&);
    HostLinks links_;
};

} // namespace calcu::peripherals
