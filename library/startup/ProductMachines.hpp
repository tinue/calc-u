#pragma once
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Machine.hpp"
#include "pacing/PacingProfile.hpp"
#include "startup/MachinePlan.hpp"

// What a product hands machine control and the startup part: its catalogue and builder, the built-in default machine per model, the
// pacing profile, sound rate and readiness bound per model. Implemented by
// the product's app layer. The fallback model, `last` and the startup
// preset's key are the settings' (settings::startupModel, startupPreset).

namespace calcu::startup {

class ProductMachines {
public:
    virtual const machine::Catalogue& catalogue() const = 0;
    virtual machine::MachineBuilder& builder() = 0;
    // The machine of `model` when no startup preset is set or it cannot be
    // loaded. Settings hold no hardware: the machine comes from the startup
    // preset or this default.
    virtual MachinePlan builtInDefault(const machine::ModelId&) = 0;
    virtual pacing::PacingProfile pacingProfile(const machine::ModelId&) const = 0;
    virtual int soundRate(const machine::ModelId&) const = 0;                          // BuildRequest::soundRate
    // The emulated time after which a wait for key readiness ends.
    virtual machine::Seconds readinessBound(const machine::ModelId&) const = 0;
    // The request that plugs `entry` into the machine of the other variant.
    // Default: its place, peripheral and variant. A product adds the content
    // and switch positions it keeps; the media part adds the media.
    virtual machine::PeripheralRequest carried(const machine::PlugEntry& entry, machine::Machine&) {
        machine::PeripheralRequest r;
        r.place = entry.place;
        r.peripheral = entry.peripheral;
        r.variant = entry.variant;
        return r;
    }
    // A request to attach `request.peripheral` (the attach command,
    // a product's own attach). Default: as the catalogue gives it. A product
    // adds the switch positions it keeps while the peripheral is detached
    // (Calc-U-59's TRACE); a switch it does not name starts at the
    // peripheral's own position.
    virtual void attaching(machine::PeripheralRequest& request) { (void)request; }
    // A ROM image the product bundles outside the RomSet (Calc-U-1600's host
    // drive): added to a request's images after the set's.
    virtual void addRoms(machine::PeripheralRequest& request) const { (void)request; }

protected:
    ~ProductMachines() = default;
};

} // namespace calcu::startup
