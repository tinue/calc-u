#pragma once
#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "../FileIO.hpp"
#include "MemoryCardDefinition.hpp"

namespace sharp {
// core/Machine/SharpRoms.hpp: a file of the ROM full set (the CE-502B's
// definition) is read by the ROM lookup only.
bool isFullSetFile(std::string_view fileName);
}  // namespace sharp

constexpr const char* kModuleFileSuffix = ".module.yaml";

// ── Catalogue of on-disk module definition files ──────────────────────
//
// Scans a directory of help/10-module-definition-format.md `.module.yaml`
// files, each parsed once (the catalogue's module definitions,
// sharp::addModuleDefinitions; the media kinds' listing, app/SharpMediaKinds).
//
// No notion of "app resource" reaches the core: the caller supplies the
// directory (the app layer: the bundled modules folder, AppPaths).

struct MemoryCardCatalogEntry {
    MemoryCardDefinition definition;
    std::string filePath;
};

// Parses every "*.module.yaml" in `dir` (non-recursive). A file that cannot
// be read or parsed is skipped, with "<filename>: <error>" in `*problems`
// -- one malformed file never hides the rest. A missing / unreadable
// directory yields an empty list and its own problem. Entries come back
// sorted by `module-name` for a stable GUI order. A file of the ROM full set
// is passed over: its definition comes from the RomSet
// (sharp::romSetModules()).
inline std::vector<MemoryCardCatalogEntry> scanMemoryCardDirectory(const std::string& dir,
                                                                   std::vector<std::string>* problems) {
    std::vector<MemoryCardCatalogEntry> out;
    auto problem = [&](std::string text) {
        if (problems) problems->push_back(std::move(text));
    };
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        problem("cannot read module directory '" + dir + "': " + ec.message());
        return out;
    }
    const std::string suffix = kModuleFileSuffix;
    for (; it != std::filesystem::directory_iterator(); it.increment(ec)) {
        if (ec) {
            problem("directory walk stopped: " + ec.message());
            break;
        }
        const std::filesystem::path& p = it->path();
        // path::extension() sees only ".yaml" of ".module.yaml": the name's tail.
        const std::string name = p.filename().string();
        if (name.size() <= suffix.size() || name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
        if (sharp::isFullSetFile(name)) continue;
        std::vector<uint8_t> bytes;
        if (!readWholeFile(p, &bytes)) {
            problem(name + ": cannot open");
            continue;
        }
        MemoryCardCatalogEntry entry{{}, p.string()};
        std::string error;
        if (!parseMemoryCardDefinition(std::string(bytes.begin(), bytes.end()), &entry.definition, &error)) {
            problem(name + ": " + error);
            continue;
        }
        out.push_back(std::move(entry));
    }
    std::sort(out.begin(), out.end(), [](const MemoryCardCatalogEntry& a, const MemoryCardCatalogEntry& b) {
        return a.definition.moduleName < b.definition.moduleName;
    });
    return out;
}
