#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "MemoryCardDefinition.hpp"
#include "NamedFileCatalog.hpp"

namespace sharp {
// core/Machine/SharpRoms.hpp: a file of the ROM full set (the CE-502B's
// definition) is read by the ROM lookup only (LIB-12 AC-14, I-2).
bool isFullSetFile(std::string_view fileName);
}  // namespace sharp

constexpr const char* kCardFileSuffix = ".card.yaml";

// ── Catalogue of on-disk memory-card definition files ─────────────────
//
// Scans a directory of help/10-module-definition-format.md `.card.yaml`
// files and indexes them by `module-name` (the catalogue's module
// definitions, sharp::addModuleDefinitions; the media kinds' listing,
// app/SharpMediaKinds).
//
// No notion of "app resource" reaches the core: the caller supplies the
// directory (the app layer: the bundled modules folder, AppPaths).

struct MemoryCardCatalogEntry {
    std::string moduleName;
    std::string description;  // MemoryCardDefinition::description
    std::vector<CardHost> compatibleHosts;
    std::string filePath;
    bool battery = false;
    bool rom = false;  // MemoryCardDefinition::isRom()
    bool isTemplate = false;  // MemoryCardDefinition::isTemplate

    bool compatibleWith(CardHost h) const {
        return std::find(compatibleHosts.begin(), compatibleHosts.end(), h) != compatibleHosts.end();
    }
};

// Parse every "*.card.yaml" in `dir` (non-recursive). A file that fails to
// parse is skipped, with "<filename>: <error>" appended to `*error`
// (newline-separated) -- one malformed file never hides the rest. A
// missing / unreadable directory yields an empty list and sets `*error`.
// Entries come back sorted by `moduleName` for a stable GUI order. A file of
// the ROM full set is passed over: its definition comes from the RomSet
// (sharp::romSetModules()).
namespace memory_card_catalog_detail {

inline bool parseEntry(const std::string& text, const std::string& path, MemoryCardCatalogEntry* out,
                       std::string* err) {
    MemoryCardDefinition def;
    if (!parseMemoryCardDefinition(text, &def, err)) return false;
    *out = {def.moduleName, def.description, def.compatibleHosts, path, def.battery, def.isRom(), def.isTemplate};
    return true;
}

}  // namespace memory_card_catalog_detail

inline std::vector<MemoryCardCatalogEntry> scanMemoryCardDirectory(const std::string& dir,
                                                                   std::string* error) {
    return scanNamedFiles<MemoryCardCatalogEntry>(
        dir, kCardFileSuffix, "module", memory_card_catalog_detail::parseEntry,
        [](const MemoryCardCatalogEntry& e) { return e.moduleName; }, error, sharp::isFullSetFile);
}
