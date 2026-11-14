#pragma once
#include <chrono>
#include <optional>

// What a product supplies per model for the pacer (LIB-17 AC-8; glossary:
// pacing profile), and the speed the machine session asks for.

namespace calcu::pacing {

enum class Speed { Regular, Full };

struct PacingProfile {
    // A fixed slice length, or std::nullopt for "the call": each call runs
    // the emulated time due up to the wall time of that call.
    std::optional<std::chrono::microseconds> slice;
    bool jitter = false;                      // slice ends jittered (AC-9; fixed slices only)
    bool fullSpeedPacedWhileWaiting = false;  // AC-12
};

} // namespace calcu::pacing
