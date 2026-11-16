#include "app/PacingProfiles.hpp"

namespace shared {

calcu::pacing::PacingProfile pacingProfile(const calcu::machine::ModelId&) {
    calcu::pacing::PacingProfile p;
    p.slice = std::nullopt;                 // the call
    p.jitter = false;
    p.fullSpeedPacedWhileWaiting = false;   // full speed never paced
    return p;
}

} // namespace shared
