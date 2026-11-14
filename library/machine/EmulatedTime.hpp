#pragma once
#include <chrono>
#include <compare>
#include <cstdint>

// Emulated time (LIB-1 design §3.2; machine.md M-1): one canonical tick per
// machine at a whole-number rate from its descriptor. Every conversion and
// every comparison of times of two machines goes through EmulatedTime
// (AC-10, EC-4); no arithmetic on the Ticks of two machines is offered.

namespace calcu::machine {

using Ticks    = std::uint64_t;
using TickRate = std::uint32_t;                  // ticks per second, a whole number
using Seconds  = std::chrono::duration<double>;

class EmulatedTime {
public:
    EmulatedTime(Ticks ticks, TickRate rate) : ticks_(ticks), rate_(rate) {}
    Ticks ticks() const { return ticks_; }
    TickRate rate() const { return rate_; }
    Seconds seconds() const;                                  // ticks / rate
    static EmulatedTime fromSeconds(Seconds, TickRate);      // rounds to the nearest tick

    // Exact: ticks1 * rate2 against ticks2 * rate1 in 96 bits.
    friend std::strong_ordering operator<=>(const EmulatedTime&, const EmulatedTime&);
    friend bool operator==(const EmulatedTime& a, const EmulatedTime& b) { return (a <=> b) == 0; }
    friend Seconds operator+(const EmulatedTime& a, const EmulatedTime& b) { return a.seconds() + b.seconds(); }
    friend Seconds operator-(const EmulatedTime& a, const EmulatedTime& b) { return a.seconds() - b.seconds(); }

private:
    Ticks ticks_;
    TickRate rate_;
};

} // namespace calcu::machine
