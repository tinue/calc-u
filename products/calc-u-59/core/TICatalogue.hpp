#pragma once
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/ProductData.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

// The Calc-U-59 catalogue (LIB-1 AC-26 to AC-31; C59-1 design §3.2): the
// TI-59, TI-58 and TI-58C as machine descriptors, available without a machine
// and without ROM files, and the peripherals (C59-6; LIB-1 AC-32): the
// PC-100C and the 14 Solid State ROM modules, each module with its ROM file
// `<ID>.rom.yaml`, so that the full ROM set takes the module files from the
// catalogue (LIB-12 AC-9). Build a machine with
// calcu::machine::build(tiCatalogue(), builder, request); a request's
// peripherals are attached at the build, with no power cycle.

/// The model family of the three descriptors; the adapters' accessors check it.
inline const calcu::machine::ModelFamilyId kTIFamily{"TI-58/59"};

/// The places (C59-6 AC-1, AC-8) and the PC-100C's ID.
inline const calcu::machine::PlaceName kTIPrinterPlace{"printer"};
inline const calcu::machine::PlaceName kTIModulePlace{"slot-1"};
inline const calcu::machine::PeripheralId kPC100C{"PC-100C"};

/// A module of C59-6 AC-8: its ID (`module-name`), its name (`description`)
/// and its ROM chip.
struct TIModuleInfo {
    const char* id;
    const char* name;
    const char* chip;
    const char* md5;   // of `<ID>.rom.yaml` (LIB-12 AC-9)
};
/// The 14 in the order of 2.3.0's roms/solid-state/cuecards.txt (design D-4).
const std::vector<TIModuleInfo>& tiModules();
/// Whether `id` names one of the 14 modules.
bool isTIModule(std::string_view id);

const calcu::machine::Catalogue& tiCatalogue();

class TIMachineBuilder final : public calcu::machine::MachineBuilder {
public:
    /// Decodes the RomImages (TIRomText) and makes a TIMachine. A request's
    /// `content` of type TIConstantMemory is loaded into a TI-58C's RAM before
    /// its processor starts (C59-1 AC-6; C59-3 AC-2 hands it from M-54).
    calcu::machine::BuildResult build(const calcu::machine::BuildRequest&,
                                      const calcu::machine::Catalogue&) override;
};

/// The TI-58C's constant memory handed in at build: all 120 RAM registers of
/// 16 nibbles (the TI-58C's 64 are the first ones), as serialised by the core.
struct TIConstantMemory final : calcu::machine::ProductData {
    std::array<std::uint8_t, 120 * 16> ram{};
};
