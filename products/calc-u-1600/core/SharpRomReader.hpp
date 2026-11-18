#pragma once
#include "roms/RomReader.hpp"

// The reader Calc-U-1600 supplies to the ROM lookup. Every ROM dump of the full set is binary and checked by its
// size, without a reader. The one text file is the CE-502B's module
// definition (`CE-502B.module.yaml`, docs/formats/modules.md §1), read by the
// module definition reader of core/Connector/: what it yields is the bytes
// of its content, every region's initial content added up (16384 for the
// CE-502B). Calc-U-1600 adds no checks of the set.

namespace sharp {

class SharpRomReader final : public calcu::roms::ProductRomReader {
public:
    Decoded decode(const calcu::machine::RomEntry&, std::span<const std::uint8_t> bytes) const override;
};

} // namespace sharp
