#include "machine/Names.hpp"

namespace calcu::machine {

std::string lowerAscii(std::string_view s) {
    std::string out(s);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return out;
}

bool sameIgnoringCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    return lowerAscii(a) == lowerAscii(b);
}

const MachineDescriptor* modelNamed(const Catalogue& cat, std::string_view name) {
    for (const auto& m : cat.models())
        if (sameIgnoringCase(m.model.value, name)) return &m;
    return nullptr;
}

const PeripheralDescriptor* peripheralNamed(const Catalogue& cat, std::string_view name) {
    for (const auto& p : cat.peripherals())
        if (sameIgnoringCase(p.id.value, name)) return &p;
    return nullptr;
}

std::optional<VariantId> variantNamed(const std::vector<VariantDescriptor>& vs, std::string_view name) {
    for (const auto& v : vs)
        if (sameIgnoringCase(v.id.value, name)) return v.id;
    return std::nullopt;
}

} // namespace calcu::machine
