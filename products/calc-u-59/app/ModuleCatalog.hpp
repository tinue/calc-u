#pragma once
#include "CueCard.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// roms/solid-state/cuecards.txt: one section per Solid State Software module
// (MODULE-ID first, then MODULE-TITLE / -SORT / -MENU-TITLE / -MENU-SORT /
// -ROM), followed by "CUECARD: n" blocks — the cue card for library program n.
// '#' starts a comment. It is still the module list and the cue cards (until
// M-25 and M-55, migration plan §2.3); the module's ROM is its `<ID>.rom.yaml`
// of the ROM set (LIB-12 AC-14), and MODULE-ROM is no longer read.

namespace cu59 {

struct ModuleMetadata {
    std::string title;
    std::string sort;
    std::string id;
    std::string menuTitle;
    int menuSort = 0;
};

/// Every module's metadata in menu order (MODULE-MENU-SORT, file order on ties).
std::vector<ModuleMetadata> allModuleMetadata(std::string_view cuecardsTxt);

/// Adapter A-3 (LIB-12 design §5, until M-25): the module file of a module ID
/// that cuecards.txt lists ("ML" → "ML.rom.yaml"); std::nullopt for another ID.
std::optional<std::string> moduleRomFile(std::string_view cuecardsTxt, std::string_view moduleID);

struct ModuleCards {
    std::map<int, CueCardContent> cards;  ///< library program number → cue card
    ModuleMetadata metadata;
};

/// Cue cards and metadata of one module.
ModuleCards moduleCards(std::string_view cuecardsTxt, std::string_view moduleID);

} // namespace cu59
