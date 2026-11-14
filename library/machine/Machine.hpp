#pragma once
#include "machine/Capability.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

// The machine interface (LIB-1 design §3.5; machine.md M-1 to M-8, ADR 0006,
// 0007): the only seam between a core and everything above it.
//
// Threading and ownership (machine.md I-2, I-3; ADR 0009): a Machine is
// owned by one std::unique_ptr and used from one thread at a time. It holds
// no lock, starts no thread and reads no clock. Pointers from capability()
// stay valid until the next attach or detach (LIB-2) or until the machine
// is destroyed.

namespace calcu::machine {

enum class KeyCall { Done, UnknownKey };   // UnknownKey: refused, nothing changed (EC-2)

struct PlugEntry {
    PlacePath place;
    PeripheralId peripheral;
    std::optional<VariantId> variant;
};
struct ConnectorPlugs {
    PlacePath owner;            // empty: the machine's own connector
    ConnectorId connector;
    std::vector<PlugEntry> plugged;
};
struct Plugged {
    std::vector<PlugEntry> byPlace;
    std::vector<ConnectorPlugs> byConnector;
};

class Machine {
public:
    virtual ~Machine() = default;

    // Identity (AC-5, EC-6).
    virtual const MachineDescriptor& descriptor() const = 0;
    virtual std::optional<VariantId> variant() const = 0;
    ModelId model() const { return descriptor().model; }

    // Emulated time (AC-7, AC-8, AC-9, EC-7, EC-8). advance() runs whole
    // instructions, at least `atLeast` ticks (none for 0), and returns the
    // ticks it ran. The tick count only grows for the machine's life and is
    // 0 when the machine is built.
    virtual Ticks advance(Ticks atLeast) = 0;
    virtual Ticks tickCount() const = 0;
    EmulatedTime elapsed() const { return EmulatedTime(tickCount(), descriptor().tickRate); }

    // Operations (AC-11, AC-42). They return at once; the ROM's start-up
    // runs with the advances that follow and is over at key readiness.
    virtual void reset() = 0;
    virtual void powerCycle() = 0;
    virtual void coldBoot() = 0;
    virtual bool isKeyReady() const = 0;

    // Keys (AC-12, AC-13, EC-2, EC-3), at the current tick count.
    virtual KeyCall keyDown(const KeyId&) = 0;
    virtual KeyCall keyUp(const KeyId&) = 0;

    // Display and sound (AC-15 to AC-19, AC-43). readDisplay() is non-const:
    // a core whose brightness is the duty since the last reading restarts
    // its window there. takeSound() gives mono samples at
    // BuildRequest::soundRate; 0 without sound.
    virtual DisplayOutput readDisplay() = 0;
    virtual std::size_t takeSound(std::span<std::int16_t> out) = 0;

    // Connectors, capabilities, debug targets (AC-20 to AC-23, AC-25, EC-5).
    virtual Plugged plugged() const = 0;
    virtual std::vector<CapabilityEntry> capabilities() const = 0;
    virtual Capability* capability(const CapabilityId&) = 0;   // nullptr if not present now
    std::vector<CapabilityEntry> capabilities(const KindId&) const;   // empty if none (EC-5)
    virtual std::vector<DebugTargetId> debugTargets() const = 0;  // their interface: LIB-10
};

} // namespace calcu::machine
