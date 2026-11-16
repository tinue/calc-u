#include "app/HarnessProduct.hpp"
#include "app/SharpMediaKinds.hpp"

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
#include "app/SharpControlRules.hpp"
#include "app/SharpDebuggerSetup.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpPrinters.hpp"
#include "app/SharpTopics.hpp"
#include "app/SharpTyping.hpp"
#include "debugger/DebuggerPart.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "paper/PaperPart.hpp"
#include "app/SharpDisplayLines.hpp"
#include "app/SharpPresetRules.hpp"
#include "app/SharpProductMachines.hpp"
#include "presets/Source.hpp"
#include "startup/ProductMachines.hpp"
#include "app/dap/DapPart.hpp"
#include "harness/ServeAddOn.hpp"
#include "presets/PresetsPart.hpp"
#ifdef CALCU_ATTACH
#include "harness/attach/LocalStream.hpp"
#endif

// The product folder (design §8, O-2), set by CMake.
#ifndef CALCU1600_DIR
#define CALCU1600_DIR "."
#endif

namespace shared {

namespace {

using namespace calcu;

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

// ---- C1600-3 AC-22 (design §3.5): `serve --dap <port>` ----
//
// The DAP server of the harness's session on attach's one-client listener
// (LocalStream; D-3: built where attach is). What the stream's I/O thread
// gets is posted into the serve loop and handed to the part there.
class DapServeAddOn final : public harness::ServeAddOn
#ifdef CALCU_ATTACH
    ,
                            public dap::DapTransport
#endif
{
public:
    explicit DapServeAddOn(dap::DapPart& part) : part_(part) {}
    std::string_view option() const override { return "--dap"; }
    std::string_view help() const override {
        return "accept a debugger (DAP, VS Code) on 127.0.0.1:<port>; emulated time passes, unpaced, while it has "
               "the machine running. A scenario that pauses by a breakpoint repeats exactly, one that pauses by hand "
               "does not.";
    }
    std::optional<std::string> start(std::string_view value, sessions::AppSession&, harness::ServeLoop loop) override {
#ifdef CALCU_ATTACH
        const std::string text(value);
        long port = -1;
        if (!text.empty() && text.size() <= 6 && text.find_first_not_of("0123456789") == std::string::npos) port = std::stol(text);
        if (std::optional<std::string> refused = part_.setRunPort(port)) return *refused;   // EC-1
        loop_ = std::move(loop);
        part_.setTransport(this);
        if (!part_.status().client && part_.status().status.rfind("Listening", 0) != 0) {
            const std::string why = part_.status().status;
            part_.setTransport(nullptr);
            return why;   // "Port <port> not available: <why>" (AC-2)
        }
        return std::nullopt;
#else
        (void)value;
        (void)loop;
        return std::string("The DAP server is not in this build.");   // D-3
#endif
    }
    bool wantsTime() const override { return part_.wantsTime(); }
    void stop() override {
#ifdef CALCU_ATTACH
        part_.setTransport(nullptr);
#endif
    }

#ifdef CALCU_ATTACH
    // ---- dap::DapTransport, on the loop's thread ----
    std::optional<std::string> listen(std::uint16_t port, std::string busy) override {
        close();
        stream_ = std::make_unique<harness::LocalStream>(
            [this](harness::StreamEvent e) {
                if (loop_.post) loop_.post([this, e = std::move(e)]() mutable { arrived(std::move(e)); });
            },
            port, std::move(busy));
        const harness::LocalStream::Opened o = stream_->open();
        if (!o.ok) {
            stream_.reset();
            return o.problem;
        }
        return std::nullopt;
    }
    void close() override {
        stream_.reset();
        client_ = 0;
    }
    void send(std::string bytes) override {
        if (stream_ && client_) stream_->write(client_, std::move(bytes));
    }
    void dropClient() override {
        if (stream_ && client_) stream_->close(client_);
        client_ = 0;
    }
    void quitProgram() override {
        if (loop_.quit) loop_.quit();   // the run ends after this request (exit 0)
    }

private:
    void arrived(harness::StreamEvent e) {
        switch (e.kind) {
        case harness::StreamEvent::Kind::Connected:
            client_ = e.client;
            part_.take(dap::DapInput{dap::DapInput::Kind::Connected, {}});
            break;
        case harness::StreamEvent::Kind::Bytes:
            if (e.client == client_) part_.take(dap::DapInput{dap::DapInput::Kind::Bytes, std::move(e.bytes)});
            break;
        case harness::StreamEvent::Kind::Disconnected:
            if (e.client != client_) break;   // one the part sent away already
            client_ = 0;
            part_.take(dap::DapInput{dap::DapInput::Kind::Disconnected, {}});
            break;
        }
    }

    harness::ServeLoop loop_;
    std::unique_ptr<harness::LocalStream> stream_;
    std::uint64_t client_ = 0;
#endif

private:
    dap::DapPart& part_;
};

std::shared_ptr<const std::vector<std::uint8_t>> harnessHostDriveRom() {
    auto rom = sharp::readHostDriveRom({std::string(CALCU1600_DIR) + "/firmware/pc1600-hostdrive"});
    return rom ? std::make_shared<const std::vector<std::uint8_t>>(std::move(*rom)) : nullptr;
}

class SharpHarnessProduct final : public harness::HarnessProduct {
public:
    explicit SharpHarnessProduct(std::string romFolder = std::string(CALCU1600_DIR) + "/roms")
        : romFolder_(std::move(romFolder)),
          catalogue_(sharp::sharpCatalogue()),
          machines_(catalogue_, harnessHostDriveRom()),
          bundle_(std::string(CALCU1600_DIR) + "/examples") {
        sharp::addModuleDefinitions(catalogue_, std::string(CALCU1600_DIR) + "/frontends/qt6/resources/cards");
        // The ROM modules of the set (the CE-502B; LIB-12 AC-14), as the app
        // has them: a preset names them by name (C1600-4 AC-8).
        if (const auto found = lookUpRoms(); found.set) sharp::addModuleDefinitions(catalogue_, *found.set);
    }
    std::string_view name() const override { return "Calc-U-1600"; }
    std::string_view shortName() const override { return "1600"; }
    std::string_view commandWord() const override { return "pc1600"; }
    std::string_view version() const override { return CALCU1600_VERSION; }   // APP_VERSION of the Qt build (CMake)
    const machine::Catalogue& catalogue() const override { return catalogue_; }
    machine::MachineBuilder& builder() override { return machines_.builder(); }

