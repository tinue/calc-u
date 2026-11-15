#include "PresetRoms.hpp"

#include <algorithm>

#include "../Machine/SharpCatalogue.hpp"

using namespace calcu::machine;

namespace presetroms {

namespace {

std::optional<VariantId> variantOf(const std::optional<std::string>& v) {
    return v ? std::optional<VariantId>(VariantId{*v}) : std::nullopt;
}

bool fromSet(const PresetRoms& roms, const std::vector<RomEntry>& entries, std::vector<RomImage>* out, std::string* error) {
    for (const RomEntry& e : entries) {
        const auto* bytes = roms.set ? roms.set->bytes(e.fileName) : nullptr;
        if (!bytes) {
            if (error) *error = "ROM file " + e.fileName + (roms.set ? " not found in the ROM set" : " not found: no ROM set was given");
            return false;
        }
        out->push_back({e.fileName, *bytes});
    }
    return true;
}

} // namespace

bool modelImages(const PresetRoms& roms, const char* model, const std::optional<std::string>& variant,
                 std::vector<RomImage>* out, std::string* error) {
    const MachineDescriptor* d = sharp::sharpCatalogue().model(ModelId{model});
    const std::optional<VariantId> v = variantOf(variant);
    if (!d || (v && std::none_of(d->variants.begin(), d->variants.end(), [&](const auto& x) { return x.id == *v; }))) {
        if (error) *error = "unknown ROM " + std::string(model) + (variant ? " " + *variant : "");
        return false;
    }
    return fromSet(roms, d->romsFor(v), out, error);
}

bool peripheralImages(const PresetRoms& roms, const char* peripheral, const std::optional<std::string>& variant,
                      std::vector<RomImage>* out, std::string* error) {
    const PeripheralDescriptor* d = sharp::sharpCatalogue().peripheral(PeripheralId{peripheral});
    if (!d) {
        if (error) *error = std::string(peripheral) + " is not a known peripheral";
        return false;
    }
    return fromSet(roms, d->romsFor(variantOf(variant)), out, error);
}

bool loadPC1500Rom(PC1500Machine& machine, const std::string& romVariant, const PresetRoms& roms, std::string* error) {
    std::vector<RomImage> images;
    if (!modelImages(roms, sharp::kPC1500, romVariant, &images, error)) return false;
    if (!machine.loadROM(images[0].bytes.data(), images[0].bytes.size())) {
        if (error) *error = "failed to load firmware ROM: " + images[0].fileName;
        return false;
    }
    return true;
}

bool loadPC1600RomSet(PC1600Machine& machine, const PresetRoms& roms, const std::string& version, std::string* error) {
    if (!isPC1600RomVersion(version)) {
        if (error) *error = "unknown PC-1600 ROM version '" + version + "' (expected new or old)";
        return false;
    }
    std::vector<RomImage> i;
    if (!modelImages(roms, sharp::kPC1600, version == "old" ? "Old" : "New", &i, error)) return false;
    // The catalogue's order (pc1600Roms()): P0-B0, P1-B0, P1-B3, P1-B3B, P2-B6, LH5803.
    // Each loader rejects an image of the wrong size.
    if (!(machine.loadBank0(i[0].bytes.data(), i[0].bytes.size(), i[1].bytes.data(), i[1].bytes.size()) &&
          machine.loadBank3Rom(i[2].bytes.data(), i[2].bytes.size()) &&
          machine.loadBank3bRom(i[3].bytes.data(), i[3].bytes.size()) &&
          machine.loadBank6Rom(i[4].bytes.data(), i[4].bytes.size()) &&
          machine.loadLH5803Rom(i[5].bytes.data(), i[5].bytes.size()))) {
        if (error) *error = "a PC-1600 " + version + " ROM image has the wrong size (a truncated dump?)";
        return false;
    }
    return true;
}

bool attachCE1600P(PC1600Machine& machine, const PresetRoms& roms, const std::string& version, std::string* error) {
    if (!isCE1600PRomVersion(version)) {
        if (error) *error = "unknown CE-1600P ROM version '" + version + "' (expected new or old)";
        return false;
    }
    std::vector<RomImage> i;
    if (!peripheralImages(roms, sharp::kCE1600P, version == "old" ? "Old" : "New", &i, error)) return false;
    if (!machine.attachCE1600P(i[0].bytes.data(), i[0].bytes.size(), i[1].bytes.data(), i[1].bytes.size())) {
        if (error) *error = "CE-1600P attach failed -- " + version + " ROM size/shape rejected";
        return false;
    }
    return true;
}

bool attachHostDrive(PC1600Machine& machine, const PresetRoms& roms, const std::filesystem::path& dir, std::string* error) {
    if (!roms.hostDrive) {
        if (error) *error = "the host drive's ROM is not bundled";
        return false;
    }
    if (!machine.attachHostDrive(roms.hostDrive->data(), roms.hostDrive->size(), dir)) {
        if (error) *error = "host-drive attach failed -- ROM size rejected";
        return false;
    }
    return true;
}

bool attachPlotterByName(PC1600Machine& machine, const std::string& plotterName, const PresetRoms& roms,
                         std::string* error, bool* outCe150Attached, const std::string& ce1600pVersion) {
    if (plotterName.empty()) return true;
    if (plotterName == "ce150") {
        if (!attachCE150(machine, roms, error)) return false;
        if (outCe150Attached) *outCe150Attached = true;
        return true;
    }
    if (plotterName == "ce1600p") return attachCE1600P(machine, roms, ce1600pVersion, error);
    if (error) *error = "plotter: '" + plotterName + "' is not a known plotter";
    return false;
}

} // namespace presetroms
