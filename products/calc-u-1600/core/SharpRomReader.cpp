#include "SharpRomReader.hpp"

#include <string>

#include "Connector/MemoryCardDefinition.hpp"

namespace sharp {

SharpRomReader::Decoded SharpRomReader::decode(const calcu::machine::RomEntry&, std::span<const std::uint8_t> bytes) const {
    Decoded d;
    MemoryCardDefinition def;
    if (!parseMemoryCardDefinition(std::string(bytes.begin(), bytes.end()), &def, &d.error)) {
        if (d.error.empty()) d.error = "not a module definition";
        return d;
    }
    for (const Region& r : def.regions)
        for (const auto& bank : r.initialContentByBank) d.amount += bank.second.size();
    return d;
}

} // namespace sharp
