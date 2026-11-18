#include "Machine/SharpRoms.hpp"

#include <algorithm>
#include <filesystem>

#include "../Connector/MemoryCardCatalog.hpp"
#include "../FileIO.hpp"
#include "../SharpRomReader.hpp"
#include "Machine/SharpCatalogue.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

bool isModuleFile(const std::string& name) {
    const std::string_view suffix = kModuleFileSuffix;
    return name.size() > suffix.size() && name.ends_with(suffix);
}

const RomEntry& hostDriveEntry() {
    return sharpCatalogue().peripheral(PeripheralId{kHostDrive})->roms.at(0);
}

} // namespace

const calcu::roms::FullSet& fullRomSet() {
    static const calcu::roms::FullSet set = [] {
        // The CE-502B: a module definition with its ROM bytes, from
        // PC-1500-ROM (modules/CE-502B.card.yaml; roms/fetch_roms.sh).
        const calcu::roms::FullSetEntry ce502b{
            RomEntry{"CE-502B.module.yaml", RomEncoding::Text, 16384, ContentUnit::Bytes, "7c7af4b7adab69b5e75297bc1c3d7279"},
            {calcu::roms::NeededBy{PeripheralId{"CE-502B"}, std::nullopt}}};
        calcu::roms::FullSet s = calcu::roms::fullSet(sharpCatalogue(), std::span(&ce502b, 1));
        const std::string hostDrive = hostDriveEntry().fileName;
        std::erase_if(s.files, [&](const calcu::roms::FullSetEntry& f) { return f.entry.fileName == hostDrive; });
        return s;
    }();
    return set;
}

calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle) {
    static const SharpRomReader reader;
    return calcu::roms::lookUp(fullRomSet(), bundle, &reader);
}

bool isFullSetFile(std::string_view fileName) {
    const auto& files = fullRomSet().files;
    return std::any_of(files.begin(), files.end(), [&](const auto& f) { return f.entry.fileName == fileName; });
}

std::vector<RomSetModule> romSetModules(const calcu::roms::RomSet& roms) {
    std::vector<RomSetModule> out;
    for (const std::string& name : roms.fileNames()) {
        if (!isModuleFile(name)) continue;
        const auto* bytes = roms.bytes(name);
        RomSetModule m{name, std::string(bytes->begin(), bytes->end()), {}};
        std::string error;
        if (!parseMemoryCardDefinition(m.text, &m.definition, &error)) continue;   // not reachable: the lookup read it
        out.push_back(std::move(m));
    }
    return out;
}

std::optional<RomSetModule> romSetModule(const calcu::roms::RomSet& roms, const std::string& moduleName) {
    for (RomSetModule& m : romSetModules(roms))
        if (m.definition.moduleName == moduleName) return std::move(m);
    return std::nullopt;
}

int addModuleDefinitions(Catalogue& catalogue, const calcu::roms::RomSet& roms) {
    int added = 0;
    for (const RomSetModule& m : romSetModules(roms)) {
        if (catalogue.peripheral(PeripheralId{m.definition.moduleName})) continue;
        catalogue.addPeripheral(moduleDescriptor(m.definition));
        ++added;
    }
    return added;
}

std::optional<std::vector<std::uint8_t>> readHostDriveRom(const std::vector<std::string>& folders) {
    const std::string& file = hostDriveEntry().fileName;
    for (const std::string& folder : folders) {
        if (folder.empty()) continue;
        std::vector<std::uint8_t> bytes;
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::path(folder) / file;
        if (std::filesystem::is_regular_file(p, ec) && readWholeFile(p, &bytes)) return bytes;
    }
    return std::nullopt;
}

} // namespace sharp
