#include "HarnessProduct.hpp"
#include "MachineModelInfo.hpp"
#include "ModuleCatalog.hpp"
#include "TIMagneticCardKind.hpp"
#include "PacingProfiles.hpp"
#include "ProductRoms.hpp"
#include "Settings.hpp"
#include "TIDebugPart.hpp"
#include "TIDebuggerSetup.hpp"
#include "TIFileKinds.hpp"
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

// C59-4 AC-22 (design §3.10): the debug panel's commands under `ti59`; no
// preset steps for them.
void addTIDebugCommands(harness::CommandRegistry& r) {
    r.addProduct("ti59", "asm", [](harness::CommandContext& c) -> harness::Reply {
        const auto w = words(c.line.argText);
        if (w.empty()) return harness::refusedReply("usage: ti59 asm load <file> | run | clear");
        if (w[0] == "load") {
            std::string path = c.line.argText.substr(4);   // after "load"
            path.erase(0, std::min(path.size(), path.find_first_not_of(" \t")));
            if (path.empty()) return harness::refusedReply("usage: ti59 asm load <file>");
            std::ifstream in(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
            std::optional<std::string> text;
            if (in) text = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            sessions::Value::Object o{{"name", sessions::Value(std::filesystem::path(path).filename().string())}};
            if (text) o.push_back({"text", sessions::Value(*text)});
            const sessions::ReplyMessage m = c.host.command(sessions::Command{commands::kAsmLoad, sessions::Value(std::move(o))});
            if (!m.done) return harness::refusedReply(m.reason);
            const sessions::Value v = m.result ? m.result->toValue() : sessions::Value();
            const std::string n = std::to_string(v.integer() ? *v.integer() : 0);
            return harness::doneReply(harness::Value(n + " instructions"), {n + " instructions"});
        }
        if (w[0] == "run") {
            const sessions::ReplyMessage m = c.host.command(sessions::Command{commands::kAsmRun, sessions::Value::object({})});
            if (!m.done) return harness::refusedReply(m.reason);
            const auto v = m.result ? m.result->toValue() : sessions::Value();
            const std::string status = v.string() ? *v.string() : std::string();
            return harness::doneReply(harness::Value(status), {status});
        }
        if (w[0] == "clear") return linesReply(c.host.command(sessions::Command{commands::kAsmClear, sessions::Value::object({})}));
        return harness::refusedReply("usage: ti59 asm load <file> | run | clear");
    }, "asm load <file> | run | clear  the ASM overlay: put over the ROM, enter at 1800, take away");
    r.addProduct("ti59", "dump", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1) return harness::refusedReply("usage: ti59 dump scom|memory|prog|vars");
        return linesReply(c.host.command(sessions::Command{commands::kDump, sessions::Value::object({{"what", w[0]}})}));
    }, "dump scom|memory|prog|vars  the lines of a dump (also into the log)");
    r.addProduct("ti59", "log-level", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1) return harness::refusedReply("usage: ti59 log-level off|info|debug");
        return linesReply(c.host.command(sessions::Command{commands::kLogLevel, sessions::Value::object({{"level", w[0]}})}));
    }, "log-level off|info|debug  the level of the machine's own log lines");
    r.addProduct("ti59", "heatmap", [](harness::CommandContext& c) {
        const auto w = words(c.line.argText);
        if (w.size() != 1 || w[0] != "reset") return harness::refusedReply("usage: ti59 heatmap reset");
        return linesReply(c.host.command(sessions::Command{commands::kHeatmapReset, sessions::Value::object({})}));
    }, "heatmap reset  set the ROM heatmap's counts to zero");
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
    // the program memory commands (C59-2 AC-9) since M-54; the magnetic card
    // in the run's save folder and slot-1's module listing (LIB-7 59, C59-7)
    // since M-55, so `card`, `arm`, `saveas { card: … }` and `media …` work.
    // The entry makes its own parts until M-59 (LIB-6 §11.5).
    void prepare(harness::RunSetup& r) override {
        parts_.reset();
        parts::LibraryPartsSetup s = r.partsSetup(tiCatalogue(), machines_, lookUpRoms());
        s.typing = &typingProduct();
        s.papers = tiPrinterPapers();
        s.printers = {kPC100C};
        TIDebuggerOptions debug;
        debug.showPanel = [this](sessions::SessionContext& c, const std::string& view) { debug_.showView(c, view); };
        s.debugger = tiDebuggerSetup(rules_, std::move(debug));
        s.presetRules = &presetRules_;
        s.bundle = &bundle_;
        s.productName = std::string(name());
        s.presetExtensions = tiPresetExtensions();
        s.fileKinds = kinds_.kinds();   // LIB-13 AC-9: `open <path>` takes every Calc-U-59 file
        s.controlRules = {&debug_};      // C59-4 AC-25
        s.mediaKinds = {magneticCardKind()};   // LIB-7 59, C59-7 (M-55)
        s.moduleListing = tiModuleListing(s.roms);
        parts_ = std::make_unique<parts::LibraryParts>(std::move(s));
        debug_.bindDebugger(parts_->debugger());
    }
    parts::LibraryParts& libraryParts() override { return *parts_; }
    std::vector<sessions::SessionPart*> sessionParts() override {
        std::vector<sessions::SessionPart*> p = parts_->sessionParts();
        p.push_back(&memory_);
        p.push_back(&debug_);
        return p;
    }
    void addCommands(harness::CommandRegistry& r) override {
        addTIHarnessCommands(r);
        addTIDebugCommands(r);
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
    TIProductMachines machines_;   // its constant memory: none (LIB-18 §11.2)
    TILevelRules rules_;
    TIPresetRules presetRules_;
    presets::DirectoryPresetBundle bundle_{CALCU59_EXAMPLES_DIR};
    ProgramMemoryPart memory_;
    // The debug panel's part (C59-4): its tabs computed as if shown, so that
    // `state calculator-debug` reads them (design §3.1).
    TIDebugPart debug_{TIDebugSetup{lookUpRoms().set, {}, true}};
    TIFileKinds kinds_{debug_};
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
