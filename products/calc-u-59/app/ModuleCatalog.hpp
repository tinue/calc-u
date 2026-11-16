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
// (C59-6 AC-8; docs/formats/modules.md §2): the ID is `module-name`, the
// picker shows `description` in `menu-sort` order. Their cue cards:
// app/ModuleCueCards (C59-7 AC-22, AC-23).

namespace cu59 {

struct ModuleMetadata {
    std::string title;
    std::string sort;
    std::string id;
    std::string menuTitle;
    int menuSort = 0;
};

/// Every module's metadata from its module file in the set, in menu order
/// (`menu-sort`; ties keep the catalogue's order, 2.3.0's cuecards.txt
/// order, so ST comes before RE: C59-6 design D-4). `menuTitle` is the
/// file's `description`, `title` its `title`.
std::vector<ModuleMetadata> allModuleMetadata(const calcu::roms::RomSet& roms);

/// The module in the slot at a start: no setting remembers the module
/// (LIB-9 AC-22, behaviour change 59-9), so it is the Master Library.
inline constexpr const char* kStartModuleID = "ML";

/// The module file of a module ID of the catalogue ("ML" → "ML.rom.yaml");
/// std::nullopt for another ID.
std::optional<std::string> moduleRomFile(std::string_view moduleID);

/// The listing of slot-1's modules for the library's media part (LIB-7
/// AC-8, AC-47; C59-6 AC-8): the module file's `description` as the label,
/// no tooltip, `menu-sort` order; slot-1 is a module slot without media.
calcu::media::ModuleListing tiModuleListing(const std::shared_ptr<const calcu::roms::RomSet>& roms);

} // namespace cu59
