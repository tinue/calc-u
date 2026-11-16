#include "HarnessProduct.hpp"
#include "MachineModelInfo.hpp"
#include "PacingProfiles.hpp"
#include "ProductRoms.hpp"
#include "Settings.hpp"
#include "TIDebuggerSetup.hpp"
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

// C59-2 AC-9 (design §3.4): `ti59 partition|program|registers`, sent as the
// product commands of ProgramMemoryPart.
std::vector<std::string> words(const std::string& t) {
    std::vector<std::string> w;
    std::istringstream in(t);
    for (std::string s; in >> s;) w.push_back(s);
    return w;
}

harness::Reply linesReply(const sessions::ReplyMessage& r) {
    if (!r.done) return harness::refusedReply(r.reason);
    std::vector<std::string> out;
    const sessions::Value result = r.result ? r.result->toValue() : sessions::Value();
    if (const auto* a = result.elements())
            for (const auto& e : *a)
                if (e.string()) out.push_back(*e.string());
    if (out.empty()) return harness::doneReply();
    harness::Value v = harness::Value::array();
    for (const auto& l : out) v.push(l);
    return harness::doneReply(std::move(v), out);
}

void addTIHarnessCommands(harness::CommandRegistry& r) {
    r.addProduct("ti59", "partition", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() > 1) return harness::refusedReply("usage: ti59 partition [<value>]");
        sessions::Value args = w.empty() ? sessions::Value::object({}) : sessions::Value::object({{"value", w[0]}});
        return linesReply(c.host.command(sessions::Command{commands::kPartition, std::move(args)}));
    }, "partition [<value>]  set the partition (479, 479.59) or read it");
    r.addProduct("ti59", "program", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.empty()) return harness::refusedReply("usage: ti59 program [<step>] <keycode>... | ti59 program <step> count=<n>");
        sessions::Value::Object o;
        std::size_t i = 0;
        if (w[0].size() >= 3 && w[0].find('=') == std::string::npos) {
            o.push_back({"step", w[0]});
            i = 1;
        }
        if (i < w.size() && w[i].rfind("count=", 0) == 0) {
            if (i != 1 || w.size() != 2) return harness::refusedReply("usage: ti59 program <step> count=<n>");
            const std::string n = w[i].substr(6);
            bool digits = !n.empty() && n.size() < 5;
            for (char ch : n) digits = digits && ch >= '0' && ch <= '9';
            if (!digits) return harness::refusedReply("count= is a number of steps, at least 1.");
            o.push_back({"count", static_cast<std::int64_t>(std::stoi(n))});
        } else {
            std::string codes;
            for (; i < w.size(); ++i) codes += (codes.empty() ? "" : " ") + w[i];
            if (codes.empty()) return harness::refusedReply("usage: ti59 program [<step>] <keycode>...");
            o.push_back({"keycodes", codes});
        }
        return linesReply(c.host.command(sessions::Command{commands::kProgram, sessions::Value(std::move(o))}));
    }, "program [<step>] <keycode>... | program <step> count=<n>  write keycodes from a step, or read n");
    r.addProduct("ti59", "registers", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.empty()) return harness::refusedReply("usage: ti59 registers <NN>=<value>... | ti59 registers <NN>...");
        const bool set = w[0].find('=') != std::string::npos;
        sessions::Value::Array a;
        for (const std::string& x : w) {
            const std::size_t eq = x.find('=');
            if ((eq != std::string::npos) != set) return harness::refusedReply("Either write registers (NN=value) or read them (NN), not both.");
            if (set) a.emplace_back(sessions::Value::Array{sessions::Value(x.substr(0, eq)), sessions::Value(x.substr(eq + 1))});
            else a.emplace_back(x);
        }
        sessions::Value args = sessions::Value::object({{set ? "set" : "get", sessions::Value(std::move(a))}});
        return linesReply(c.host.command(sessions::Command{commands::kRegisters, std::move(args)}));
    }, "registers <NN>=<value>... | registers <NN>...  write data registers (H00-H03 on the TI-58C) or read them");
}

class TIHarnessProduct final : public harness::HarnessProduct {
public:
    std::string_view name() const override { return "Calc-U-59"; }
    std::string_view shortName() const override { return "59"; }
    std::string_view commandWord() const override { return "ti59"; }
    std::string_view version() const override { return CALCU59_VERSION; }   // MARKETING_VERSION (CMake)
    const machine::Catalogue& catalogue() const override { return tiCatalogue(); }
    machine::MachineBuilder& builder() override { return machines_.builder(); }

    const calcu::settings::ProductDecl& settingsDecl() const override { return cu59::settings::settingsDecl(); }

    roms::LookupResult lookUpRoms() const override { return cu59::lookUpRoms(roms::DirectoryBundle(CALCU59_ROM_DIR)); }

    startup::ProductMachines& machines() override { return machines_; }
    // The library's parts of the run: LIB-5's typing with the TI profiles and
    // map (`key`; no `type` on a TI model, D-8), in virtual time; the
    // PC-100C's paper (LIB-8 59) and the debugger (LIB-10 59: the run control,
    // traces into the run's folder) since M-53; the presets (LIB-6 59: the
    // TI rules and keywords, the bundle of the source tree's examples/) and
    // the program memory commands (C59-2 AC-9) since M-54; no media until
    // M-55. The entry makes its own parts until M-59 (LIB-6 §11.5).
    void prepare(harness::RunSetup& r) override {
        parts_.reset();
        parts::LibraryPartsSetup s = r.partsSetup(tiCatalogue(), machines_, lookUpRoms());
        s.typing = &typingProduct();
        s.papers = tiPrinterPapers();
        s.printers = {kPC100C};
        s.debugger = tiDebuggerSetup(rules_, {});
        s.presetRules = &presetRules_;
        s.bundle = &bundle_;
        s.productName = std::string(name());
        s.presetExtensions = {".ti59", ".ti58", ".ti58c"};
        parts_ = std::make_unique<parts::LibraryParts>(std::move(s));
    }
    parts::LibraryParts& libraryParts() override { return *parts_; }
    std::vector<sessions::SessionPart*> sessionParts() override {
        std::vector<sessions::SessionPart*> p = parts_->sessionParts();
        p.push_back(&memory_);
        return p;
    }
    void addCommands(harness::CommandRegistry& r) override { addTIHarnessCommands(r); }

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
    TIProductMachines machines_;   // its constant memory: none (LIB-18 §11.2)
    TILevelRules rules_;
    TIPresetRules presetRules_;
    presets::DirectoryPresetBundle bundle_{CALCU59_EXAMPLES_DIR};
    ProgramMemoryPart memory_;
    std::unique_ptr<parts::LibraryParts> parts_;
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
