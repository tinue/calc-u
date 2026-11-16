#pragma once
#include <string>

#include "app/SharpPresetKeys.hpp"
#include "presets/ProductRules.hpp"

// ── Calc-U-1600's preset rules (LIB-6 design §10.3) ─────────────────────
//
// What Calc-U-1600 tells the library's preset loader, per model (AC-44 to
// AC-48, AC-53, AC-62, AC-67): idle is key readiness held for 20 frames,
// asked every frame (AC-45); after the build a PC-1500 or PC-1500A answers
// `NEW0? :CHECK` with CL, at most five times (AC-46); `screenshot:` writes
// Copy Screen's picture (AC-48); `expect:` finds its text in one line of the
// display read with the ROM's font (AC-48, AC-67); the markers of preset.md
// §7; the product's keywords (C1600-4, app/SharpPresetKeys).

namespace shared {

class SharpModelRules final : public calcu::presets::ModelRules {
public:
    explicit SharpModelRules(std::string model) : m_model(std::move(model)) {}
    calcu::presets::IdleRule idle() const override;
    calcu::sessions::Task<calcu::presets::ItemResult> afterBuild(calcu::presets::RunContext&) override;
    std::optional<std::vector<std::uint8_t>> screenshot(calcu::machine::Machine&,
                                                        const calcu::machine::DisplayOutput&) override;
    calcu::presets::ExpectReading compare(calcu::machine::Machine&, const calcu::machine::DisplayOutput&,
                                          const calcu::presets::ExpectText&) override;
    calcu::presets::Applies applies(std::string_view keyword) const override;
    void checkHeader(const calcu::presets::LoadPlan&, calcu::presets::ReadContext&) const override;
    void amendPlan(const calcu::presets::LoadPlan&, calcu::startup::MachinePlan&) const override;

private:
    bool pc1600() const;
    std::string m_model;
};

class SharpPresetRules final : public calcu::presets::ProductRules {
public:
    explicit SharpPresetRules(SharpKeywordSetup setup = {}) : m_setup(std::move(setup)) {}
    std::string_view productName() const override { return "Calc-U-1600"; }
    // D-30: the hint of a refused preset (a user-visible sentence).
    std::string_view formatReference() const override { return "the chapter Presets of the User Guide"; }
    calcu::presets::ModelRules& forModel(const calcu::machine::ModelId&) override;
    void addKeywords(calcu::presets::Vocabulary& v) override { addSharpKeywords(v, m_setup); }

private:
    SharpKeywordSetup m_setup;
    SharpModelRules m_pc1500{"PC-1500"}, m_pc1500a{"PC-1500A"}, m_pc1600{"PC-1600"};
};

// The PNG of Copy Screen's picture of the display (AC-48): the dot matrix,
// without the indicators, at the real display's size.
std::vector<std::uint8_t> screenPng(const calcu::machine::DisplayOutput&, bool pc1600);

} // namespace shared
