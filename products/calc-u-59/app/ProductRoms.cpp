#include "ProductRoms.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TIRomReader.hpp"
#include "machine/Build.hpp"
#include <stdexcept>
#include <utility>

namespace cu59 {

std::vector<calcu::roms::FullSetEntry> moduleRomEntries() {
    // LIB-12 AC-9: the module files of TI-59-ROM modules/ (90cd4a7), as
    // roms/fetch_roms.sh fetches and checks them.
    static const std::pair<const char*, const char*> kModules[] = {
        {"ML", "ab6db47478041bb1bd1b50b3114b8949"}, {"ST", "6e7e4045c9e31720dba5fbee0ace248a"},
        {"RE", "6ea276ed76b4b67df59141d73708e6ec"}, {"SY", "d87b45b1f0fc6963db7dc3e537eeaf05"},
        {"NG", "889d6f5c516afd62f7bd7680cecf47ed"}, {"AV", "84823786f1b065a74ba7b36319c05ba4"},
        {"LE", "f67efb0d1b10c46266d9a32891c9aa6c"}, {"SA", "f26fd2f1cbe52ec93387c76fe26122e6"},
        {"BD", "4ee8f14c5ce0f088cff3ea3f0c149f76"}, {"MU", "81403a7ed013e53c0334a156a7ca8c0e"},
        {"EE", "63fdc4324deb68025c9e98af817a91fc"}, {"SE", "ea082fed55d93610d95f16cf2577f1e1"},
        {"AG", "4903315a721f4ff41e60b40abbaeb284"}, {"RP", "d62d3ea22600d0df8f24a6355ee0ea95"},
    };
    std::vector<calcu::roms::FullSetEntry> out;
    for (const auto& [id, md5] : kModules)
        out.push_back({calcu::machine::RomEntry{std::string(id) + ".rom.yaml", calcu::machine::RomEncoding::Text,
                                                kModuleRomMaxBytes, calcu::machine::ContentUnit::Bytes, md5},
                       {calcu::roms::NeededBy{calcu::machine::PeripheralId{id}, std::nullopt}}});
    return out;
}

const calcu::roms::FullSet& fullRomSet() {
    static const calcu::roms::FullSet set = [] {
        const auto modules = moduleRomEntries();
        return calcu::roms::fullSet(tiCatalogue(), modules);
    }();
    return set;
}

calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle) {
    static const TIRomReader reader;
    return calcu::roms::lookUp(fullRomSet(), bundle, &reader);
}

std::unique_ptr<calcu::machine::Machine> buildMachine(Model model, const calcu::roms::RomSet& roms,
                                                      std::shared_ptr<const calcu::machine::ProductData> content) {
    const auto& catalogue = tiCatalogue();
    calcu::machine::BuildRequest request;
    request.model = modelId(model);
    request.roms = roms.imagesFor(*catalogue.model(request.model), std::nullopt);
    request.content = std::move(content);
    TIMachineBuilder builder;
    calcu::machine::BuildResult r = calcu::machine::build(catalogue, builder, request);
    if (!r.machine)
        throw std::logic_error("a looked-up ROM set did not build " + request.model.value + ": " +
                               (r.missingRoms.empty() ? r.refusal : r.missingRoms.front()));
    return std::move(r.machine);
}

std::optional<std::vector<uint8_t>> moduleImage(const calcu::roms::RomSet& roms, const std::string& romFile) {
    const auto* bytes = roms.bytes(romFile);
    if (!bytes) return std::nullopt;
    std::string error;
    TIModuleFile m = readModuleFile({reinterpret_cast<const char*>(bytes->data()), bytes->size()}, error);
    if (!error.empty()) return std::nullopt;   // not reachable: the lookup read it (AC-6)
    return std::move(m.rom);
}

} // namespace cu59
