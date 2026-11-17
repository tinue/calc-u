#pragma once
#include "TIProductKinds.hpp"
#include "machine/ProductData.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

// The content of a TI-59 magnetic card (C59-7 design §3.1; spec AC-1, AC-19;
// LIB-7 AC-46, D-1, D-2): what the card reader's media place takes and gives
// back, and what the app layer's codec (app/TIMagneticCardKind) reads from
// and writes to a card file (docs/formats/media.md §1). A card has two
// sides, 0 and 1; a side is fresh (never written), the 246 bytes of a bank
// as the ROM writes them, or an in-file definition (a preset's
// `magnetic-cards:`, preset.md §3.1) that becomes those bytes when the side
// goes in (buildCardBank). The bank format: rom/TI59-ROM-Guide.md,
// "Magnetic cards".

namespace cu59 {

inline constexpr std::size_t kCardBankBytes = 246;

/// An in-file side, before it goes in (C59-7 AC-18, AC-19).
struct TICardSideDef {
    int bank = 1;                                   ///< 1–4
    bool isProtected = false;                       ///< written as −n Write
    std::optional<int> partitionSteps;              ///< none: the calculator's when the side goes in
    std::map<int, std::uint8_t> steps;              ///< program step → keycode (inside the bank)
    std::map<int, RegisterNibbles> registers;       ///< data register → value (inside the bank)
    bool operator==(const TICardSideDef&) const = default;
};

struct TIMagneticCardData final : calcu::machine::ProductData {
    /// Nothing (fresh), the 246 bytes the ROM writes, or an in-file definition.
    using Side = std::variant<std::monostate, std::vector<std::uint8_t>, TICardSideDef>;
    std::array<Side, 2> sides;
    /// The side the last pass wrote (the codec writes only it into a file
    /// that holds the card, C59-7 AC-5).
    std::optional<int> lastWritten;
    /// The app layer's: the card's name, description and cue card. The core
    /// carries it through a pass untouched and never looks into it (D-1).
    std::shared_ptr<const calcu::machine::ProductData> label;
};

/// The 246 bytes the ROM writes for `side` with `programRegs` program
/// registers (a multiple of 10) unless the side names its partition (AC-19).
/// Steps and registers it does not list are 00; the header and the checksum
/// as the ROM makes them.
std::vector<std::uint8_t> buildCardBank(const TICardSideDef& side, int programRegs);

/// The bank number a side holds, for the corner of the cue card (AC-11):
/// none for a fresh side; a definition's `bank`; else the bank byte's.
std::optional<int> sideBank(const TIMagneticCardData::Side&);

/// The bank number from a bank's page byte (0x10, 0x13, 0x16, 0x19 → 1–4), and back.
int bankOfPageByte(std::uint8_t pageByte);
std::uint8_t pageByteOfBank(int bank);

/// The first program step and the data registers of bank n (preset.md §3.1):
/// steps 240·(n−1) … +239; registers 120 − 30·n … 149 − 30·n (only those below
/// 100 are data registers).
int firstStepOfBank(int bank);
bool stepInBank(int step, int bank);
bool registerInBank(int reg, int bank);

} // namespace cu59
