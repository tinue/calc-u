#include "app/SharpProductMachines.hpp"

#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMachine.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "app/PacingProfiles.hpp"

namespace cu1600 {

using namespace calcu::machine;


SharpProductMachines::SharpProductMachines(const Catalogue& catalogue,
                                           std::shared_ptr<const std::vector<std::uint8_t>> hostDriveRom)
    : catalogue_(catalogue), hostDriveRom_(std::move(hostDriveRom)) {}

calcu::startup::MachinePlan SharpProductMachines::builtInDefault(const ModelId& model) {
    // The default variant (A04, New), nothing plugged in.
    return calcu::startup::MachinePlan{model, std::nullopt, {}, nullptr};
}

calcu::pacing::PacingProfile SharpProductMachines::pacingProfile(const ModelId& model) const {
    return cu1600::pacingProfile(model);
}

int SharpProductMachines::soundRate(const ModelId&) const { return sharp::kSoundRate; }

Seconds SharpProductMachines::readinessBound(const ModelId& model) const {
    return Seconds(model.value == sharp::kPC1600 ? 27.0 : 7.0);
}

PeripheralRequest SharpProductMachines::carried(const PlugEntry& entry, Machine& m) {
    PeripheralRequest r = ProductMachines::carried(entry, m);
    if (entry.peripheral.value == sharp::kHostDrive) {   // the folder goes along
        if (auto* f = calcu::machine::first<sharp::HostDriveFolder>(m)) r.content = std::make_shared<sharp::HostDriveRequest>(f->folder());
    }
    return r;
}

void SharpProductMachines::addRoms(PeripheralRequest& r) const {
    if (r.peripheral.value != sharp::kHostDrive || !hostDriveRom_) return;
    const PeripheralDescriptor* d = catalogue_.peripheral(r.peripheral);
    if (!d || d->roms.empty()) return;
    for (const RomImage& i : r.roms)
        if (i.fileName == d->roms.front().fileName) return;
    r.roms.push_back({d->roms.front().fileName, *hostDriveRom_});
}

} // namespace cu1600
