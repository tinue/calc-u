#pragma once
#include "control/ControlRules.hpp"
#include "control/ControlState.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include <vector>

// The computation of the control state: a pure function of the running
// machine, the product's setup and whether an operation runs. Nothing is
// cached between computations; the session's topic sends only a change.
//
// Availability, in this order: the first that applies gives the reason.
//   1. An operation runs: every entry but the models, `reset` and
//      `cold-boot` (they end the operation); the sentence
//      sessions::kAnotherOperationRuns, the refusal the command would get,
//      so the tooltip and the refusal read the same.
//   2. A rule says so; a refused place passes its reason to its empty
//      choice and to every peripheral and variant in it, so a frontend
//      need not combine two entries.
//   3. The machine says no: a peripheral or variant choice that is not what
//      is plugged in, asked as canAttach for a place with room and as
//      canReplace for a full one. A detach is never refused by the
//      machine.

namespace calcu::control {

struct ControlSetup {
    const machine::Catalogue* catalogue = nullptr;   // the product's, never null; read at each computation
    std::vector<machine::PeripheralId> printers;     // who can own the paper panel
    const ControlRules* rules = nullptr;             // the product's; null: none
};

ControlState computeControlState(const machine::Machine&, const ControlSetup&, bool operationRuns);

} // namespace calcu::control
