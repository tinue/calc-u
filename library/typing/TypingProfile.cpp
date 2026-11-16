#include "typing/TypingProfile.hpp"

namespace calcu::typing {


machine::Ticks ticksOf(double seconds, machine::TickRate rate) {
    return machine::EmulatedTime::fromSeconds(machine::Seconds(seconds), rate).ticks();
}

} // namespace calcu::typing
