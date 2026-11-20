#include "app/PacingProfiles.hpp"

namespace cu1600 {

calcu::pacing::PacingProfile pacingProfile(const calcu::machine::ModelId&) {
    // Slices much shorter than the host's call (60 Hz), so every call runs
    // about the time that is due: slices of the call's length would line up
    // with the calls, and a timer's jitter would run none or two of them.
    calcu::pacing::PacingProfile p;
    p.slice = std::chrono::microseconds(2'000);
    p.jitter = false;
    return p;
}

} // namespace cu1600
