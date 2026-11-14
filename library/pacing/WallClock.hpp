#pragma once
#include "machine/EmulatedTime.hpp"
#include <chrono>
#include <functional>

// The wall clock of the pacer (LIB-17 design §2, §3; ADR 0009): the caller
// supplies it, so a test supplies its own. It is monotonic and never goes
// back; its epoch is the caller's.

namespace calcu::pacing {

using WallTime  = std::chrono::nanoseconds;
using WallClock = std::function<WallTime()>;

// A wall duration in whole ticks at `rate`, rounded down; 0 for a duration
// of zero or below. LIB-1's emulated-time type has no such conversion, so it
// is here (design §3, "State"). Integers only: whole seconds × rate plus the
// rest's nanoseconds × rate / 10⁹, so there is no overflow for any 32-bit
// rate over centuries and no floating point in the deadline.
inline machine::Ticks ticksIn(WallTime d, machine::TickRate rate) {
    if (d.count() <= 0) return 0;
    constexpr std::uint64_t kNs = 1'000'000'000;
    const auto ns = static_cast<std::uint64_t>(d.count());
    return (ns / kNs) * rate + (ns % kNs) * rate / kNs;
}

// The wall duration of `ticks` at `rate`, rounded down to the nanosecond.
inline WallTime wallTimeOf(machine::Ticks ticks, machine::TickRate rate) {
    constexpr std::uint64_t kNs = 1'000'000'000;
    return WallTime(static_cast<WallTime::rep>((ticks / rate) * kNs + (ticks % rate) * kNs / rate));
}

} // namespace calcu::pacing
