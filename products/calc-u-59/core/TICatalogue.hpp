#pragma once
#include "machine/Build.hpp"
#include "machine/Catalogue.hpp"
#include "machine/ProductData.hpp"
#include <array>
#include <cstdint>

// The Calc-U-59 catalogue (LIB-1 AC-26 to AC-31; C59-1 design §3.2): the
// TI-59, TI-58 and TI-58C as machine descriptors, available without a machine
// and without ROM files; the peripherals (PC-100C, the 14 modules) come with
// C59-6 (M-25). Build a machine with calcu::machine::build(tiCatalogue(),
// builder, request).

/// The model family of the three descriptors; the adapters' accessors check it.
inline const calcu::machine::ModelFamilyId kTIFamily{"TI-58/59"};

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
