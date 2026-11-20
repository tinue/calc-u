#pragma once
#include "pacing/PacingProfile.hpp"
#include <chrono>

// The pacing profiles of the TI machines: a fixed slice of 20 ms and
// jittered slice ends (for the TI-58's error blink). The same for the
// TI-59, TI-58 and TI-58C, so it takes no model; the slice in ticks follows
// the model's rate.

namespace cu59 {

inline calcu::pacing::PacingProfile pacingProfile() {
    calcu::pacing::PacingProfile p;
    p.slice = std::chrono::microseconds(20'000);
    p.jitter = true;
    return p;
}

} // namespace cu59
