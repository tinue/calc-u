#pragma once
#include "machine/Build.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The ROM set: every file of the full set, read
// once at the start and held in memory. Made only by lookUp(); immutable,
// so the app session shares it as std::shared_ptr<const RomSet> without a
// lock. No file is read while the product runs.

namespace calcu::roms {

class RomSet {
public:
    // The file as bundled; nullptr: not in the set.
    const std::vector<std::uint8_t>* bytes(std::string_view fileName) const;
    // The images of the entries a model's or peripheral's variant needs
    // (std::nullopt: the default or no variant), for BuildRequest::roms and
    // PeripheralRequest::roms.
    std::vector<machine::RomImage> imagesFor(const machine::MachineDescriptor&,
                                             const std::optional<machine::VariantId>&) const;
    std::vector<machine::RomImage> imagesFor(const machine::PeripheralDescriptor&,
                                             const std::optional<machine::VariantId>&) const;
    // The full set, in its order (for the Web worker and the tests).
    std::vector<std::string> fileNames() const { return names_; }

private:
    friend struct RomSetMaker;
    RomSet() = default;
    std::vector<machine::RomImage> images(const std::vector<machine::RomEntry>&) const;
    std::vector<std::string> names_;
    std::map<std::string, std::vector<std::uint8_t>, std::less<>> files_;
};

} // namespace calcu::roms
