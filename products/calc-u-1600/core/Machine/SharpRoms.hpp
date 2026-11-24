#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../Connector/MemoryCardDefinition.hpp"
#include "machine/Catalogue.hpp"
#include "roms/Lookup.hpp"

// Calc-U-1600's ROMs through the library's one lookup. A frontend or tool makes the Bundle of its packaging (a folder:
// the bundled resources, /bundle on the Web, roms/ for the tools and tests)
// and the lookup runs once at its start; on success every machine and
// every peripheral is built and attached from the RomSet and no ROM file is
// read again; otherwise no machine starts and describe() of the
// result is the message.
//
// In the core, not in app/: the core's scan of module
// definition folders (Connector/MemoryCardCatalog.hpp, used by
// SharpCatalogue's addModuleDefinitions) passes over the files of the full
// set (isFullSetFile), so the full set lives beside it.

namespace sharp {

/// The full set: every ROM entry of sharpCatalogue() but the host
/// drive's, and the CE-502B's module definition.
const calcu::roms::FullSet& fullRomSet();

/// The lookup over `bundle`, with SharpRomReader.
calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle);

/// True for a file of the full set. Only the lookup reads such a file: the
/// scans of module definition folders pass over it (the CE-502B's), so
/// that its definition comes from the RomSet alone.
bool isFullSetFile(std::string_view fileName);

/// The module definitions among the files of the set (the CE-502B's).
struct RomSetModule {
    std::string fileName;
    std::string text;                 // the file as bundled
    MemoryCardDefinition definition;
};
std::vector<RomSetModule> romSetModules(const calcu::roms::RomSet& roms);
/// The module of `moduleName` among them, or std::nullopt.
std::optional<RomSetModule> romSetModule(const calcu::roms::RomSet& roms, const std::string& moduleName);
/// Adds the set's module definitions to `catalogue`; returns
/// how many it added.
int addModuleDefinitions(calcu::machine::Catalogue& catalogue, const calcu::roms::RomSet& roms);

/// The host drive's own ROM, Calc-U-1600's firmware: always bundled, not
/// in the full set and not looked up. Read from
/// the first of `folders` that has it; std::nullopt when none has, with
/// the reason in `problem` if given. The one
/// ROM file read outside the lookup (CalcU1600NoOwnRomLookup lists it).
std::optional<std::vector<std::uint8_t>> readHostDriveRom(const std::vector<std::string>& folders,
                                                          std::string* problem = nullptr);

} // namespace sharp
