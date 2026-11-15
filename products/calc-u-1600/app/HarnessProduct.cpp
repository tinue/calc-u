#include "app/HarnessProduct.hpp"

#include <cstdio>

#include "Display/DisplayOutputText.hpp"
#include "Machine/SharpBuilder.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "roms/Bundle.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/Settings.hpp"
#include "sessions/SessionPart.hpp"
#include "app/PacingProfiles.hpp"
#include "app/SharpTyping.hpp"
#include "startup/ProductMachines.hpp"

// The product folder (design §8, O-2), set by CMake.
#ifndef CALCU1600_DIR
#define CALCU1600_DIR "."
#endif

namespace shared {

namespace {

using namespace calcu;

// A cell of the ROM font as the harness reads it, in the encoding of
// --lcd-text's rows: printable ASCII as is, `\` as `\\`, other codes `\xHH`.
std::string codeText(std::uint8_t code) {
    if (code == '\\') return "\\\\";
    if (code >= 0x20 && code < 0x7F) return std::string(1, static_cast<char>(code));
    char hex[8];
    std::snprintf(hex, sizeof hex, "\\x%02X", code);
    return hex;
}

harness::GlyphFont glyphFontOf(const LcdFont& f) {
    harness::GlyphFont g;
    g.cellWidth = f.cellWidth;
    g.cellHeight = f.cellHeight;
    for (const LcdGlyph& l : f.glyphs) {
        harness::Glyph x;
        x.text = codeText(l.code);
        for (std::size_t i = 0; i < 8; ++i) x.columns[i] = l.cell[i];
        g.glyphs.push_back(std::move(x));
    }
    return g;
}

// A-4: the font of the machine the session runs, read when it changes.
struct FontPart final : sessions::SessionPart {
    harness::GlyphFont font;
    void attach(sessions::SessionSetup&) override {}
    void machineChanged(sessions::SessionContext& c) override {
        font = c.machine() ? glyphFontOf(sharpLcdFont(*c.machine())) : harness::GlyphFont{};
    }
};

Model modelOf(const machine::ModelId& id) {
    if (id.value == "PC-1500") return Model::PC1500;
    if (id.value == "PC-1600") return Model::PC1600;
    return Model::PC1500A;
}

// Adapter HarnessMachines (LIB-18 design §5; the successor of LIB-11's A-1):
// the bare model in its default variant, the Sharp pacing profile, the sound
// rate, the bounds of LIB-18 EC-7 (7 s for the PC-1500 and PC-1500A, 27 s for
// the PC-1600); startUp() is adapter SharpBuildColdBoot, the first cold boot
// that SharpMachineBuilder does not make. Removed in M-46.
class HarnessMachines final : public startup::ProductMachines {
public:
    explicit HarnessMachines(const machine::Catalogue& c) : catalogue_(c) {}
    const machine::Catalogue& catalogue() const override { return catalogue_; }
    machine::MachineBuilder& builder() override { return builder_; }
    startup::MachinePlan builtInDefault(const machine::ModelId& model) override {
        return startup::MachinePlan{model, std::nullopt, {}, nullptr};
    }
    pacing::PacingProfile pacingProfile(const machine::ModelId& model) const override {
        return shared::pacingProfile(modelOf(model));
    }
    int soundRate(const machine::ModelId&) const override { return sharp::kSoundRate; }
    machine::Seconds readinessBound(const machine::ModelId& model) const override {
        return machine::Seconds(modelOf(model) == Model::PC1600 ? 27.0 : 7.0);
    }
    void startUp(machine::Machine& m) override { m.coldBoot(); }   // SharpBuildColdBoot

private:
    const machine::Catalogue& catalogue_;
    sharp::SharpMachineBuilder builder_;
};

class SharpHarnessProduct final : public harness::HarnessProduct {
public:
    SharpHarnessProduct() : catalogue_(sharp::sharpCatalogue()), machines_(catalogue_) {
        sharp::addModuleDefinitions(catalogue_, std::string(CALCU1600_DIR) + "/frontends/qt6/resources/cards");
    }
    std::string_view name() const override { return "Calc-U-1600"; }
    std::string_view shortName() const override { return "1600"; }
    std::string_view commandWord() const override { return "pc1600"; }
    std::string_view version() const override { return CALCU1600_VERSION; }   // APP_VERSION of the Qt build (CMake)
    const machine::Catalogue& catalogue() const override { return catalogue_; }
    machine::MachineBuilder& builder() override { return machines_.builder(); }

    const settings::ProductDecl& settingsDecl() const override { return shared::settingsDecl(); }

    roms::LookupResult lookUpRoms() const override {
        return sharp::lookUpRoms(roms::DirectoryBundle(std::string(CALCU1600_DIR) + "/roms"));
    }

    startup::ProductMachines& machines() override { return machines_; }

    harness::DisplayText readDisplay(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) override {
        const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame);
        if (!f) return {};
        return harness::readDotMatrix(*f, font_.font, o.indicators, d.display.indicators);
    }

    // LIB-5: the typing part (key, type; the Sharp profiles), then the font.
    std::vector<sessions::SessionPart*> sessionParts() override { return {&typing_, &font_}; }

private:
    machine::Catalogue catalogue_;
    HarnessMachines machines_;
    FontPart font_;
    calcu::typing::TypingPart typing_{typingProduct(), sessions::Hosting::VirtualTime};
};

} // namespace

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<SharpHarnessProduct>(); }

} // namespace shared
