#pragma once
#include "TIPresetKeys.hpp"
#include "presets/ProductRules.hpp"
#include <string>

// Calc-U-59's preset rules: per model, idle is key readiness (or the card
// wait of Write / INV Write) without a break for 100 ms of emulated time,
// asked every 1 ms; nothing is pressed after the build; `screenshot:` writes
// the classic segment picture (app/TIDisplayPicture); `expect:` compares a
// number with X in its 13 digits (read as `data:` reads a value, so the two
// agree on what "13 digits" means) and is refused when read for a regular
// expression or a text that is no number; the markers of preset.md §7;
// `slot-1` by `name:` only; the product's keywords (app/TIPresetKeys).

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
    // The User Guide's chapter, by its name.
    calcu::presets::ModelRules& forModel(const calcu::machine::ModelId&) override;
    void addKeywords(calcu::presets::Vocabulary& v) override { addTIKeywords(v, m_setup); }

private:
    TIKeywordSetup m_setup;
    TIModelRules m_ti59{"TI-59"}, m_ti58{"TI-58"}, m_ti58c{"TI-58C"};
};

/// The display as text, one position after the other from the left (the
/// harness's `display-text` rule, tiPositionText), trimmed.
std::string tiDisplayText(const calcu::machine::DisplayOutput&);

/// X, the value in the display register, as a register: from C
/// while a result is shown, else the number being keyed in as the display
/// shows it; nullopt when neither can be read.
std::optional<RegisterNibbles> displayRegisterX(calcu::machine::Machine&, const calcu::machine::DisplayOutput&);

/// Whether two registers hold the same value: sign, 13 mantissa digits, exponent.
bool sameValue(const RegisterNibbles&, const RegisterNibbles&);

} // namespace cu59
