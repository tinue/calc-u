#include "typing/TypingProfile.hpp"

namespace calcu::typing {

machine::Ticks frames(int n, machine::TickRate rate) { return static_cast<machine::Ticks>(n) * rate / 60; }

machine::Ticks ticksOf(double seconds, machine::TickRate rate) {
    return machine::EmulatedTime::fromSeconds(machine::Seconds(seconds), rate).ticks();
}

} // namespace calcu::typing
