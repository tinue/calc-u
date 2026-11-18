#pragma once
#include "machine/Attach.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// What one machine is built with, and the changes of a plug.

namespace calcu::startup {

// Preset: a user's load (`load-preset`). StartupPreset: a load through
// PresetLoader::load, at the start or a model switch.
enum class BuildOrigin { Start, ModelSwitch, VariantSwitch, Preset, StartupPreset };

// A build or change that is refused. `place`: the place whose request was
// refused, none for the model itself.
struct BuildRefusal {
    std::string reason;
    std::optional<machine::PlacePath> place;
};

// What one machine is built with, in one step. ROM images are not in it:
// MachineControl takes them from the RomSet.
struct MachinePlan {
    machine::ModelId model;
    std::optional<machine::VariantId> variant;            // nullopt: the default
    std::vector<machine::PeripheralRequest> peripherals;   // `roms` left empty; filled by MachineControl
    std::shared_ptr<const machine::ProductData> content;   // e.g. the TI-58C's constant memory
};

// A change of a plug (also of a module medium).
struct PlugChange {
    enum class Kind { Attach, Replace, Detach } kind = Kind::Attach;
    machine::PlacePath place;
    machine::PeripheralRequest request;   // Attach, Replace: full, content included; `roms` filled by MachineControl
};

// The last build.
struct BuiltWith {
    MachinePlan plan;
    BuildOrigin origin;
};

} // namespace calcu::startup
