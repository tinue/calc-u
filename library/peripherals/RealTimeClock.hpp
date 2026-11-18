#pragma once
#include "machine/Capability.hpp"
#include "machine/Machine.hpp"
#include <chrono>
#include <optional>

// The generic kind real-time clock. The clock advances in the core by ticks
// only; nothing in a machine reads the host's clock. Whoever builds a
// machine or runs an operation on it hands it the host's date and time
// the app session, through its peripherals part
// (PeripheralsPart.hpp).

namespace calcu::peripherals {

// The fields a chip keeps; a field it lacks stays empty. weekday: 0 is
// Sunday (as std::tm). month: 1 to 12.
struct ClockFields {
    std::optional<int> year, month, day, weekday, hour, minute, second;
    bool operator==(const ClockFields&) const = default;
};

struct HostDateTime {
    int year, month, day, weekday, hour, minute, second, millisecond;
    bool operator==(const HostDateTime&) const = default;
};

class RealTimeClock : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"real-time-clock"};
    machine::KindId kind() const override { return kKind; }

    virtual ClockFields read() const = 0;   // only the kept fields are set
    // A field the chip lacks, or one left empty, is ignored. `millisecond`
    // preloads the chip's second, as today.
    virtual void set(const ClockFields&, int millisecond = 0) = 0;
};

// The host's local date and time at `t`.
HostDateTime hostDateTime(std::chrono::system_clock::time_point t);
ClockFields fieldsOf(const HostDateTime&);
// Sets every real-time clock capability of the machine.
void setClocksFromHost(machine::Machine&, const HostDateTime&);

} // namespace calcu::peripherals