    const settings::ProductDecl& settingsDecl() const override { return shared::settingsDecl(); }

    roms::LookupResult lookUpRoms() const override {
        return sharp::lookUpRoms(roms::DirectoryBundle(romFolder_));
    }

    startup::ProductMachines& machines() override { return machines_; }

    // LIB-7 (§10.5): the modules, disks and tapes; the bundled templates
    // beside the module definitions.
    std::vector<calcu::media::KindDecl> mediaKinds() override {
        return shared::mediaKinds(std::string(CALCU1600_DIR) + "/frontends/qt6/resources/cards", catalogue_);
    }
    calcu::media::ModuleListing moduleListing() const override { return shared::moduleListing(); }

    harness::DisplayText readDisplay(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) override {
        return displayLines(o, d, font_.font);
    }

    // LIB-6 (§10.2): the product's rules and its bundled presets.
    presets::ProductRules* presetRules() override { return &rules_; }
    const presets::PresetBundle* presetBundle() const override { return &bundle_; }
    typing::TypingPart* typingPart() override { return &typing_; }

    // LIB-5: the typing part (key, type; the Sharp profiles), the loaders
    // (C1600-2), the font, then LIB-8's papers of the two plotters (`paper
    // ...`, `cut-paper`; the run hands the part its machine control, A-3) and
    // LIB-10's debugger (pause, step, break, watch, trace, symbols, ...; the
    // run hands it its trace folder and limit).
    // C1600-3: the DAP server after the debugger, on the run's parts.
    std::vector<sessions::SessionPart*> sessionParts() override {
        return {&typing_, &loaders_, &font_, &paper_, &debugger_, &dap_};
    }
    void runParts(const harness::RunParts& r) override {
        dap_.deps().presets = r.presets;
        dap_.deps().control = r.control;
        dap_.deps().settings = r.settings;
    }
    std::vector<harness::ServeAddOn*> serveAddOns() override { return {&dapAddOn_}; }
    paper::PaperPart* paperPart() override { return &paper_; }

    // LIB-13 (Part D, D4.2): `open` takes the presets by their extensions,
    // the media and the two program kinds, which load at once here (no
    // dialogs); the harness way has no limit (D-15).
    std::vector<std::string> presetExtensions() const override { return shared::presetExtensions(); }
    std::vector<opening::FileKind*> fileKinds() override { return {&basicKind_, &codeKind_}; }
    debugger::DebuggerPart* debuggerPart() override { return &debugger_; }
    // LIB-4 (Part D, T16-7): the printers that own the paper panel and the
    // product's rules; the harness can mount a host folder (C1600-5 AC-11).
    control::ControlSetup controlSetup() const override { return sharpControlSetup(catalogue_, controlRules_); }

    // C1600-2 AC-21: the loaders under the product's word (LIB-11 AC-13).
    void addCommands(harness::CommandRegistry& r) override {
        r.addProduct("pc1600", "file-kind", fileKindCmd, "file-kind <file>  what the file is");
        r.addProduct("pc1600", "load-basic", loadBasicCmd, "load-basic <file> [<n>]  load a BASIC program");
        r.addProduct("pc1600", "type-basic", typeBasicCmd, "type-basic <file>  type a BASIC listing in (PRO mode)");
        r.addProduct("pc1600", "load-code", loadCodeCmd,
                     "load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]  load machine code");
    }

private:
    std::string romFolder_;
    machine::Catalogue catalogue_;
    SharpProductMachines machines_;
    SharpPresetRules rules_;
    presets::DirectoryPresetBundle bundle_;
    FontPart font_;
    calcu::typing::TypingPart typing_{typingProduct(), sessions::Hosting::VirtualTime};
    SharpLoaderPart loaders_{typing_};
    BasicProgramKind basicKind_;
    MachineCodeKind codeKind_;
    paper::PaperPart paper_{catalogue_, sharpPrinterPapers()};
    debugger::DebuggerPart debugger_{sharpDebuggerSetup(nullptr, nullptr, /*hasTraceFolder=*/false)};
    SharpControlRules controlRules_{SharpControlRules::Inputs{true, {}}};
    dap::DapPart dap_{dap::DapDeps{&debugger_, &typing_, nullptr, nullptr, nullptr, &bundle_}};
    DapServeAddOn dapAddOn_{dap_};
};

} // namespace

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct() { return std::make_unique<SharpHarnessProduct>(); }

std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct(std::string romFolder) {
    return std::make_unique<SharpHarnessProduct>(std::move(romFolder));
}

} // namespace shared
