#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

// The full set (LIB-12 design §3.3; AC-4, EC-1): every ROM file that any
// model, variant or peripheral of a product's catalogue needs, each once,
// with who needs it.

namespace calcu::roms {

struct NeededBy {
    std::variant<machine::ModelId, machine::PeripheralId> who;
    std::optional<machine::VariantId> variant;   // std::nullopt: no variant
    bool operator==(const NeededBy&) const = default;
};

struct FullSetEntry {
    machine::RomEntry entry;
    std::vector<NeededBy> neededBy;
};

struct FullSet {
    std::vector<FullSetEntry> files;            // catalogue order, no file twice
    std::vector<std::string> catalogueErrors;   // file names two entries give in different forms
};

// Every RomEntry of every model, every variant of a model, every peripheral
// and every variant of a peripheral, in catalogue order. `extra`: entries
// outside the catalogue (Calc-U-1600's CE-502B module definition).
FullSet fullSet(const machine::Catalogue&, std::span<const FullSetEntry> extra = {});

// "TI-59", "PC-1600 (variant Old)".
std::string describe(const NeededBy&);

} // namespace calcu::roms
