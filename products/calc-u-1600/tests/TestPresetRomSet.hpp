#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "roms/RomSet.hpp"

// The ROMs the core tests hand the old machine classes (the preset loaders'
// until M-46; LIB-12 design §2). Every image comes from
// the set the lookup made at the start (AC-6); the host drive's own ROM,
// which is not looked up (roms.md Q-1), comes along beside it. A preset
// names WHAT it wants ("ce150", "A04", "old"); the catalogue says which
// files that is, and the set holds their bytes. Nothing here reads a file.
struct PresetRoms {
    std::shared_ptr<const calcu::roms::RomSet> set;                 // null: no ROMs (every load fails)
    std::shared_ptr<const std::vector<std::uint8_t>> hostDrive;     // null: no host drive ROM
};

