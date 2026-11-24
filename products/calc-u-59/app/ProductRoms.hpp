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

// Calc-U-59's ROMs through the library's one lookup.
// A frontend makes the Bundle of its packaging and calls lookUpRoms() once at
// its start; on success the app layer builds every machine and loads every
// module from the RomSet and reads no ROM file; otherwise no machine
// starts and describe() of the result is the message.

namespace cu59 {

/// The full set: the catalogue's ROM entries, the 14 module files among them
/// (the module peripherals carry them).
const calcu::roms::FullSet& fullRomSet();

/// The lookup over `bundle`, with TIRomReader.
calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle);

/// What is plugged into the places at a build: a module in `slot-1` by its
/// ID (none: the slot is empty), the PC-100C in `printer` with TRACE's
/// position. The default: a printer, no module.
struct MachinePeripherals {
    std::optional<std::string> module;
    bool printer = true;
    bool trace = false;
};

/// The attach requests of the two places, with the module file from the set
/// (an ID that is no module, or a file the set lacks, gives a request whose
/// attach is refused and names the file).
calcu::machine::PeripheralRequest moduleRequest(const std::string& moduleID, const calcu::roms::RomSet& roms);
calcu::machine::PeripheralRequest printerRequest(bool trace);

/// A machine of `model` built from the set through the machine interface
/// (calcu::machine::build with tiCatalogue() and TIMachineBuilder), in the
/// state after a cold boot, with `peripherals` attached at the
/// build (no power cycle). `content`: a TI-58C's constant memory
/// (TIConstantMemory). A set that lookUpRoms() made always builds;
/// anything else is a programming error (std::logic_error).
std::unique_ptr<calcu::machine::Machine> buildMachine(const ModelId& model, const calcu::roms::RomSet& roms,
                                                      const MachinePeripherals& peripherals = {},
                                                      std::shared_ptr<const calcu::machine::ProductData> content = nullptr);

/// The ROM image of the module file `romFile` (`<ID>.rom.yaml`) in the set;
/// std::nullopt when the set has no such file, or when it does not read
/// (the reason in `problem`, if given).
std::optional<std::vector<uint8_t>> moduleImage(const calcu::roms::RomSet& roms, const std::string& romFile,
                                                std::string* problem = nullptr);

} // namespace cu59
