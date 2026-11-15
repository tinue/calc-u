#include "app/HarnessProduct.hpp"

#include <cstdio>

#include "Display/DisplayOutputText.hpp"
#include "Machine/SharpBuilder.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "roms/Bundle.hpp"
#include "roms/FullSet.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Startup.hpp"

// The product folder (design §8, O-2), set by CMake.
#ifndef CALCU1600_DIR
#define CALCU1600_DIR "."
#endif

namespace shared {

namespace {

using namespace calcu;

// The bundled folders the tests read ROM files from (tests/SharpTestSupport.hpp),
// in order; the first that has a file gives it.
class FoldersBundle final : public roms::Bundle {
public:
    FoldersBundle() : roms_(std::string(CALCU1600_DIR) + "/roms"), firmware_(std::string(CALCU1600_DIR) + "/firmware/pc1600-hostdrive") {}
    std::optional<std::vector<std::uint8_t>> read(std::string_view name) const override {
        if (auto b = roms_.read(name)) return b;
        return firmware_.read(name);
    }
    std::string where() const override { return roms_.where() + " and " + firmware_.where(); }

private:
    roms::DirectoryBundle roms_, firmware_;
};

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

class SharpHarnessProduct final : public harness::HarnessProduct {
public:
    SharpHarnessProduct() : catalogue_(sharp::sharpCatalogue()) {
        sharp::addModuleDefinitions(catalogue_, std::string(CALCU1600_DIR) + "/frontends/qt6/resources/cards");
    }
    std::string_view name() const override { return "Calc-U-1600"; }
    std::string_view shortName() const override { return "1600"; }
    std::string_view commandWord() const override { return "pc1600"; }
    std::string_view version() const override { return "0.8.1"; }   // the pin's (CHANGELOG.md)
    const machine::Catalogue& catalogue() const override { return catalogue_; }
    machine::MachineBuilder& builder() override { return builder_; }

    const settings::ProductDecl& settingsDecl() const override {   // A-3
        static const settings::ProductDecl decl = [] {
            settings::ProductDecl d;
            d.productName = "Calc-U-1600";
            d.models = {sharp::kPC1500, sharp::kPC1500A, sharp::kPC1600};
            d.fallbackModel = sharp::kPC1500A;
            d.keys = {settings::family::startupModel()};
            return d;
        }();
        return decl;
    }

    roms::LookupResult lookUpRoms() const override {
        static const roms::FullSet set = roms::fullSet(sharp::sharpCatalogue());
        return roms::lookUp(set, FoldersBundle(), nullptr);
    }

    machine::BuildRequest startupRequest(const settings::SettingsFile& s, const roms::RomSet& set) override {   // A-1
        machine::BuildRequest r;
        r.model = machine::ModelId{settings::startupModel(s)};
        if (const machine::MachineDescriptor* d = catalogue_.model(r.model)) r.roms = set.imagesFor(*d, std::nullopt);
        r.soundRate = sharp::kSoundRate;
        return r;
    }

    void startUp(machine::Machine& m) override { m.coldBoot(); }   // A-1: the builder runs nothing

    harness::DisplayText readDisplay(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) override {
        const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame);
        if (!f) return {};
        return harness::readDotMatrix(*f, font_.font, o.indicators, d.display.indicators);
    }

    std::vector<sessions::SessionPart*> sessionParts() override { return {&font_}; }

private:
    machine::Catalogue catalogue_;
    sharp::SharpMachineBuilder builder_;
    FontPart font_;
};

} // namespace

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<SharpHarnessProduct>(); }

} // namespace shared
