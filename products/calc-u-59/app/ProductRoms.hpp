#pragma once
#include "MachineModelInfo.hpp"
#include "machine/Machine.hpp"
#include "machine/ProductData.hpp"
#include "roms/Lookup.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Calc-U-59's ROMs through the library's one lookup (LIB-12; design §2, §4).
// A frontend makes the Bundle of its packaging and calls lookUpRoms() once at
// its start; on success the app layer builds every machine and loads every
// module from the RomSet and reads no ROM file (AC-6); otherwise no machine
// starts and describe() of the result is the message (AC-5, AC-10).

namespace cu59 {

/// Adapter A-2 (LIB-12 design §5, until M-25): the 14 Solid State ROM module
/// files, which the module peripherals of C59-6 will carry, as entries
/// outside the catalogue. 5000 bytes each (D-3), md5 of AC-9.
std::vector<calcu::roms::FullSetEntry> moduleRomEntries();

/// The full set: the catalogue's ROM entries and the 14 module files (AC-9).
const calcu::roms::FullSet& fullRomSet();

/// The lookup over `bundle`, with TIRomReader (AC-8).
calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle);

/// A machine of `model` built from the set through the machine interface
/// (calcu::machine::build with tiCatalogue() and TIMachineBuilder), in the
/// state after a cold boot (C59-1 AC-7). `content`: a TI-58C's constant
/// memory (TIConstantMemory). A set that lookUpRoms() made always builds
/// (AC-6); anything else is a programming error (std::logic_error).
std::unique_ptr<calcu::machine::Machine> buildMachine(Model model, const calcu::roms::RomSet& roms,
                                                      std::shared_ptr<const calcu::machine::ProductData> content = nullptr);

/// The ROM image of the module file `romFile` (`<ID>.rom.yaml`, from
/// moduleRomFile(), adapter A-3) in the set; std::nullopt when the set has
/// no such file.
std::optional<std::vector<uint8_t>> moduleImage(const calcu::roms::RomSet& roms, const std::string& romFile);

} // namespace cu59
