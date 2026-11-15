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

/// The full set: the catalogue's ROM entries, the 14 module files among them
/// (AC-9; the module peripherals carry them since M-25).
const calcu::roms::FullSet& fullRomSet();

/// The lookup over `bundle`, with TIRomReader (AC-8).
calcu::roms::LookupResult lookUpRoms(const calcu::roms::Bundle& bundle);

/// What is plugged into the places at a build (C59-6): a module in `slot-1`
/// by its ID (none: the slot is empty, AC-10), the PC-100C in `printer` with
/// TRACE's position (AC-6). The default is 2.3.0's core: a printer, no module.
struct MachinePeripherals {
    std::optional<std::string> module;
    bool printer = true;
    bool trace = false;
};

/// The attach requests of the two places, with the module file from the set
/// (an ID that is no module, or a file the set lacks, gives a request whose
/// attach is refused and names the file: EC-1).
calcu::machine::PeripheralRequest moduleRequest(const std::string& moduleID, const calcu::roms::RomSet& roms);
calcu::machine::PeripheralRequest printerRequest(bool trace);

/// A machine of `model` built from the set through the machine interface
/// (calcu::machine::build with tiCatalogue() and TIMachineBuilder), in the
/// state after a cold boot (C59-1 AC-7), with `peripherals` attached at the
/// build (no power cycle). `content`: a TI-58C's constant memory
/// (TIConstantMemory). A set that lookUpRoms() made always builds (AC-6);
/// anything else is a programming error (std::logic_error).
std::unique_ptr<calcu::machine::Machine> buildMachine(Model model, const calcu::roms::RomSet& roms,
                                                      const MachinePeripherals& peripherals = {},
                                                      std::shared_ptr<const calcu::machine::ProductData> content = nullptr);

/// The ROM image of the module file `romFile` (`<ID>.rom.yaml`) in the set;
/// std::nullopt when the set has no such file.
std::optional<std::vector<uint8_t>> moduleImage(const calcu::roms::RomSet& roms, const std::string& romFile);

} // namespace cu59
