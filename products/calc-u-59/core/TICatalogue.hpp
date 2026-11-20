#pragma once
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/ProductData.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

// The Calc-U-59 catalogue: the
// TI-59, TI-58 and TI-58C as machine descriptors, available without a machine
// and without ROM files, and the peripherals: the
// PC-100C and the 14 Solid State ROM modules, each module with its ROM file
// `<ID>.rom.yaml`, so that the full ROM set takes the module files from the
// catalogue. Build a machine with
// calcu::machine::build(tiCatalogue(), builder, request); a request's
// peripherals are attached at the build, with no power cycle.

/// The model family of the three descriptors; the adapters' accessors check it.
inline const calcu::machine::ModelFamilyId kTIFamily{"TI-58/59"};

/// The places and the PC-100C's ID.
inline const calcu::machine::PlaceName kTIPrinterPlace{"printer"};
inline const calcu::machine::PlaceName kTIModulePlace{"slot-1"};
inline const calcu::machine::PeripheralId kPC100C{"PC-100C"};

/// A module: its ID (`module-name`), its name (`description`)
/// and its ROM chip.
struct TIModuleInfo {
    const char* id;
    const char* name;
    const char* chip;
    const char* md5;   // of `<ID>.rom.yaml`
};
/// The 14 in catalogue order (ST before RE).
const std::vector<TIModuleInfo>& tiModules();
/// Whether `id` names one of the 14 modules.
bool isTIModule(std::string_view id);

const calcu::machine::Catalogue& tiCatalogue();

class TIMachineBuilder final : public calcu::machine::MachineBuilder {
public:
    /// Decodes the RomImages (TIRomText) and makes a TIMachine. A request's
    /// `content` of type TIConstantMemory is loaded into a TI-58C's RAM before
    /// its processor starts.
    calcu::machine::BuildResult build(const calcu::machine::BuildRequest&,
                                      const calcu::machine::Catalogue&) override;
};

/// The TI-58C's constant memory handed in at build: all 120 RAM registers of
/// 16 nibbles (the TI-58C's 64 are the first ones), as serialised by the core.
struct TIConstantMemory final : calcu::machine::ProductData {
    std::array<std::uint8_t, 120 * 16> ram{};
};
