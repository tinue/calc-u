#pragma once
#include <chrono>
#include <optional>

// What a product supplies per model for the pacer (glossary: pacing
// profile), and the speed the machine session asks for.

namespace calcu::pacing {

enum class Speed { Regular, Full };

struct PacingProfile {
    // A fixed slice length, or std::nullopt for "the call": each call runs
    // the emulated time due up to the wall time of that call.
    std::optional<std::chrono::microseconds> slice;
    bool jitter = false;                      // slice ends jittered (fixed slices only)
    bool fullSpeedPacedWhileWaiting = false;  // full speed paced while the machine waits for input
};

} // namespace calcu::pacing
