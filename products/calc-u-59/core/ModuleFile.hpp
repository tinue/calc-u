#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// A Solid State ROM module file, `<ID>.rom.yaml` (docs/formats/modules.md §2;
// LIB-12 AC-14): its name tag, its visible name and its ROM bytes. Its cue
// cards are read by the app layer (app/ModuleCueCards, C59-7 AC-22, AC-23);
// the module list comes from the files since M-25 (C59-6 AC-8). Read with the library's YAML reader (calcu::formats, LIB-12 D-4); the
// version is checked first (format-version 1). Failures are return values.

struct TIModuleFile {
    std::string moduleName;           // `module-name`, the catalogue ID ("ML")
    std::string description;          // `description` ("Master Library"), what the picker shows
    std::string title;                // `title` ("MASTER LIBRARY MODULE -1-"), optional
    int menuSort = 0;                 // `menu-sort`, the picker's order (ties: file order)
    std::vector<std::uint8_t> rom;    // `rom.size` bytes, at most 5000
};

inline constexpr std::size_t kModuleRomMaxBytes = 5000;

/// Reads the file as bundled. Line endings \n, \r\n and \r read alike
/// (LIB-12 EC-2). `error` names what is wrong; empty on success.
TIModuleFile readModuleFile(std::string_view text, std::string& error);
