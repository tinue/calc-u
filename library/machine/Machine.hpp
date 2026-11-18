#pragma once
#include "machine/Attach.hpp"
#include "machine/Capability.hpp"
#include "machine/Catalogue.hpp"
#include "machine/DebugTarget.hpp"
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

// The machine interface: the only seam between a core and everything above it.
//
// Threading and ownership: a Machine is
// owned by one std::unique_ptr and used from one thread at a time. It holds
// no lock, starts no thread and reads no clock. Pointers from capability()
// stay valid until the next attach, detach or replace or until the
// machine is destroyed.

namespace calcu::machine {

enum class KeyCall { Done, UnknownKey };   // UnknownKey: refused, nothing changed

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

    // Identity.
    virtual const MachineDescriptor& descriptor() const = 0;
    virtual std::optional<VariantId> variant() const = 0;
    ModelId model() const { return descriptor().model; }

    // Emulated time. advance() runs whole
    // instructions, at least `atLeast` ticks (none for 0), and returns the
    // ticks it ran. The tick count only grows for the machine's life and is
    // 0 when the machine is built.
    virtual Ticks advance(Ticks atLeast) = 0;
    virtual Ticks tickCount() const = 0;
    EmulatedTime elapsed() const { return EmulatedTime(tickCount(), descriptor().tickRate); }

    // Operations. They return at once; the ROM's start-up
    // runs with the advances that follow and is over at key readiness.
    virtual void reset() = 0;
    virtual void powerCycle() = 0;
    virtual void coldBoot() = 0;
    virtual bool isKeyReady() const = 0;

    // Keys, at the current tick count.
    virtual KeyCall keyDown(const KeyId&) = 0;
    virtual KeyCall keyUp(const KeyId&) = 0;

    // Display and sound. readDisplay() is non-const:
    // a core whose brightness is the duty since the last reading restarts
    // its window there. takeSound() gives mono samples at
    // BuildRequest::soundRate; 0 without sound.
    virtual DisplayOutput readDisplay() = 0;
    virtual std::size_t takeSound(std::span<std::int16_t> out) = 0;

    // Connectors, capabilities, debug targets.
    virtual Plugged plugged() const = 0;
    virtual std::vector<CapabilityEntry> capabilities() const = 0;
    virtual Capability* capability(const CapabilityId&) = 0;   // nullptr if not present now
    // The same for a reader of a const machine: capability() only looks up.
    // A machine that overrides capability() says `using Machine::capability;`.
    const Capability* capability(const CapabilityId& id) const { return const_cast<Machine*>(this)->capability(id); }
    std::vector<CapabilityEntry> capabilities(const KindId&) const;   // empty if none
    virtual std::vector<DebugTargetId> debugTargets() const = 0;  // their interface: DebugTarget.hpp
    // The debug targets with the checks of the
    // advance; nullptr: the machine offers none (the default).
    virtual Debugging* debugging() { return nullptr; }
    // The target of that name, through debugging(); nullptr if none.
    DebugTarget* debugTarget(const DebugTargetId&);

    // Attach, detach, replace (Attach.hpp). Called on the
    // session's thread between two advances, never inside one. Capability
    // pointers from before a change are invalid after it.
    //
    // canAttach checks, in this order: the place exists (on the machine for
    // a one-step path, on the peripheral plugged in at the path's prefix
    // otherwise); the peripheral is in the catalogue; it fits
    // the place's pin-out; the variant belongs to it; every ROM entry
    // of the variant has an image; the place holds fewer than it
    // takes, and on a Takes::One connector no other place holds one;
    // productRefusal(). attach refuses in exactly these cases
    // with the same reason and changes nothing then.
    std::optional<Refusal> canAttach(const PeripheralRequest&, const Catalogue&) const;
    ChangeResult attach(PeripheralRequest, const Catalogue&);
    // Nothing at the place, or another peripheral there: nothing changes and
    // nothing follows. The peripherals plugged into it go with it, and
    // what follows is asked once, at its place.
    ChangeResult detach(const PlacePath&, const PeripheralId&, const Catalogue&);
    // `old` out of the request's place and the request's peripheral in, in one
    // step: checked as canAttach without `old`, what follows asked once.
    ChangeResult replace(const PeripheralId& old, PeripheralRequest, const Catalogue&);
    // The refusal replace() would give, without the change: `old` not at the
    // request's place, or canAttach's checks without `old`.
    std::optional<Refusal> canReplace(const PeripheralId& old, const PeripheralRequest&, const Catalogue&) const;

protected:
    // A product's rule: the sentence of a refusal, or none. `replacing`
    // is the peripheral a replace takes out.
    virtual std::optional<std::string> productRefusal(const PeripheralRequest&,
                                                      const std::optional<PeripheralId>& replacing) const;
    // Plugs the checked request in (its variant resolved); never fails. The
    // host's tick rate is how a peripheral learns it.
    virtual void plug(PeripheralRequest, const PeripheralDescriptor&, TickRate hostRate);
    // Takes the peripheral out, and the peripherals plugged into it.
    virtual void unplug(const PlacePath&, const PeripheralId&);
    // What follows a change on this pin-out now: by default its `after`; a
    // machine that is switched off answers Nothing.
    virtual AfterChange followsNow(const PinOutDescriptor&) const;

private:
    std::optional<Refusal> check(const PeripheralRequest&, const Catalogue&,
                                 const std::optional<PeripheralId>& replacing) const;
};

} // namespace calcu::machine
