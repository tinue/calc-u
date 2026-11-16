#include "app/HarnessProduct.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>

#include "Display/DisplayOutputText.hpp"
#include "Machine/SharpBuilder.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "roms/Bundle.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/Settings.hpp"
#include "sessions/SessionPart.hpp"
#include "app/PacingProfiles.hpp"
#include "MachineCodeFile.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpTopics.hpp"
#include "app/SharpTyping.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
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

// ---- C1600-2 AC-21 (design §3.7): the loaders under `pc1600` ----

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// The file's bytes, or nullopt (it cannot be read).
std::optional<std::vector<std::uint8_t>> readFile(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(pathOf(path), ec)) return std::nullopt;
    std::ifstream in(pathOf(path), std::ios::binary);
    if (!in) return std::nullopt;
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::vector<std::string> wordsOf(const std::string& text) {
    std::vector<std::string> w;
    std::istringstream in(text);
    for (std::string s; in >> s;) w.push_back(s);
    return w;
}

harness::Reply fromSession(const sessions::ReplyMessage& r) {
    if (!r.done) return harness::refusedReply(r.reason);
    return harness::doneReply();
}

// The file and its options: `<file> [<n>] [address=<a>] [length=<l>] [cpu=<name>] [run]`.
struct LoaderArgs {
    std::string file;
    sessions::Value args;
    std::string error;
};
LoaderArgs loaderArgs(const std::string& argText, bool codeOptions) {
    LoaderArgs out;
    const std::vector<std::string> w = wordsOf(argText);
    if (w.empty()) {
        out.error = codeOptions ? "usage: pc1600 load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]"
                                : "usage: pc1600 load-basic <file> [<n>]";
        return out;
    }
    out.file = w[0];
    const auto bytes = readFile(out.file);
    if (!bytes) {
        out.error = "The file " + out.file + " cannot be read.";
        return out;
    }
    sessions::Value::Object o{{"bytes", sessions::Value(*bytes)}, {"name", sessions::Value(out.file)}};
    for (std::size_t i = 1; i < w.size(); ++i) {
        const std::string& a = w[i];
        if (a.find_first_not_of("0123456789") == std::string::npos) {
            o.push_back({"file", sessions::Value(std::stoll(a))});
            continue;
        }
        if (!codeOptions) {
            out.error = "unknown argument " + a;
            return out;
        }
        if (a == "run") {
            o.push_back({"run", sessions::Value(true)});
        } else if (a.rfind("address=", 0) == 0) {
            o.push_back({"address", sessions::Value(a.substr(8))});
        } else if (a.rfind("length=", 0) == 0) {
            const std::string n = a.substr(7);
            if (n.empty() || n.find_first_not_of("0123456789") != std::string::npos) {
                out.error = "length= takes a number of bytes";
                return out;
            }
            o.push_back({"length", sessions::Value(std::stoll(n))});
        } else if (a.rfind("cpu=", 0) == 0) {
            o.push_back({"cpu", sessions::Value(a.substr(4))});
        } else {
            out.error = "unknown argument " + a;
            return out;
        }
    }
    out.args = sessions::Value(std::move(o));
    return out;
}

harness::Reply fileKindCmd(harness::CommandContext& c) {
    const std::vector<std::string> w = wordsOf(c.line.argText);
    if (w.size() != 1) return harness::refusedReply("usage: pc1600 file-kind <file>");
    const auto bytes = readFile(w[0]);
    if (!bytes) return harness::refusedReply("The file " + w[0] + " cannot be read.");
    const sessions::ReplyMessage r =
        c.host.ask(sessions::Request{requests::kFileKind, sessions::Value::object({{"bytes", *bytes}})});
    if (!r.done) return harness::refusedReply(r.reason);
    const auto* k = sessions::payloadAs<FileKindReply>(r.result);
    if (!k) return harness::doneReply();
    std::vector<std::string> lines{"kind: " + k->kind};
    harness::Value v = harness::Value::object();
    v.set("kind", k->kind);
    if (k->hasHeader) {
        const bool code = k->kind.rfind("code-", 0) == 0;
        if (!k->headerName.empty()) lines.push_back("name: " + k->headerName);
        if (code) lines.push_back("load: " + machinecode::hexAddress(k->loadAddress));
        if (k->bank != 0) lines.push_back("bank: " + std::to_string(k->bank));
        lines.push_back("length: " + std::to_string(k->length));
        if (code) lines.push_back("start: " + (k->start ? machinecode::hexAddress(k->start) : std::string("none")));
        v.set("name", k->headerName);
        v.set("load", static_cast<long long>(k->loadAddress));
        v.set("bank", k->bank);
        v.set("length", static_cast<unsigned long long>(k->length));
        v.set("start", static_cast<long long>(k->start));
    }
    if (!k->looksLike.empty()) {
        lines.push_back("looks-like: " + k->looksLike);
        v.set("looksLike", k->looksLike);
    }
    if (k->damaged) lines.push_back("damaged: yes");
    if (!k->words.empty()) {
        lines.push_back("words: " + k->words);
        v.set("words", k->words);
    }
    harness::Value files = harness::Value::array();
    for (std::size_t i = 0; i < k->tapeFiles.size(); ++i) {
        lines.push_back("file " + std::to_string(i + 1) + ": " + k->tapeFiles[i]);
        files.push(k->tapeFiles[i]);
    }
    if (!k->tapeFiles.empty()) v.set("files", std::move(files));
    return harness::doneReply(std::move(v), std::move(lines));
}

