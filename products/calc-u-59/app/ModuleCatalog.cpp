#include "ModuleCatalog.hpp"
#include "TextUtil.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>

namespace cu59 {

namespace {

/// Value after a "KEY:" prefix that `upper` (the uppercased line) starts with.
bool takeValue(std::string_view line, const std::string& upper, std::string_view key, std::string_view& value) {
    if (!startsWith(upper, key)) return false;
    value = trim(line.substr(key.size()));
    return true;
}

template <class F> void forEachLine(std::string_view text, F f) {
    for (std::string_view raw : splitLines(text)) {
        std::string_view line = stripInlineComment(raw);
        if (!line.empty()) f(line, toUpper(line));
    }
}

int toInt(std::string_view v, int fallback) {
    auto n = parseDec(v);
    return n ? static_cast<int>(*n) : fallback;
}

} // namespace

std::vector<ModuleMetadata> allModuleMetadata(const calcu::roms::RomSet& roms) {
    std::vector<ModuleMetadata> all;
    for (const auto& m : tiModules()) {
        ModuleMetadata md;
        md.id = m.id;
        md.menuTitle = m.name;
        if (const auto* bytes = roms.bytes(std::string(m.id) + ".rom.yaml")) {
            std::string error;
            TIModuleFile f = readModuleFile({reinterpret_cast<const char*>(bytes->data()), bytes->size()}, error);
            if (error.empty()) {
                md.id = f.moduleName;
                md.menuTitle = f.description;
                md.title = f.title;
                md.menuSort = f.menuSort;
            }
        }
        all.push_back(std::move(md));
    }
    std::stable_sort(all.begin(), all.end(),
                     [](const ModuleMetadata& a, const ModuleMetadata& b) { return a.menuSort < b.menuSort; });
    return all;
}

std::optional<std::string> moduleRomFile(std::string_view moduleID) {
    if (!isTIModule(moduleID)) return std::nullopt;
    return std::string(moduleID) + ".rom.yaml";
}

ModuleCards moduleCards(std::string_view text, std::string_view moduleID) {
    ModuleCards out;
    std::optional<CueCardContent> current;
    int currentKey = 0;
    bool inTarget = false, done = false;
    forEachLine(text, [&](std::string_view line, const std::string& upper) {
        if (done) return;
        std::string_view v;
        // MODULE-ID starts a section; leaving the target module ends the scan.
        if (takeValue(line, upper, "MODULE-ID:", v)) {
            if (inTarget) {
                if (current) out.cards[currentKey] = *current;
                current.reset();
                done = true;
                return;
            }
            inTarget = v == moduleID;
            out.metadata.id = std::string(v);
            return;
        }
        if (!inTarget) return;
        if (takeValue(line, upper, "MODULE-TITLE:", v)) { out.metadata.title = std::string(v); return; }
        if (takeValue(line, upper, "MODULE-SORT:", v)) { out.metadata.sort = std::string(v); return; }
        if (takeValue(line, upper, "MODULE-MENU-TITLE:", v)) { out.metadata.menuTitle = std::string(v); return; }
        if (takeValue(line, upper, "MODULE-MENU-SORT:", v)) { out.metadata.menuSort = toInt(v, out.metadata.menuSort); return; }
        if (startsWith(upper, "MODULE-ROM:")) return;
        if (takeValue(line, upper, "CUECARD:", v)) {
            if (current) out.cards[currentKey] = *current;
            currentKey = toInt(v, 0);
            current = CueCardContent();
            return;
        }
        if (current) current->parseLine(line);
    });
    if (inTarget && !done && current) out.cards[currentKey] = *current;
    return out;
}

} // namespace cu59
