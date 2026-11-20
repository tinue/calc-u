#pragma once
#include "CueCard.hpp"
#include "media/Kind.hpp"
#include "roms/RomSet.hpp"
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The 14 Solid State ROM modules from their module files in the ROM set
// (docs/formats/modules.md §2): the ID is `module-name`, the picker shows
// `description` in `menu-sort` order. Their cue cards: app/ModuleCueCards.

namespace cu59 {

struct ModuleMetadata {
    std::string title;
    std::string sort;
    std::string id;
    std::string menuTitle;
    int menuSort = 0;
};

/// Every module's metadata from its module file in the set, in menu order
/// (`menu-sort`; ties keep the catalogue's order, so ST comes before RE). `menuTitle` is the
/// file's `description`, `title` its `title`.
std::vector<ModuleMetadata> allModuleMetadata(const calcu::roms::RomSet& roms);

/// The module in the slot at a start: no setting remembers the module, so
/// it is the Master Library.
inline constexpr const char* kStartModuleID = "ML";

/// The module file of a module ID of the catalogue ("ML" → "ML.rom.yaml");
/// std::nullopt for another ID.
std::optional<std::string> moduleRomFile(std::string_view moduleID);

/// The listing of slot-1's modules for the library's media part: the module file's `description` as the label,
/// no tooltip, `menu-sort` order; slot-1 is a module slot without media.
calcu::media::ModuleListing tiModuleListing(const std::shared_ptr<const calcu::roms::RomSet>& roms);

} // namespace cu59
