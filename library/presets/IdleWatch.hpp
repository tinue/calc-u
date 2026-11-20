#pragma once
#include "machine/EmulatedTime.hpp"
#include "presets/ProductRules.hpp"
#include "sessions/Operation.hpp"
#include <optional>

// Settled (or finished) by a product's rule: the machine runs at the
// operation's speed in pieces of min(rule.sample, OperationContext::runPiece),
// the rule asked after each; done when it has waited (and its program has
// ended) without a break for the rule's stretch. A paused machine is not idle: the wait goes on in the host's time
// until it runs again, and no tick passes meanwhile, so the bound stands
// still.

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
    // Until settled; nullopt: without a bound (`let-it-finish:`).
    sessions::Task<IdleOutcome> wait(sessions::OperationContext&, std::optional<machine::Ticks> bound);
    // Until finished: settled with its program ended.
    sessions::Task<IdleOutcome> waitFinished(sessions::OperationContext&, std::optional<machine::Ticks> bound);

private:
    sessions::Task<IdleOutcome> watch(sessions::OperationContext&, std::optional<machine::Ticks> bound, bool finished);
    IdleRule rule_;
};

// Waits while the machine is paused, in the host's time. False when
// the session has no wall clock (virtual time) and the machine is paused.
sessions::Task<bool> untilRunning(sessions::OperationContext&);

} // namespace calcu::presets
