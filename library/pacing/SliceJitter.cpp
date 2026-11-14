#include "pacing/SliceJitter.hpp"

namespace calcu::pacing {

SliceJitter::SliceJitter(std::optional<std::uint32_t> seed) : rng_(seed ? *seed : std::random_device{}()) {}

machine::Ticks SliceJitter::offset(machine::Ticks slice) {
    // rng_() is below 2^31; the bias of the remainder is below 2^-11 for a
    // slice of under a million ticks.
    return static_cast<machine::Ticks>(rng_()) % (slice / 2 + 1);
}

} // namespace calcu::pacing
