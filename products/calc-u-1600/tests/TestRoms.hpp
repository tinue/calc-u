#pragma once
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "../core/Connector/SoftwareDefinedCard.hpp"
#include "../core/Machine/SharpRoms.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/Preset/PresetRoms.hpp"

// ROM loading for the ROM-gated tests. Run from the product folder
// (CoreTests' working directory): the images live in roms/. Every ROM a
// test needs comes from the ROM set of roms/, looked up once through the
// library's lookup (LIB-12) as the app does; the helpers return null /
// false when a file of the full set is missing, so a test can print SKIP
// instead of failing.

// One ROM image, whole file. Size is left to the loader it's handed to
// (loadBank0/loadBank3Rom/... reject a wrong-size image).
inline bool readRomImage(const char* path, std::vector<uint8_t>* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    *out = std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return !out->empty();
}

// The ROM set of roms/; null when a file of the full set is missing or bad.
inline const std::shared_ptr<const calcu::roms::RomSet>& testRomSet() {
    static const std::shared_ptr<const calcu::roms::RomSet> set =
        sharp::lookUpRoms(calcu::roms::DirectoryBundle("roms")).set;
    return set;
}

// What the preset loaders get in the tests: the set, and the host drive's
// own ROM from firmware/pc1600-hostdrive/ (roms.md Q-1).
inline const PresetRoms& testPresetRoms() {
    static const PresetRoms roms = [] {
        PresetRoms r{testRomSet(), nullptr};
        if (auto rom = sharp::readHostDriveRom({"firmware/pc1600-hostdrive"}))
            r.hostDrive = std::make_shared<const std::vector<uint8_t>>(std::move(*rom));
        return r;
    }();
    return roms;
}

// A ROM module of the test set (the CE-502B) as a card for `host`; null
// without the set, or when it does not fit `host`.
inline std::unique_ptr<SoftwareDefinedCard> romSetCard(const std::string& moduleName, CardHost host) {
    if (!testRomSet()) return nullptr;
    auto m = sharp::romSetModule(*testRomSet(), moduleName);
    if (!m || !m->definition.compatibleWith(host)) return nullptr;
    return std::make_unique<SoftwareDefinedCard>(m->definition);
}

// The PC-1600's six-file ROM set, through the same loader the presets use.
inline bool loadPC1600Roms(PC1600Machine& m) {
    std::string error;
    return presetroms::loadPC1600RomSet(m, testPresetRoms(), "new", &error);
}

// ROMs loaded, ALL RESET, then the boot run to the prompt -- the same
// sequence the GUI's Reset All and the preset loader use. Attach modules
// or a plotter before calling, so the boot ROM sees them.
inline bool bootPC1600(PC1600Machine& m) {
    if (!loadPC1600Roms(m)) return false;
    m.allReset();
    runBootToPrompt(m);
    return true;
}
