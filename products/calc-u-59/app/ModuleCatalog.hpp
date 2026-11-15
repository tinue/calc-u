#pragma once
#include "CueCard.hpp"
#include "roms/RomSet.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// roms/solid-state/cuecards.txt: one section per Solid State Software module
// (MODULE-ID first, then MODULE-TITLE / -SORT / -MENU-TITLE / -MENU-SORT /
// -ROM), followed by "CUECARD: n" blocks — the cue card for library program n.
// '#' starts a comment. It is still the cue cards (until M-55, migration plan
// §2.3). The module list comes from the 14 module files of the ROM set since
// M-25 (C59-6 AC-8; docs/formats/modules.md §2): the ID is `module-name`, the
// picker shows `description` in `menu-sort` order. MODULE-ROM is not read.

namespace cu59 {

struct ModuleMetadata {
    std::string title;
    std::string sort;
    std::string id;
    std::string menuTitle;
    int menuSort = 0;
};

/// Every module's metadata from its module file in the set, in menu order
/// (`menu-sort`; ties keep the catalogue's order, today's cuecards.txt
/// order, so ST comes before RE: C59-6 design D-4). `menuTitle` is the
/// file's `description`, `title` its `title`.
std::vector<ModuleMetadata> allModuleMetadata(const calcu::roms::RomSet& roms);

/// The module file of a module ID of the catalogue ("ML" → "ML.rom.yaml");
/// std::nullopt for another ID.
std::optional<std::string> moduleRomFile(std::string_view moduleID);

struct ModuleCards {
    std::map<int, CueCardContent> cards;  ///< library program number → cue card
    ModuleMetadata metadata;
};

/// Cue cards and metadata of one module.
ModuleCards moduleCards(std::string_view cuecardsTxt, std::string_view moduleID);

} // namespace cu59
