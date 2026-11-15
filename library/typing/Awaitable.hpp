#pragma once
#include "sessions/Operation.hpp"
#include "typing/TypingEngine.hpp"
#include <string>
#include <vector>

// Typing inside an operation (LIB-5 design §3.6; spec AC-14, AC-17): for
// LIB-6's `key:` and `type:` steps. The presses are queued, then the
// machine runs in pieces until they are carried out and the engine is idle;
// the session cuts the pieces at the engine's nextDue, so the cadence is
// the same as outside an operation.

namespace calcu::typing {

struct TypeOptions {
    Origin origin = Origin::Preset;
    bool firstPressAtOnce = false;   // LIB-6 AC-58: after `skip-wait:` the step's first press goes down at once
};

sessions::Task<TypeResult> typeAndWait(sessions::OperationContext&, TypingEngine&, std::vector<Press>,
                                       TypeOptions = {});
// With refuseUnknown: a character without a key refuses the whole text.
sessions::Task<TypeResult> typeTextAndWait(sessions::OperationContext&, TypingEngine&, std::string text, bool enter,
                                           TypeOptions = {});

} // namespace calcu::typing
