#pragma once
#include "machine/Attach.hpp"
#include "machine/Ids.hpp"
#include "machine/ProductData.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// What one machine is built with, and the changes of a plug (LIB-18 design
// §3.1, §3.4; spec AC-1, AC-4, AC-6, AC-7, AC-10 to AC-12).

namespace calcu::startup {

// Preset: a user's load (LIB-6 `load-preset`). StartupPreset: a load through
// PresetLoader::load, at the start or a model switch (LIB-6 D-24; LIB-8 AC-8).
enum class BuildOrigin { Start, ModelSwitch, VariantSwitch, Preset, StartupPreset };

// A build or change that is refused (AC-12, LIB-1 EC-1). `place`: the place
// whose request was refused, none for the model itself (LIB-6 AC-52).
struct BuildRefusal {
    std::string reason;
    std::optional<machine::PlacePath> place;
};

// What one machine is built with, in one step (AC-1). ROM images are not in
// it: MachineControl takes them from the RomSet (LIB-12).
struct MachinePlan {
    machine::ModelId model;
    std::optional<machine::VariantId> variant;            // nullopt: the default
    std::vector<machine::PeripheralRequest> peripherals;   // `roms` left empty; filled by MachineControl
    std::shared_ptr<const machine::ProductData> content;   // e.g. the TI-58C's constant memory (AC-19)
};

// A change of a plug (AC-10 to AC-12; LIB-7's module media, its §3.10).
struct PlugChange {
    enum class Kind { Attach, Replace, Detach } kind = Kind::Attach;
    machine::PlacePath place;
    machine::PeripheralRequest request;   // Attach, Replace: full, content included; `roms` filled by MachineControl
};

// The last build (LIB-8 A-3).
struct BuiltWith {
    MachinePlan plan;
    BuildOrigin origin;
};

} // namespace calcu::startup
