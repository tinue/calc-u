#pragma once
#include "TIPresetKeys.hpp"
#include "presets/ProductRules.hpp"
#include <string>

// Calc-U-59's preset rules (LIB-6 design §11.3; spec AC-35 to AC-40, AC-53,
// AC-62, AC-68; C59-2 AC-8): per model, idle is key readiness (or the card
// wait of Write / INV Write) without a break for 100 ms of emulated time,
// asked every 1 ms (AC-36, D-43); nothing is pressed after the build;
// `screenshot:` writes the classic segment picture (app/TIDisplayPicture,
// D-39); `expect:` compares a number with X in its 13 digits (D-44) and is
// refused when read for a regular expression or a text that is no number
// (AC-68, D-45); the markers of preset.md §7; `slot-1` by `name:` only
// (AC-35, D-42); the product's keywords (C59-2, app/TIPresetKeys).

namespace cu59 {

class TIModelRules final : public calcu::presets::ModelRules {
public:
    explicit TIModelRules(std::string model) : m_model(std::move(model)) {}
    calcu::presets::IdleRule idle() const override;
    std::optional<std::vector<std::uint8_t>> screenshot(calcu::machine::Machine&,
                                                        const calcu::machine::DisplayOutput&) override;
    calcu::presets::ExpectReading compare(calcu::machine::Machine&, const calcu::machine::DisplayOutput&,
                                          const calcu::presets::ExpectText&) override;
    std::optional<std::string> checkExpect(const calcu::presets::ExpectText&, std::string_view keyword) const override;
    calcu::presets::Applies applies(std::string_view keyword) const override;
    calcu::presets::PlaceKeyRule placeKey(const calcu::machine::PlaceName&) const override;
    void checkHeader(const calcu::presets::LoadPlan&, calcu::presets::ReadContext&) const override;

private:
    std::string m_model;
};

class TIPresetRules final : public calcu::presets::ProductRules {
public:
    explicit TIPresetRules(TIKeywordSetup setup = {}) : m_setup(std::move(setup)) {}
    std::string_view productName() const override { return "Calc-U-59"; }
    // D-38: until M-60 gives the help topic's address.
    std::string_view formatReference() const override { return "the preset format in the User Guide, chapter Presets"; }
    calcu::presets::ModelRules& forModel(const calcu::machine::ModelId&) override;
    void addKeywords(calcu::presets::Vocabulary& v) override { addTIKeywords(v, m_setup); }

private:
    TIKeywordSetup m_setup;
    TIModelRules m_ti59{"TI-59"}, m_ti58{"TI-58"}, m_ti58c{"TI-58C"};
};

/// The display as text, one position after the other from the left (the
/// harness's `display-text` rule, tiPositionText), trimmed.
std::string tiDisplayText(const calcu::machine::DisplayOutput&);

/// X, the value in the display register, as a register (AC-40, D-44): from C
/// while a result is shown, else the number being keyed in as the display
/// shows it; nullopt when neither can be read.
std::optional<RegisterNibbles> displayRegisterX(calcu::machine::Machine&, const calcu::machine::DisplayOutput&);

/// Whether two registers hold the same value: sign, 13 mantissa digits, exponent.
bool sameValue(const RegisterNibbles&, const RegisterNibbles&);

} // namespace cu59
