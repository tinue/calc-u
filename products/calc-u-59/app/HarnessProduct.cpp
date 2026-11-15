#include "HarnessProduct.hpp"
#include "MachineModelInfo.hpp"
#include "ProductRoms.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TISegments.hpp"
#include "roms/Bundle.hpp"
#include "settings/Startup.hpp"

// The ROM folder the product's tests use (design §8, O-2), set by CMake.
#ifndef CALCU59_ROM_DIR
#define CALCU59_ROM_DIR "roms"
#endif

namespace cu59 {

namespace {

using namespace calcu;

class TIHarnessProduct final : public harness::HarnessProduct {
public:
    std::string_view name() const override { return "Calc-U-59"; }
    std::string_view shortName() const override { return "59"; }
    std::string_view commandWord() const override { return "ti59"; }
    std::string_view version() const override { return CALCU59_VERSION; }   // MARKETING_VERSION (CMake)
    const machine::Catalogue& catalogue() const override { return tiCatalogue(); }
    machine::MachineBuilder& builder() override { return builder_; }

    const settings::ProductDecl& settingsDecl() const override {   // A-3
        static const settings::ProductDecl decl = [] {
            settings::ProductDecl d;
            d.productName = "Calc-U-59";
            d.models = {"TI-59", "TI-58", "TI-58C"};
            d.fallbackModel = "TI-59";
            d.keys = {settings::family::startupModel()};
            return d;
        }();
        return decl;
    }

    roms::LookupResult lookUpRoms() const override { return cu59::lookUpRoms(roms::DirectoryBundle(CALCU59_ROM_DIR)); }

    machine::BuildRequest startupRequest(const settings::SettingsFile& s, const roms::RomSet& set) override {   // A-1
        machine::BuildRequest r;
        r.model = machine::ModelId{settings::startupModel(s)};
        if (const machine::MachineDescriptor* d = tiCatalogue().model(r.model)) r.roms = set.imagesFor(*d, std::nullopt);
        const MachinePeripherals start;   // 2.3.0's core: the PC-100C, no module
        if (start.printer) r.peripherals.push_back(printerRequest(start.trace));
        return r;
    }

    harness::DisplayText readDisplay(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) override {
        const auto* frame = std::get_if<machine::SegmentFrame>(&o.frame);
        const auto* g = std::get_if<machine::SegmentGeometry>(&d.display.geometry);
        if (!frame || !g) return {};
        return harness::readSegments(*frame, g->layout,
                                     [](const machine::SegmentPosition& p, const machine::SegmentLayout&) {
                                         return tiPositionText(p);
                                     },
                                     true, o.indicators, d.display.indicators);
    }

private:
    TIMachineBuilder builder_;
};

} // namespace

std::string tiPositionText(const calcu::machine::SegmentPosition& p) {
    std::uint8_t bits = 0;
    for (std::size_t s = 0; s < 7 && s < p.lit.size(); ++s)
        if (p.lit[s]) bits = static_cast<std::uint8_t>(bits | (1u << s));
    std::string t = " ";
    if (p.brightness.value_or(0) > 0) {
        for (int i = 0; i < 10; ++i)
            if (bits == kDigitSegments[i]) t = std::string(1, static_cast<char>('0' + i));
        if (bits == kSegmentsMinus) t = "-";
        if (bits == kSegmentsDegree) t = "\xC2\xB0";   // °
        if (bits == kSegmentsQuote) t = "\"";
        if (bits == kSegmentsApostrophe) t = "'";
    }
    if (p.lit.size() > static_cast<std::size_t>(kTIDecimalPoint) && p.lit[kTIDecimalPoint]) t += ".";
    return t;
}

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<TIHarnessProduct>(); }

} // namespace cu59
