#pragma once
#include "roms/Bundle.hpp"

// The Qt6 app's bundled ROM files for the lookup (LIB-12 design §3.1, D-2):
// the :/roms/ resources of the .qrc the CMake build makes from roms/. Names
// are compared exactly, as DirectoryBundle does.
class QtResourceBundle final : public calcu::roms::Bundle {
public:
    std::optional<std::vector<std::uint8_t>> read(std::string_view fileName) const override;
    std::string where() const override;
};
