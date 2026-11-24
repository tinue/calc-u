#include "ProductRoms.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/PC100C.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TIRomReader.hpp"
#include "machine/Build.hpp"
#include <stdexcept>
#include <utility>

namespace cu59 {

const calcu::roms::FullSet& fullRomSet() {
    static const calcu::roms::FullSet set = calcu::roms::fullSet(tiCatalogue());
    return set;
}

calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle) {
    static const TIRomReader reader;
    return calcu::roms::lookUp(fullRomSet(), bundle, &reader);
}

calcu::machine::PeripheralRequest moduleRequest(const std::string& moduleID, const calcu::roms::RomSet& roms) {
    calcu::machine::PeripheralRequest r;
    r.place = calcu::machine::PlacePath{{kTIModulePlace}};
    r.peripheral = calcu::machine::PeripheralId{moduleID};
    if (const auto* d = tiCatalogue().peripheral(r.peripheral)) r.roms = roms.imagesFor(*d, std::nullopt);
    return r;
}

calcu::machine::PeripheralRequest printerRequest(bool trace) {
    calcu::machine::PeripheralRequest r;
    r.place = calcu::machine::PlacePath{{kTIPrinterPlace}};
    r.peripheral = kPC100C;
    r.switches = {{PC100C::kTrace, trace}};
    return r;
}

std::unique_ptr<calcu::machine::Machine> buildMachine(const ModelId& model, const calcu::roms::RomSet& roms,
                                                      const MachinePeripherals& peripherals,
                                                      std::shared_ptr<const calcu::machine::ProductData> content) {
    const auto& catalogue = tiCatalogue();
    calcu::machine::BuildRequest request;
    request.model = model;
    request.roms = roms.imagesFor(*catalogue.model(request.model), std::nullopt);
    if (peripherals.module) request.peripherals.push_back(moduleRequest(*peripherals.module, roms));
    if (peripherals.printer) request.peripherals.push_back(printerRequest(peripherals.trace));
    request.content = std::move(content);
    TIMachineBuilder builder;
    calcu::machine::BuildResult r = calcu::machine::build(catalogue, builder, request);
    if (!r.machine) {
        // The terminate handler logs what(): it names every missing file.
        std::string why = r.refusal;
        if (!r.missingRoms.empty()) {
            why = "missing ";
            for (std::size_t i = 0; i < r.missingRoms.size(); ++i) why += (i ? ", " : "") + r.missingRoms[i];
        }
        throw std::logic_error("A looked-up ROM set did not build the " + request.model.value + ": " + why);
    }
    return std::move(r.machine);
}

std::optional<std::vector<uint8_t>> moduleImage(const calcu::roms::RomSet& roms, const std::string& romFile,
                                                std::string* problem) {
    const auto* bytes = roms.bytes(romFile);
    if (!bytes) return std::nullopt;
    std::string error;
    TIModuleFile m = readModuleFile({reinterpret_cast<const char*>(bytes->data()), bytes->size()}, error);
    if (!error.empty()) {   // the lookup read it already: a slip of the set
        if (problem) *problem = error;
        return std::nullopt;
    }
    return std::move(m.rom);
}

} // namespace cu59
