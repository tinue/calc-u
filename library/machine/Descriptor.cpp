#include "machine/Descriptor.hpp"
#include <algorithm>

namespace calcu::machine {

namespace {
const std::vector<RomEntry> kNoRoms;

const std::vector<RomEntry>& romsOf(const std::vector<VariantDescriptor>& variants,
                                    const std::optional<VariantId>& defaultVariant,
                                    const std::vector<RomEntry>& roms, const std::optional<VariantId>& variant) {
    if (variants.empty()) return variant ? kNoRoms : roms;
    const std::optional<VariantId>& wanted = variant ? variant : defaultVariant;
    for (const auto& v : variants)
        if (wanted && v.id == *wanted) return v.roms;
    return kNoRoms;
}
} // namespace

const std::vector<RomEntry>& MachineDescriptor::romsFor(const std::optional<VariantId>& variant) const {
    return romsOf(variants, defaultVariant, roms, variant);
}

const PlaceDescriptor* MachineDescriptor::place(const PlaceName& name) const {
    for (const auto& p : places)
        if (p.name == name) return &p;
    return nullptr;
}

const ConnectorDescriptor* MachineDescriptor::connector(const ConnectorId& id) const {
    for (const auto& c : connectors)
        if (c.id == id) return &c;
    return nullptr;
}

bool MachineDescriptor::hasKey(const KeyId& id) const {
    return std::any_of(keys.begin(), keys.end(), [&](const KeyDescriptor& k) { return k.id == id; });
}

const std::vector<RomEntry>& PeripheralDescriptor::romsFor(const std::optional<VariantId>& variant) const {
    return romsOf(variants, defaultVariant, roms, variant);
}

bool PeripheralDescriptor::fitsPinOut(const PinOutId& id) const {
    return std::find(fits.begin(), fits.end(), id) != fits.end();
}

} // namespace calcu::machine
