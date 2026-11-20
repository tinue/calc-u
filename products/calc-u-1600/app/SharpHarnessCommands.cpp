#include "app/SharpHarnessCommands.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Audio/WavFile.hpp"
#include "Connector/PrintableBytes.hpp"
#include "Debug/Inspect/Inspector.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMediaPlaces.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "MachineCodeFile.hpp"
#include "Tape/TapeDeck.hpp"
#include "app/BasicListing.hpp"
#include "app/SerialScripts.hpp"
#include "app/SharpAppPart.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/SharpLoaderPart.hpp"
#include "app/SharpTopics.hpp"
#include "debugger/RunState.hpp"
#include "files/Files.hpp"
#include "formats/Duration.hpp"
#include "formats/Number.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include "machine/Machine.hpp"
#include "media/Kind.hpp"
#include "media/MediaPart.hpp"
#include "media/MediaPlace.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/Messages.hpp"

namespace cu1600 {

namespace {

using namespace calcu;
namespace fs = std::filesystem;
using harness::CommandContext;
using harness::doneReply;
using harness::refusedReply;
using harness::Reply;


// The file's bytes, or nullopt (it cannot be read).
std::optional<std::vector<std::uint8_t>> readFile(const std::string& path) {
    std::error_code ec;
    if (!fs::is_regular_file(calcu::files::toPath(path), ec)) return std::nullopt;
    return calcu::files::readBytes(calcu::files::toPath(path));
}

std::string hex(std::uint64_t v, int digits) {
    char b[24];
    std::snprintf(b, sizeof b, "%0*llx", digits, static_cast<unsigned long long>(v));
    return b;
}

machine::Machine* machineOf(CommandContext& c) { return c.host.session().context().machine(); }

Reply noMachine() { return refusedReply(sessions::kNoMachine); }
Reply notInThisRun() { return refusedReply("This command needs a run of calcu; it is not offered through attach."); }

// ── The loaders ─────────────────────────────────────────────────────────

// The file and its options: `<file> [<n>] [address=<a>] [length=<l>] [cpu=<name>] [run]`.
struct LoaderArgs {
    std::string file;
    sessions::Value args;
    std::string error;
};
LoaderArgs loaderArgs(const std::string& argText, bool codeOptions) {
    LoaderArgs out;
    const std::vector<std::string> w = harness::words(argText);
    if (w.empty()) {
        out.error = codeOptions ? "usage: c1600 load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]"
                                : "usage: c1600 load-basic <file> [<n>]";
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

Reply fileKindCmd(CommandContext& c) {
    const std::vector<std::string> w = harness::words(c.line.argText);
    if (w.size() != 1) return refusedReply("usage: c1600 file-kind <file>");
    const auto bytes = readFile(w[0]);
    if (!bytes) return refusedReply("The file " + w[0] + " cannot be read.");
    const sessions::ReplyMessage r =
        c.host.ask(sessions::Request{requests::kFileKind, sessions::Value::object({{"bytes", *bytes}})});
    if (!r.done) return refusedReply(r.reason);
    const auto* k = sessions::payloadAs<FileKindReply>(r.result);
    if (!k) return doneReply();
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
    return doneReply(std::move(v), std::move(lines));
}

Reply loadBasicCmd(CommandContext& c) {
    LoaderArgs a = loaderArgs(c.line.argText, false);
    if (!a.error.empty()) return refusedReply(a.error);
    return harness::fromSessionReply(c.host.command(sessions::Command{commands::kLoadBasic, std::move(a.args)}));
}

Reply typeBasicCmd(CommandContext& c) {
    const std::vector<std::string> w = harness::words(c.line.argText);
    if (w.size() != 1) return refusedReply("usage: c1600 type-basic <file>");
    const auto bytes = readFile(w[0]);
    if (!bytes) return refusedReply("The file " + w[0] + " cannot be read.");
    return harness::fromSessionReply(c.host.command(
        sessions::Command{commands::kTypeBasic, sessions::Value::object({{"bytes", *bytes}, {"name", w[0]}})}));
}

Reply loadCodeCmd(CommandContext& c) {
    LoaderArgs a = loaderArgs(c.line.argText, true);
    if (!a.error.empty()) return refusedReply(a.error);
    const sessions::ReplyMessage r = c.host.command(sessions::Command{commands::kLoadCode, std::move(a.args)});
    if (!r.done) return refusedReply(r.reason);
    const auto* l = sessions::payloadAs<CodeLoadedReply>(r.result);
    if (!l) return doneReply();
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
    return doneReply(std::move(v), std::move(lines));
}

// ── list-basic ──────────────────────────────────────────────────────────

Reply listBasicCmd(CommandContext& c) {
    if (!c.line.argText.empty()) return refusedReply("usage: c1600 list-basic");
    machine::Machine* m = machineOf(c);
    if (!m) return noMachine();
    const sharp::LoaderMemory* mem = calcu::machine::first<sharp::LoaderMemory>(*m);
    if (!mem) return refusedReply("The machine has no BASIC memory.");
    BasicListingReply l = basicListing(*mem);
    if (!l.problem.empty()) return refusedReply(l.problem);
    harness::Value arr = harness::Value::array();
    for (const std::string& line : l.lines) arr.push(line);
    return doneReply(std::move(arr), std::move(l.lines));
}

// ── inspect ─────────────────────────────────────────────────────────────

constexpr inspect::View kViews[] = {
    inspect::View::Pointers,      inspect::View::Inventory,     inspect::View::Z80View,     inspect::View::LhView,
    inspect::View::BasicArea,     inspect::View::ProgramAreas,  inspect::View::DiskAreas,   inspect::View::PhysicalRam,
    inspect::View::DumpBasicArea, inspect::View::DumpProgramS1, inspect::View::DumpProgramS2, inspect::View::DumpDiskS1,
    inspect::View::DumpDiskS2,    inspect::View::DumpMlArea,
};

Reply inspectCmd(CommandContext& c) {
    const std::vector<std::string> w = harness::words(c.line.argText);
    if (w.size() != 1) return refusedReply("usage: c1600 inspect <view>");
    inspect::View view;
    if (!inspect::viewFromName(w[0], &view)) {
        std::string names;
        for (inspect::View x : kViews) names += std::string(names.empty() ? "" : ", ") + inspect::viewName(x);
        return refusedReply("No view " + w[0] + ". The views: " + names + ".");
    }
    machine::Machine* m = machineOf(c);
    if (!m) return noMachine();
    const sharp::InspectorViews* k = calcu::machine::first<sharp::InspectorViews>(*m);
    if (!k) return refusedReply("The machine has no INSPECTOR.");
    std::vector<std::string> lines = k->view(view);
    harness::Value arr = harness::Value::array();
    for (const std::string& l : lines) arr.push(l);
    return doneReply(std::move(arr), std::move(lines));
}

// ── cassette ────────────────────────────────────────────────────────────

// The recording of `cassette record <path>`: the tape's sound alone, as a WAV
// at the path; no cassette definition.
class RecordingCodec final : public media::MediumCodec {
public:
    media::ReadOutcome read(const std::string&, const Companion&) const override {
        media::ReadOutcome r;
        r.problem = "A recording is only written.";
        return r;
    }
    std::vector<media::FileOut> write(const Write& w, std::string& problem) const override {
        const auto* tape = dynamic_cast<const sharp::TapeContent*>(w.content.get());
        if (!tape) {
            problem = "It is not a cassette.";
            return {};
        }
        return {media::FileOut{"", std::string(tape->wav.begin(), tape->wav.end())}};
    }
};

const media::KindDecl& recordingKind() {
    static const RecordingCodec codec;
    static const media::KindDecl k = [] {
        media::KindDecl d;
        d.id = "cassette-recording";
        d.label = "cassette recording";
        d.suffix = ".wav";
        d.codec = &codec;
        return d;
    }();
    return k;
}

const char* const kNoCassette =
    "The machine has no cassette: a CE-150 on the PC-1500 or PC-1500A, or a CE-1600P on the PC-1600, brings one.";

bool hasCassette(machine::Machine& m) {
    for (const auto& e : m.capabilities(media::MediaPlace::kKind))
        if (auto* p = machine::capabilityAs<media::MediaPlace>(m, e.id); p && p->id().value == "cassette-recorder") return true;
    return false;
}

std::string seconds(double s) {
    char b[40];
    std::snprintf(b, sizeof b, "%.9f", s);
    return b;
}

Reply cassetteCmd(CommandContext& c, const SharpHarnessParts& parts) {
    const std::vector<std::string> w = harness::words(c.line.argText);
    machine::Machine* m = machineOf(c);
    if (!m) return noMachine();
    if (!w.empty() && w[0] == "status" && w.size() == 1) {
        const sharp::TapeDeckAccess* deck = calcu::machine::first<sharp::TapeDeckAccess>(*m);
        if (!deck) return refusedReply(kNoCassette);
        const TapeDeck::Status s = deck->status();
        const char* mode = s.mode == TapeDeck::Mode::Play ? "playing" : s.mode == TapeDeck::Mode::Record ? "recording" : "empty";
        std::vector<std::string> lines;
        if (s.mode == TapeDeck::Mode::Play) lines.push_back("cassette: playing " + seconds(s.position) + " of " + seconds(s.length));
        else if (s.mode == TapeDeck::Mode::Record) lines.push_back("cassette: recording " + seconds(s.length));
        else lines.push_back("cassette: empty");
        lines.push_back(std::string("motor: ") + (s.motor ? "on" : "off"));
        harness::Value v = harness::Value::object();
        v.set("cassette", mode);
        v.set("position", seconds(s.position));
        v.set("length", seconds(s.length));
        v.set("motor", s.motor);
        return doneReply(std::move(v), std::move(lines));
    }
    if (w.size() != 2 || w[0] != "record") return refusedReply("usage: c1600 cassette record <file.wav> | cassette status");
    if (!hasCassette(*m)) return refusedReply(kNoCassette);
    SharpAppPart* app = parts.app ? parts.app() : nullptr;
    if (!app) return notInThisRun();
    std::error_code ec;
    const fs::path target = fs::absolute(calcu::files::toPath(w[1]), ec);
    media::Binding b;
    b.kind = &recordingKind();
    b.name = calcu::files::fromPathNative(target.filename());
    b.origin = media::Origin::Saved;
    b.isTemplate = false;   // an instance: written each time the motor stops and at eject
    b.path = calcu::files::fromPathNative(target);
    // A tape in the cassette is written as the media part writes it and
    // taken out first; a new, empty tape records.
    sessions::SessionContext& ctx = c.host.session().context();
    app->mediaPart().eject(ctx, "cassette");
    const media::Content blank = std::make_shared<sharp::TapeContent>(wavMono16Bytes({}, TapeDeck::kRecordSampleRate));
    if (std::optional<std::string> refused = app->mediaPart().insert(ctx, "cassette", std::move(b), blank, 0)) return refusedReply(*refused);
    return doneReply();
}

// ── serial ──────────────────────────────────────────────────────────────

const char* linkFile(const std::string& port) {
    return port == "ce158" ? "calcu1600-ce158.serial" : "calcu1600-rs232c.serial";
}

void addText(std::vector<std::string>& lines, const std::string& title, const std::vector<std::uint8_t>& bytes) {
    lines.push_back(title + ": " + std::to_string(bytes.size()) + " bytes");
    const std::string text = printableBytes(bytes.data(), bytes.size());
    if (text.empty()) return;
    std::size_t from = 0;
    for (;;) {   // one reply line per line of the text (a line end stands as it is)
        const std::size_t nl = text.find('\n', from);
        lines.push_back("| " + text.substr(from, nl == std::string::npos ? std::string::npos : nl - from));
        if (nl == std::string::npos || nl + 1 == text.size()) break;
        from = nl + 1;
    }
}

Reply serialCmd(CommandContext& c, const SharpHarnessParts& parts) {
    std::vector<std::string> w = harness::words(c.line.argText);
    const char* usage = "usage: c1600 serial ce158|rs232c feed <file> [hold=<n>] | capture <file> | link | report";
    if (w.size() < 2 || (w[0] != "ce158" && w[0] != "rs232c")) return refusedReply(usage);
    SerialScriptsPart* serial = parts.serial ? parts.serial() : nullptr;
    SharpAppPart* app = parts.app ? parts.app() : nullptr;
    if (!serial || !app || !serial->attached()) return notInThisRun();
    const std::string& port = w[0];
    const std::string& what = w[1];
    // The port of the running machine, or none; nothing waits for a later machine.
    if (!serial->port(port))
        return refusedReply(port == "ce158" ? "The machine has no CE-158." : "The machine has no RS-232C port: only the PC-1600 has one.");
    const bool linked = app->serialLinked(linkFile(port));
    if (what == "feed") {
        if (w.size() < 3 || w.size() > 4) return refusedReply(usage);
        std::uint64_t hold = 0;
        if (w.size() == 4) {
            const std::string& h = w[3];
            if (h.rfind("hold=", 0) != 0 || h.size() == 5 || h.find_first_not_of("0123456789", 5) != std::string::npos)
                return refusedReply("hold= takes a number of character times.");
            hold = std::stoull(h.substr(5));
        }
        if (linked) return refusedReply("The port has a host link; feed and capture are refused for it.");
        auto bytes = readFile(w[2]);
        if (!bytes) return refusedReply("The file " + w[2] + " cannot be read.");
        serial->feed(port, std::move(*bytes), hold);
        return doneReply();
    }
    if (what == "capture") {
        if (w.size() != 3) return refusedReply(usage);
        if (linked) return refusedReply("The port has a host link; feed and capture are refused for it.");
        if (std::optional<std::string> refused = serial->capture(port, w[2])) return refusedReply(*refused);
        return doneReply();
    }
    if (w.size() != 2) return refusedReply(usage);
    if (what == "link") {
        const std::string path = app->requestSerialLink(linkFile(port));
        if (path.empty()) return refusedReply("This host has no pseudo-terminals: the port gets no host link.");
        harness::Value v = harness::Value::object();
        v.set("path", path);
        return doneReply(std::move(v), {"path: " + path});
    }
    if (what == "report") {
        const SerialScriptsPart::Report r = serial->report(port);
        std::vector<std::string> lines;
        lines.push_back("speed: " + (r.baud ? std::to_string(r.baud) + " baud" : std::string("not set")));
        lines.push_back("bits: " + (r.bitsPerChar ? std::to_string(*r.bitsPerChar) : std::string("not set")));
        lines.push_back(std::string("used: ") + (r.used ? "yes" : "no"));
        lines.push_back(std::string("host link: ") + (linked ? "yes" : "no"));
        lines.push_back("received: " + std::to_string(r.taken) + " of " + std::to_string(r.fed) + " bytes fed");
        addText(lines, "sent", r.sent);
        harness::Value v = harness::Value::object();
        v.set("baud", static_cast<unsigned>(r.baud));
        v.set("used", r.used);
        v.set("taken", static_cast<unsigned long long>(r.taken));
        v.set("fed", static_cast<unsigned long long>(r.fed));
        v.set("sent", printableBytes(r.sent.data(), r.sent.size()));
        if (port == "ce158") {
            addText(lines, "printer", r.printer);
            v.set("printer", printableBytes(r.printer.data(), r.printer.size()));
        }
        return doneReply(std::move(v), std::move(lines));
    }
    return refusedReply(usage);
}

// ── host-drive ──────────────────────────────────────────────────────────

Reply hostDriveCmd(CommandContext& c) {
    const std::string arg = c.line.argText;
    if (arg.empty()) return refusedReply("usage: c1600 host-drive <folder> | off");
    machine::Machine* m = machineOf(c);
    if (!m) return noMachine();
    if (m->descriptor().model.value != sharp::kPC1600) return refusedReply("The host drive is a PC-1600 peripheral.");
    std::string dir;
    if (arg != "off") {
        std::error_code ec;
        if (!fs::is_directory(calcu::files::toPath(arg), ec)) return refusedReply("The path " + arg + " is no folder.");
        dir = calcu::files::fromPathNative(fs::absolute(calcu::files::toPath(arg), ec));
    }
    return harness::fromSessionReply(c.host.command(sessions::Command{commands::kMountHostDrive, sessions::Value::object({{"dir", dir}})}));
}

// ── call ────────────────────────────────────────────────────────────────

std::vector<std::string> registerLines(CommandContext& c, const std::string& target, harness::Value& out) {
    std::vector<std::string> lines;
    const sessions::ReplyMessage r =
        c.host.ask(sessions::Request{debugger::kRegistersRequest, sessions::Value::object({{"target", target}})});
    if (!r.done || !r.result) return lines;
    const sessions::Value v = r.result->toValue();
    const sessions::Value* regs = v.find("registers");
    if (!regs || !regs->elements()) return lines;
    for (const auto& reg : *regs->elements()) {
        const sessions::Value* n = reg.find("name");
        const sessions::Value* x = reg.find("value");
        const sessions::Value* b = reg.find("bits");
        if (!n || !n->string()) continue;
        const int bits = b && b->integer() ? static_cast<int>(*b->integer()) : 16;
        const std::uint64_t value = x && x->integer() ? static_cast<std::uint64_t>(*x->integer()) : 0;
        const std::string h = "0x" + hex(value, std::max(1, (bits + 3) / 4));
        lines.push_back(*n->string() + " = " + h);
        out.set(*n->string(), h);
    }
    return lines;
}

Reply callCmd(CommandContext& c) {
    const std::vector<std::string> w = harness::words(c.line.argText);
    const char* usage = "usage: c1600 call <target> <address> [timeout=<duration>]";
    if (w.size() < 2 || w.size() > 3) return refusedReply(usage);
    machine::Machine* m = machineOf(c);
    if (!m) return noMachine();
    std::int64_t timeoutUs = 10'000'000;   // 10 s of emulated time
    if (w.size() == 3) {
        if (w[2].rfind("timeout=", 0) != 0) return refusedReply(usage);
        std::string error;
        const auto us = formats::parseDuration(w[2].substr(8), &error);
        if (!us) return refusedReply("timeout= takes a duration: " + error);
        timeoutUs = *us;
    }
    const auto address = formats::parseNumber(w[1]);
    if (!address || *address < 0 || *address > 0xFFFF) return refusedReply("The address " + w[1] + " is no address of 16 bits.");
    if (!c.host.session().machineSession().paused()) return refusedReply("The machine is not paused (pause first).");
    // The target as the debugger names it, any case.
    std::string target;
    for (const machine::DebugTargetId& t : m->debugTargets()) {
        std::string a = t.value, b = w[0];
        std::transform(a.begin(), a.end(), a.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        std::transform(b.begin(), b.end(), b.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        if (a == b) target = t.value;
    }
    sharp::RoutineCall* call = calcu::machine::first<sharp::RoutineCall>(*m);
    if (target.empty() || !call || !call->call(target, static_cast<std::uint16_t>(*address)))
        return refusedReply("The machine has no debug target " + w[0] + ".");
    const machine::TickRate rate = m->descriptor().tickRate;
    const machine::Ticks before = m->tickCount();
    const machine::Ticks within = harness::ticksFor(timeoutUs, rate);
    // The debugger's step-out: until the routine the target is in has returned.
    sessions::Value::Object args{{"level", "cpu"}, {"count", 1}, {"kind", "out"}, {"target", target}};
    const sessions::ReplyMessage r =
        c.host.command(sessions::Command{debugger::kStepCommand, sessions::Value(std::move(args))}, "timeout", within);
    bool timedOut = false;
    if (!r.done) {
        if (r.reason != "timeout") return refusedReply(r.reason);
        timedOut = true;   // past the timeout: paused, and the reply says so
        c.host.command(sessions::Command{debugger::kPauseCommand, sessions::Value::object({{"level", "cpu"}})});
    }
    machine::Machine* after = machineOf(c);
    const machine::Ticks ran = after ? after->tickCount() - before : 0;
    harness::Value v = harness::Value::object();
    harness::Value regs = harness::Value::object();
    std::vector<std::string> lines{std::string("returned: ") + (timedOut ? "no, timeout" : "yes")};
    for (std::string& l : registerLines(c, target, regs)) lines.push_back(std::move(l));
    const harness::Decimal s = harness::secondsOf(ran, rate);
    lines.push_back("ticks: " + std::to_string(ran));
    lines.push_back("seconds: " + harness::toText(s));
    v.set("returned", !timedOut);
    v.set("registers", std::move(regs));
    v.set("ticks", static_cast<unsigned long long>(ran));
    v.set("seconds", s);
    return doneReply(std::move(v), std::move(lines));
}

} // namespace

void addSharpHarnessCommands(harness::CommandRegistry& r, SharpHarnessParts parts) {
    r.addProduct("c1600", "file-kind", fileKindCmd, "file-kind <file>  what the file is");
    r.addProduct("c1600", "load-basic", loadBasicCmd, "load-basic <file> [<n>]  load a BASIC program");
    r.addProduct("c1600", "type-basic", typeBasicCmd, "type-basic <file>  type a BASIC listing in (PRO mode)");
    r.addProduct("c1600", "load-code", loadCodeCmd,
                 "load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]  load machine code");
    r.addProduct("c1600", "list-basic", listBasicCmd, "list-basic  the BASIC program as listing lines");
    r.addProduct("c1600", "inspect", inspectCmd, "inspect <view>  an INSPECTOR view as text");
    r.addProduct("c1600", "cassette", [parts](CommandContext& c) { return cassetteCmd(c, parts); },
                 "cassette record <file.wav> | cassette status  record the cassette into a WAV; the tape counter");
    r.addProduct("c1600", "serial", [parts](CommandContext& c) { return serialCmd(c, parts); },
                 "serial ce158|rs232c feed <file> [hold=<n>] | capture <file> | link | report  a serial port's peer");
    r.addProduct("c1600", "host-drive", hostDriveCmd, "host-drive <folder> | off  the PC-1600's host drive");
    r.addProduct("c1600", "call", callCmd,
                 "call <target> <address> [timeout=<duration>]  run a routine until it returns (machine paused)");
}

} // namespace cu1600
