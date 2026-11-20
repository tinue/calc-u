#pragma once
#include <chrono>

// What a product supplies per model for the pacer (glossary: pacing
// profile), and the speed the machine session asks for. At full speed the
// pacer runs a machine that waits for the user at regular speed, so it never
// spins at the prompt; an operation never reports waiting (AppSession).

namespace calcu::pacing {

enum class Speed { Regular, Full };

struct PacingProfile {
    // The slice length: the pacer runs the machine in slices of this much
    // emulated time on a fixed grid, at regular speed and at full speed.
    std::chrono::microseconds slice{20'000};   // each product sets its own
    bool jitter = false;   // slice ends jittered around the grid
};

} // namespace calcu::pacing
