#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// A Solid State ROM module file, `<ID>.rom.yaml` (docs/formats/modules.md §2;
// LIB-12 AC-14): its name tag, its visible name and its ROM bytes. The cue
// cards of the file are not read yet: the module list and the cue cards still
// come from roms/solid-state/cuecards.txt (migration plan §2.3, until M-25 and
// M-55). Read with the library's YAML reader (calcu::formats, LIB-12 D-4); the
// version is checked first (format-version 1). Failures are return values.

struct TIModuleFile {
    std::string moduleName;           // `module-name`, the catalogue ID ("ML")
    std::string description;          // `description` ("Master Library")
    std::vector<std::uint8_t> rom;    // `rom.size` bytes, at most 5000
};

inline constexpr std::size_t kModuleRomMaxBytes = 5000;

/// Reads the file as bundled. Line endings \n, \r\n and \r read alike
/// (LIB-12 EC-2). `error` names what is wrong; empty on success.
TIModuleFile readModuleFile(std::string_view text, std::string& error);
