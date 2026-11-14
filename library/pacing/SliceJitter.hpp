#pragma once
#include "machine/EmulatedTime.hpp"
#include <cstdint>
#include <optional>
#include <random>

// The jittered slice ends (LIB-17 AC-9 to AC-11; design §3, "Jitter"): the
// offset of a slice end after its grid point, drawn evenly from 0 to half a
// slice. std::minstd_rand is defined exactly by the standard and the draw is
// our own (no std::uniform_int_distribution, whose results differ between
// standard libraries), so a seed gives one sequence on every platform.

namespace calcu::pacing {

class SliceJitter {
public:
    // The caller's seed, or std::random_device without one (AC-10).
    explicit SliceJitter(std::optional<std::uint32_t> seed = std::nullopt);
    // The offset of the next slice end: 0 to slice / 2 ticks.
    machine::Ticks offset(machine::Ticks slice);

private:
    std::minstd_rand rng_;
};

} // namespace calcu::pacing
