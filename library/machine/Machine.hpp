#pragma once
#include "machine/Attach.hpp"
#include "machine/Capability.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

// The machine interface (LIB-1 design §3.5; machine.md M-1 to M-8, ADR 0006,
// 0007): the only seam between a core and everything above it.
//
// Threading and ownership (machine.md I-2, I-3; ADR 0009): a Machine is
// owned by one std::unique_ptr and used from one thread at a time. It holds
// no lock, starts no thread and reads no clock. Pointers from capability()
// stay valid until the next attach, detach or replace (LIB-2) or until the
// machine is destroyed.

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

    // Attach, detach, replace (LIB-2 design §3.1; Attach.hpp). Called on the
    // session's thread between two advances, never inside one. Capability
    // pointers from before a change are invalid after it.
    //
    // canAttach checks, in this order: the place exists (on the machine for
    // a one-step path, on the peripheral plugged in at the path's prefix
    // otherwise, AC-9); the peripheral is in the catalogue (AC-2); it fits
    // the place's pin-out (EC-1); the variant belongs to it; every ROM entry
    // of the variant has an image (EC-4); the place holds fewer than it
    // takes, and on a Takes::One connector no other place holds one (AC-3,
    // AC-7); productRefusal() (AC-29). attach refuses in exactly these cases
    // with the same reason and changes nothing then (AC-6).
    std::optional<Refusal> canAttach(const PeripheralRequest&, const Catalogue&) const;
    ChangeResult attach(PeripheralRequest, const Catalogue&);
    // Nothing at the place, or another peripheral there: nothing changes and
    // nothing follows (EC-2). The peripherals plugged into it go with it, and
    // what follows is asked once, at its place (EC-3).
    ChangeResult detach(const PlacePath&, const PeripheralId&, const Catalogue&);
    // `old` out of the request's place and the request's peripheral in, in one
    // step: checked as canAttach without `old`, what follows asked once (AC-35).
    ChangeResult replace(const PeripheralId& old, PeripheralRequest, const Catalogue&);

protected:
    // A product's rule (AC-29): the sentence of a refusal, or none. `replacing`
    // is the peripheral a replace takes out.
    virtual std::optional<std::string> productRefusal(const PeripheralRequest&,
                                                      const std::optional<PeripheralId>& replacing) const;
    // Plugs the checked request in (its variant resolved); never fails. The
    // host's tick rate is how a peripheral learns it (P-2, AC-12).
    virtual void plug(PeripheralRequest, const PeripheralDescriptor&, TickRate hostRate);
    // Takes the peripheral out, and the peripherals plugged into it (EC-3).
    virtual void unplug(const PlacePath&, const PeripheralId&);
    // What follows a change on this pin-out now: by default its `after`; a
    // machine that is switched off answers Nothing (AC-4).
    virtual AfterChange followsNow(const PinOutDescriptor&) const;

private:
    std::optional<Refusal> check(const PeripheralRequest&, const Catalogue&,
                                 const std::optional<PeripheralId>& replacing) const;
};

} // namespace calcu::machine
