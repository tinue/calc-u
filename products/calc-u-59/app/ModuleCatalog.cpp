#include "ModuleCatalog.hpp"
#include "TextUtil.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>
#include <map>
#include <memory>

namespace cu59 {

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

// The modules of slot-1 by their module files'
// `description`, no tooltip, in `menu-sort` order.
calcu::media::ModuleListing tiModuleListing(const std::shared_ptr<const calcu::roms::RomSet>& roms) {
    auto meta = std::make_shared<std::map<std::string, ModuleMetadata>>();
    if (roms)
        for (const ModuleMetadata& m : allModuleMetadata(*roms)) (*meta)[m.id] = m;
    calcu::media::ModuleListing l;
    l.label = [meta](const calcu::machine::PeripheralDescriptor& d) {
        const auto it = meta->find(d.id.value);
        return it != meta->end() && !it->second.menuTitle.empty() ? it->second.menuTitle : d.name;
    };
    l.tooltip = [](const calcu::machine::PeripheralDescriptor&) { return std::optional<std::string>(); };
    l.sortKey = [meta](const calcu::machine::PeripheralDescriptor& d) {
        const auto it = meta->find(d.id.value);
        return it != meta->end() ? it->second.menuSort : 0;
    };
    l.moduleSlots = {kTIModulePlace};
    return l;
}

} // namespace cu59
