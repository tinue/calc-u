#pragma once
#include "MachineModelInfo.hpp"
#include "pacing/PacingProfile.hpp"
#include <chrono>

// The pacing profiles of the TI machines: a fixed slice of 20 ms, jittered
// slice ends (for the TI-58's error blink), and full speed paced while the
// machine waits for input. The same for
// the TI-59, TI-58 and TI-58C; the slice in ticks follows the model's rate.

namespace cu59 {

inline calcu::pacing::PacingProfile pacingProfile(Model) {
    calcu::pacing::PacingProfile p;
    p.slice = std::chrono::microseconds(20'000);
    p.jitter = true;
    p.fullSpeedPacedWhileWaiting = true;
    return p;
}

} // namespace cu59
