#pragma once
#include "machine/EmulatedTime.hpp"
#include "presets/ProductRules.hpp"
#include "sessions/Operation.hpp"
#include <optional>

// Idle by a product's rule (LIB-6 design §3.6, D-8; spec AC-53, AC-56,
// AC-57, AC-19): the machine runs at the operation's speed in pieces of
// min(rule.sample, OperationContext::runPiece), readyNow asked after each;
// idle when it has been ready without a break for the rule's stretch. A
// paused machine is not idle: the wait goes on in the host's time until it
// runs again, and no tick passes meanwhile, so the bound stands still.

namespace calcu::presets {

struct IdleOutcome {
    bool idle = false;
    bool boundReached = false;   // `bound` ticks ran without the machine becoming idle
    bool noMachine = false;
    bool paused = false;         // paused in virtual time, which cannot wait for it
    machine::Ticks waited = 0;   // the ticks that ran
};

class IdleWatch {
public:
    explicit IdleWatch(IdleRule rule) : rule_(std::move(rule)) {}
    // nullopt: without a bound (`let-it-finish:`, AC-59).
    sessions::Task<IdleOutcome> wait(sessions::OperationContext&, std::optional<machine::Ticks> bound);

private:
    IdleRule rule_;
};

// Waits while the machine is paused, in the host's time (AC-19). False when
// the session has no wall clock (virtual time) and the machine is paused.
sessions::Task<bool> untilRunning(sessions::OperationContext&);

} // namespace calcu::presets
