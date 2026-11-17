#pragma once
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Machine.hpp"
#include "pacing/PacingProfile.hpp"
#include "startup/MachinePlan.hpp"

// What a product hands LIB-18 (design §3.2; spec AC-3, AC-4, AC-7, EC-7):
// its catalogue and builder, the built-in default machine per model, the
// pacing profile, sound rate and readiness bound per model. Implemented by
// the product's app layer (the adapters HarnessMachines and
// SharpBuildColdBoot went in M-46 and M-51). The fallback model, `last` and the startup preset's key
// are LIB-9's (settings::startupModel, startupPreset).

namespace calcu::startup {

class ProductMachines {
public:
    virtual const machine::Catalogue& catalogue() const = 0;
    virtual machine::MachineBuilder& builder() = 0;
    // ADR 0021: the machine of `model` when no startup preset is set or it
    // cannot be loaded (AC-4, EC-1; AC-18, AC-19, AC-26 in the products).
    virtual MachinePlan builtInDefault(const machine::ModelId&) = 0;
    virtual pacing::PacingProfile pacingProfile(const machine::ModelId&) const = 0;   // LIB-17 AC-14
    virtual int soundRate(const machine::ModelId&) const = 0;                          // BuildRequest::soundRate
    // EC-7: the emulated time after which a wait for key readiness ends.
    virtual machine::Seconds readinessBound(const machine::ModelId&) const = 0;
    // AC-7: the request that plugs `entry` into the machine of the other
    // variant. Default: its place, peripheral and variant (D-13). A product
    // adds the content and switch positions it keeps; LIB-7 adds the media
    // (M-41).
    virtual machine::PeripheralRequest carried(const machine::PlugEntry& entry, machine::Machine&) {
        machine::PeripheralRequest r;
        r.place = entry.place;
        r.peripheral = entry.peripheral;
        r.variant = entry.variant;
        return r;
    }
    // AC-10: a request to attach `request.peripheral` (the attach command,
    // a product's own attach). Default: as the catalogue gives it. A product
    // adds the switch positions it keeps while the peripheral is detached
    // (Calc-U-59's TRACE, C59-6 AC-6); a switch it does not name starts at
    // the peripheral's own position (LIB-2 D-3).
    virtual void attaching(machine::PeripheralRequest& request) { (void)request; }
    // A ROM image the product bundles outside the RomSet (Calc-U-1600's host
    // drive, roms.md Q-1; M-46): added to a request's images after the set's.
    virtual void addRoms(machine::PeripheralRequest& request) const { (void)request; }

protected:
    ~ProductMachines() = default;
};

} // namespace calcu::startup
