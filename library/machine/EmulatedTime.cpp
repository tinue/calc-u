#include "machine/EmulatedTime.hpp"
#include <cmath>
#include <utility>

namespace calcu::machine {

Seconds EmulatedTime::seconds() const {
    // Exact for counts below 2^53 (one year at 3,580,000 per second is about
    // 1.13e14): the quotient is rounded once, and a whole multiple of the
    // rate gives whole seconds.
    return Seconds(static_cast<double>(ticks_) / static_cast<double>(rate_));
}

EmulatedTime EmulatedTime::fromSeconds(Seconds s, TickRate rate) {
    const double t = std::round(s.count() * static_cast<double>(rate));
    return EmulatedTime(t > 0 ? static_cast<Ticks>(t) : 0, rate);
}

namespace {
// ticks * rate as a 96-bit number: {bits 32..95, bits 0..31}. Split by hand
// because MSVC has no __int128.
std::pair<std::uint64_t, std::uint64_t> product(Ticks ticks, TickRate rate) {
    const std::uint64_t lo = (ticks & 0xFFFF'FFFFull) * rate;   // < 2^64
    const std::uint64_t hi = (ticks >> 32) * rate;              // < 2^64
    return {hi + (lo >> 32), lo & 0xFFFF'FFFFull};             // hi + carry < 2^64
}
} // namespace

std::strong_ordering operator<=>(const EmulatedTime& a, const EmulatedTime& b) {
    return product(a.ticks_, b.rate_) <=> product(b.ticks_, a.rate_);
}

} // namespace calcu::machine
