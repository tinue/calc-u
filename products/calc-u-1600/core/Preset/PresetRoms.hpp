#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PC1500/PC1500Machine.hpp"
#include "../PC1600/PC1600Machine.hpp"
#include "PresetRomSet.hpp"
#include "machine/Descriptor.hpp"
#include <filesystem>
#include <optional>

// Loading and attaching from PresetRoms, for the preset loaders (in place of
// the folder search they did before LIB-12).
namespace presetroms {

// The images of a model's / peripheral's variant (std::nullopt: none or the
// default) from the set, in the catalogue's order. False with `error` when
// there is no set or a file is not in it.
bool modelImages(const PresetRoms& roms, const char* model, const std::optional<std::string>& variant,
                 std::vector<calcu::machine::RomImage>* out, std::string* error);
bool peripheralImages(const PresetRoms& roms, const char* peripheral, const std::optional<std::string>& variant,
                      std::vector<calcu::machine::RomImage>* out, std::string* error);

// The PC-1500 firmware ROM revision `romVariant` ("A01"/"A03"/"A04", see
// PresetFile::romVariant) into `machine`.
bool loadPC1500Rom(PC1500Machine& machine, const std::string& romVariant, const PresetRoms& roms, std::string* error);

// True for the two PC-1600 calculator ROM versions: "new" (PEEK #(0,&7FFF)
// = 4 or 5) and "old" (= 130). See PresetFile::romVariant.
inline bool isPC1600RomVersion(const std::string& version) { return version == "new" || version == "old"; }
// True for the two CE-1600P ROM versions: "new" (`PEEK #(5,&7FFE)` = 5) and
// "old" (= 4). Independent of the PC-1600 calculator ROM version.
inline bool isCE1600PRomVersion(const std::string& version) { return version == "new" || version == "old"; }

// The PC-1600's six-file calculator ROM set of `version` ("new"/"old") into
// `machine`. The CE-1600P ROMs are separate (attachCE1600P).
bool loadPC1600RomSet(PC1600Machine& machine, const PresetRoms& roms, const std::string& version, std::string* error);

// The CE-150 plotter (a PC-1500/1500A, or the PC-1600's LH5803 side: the
// same attachCE150(bytes, size) shape).
template <typename Machine>
bool attachCE150(Machine& machine, const PresetRoms& roms, std::string* error) {
    std::vector<calcu::machine::RomImage> images;
    if (!peripheralImages(roms, "CE-150", std::nullopt, &images, error)) return false;
    if (!machine.attachCE150(images[0].bytes.data(), images[0].bytes.size())) {
        if (error) *error = "CE-150 attach failed -- ROM size/shape rejected (expected 8192 bytes)";
        return false;
    }
    return true;
}

// The CE-158 RS-232C / Centronics interface (same shape on both models).
template <typename Machine>
bool attachCE158(Machine& machine, const PresetRoms& roms, std::string* error) {
    std::vector<calcu::machine::RomImage> images;
    if (!peripheralImages(roms, "CE-158", std::nullopt, &images, error)) return false;
    if (!machine.attachCE158(images[0].bytes.data(), images[0].bytes.size())) {
        if (error) *error = "CE-158 attach failed -- ROM size/shape rejected (expected 16384 bytes)";
        return false;
    }
    return true;
}

// The CE-1600P plotter (and, per its union attach, the CE-1600F floppy,
// its drive empty) on a PC-1600, with the ROM pair of `version`
// ("new"/"old"; the CE-1600F follows it).
bool attachCE1600P(PC1600Machine& machine, const PresetRoms& roms, const std::string& version, std::string* error);

// Calc-U-1600's host-directory drive (S3: / Y:) on a PC-1600, serving `dir`.
bool attachHostDrive(PC1600Machine& machine, const PresetRoms& roms, const std::filesystem::path& dir, std::string* error);

// The plotter `plotterName` ("ce150"/"ce1600p"/"" for none) on a PC-1600 --
// a preset's `plotter:`. True and nothing done for "". `ce1600pVersion`
// ("new"/"old") is consulted for "ce1600p" only.
bool attachPlotterByName(PC1600Machine& machine, const std::string& plotterName, const PresetRoms& roms,
                         std::string* error, bool* outCe150Attached = nullptr, const std::string& ce1600pVersion = "new");

} // namespace presetroms
