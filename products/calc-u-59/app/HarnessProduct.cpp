#include "HarnessProduct.hpp"
#include "TIAppPart.hpp"
#include "MachineModelInfo.hpp"
#include "ModuleCatalog.hpp"
#include "TIMagneticCardKind.hpp"
#include "PacingProfiles.hpp"
#include "ProductRoms.hpp"
#include "Settings.hpp"
#include "TIDebugPart.hpp"
#include "TIDebuggerSetup.hpp"
#include "TIFileKinds.hpp"
#include "TIHarnessCommands.hpp"
#include "TIPrinters.hpp"
#include "TITyping.hpp"
#include "TIPresetCommands.hpp"
#include "TIPresetRules.hpp"
#include "TIProductMachines.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "presets/Source.hpp"
#include "sessions/Args.hpp"
#include <sstream>
#include "../core/TICatalogue.hpp"
#include "../core/TISegments.hpp"
#include "harness/Run.hpp"
#include "parts/LibraryParts.hpp"
#include "roms/Bundle.hpp"
#include "startup/ProductMachines.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

// The ROM folder the product's tests use (design §8, O-2), set by CMake.
#ifndef CALCU59_ROM_DIR
#define CALCU59_ROM_DIR "roms"
#endif
// The bundled presets (examples/bundle.yaml; LIB-6 AC-31, AC-33), set by CMake.
#ifndef CALCU59_EXAMPLES_DIR
#define CALCU59_EXAMPLES_DIR "examples"
#endif

namespace cu59 {

namespace {

using namespace calcu;

class TIHarnessProduct final : public harness::HarnessProduct {
public:
    std::string_view name() const override { return "Calc-U-59"; }
    std::string_view shortName() const override { return "c59"; }
    std::string_view version() const override { return CALCU59_VERSION; }   // MARKETING_VERSION (CMake)

    const calcu::settings::ProductDecl& settingsDecl() const override { return cu59::settings::settingsDecl(); }

    // LIB-11 §11.3 (Part D simplification §9, as Calc-U-1600's since M-51):
    // the run's app layer is TIAppPart, as the frontends host it, on the
    // run's settings and places (the save folder as the run passes it, the
    // traces and screenshots into the output folder), typing in emulated time
    // (Hosting::VirtualTime), the ROM set of the source tree's roms/ (or the
    // folder CALCU59_ROMS_DIR names: the build with one ROM file taken away,
    // tests/rom_missing_build.sh, as frontends/web/build.sh reads it) and the
    // bundle of its examples/; every part in the one order (LibraryParts:
    // the control part, the run's own, typing, peripherals, presets, startup,
    // media, the debug panel (its tabs computed as if shown), open, paper,
    // debugger, then the program memory commands and TIAppPart).
    roms::LookupResult prepare(harness::RunSetup& r) override {
        part_.reset();
        const char* dir = std::getenv("CALCU59_ROMS_DIR");
        TIAppSetup s;
        s.roms = cu59::lookUpRoms(roms::DirectoryBundle(dir && *dir ? dir : CALCU59_ROM_DIR));
        s.settings = &r.settings();
        s.paths = &r.paths();
        s.examplesFolder = CALCU59_EXAMPLES_DIR;
        s.hosting = sessions::Hosting::VirtualTime;
        s.debugTabsShown = true;
        part_ = std::make_unique<TIAppPart>(std::move(s));
        return part_->romLookup();
    }
    parts::LibraryParts& libraryParts() override { return part_->libraryParts(); }
    std::vector<sessions::SessionPart*> sessionParts(const std::vector<sessions::SessionPart*>& host) override {
        return part_->sessionParts(host);
    }
    void addCommands(harness::CommandRegistry& r) override {
        addTIHarnessCommands(r);   // C59-2, C59-4, C59-9 (app/TIHarnessCommands)
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
    std::unique_ptr<TIAppPart> part_;
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
