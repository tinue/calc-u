#pragma once
#include "control/ControlRules.hpp"
#include "control/ControlState.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include <vector>

// The computation of the control state (LIB-4 design §3.4; spec AC-1 to
// AC-3, AC-5, EC-1, EC-3 to EC-5): a pure function of the running machine,
// the product's setup and whether an operation runs. Nothing is cached
// between computations (D-1).
//
// Availability, the order of AC-2 (EC-5): the first that applies gives the
// reason.
//   1. An operation runs (AC-3): every entry but the models, `reset` and
//      `cold-boot` (they end the operation, LIB-18 AC-16); the sentence
//      sessions::kAnotherOperationRuns (D-6).
//   2. A rule says so (AC-7); a refused place passes its reason to its empty
//      choice and to every peripheral and variant in it (D-5).
//   3. The machine says no (LIB-2 AC-6): a peripheral or variant choice that
//      is not what is plugged in, asked as canAttach for a place with room
//      and as canReplace for a full one (§3.2). A detach is never refused by
//      the machine (LIB-2 EC-2).

namespace calcu::control {

struct ControlSetup {
    const machine::Catalogue* catalogue = nullptr;   // the product's; read at each computation (EC-3)
    std::vector<machine::PeripheralId> printers;     // who can own the paper panel (D-3)
    std::vector<const ControlRules*> rules;          // asked in this order
};

ControlState computeControlState(const machine::Machine&, const ControlSetup&, bool operationRuns);

} // namespace calcu::control