harness::Reply loadBasicCmd(harness::CommandContext& c) {
    LoaderArgs a = loaderArgs(c.line.argText, false);
    if (!a.error.empty()) return harness::refusedReply(a.error);
    return fromSession(c.host.command(sessions::Command{commands::kLoadBasic, std::move(a.args)}));
}

harness::Reply typeBasicCmd(harness::CommandContext& c) {
    const std::vector<std::string> w = wordsOf(c.line.argText);
    if (w.size() != 1) return harness::refusedReply("usage: pc1600 type-basic <file>");
    const auto bytes = readFile(w[0]);
    if (!bytes) return harness::refusedReply("The file " + w[0] + " cannot be read.");
    return fromSession(c.host.command(
        sessions::Command{commands::kTypeBasic, sessions::Value::object({{"bytes", *bytes}, {"name", w[0]}})}));
}

harness::Reply loadCodeCmd(harness::CommandContext& c) {
    LoaderArgs a = loaderArgs(c.line.argText, true);
    if (!a.error.empty()) return harness::refusedReply(a.error);
    const sessions::ReplyMessage r = c.host.command(sessions::Command{commands::kLoadCode, std::move(a.args)});
    if (!r.done) return harness::refusedReply(r.reason);
    const auto* l = sessions::payloadAs<CodeLoadedReply>(r.result);
    if (!l) return harness::doneReply();
    std::vector<std::string> lines{"range: " + l->range};
    harness::Value v = harness::Value::object();
    v.set("range", l->range);
    if (!l->area.empty()) {
        lines.push_back("area: " + l->area);
        lines.push_back("bank: " + std::to_string(l->bank));
        v.set("area", l->area);
        v.set("bank", l->bank);
    }
    lines.push_back("new: " + (l->newCommand.empty() ? std::string("none") : l->newCommand));
    lines.push_back("new-note: " + l->newNote);
    lines.push_back("start: " + l->callCommand);
    lines.push_back("start-note: " + l->callNote);
    v.set("new", l->newCommand);
    v.set("newNote", l->newNote);
    v.set("start", l->callCommand);
    v.set("startNote", l->callNote);
    harness::Value warnings = harness::Value::array();
    for (const std::string& w : l->warnings) {
        lines.push_back("warning: " + w);
        warnings.push(w);
    }
    v.set("warnings", std::move(warnings));
    lines.push_back(std::string("started: ") + (l->started ? "yes" : "no"));
    v.set("started", l->started);
    return harness::doneReply(std::move(v), std::move(lines));
}

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

    // LIB-5: the typing part (key, type; the Sharp profiles), the loaders
    // (C1600-2), then the font.
    std::vector<sessions::SessionPart*> sessionParts() override { return {&typing_, &loaders_, &font_}; }

    // C1600-2 AC-21: the loaders under the product's word (LIB-11 AC-13).
    void addCommands(harness::CommandRegistry& r) override {
        r.addProduct("pc1600", "file-kind", fileKindCmd, "file-kind <file>  what the file is");
        r.addProduct("pc1600", "load-basic", loadBasicCmd, "load-basic <file> [<n>]  load a BASIC program");
        r.addProduct("pc1600", "type-basic", typeBasicCmd, "type-basic <file>  type a BASIC listing in (PRO mode)");
        r.addProduct("pc1600", "load-code", loadCodeCmd,
                     "load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]  load machine code");
    }

private:
    machine::Catalogue catalogue_;
    HarnessMachines machines_;
    FontPart font_;
    calcu::typing::TypingPart typing_{typingProduct(), sessions::Hosting::VirtualTime};
    SharpLoaderPart loaders_{typing_};
};

} // namespace

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<SharpHarnessProduct>(); }

} // namespace shared
