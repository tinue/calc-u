#include "app/PacingProfiles.hpp"

namespace cu1600 {

calcu::pacing::PacingProfile pacingProfile(const calcu::machine::ModelId&) {
    calcu::pacing::PacingProfile p;
    p.slice = std::nullopt;                 // the call
    p.jitter = false;
    p.fullSpeedPacedWhileWaiting = true;    // full speed runs at real time while the machine waits for the user
    return p;
}

} // namespace cu1600
