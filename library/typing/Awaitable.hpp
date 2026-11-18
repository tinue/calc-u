#pragma once
#include "sessions/Operation.hpp"
#include "typing/TypingEngine.hpp"
#include <string>
#include <vector>

// Typing inside an operation: for a preset's `key:` and `type:` steps. The presses are queued, then the
// machine runs in pieces until they are carried out and the engine is idle;
// the session cuts the pieces at the engine's nextDue, so the cadence is
// the same as outside an operation.

namespace calcu::typing {

struct TypeOptions {
    Origin origin = Origin::Preset;
    bool firstPressAtOnce = false;   // after `skip-wait:` the step's first press goes down at once
    // Done when the last key is up, without the profile's wait for readiness
    // after the release (TypingProfile::waitsForReadinessAfterRelease): a
    // preset's `key:` (carried out when its keys are pressed; the implicit
    // wait for idle follows, except after the last item and before
    // `skip-wait:`). Otherwise a last key that starts a program that never
    // stops (life.ti59) would never finish its item.
    bool untilReleased = false;
};

sessions::Task<TypeResult> typeAndWait(sessions::OperationContext&, TypingEngine&, std::vector<Press>,
                                       TypeOptions = {});
// With refuseUnknown: a character without a key refuses the whole text.
sessions::Task<TypeResult> typeTextAndWait(sessions::OperationContext&, TypingEngine&, std::string text, bool enter,
                                           TypeOptions = {});

} // namespace calcu::typing
