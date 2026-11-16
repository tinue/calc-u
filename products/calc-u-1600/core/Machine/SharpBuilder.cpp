#include "Machine/SharpBuilder.hpp"

#include <map>

#include "../PC1500/PC1500InterfaceMachine.hpp"
#include "../PC1600/PC1600InterfaceMachine.hpp"
#include "Machine/SharpCatalogue.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

const std::vector<std::uint8_t>* image(const BuildRequest& req, const std::string& file) {
    for (const RomImage& i : req.roms)
        if (i.fileName == file) return &i.bytes;
    return nullptr;
}

} // namespace

BuildResult SharpMachineBuilder::build(const BuildRequest& req, const Catalogue& catalogue) {
    const MachineDescriptor* d = catalogue.model(req.model);
    if (!d || d->family.value != kSharpFamily)
        return {nullptr, {}, "not a Sharp PC-1500/1600 model: \"" + req.model.value + "\""};
    if (req.soundRate != kSoundRate)
        return {nullptr, {}, "The " + d->model.value + " gives sound at 48,000 samples per second only."};
    const VariantId variant = req.variant.value_or(*d->defaultVariant);
    const std::vector<RomEntry>& roms = d->romsFor(variant);
    std::vector<const std::vector<std::uint8_t>*> bytes;
    for (const RomEntry& e : roms) bytes.push_back(image(req, e.fileName));   // build() checked they are there

    std::unique_ptr<SharpMachine> machine;
    if (d->model.value == kPC1600) {
        auto m = std::make_unique<PC1600InterfaceMachine>(*d, variant);
        // Order of pc1600Roms(): P0-B0, P1-B0, P1-B3, P1-B3B, P2-B6, LH5803.
        PC1600Machine& c = m->core();
        const bool ok = c.loadBank0(bytes[0]->data(), bytes[0]->size(), bytes[1]->data(), bytes[1]->size()) &&
                        c.loadBank3Rom(bytes[2]->data(), bytes[2]->size()) &&
                        c.loadBank3bRom(bytes[3]->data(), bytes[3]->size()) &&
                        c.loadBank6Rom(bytes[4]->data(), bytes[4]->size()) &&
                        c.loadLH5803Rom(bytes[5]->data(), bytes[5]->size());
        if (!ok)
            return {nullptr, {}, "a PC-1600 " + variant.value + " ROM image has the wrong size (a truncated dump?)"};
        machine = std::move(m);
    } else {
        auto m = std::make_unique<PC1500InterfaceMachine>(*d, variant);
        if (!m->core().loadROM(bytes[0]->data(), bytes[0]->size()))
            return {nullptr, {}, "failed to load firmware ROM: " + roms[0].fileName};
        machine = std::move(m);
    }
    // The bus ROMs after everything else, in list order: each goes in front
    // of the chain, so a later one is found before an earlier one and before
    // a peripheral's ROM at the same place (C1600-4 AC-10, D-6).
    for (const bool busRoms : {false, true})
        for (const PeripheralRequest& p : req.peripherals) {
            if ((p.peripheral.value == kBusRom) != busRoms) continue;
            const ChangeResult r = machine->attach(p, catalogue);
            if (r.refused) return {nullptr, r.refused->missingRoms, r.refused->reason};
        }
    // LIB-1 AC-42, LIB-18 D-3/D-23: a new machine is left as a cold boot
    // leaves it (the adapter SharpBuildColdBoot goes, M-46).
    machine->coldBoot();
    return {std::move(machine), {}, {}};
}

} // namespace sharp
