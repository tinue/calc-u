#include "roms/RomSet.hpp"

namespace calcu::roms {

const std::vector<std::uint8_t>* RomSet::bytes(std::string_view fileName) const {
    auto it = files_.find(fileName);
    return it == files_.end() ? nullptr : &it->second;
}

std::vector<machine::RomImage> RomSet::images(const std::vector<machine::RomEntry>& entries) const {
    std::vector<machine::RomImage> out;
    for (const auto& e : entries)
        if (const auto* b = bytes(e.fileName)) out.push_back({e.fileName, *b});
    return out;
}

std::vector<machine::RomImage> RomSet::imagesFor(const machine::MachineDescriptor& d,
                                                 const std::optional<machine::VariantId>& variant) const {
    return images(d.romsFor(variant));
}

std::vector<machine::RomImage> RomSet::imagesFor(const machine::PeripheralDescriptor& d,
                                                 const std::optional<machine::VariantId>& variant) const {
    return images(d.romsFor(variant));
}

} // namespace calcu::roms
