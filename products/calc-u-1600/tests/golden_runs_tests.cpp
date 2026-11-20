// Golden runs of the Sharp machines, replayed through the machine
// interface with today's code paths: the boot to key readiness (as the
// app's start) and keys through the library's typing engine with the
// model's typing profile (tests/support/Typing.hpp). A run that differs is
// a finding, not a new reference.
//
// Each tests/golden-runs/<name>.run is a script; <name>.golden is what the
// machine did. CALCU1600_GOLDEN_RECORD=<folder> writes the recording of
// every script into that folder (a draft beside the references; it never
// writes into tests/golden-runs/).
//
// Script lines (# starts a comment), and what each is on the interface:
//   model PC-1500|PC-1500A|PC-1600     rom A01|A03|A04 (PC-1500) or new|old (PC-1600)
//   attach ce150|ce158|ce1600p         a peripheral of the BuildRequest
//   card <file> [slot 1|2]             a bundled module, a peripheral of the BuildRequest
//   boot                               build(), coldBoot(), on to key readiness
//   keys <name> ...                    the keys in turn through the typing engine
//   type <text>                        the same, a key per character (space = space)
//   press <name> / release <name>      keyDown / keyUp
//   run <seconds>                      advance(tick rate × seconds)
//   reset                              reset()
//   allreset                           coldBoot(), on to key readiness
//   powercycle [attach <peripheral>]   attach (it waits for the off point), powerCycle(),
//                                      on to key readiness
// After every line the run records the display as text with its status
// words (displayText of readDisplay()), the sound (takeSound: samples,
// non-zero samples, a checksum), the plotters' paper (paper-output, read
// back into raw steps: sharp::rawPlotPoints);
// at the end a checksum of the memory (c1600.loader-memory).
//
// The status words of a record taken while the display is off are not
// compared: through the interface a display that is off shows no indicator.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../core/Connector/MemoryCardDefinition.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "support/RawPlotPoints.hpp"
#include "../core/Display/DisplayOutputText.hpp"
#include "../core/Machine/SharpBuilder.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpMachine.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "SharpTestSupport.hpp"
#include "TestCards.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
using namespace calcu::machine;

struct Fnv {
    uint64_t h = 1469598103934665603ull;
    void add(const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 1099511628211ull; }
    }
    std::string hex() const { char s[17]; std::snprintf(s, sizeof s, "%016llx", static_cast<unsigned long long>(h)); return s; }
};

std::string readText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string plotSummary(const std::string& name, const std::vector<sharp::RawPlotPoint>& pts) {
    Fnv f;
    for (const auto& p : pts) { f.add(&p.x, 4); f.add(&p.y, 4); f.add(&p.color, 1); f.add(&p.newStroke, 1); }
    return "  paper " + name + " points=" + std::to_string(pts.size()) + " sum=" + f.hex() + "\n";
}

PeripheralRequest busRequest(const Catalogue& cat, const char* id, const char* place, std::string* error) {
    PeripheralRequest r{PlacePath{{PlaceName{place}}}, PeripheralId{id}, std::nullopt, {}, nullptr, {}};
    if (!sharptest::peripheralImages(cat, r.peripheral, std::nullopt, &r.roms, error)) *error = id + (": " + *error);
    return r;
}

struct Run {
    std::string model = "PC-1500A";
    std::string rom;  // empty: the default of the model
    bool ce150 = false, ce158 = false, ce1600p = false;
    std::vector<std::pair<std::string, int>> cards;
    Catalogue catalogue = sharp::sharpCatalogue();
    std::unique_ptr<Machine> m;
    std::string out, error;
    std::vector<size_t> statusWhileOff;
    // A paper per plotter, connected to its paper output when first seen
    // (it reads what the plotter kept until then); a new card, a new paper.
    std::map<std::string, std::unique_ptr<calcu::paper::Paper>> papers;   // 0-based lines of `out` (see the header)

    bool is1600() const { return model == "PC-1600"; }

    void advance(double seconds) { m->advance(sharptest::secondsTicks(*m, seconds)); }

    std::optional<VariantId> variant() const {
        if (rom.empty()) return std::nullopt;
        if (rom == "new") return VariantId{"New"};
        if (rom == "old") return VariantId{"Old"};
        return VariantId{rom};
    }

    void boot() {
        BuildRequest req;
        req.model = ModelId{model};
        req.variant = variant();
        req.soundRate = sharp::kSoundRate;
        std::string err;
        if (!sharptest::modelImages(catalogue, req.model, req.variant, &req.roms, &err)) { error = err; return; }
        // The order of the recorder: CE-150, CE-158, CE-1600P, then the modules.
        if (ce150) req.peripherals.push_back(busRequest(catalogue, sharp::kCE150, sharp::kPrinter, &err));
        if (ce158) req.peripherals.push_back(busRequest(catalogue, sharp::kCE158, sharp::kInterface, &err));
        if (ce1600p) req.peripherals.push_back(busRequest(catalogue, sharp::kCE1600P, sharp::kPrinter, &err));
        if (!err.empty()) { error = err; return; }
        for (auto& [file, slot] : cards) {
            const CardHost host = is1600() ? (slot == 2 ? CardHost::PC1600Slot2 : CardHost::PC1600Slot1)
                                           : (model == "PC-1500" ? CardHost::PC1500 : CardHost::PC1500A);
            auto c = bundledCard(file, host);
            if (!c) { error = "card " + file; return; }
            catalogue.addPeripheral(sharp::moduleDescriptor(c->definition()));
            req.peripherals.push_back({PlacePath{{PlaceName{slot == 2 ? sharp::kSlot2 : sharp::kSlot1}}},
                                       PeripheralId{c->definition().moduleName}, std::nullopt, {},
                                       std::make_shared<sharp::ModuleCard>(std::move(c)), {}});
        }
        sharp::SharpMachineBuilder builder;
        BuildResult built = build(catalogue, builder, req);
        if (!built.machine) { error = built.refusal; return; }
        m = std::move(built.machine);
        m->coldBoot();
        sharptest::runUntilKeyReady(*m);
    }

    void key(const std::string& name, bool down) {
        if (down) m->keyDown(KeyId{name}); else m->keyUp(KeyId{name});   // an ID the machine lacks is ignored
    }
    void keys(const std::vector<std::string>& ids) {
        if (!sharptest::typeKeys(*m, ids)) error = "keys not carried out";
    }

    void powerCycle(const std::string& attach) {
        bool cycle = m->readDisplay().on;   // a machine that is off only gets the change
        if (!attach.empty()) {
            std::string err;
            const char* id = attach == "ce150" ? sharp::kCE150 : attach == "ce158" ? sharp::kCE158 : sharp::kCE1600P;
            const char* place = attach == "ce158" ? sharp::kInterface : sharp::kPrinter;
            const ChangeResult r = m->attach(busRequest(catalogue, id, place, &err), catalogue);
            if (!err.empty() || r.refused) { error = "attach " + attach + ": " + (r.refused ? r.refused->reason : err); return; }
            cycle = r.follows == AfterChange::PowerCycle;
        }
        m->powerCycle();
        if (cycle) sharptest::runUntilKeyReady(*m);
    }

    void record(const std::string& line) {
        out += "> " + line + "\n";
        if (!m) return;
        LcdText t = displayText(*m);
        out += "  cycles " + std::to_string(m->tickCount()) + "\n";
        for (const auto& r : t.rows) out += "  lcd |" + r + "|\n";
        if (!t.poweredOn) statusWhileOff.push_back(static_cast<size_t>(std::count(out.begin(), out.end(), '\n')));
        out += "  status";
        for (const auto& s : t.status) out += " " + s;
        out += "\n";
        std::vector<int16_t> audio;
        int16_t chunk[4096];
        size_t n;
        while ((n = m->takeSound(chunk)) > 0) audio.insert(audio.end(), chunk, chunk + n);
        size_t loud = 0;
        for (int16_t s : audio) if (s != 0) ++loud;
        Fnv f;
        f.add(audio.data(), audio.size() * sizeof(int16_t));
        out += "  audio samples=" + std::to_string(audio.size()) + " nonzero=" + std::to_string(loud) + " sum=" + f.hex() + "\n";
        for (const auto& e : m->capabilities(calcu::paper::PaperOutput::kKind))
            if (auto* output = capabilityAs<calcu::paper::PaperOutput>(*m, e.id)) {
                const std::string name = std::get<PeripheralOwner>(e.owner).peripheral.value;
                std::unique_ptr<calcu::paper::Paper>& paper = papers[name];
                if (!paper || output->connected() != paper.get()) {
                    if (paper) paper->release();
                    paper = std::make_unique<calcu::paper::Paper>(output->description());
                    paper->bind(output);
                }
                out += plotSummary(name, sharp::rawPlotPoints(*paper));
            }
    }

    void command(const std::string& line) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        auto rest = [&] { std::string a; std::getline(in, a); size_t p = a.find_first_not_of(' '); return p == std::string::npos ? std::string() : a.substr(p); };
        if (cmd == "model") { model = rest(); return; }
        if (cmd == "rom") { rom = rest(); return; }
        if (cmd == "attach") {
            std::string p = rest();
            if (p == "ce150") ce150 = true; else if (p == "ce158") ce158 = true; else if (p == "ce1600p") ce1600p = true;
            else error = "unknown peripheral " + p;
            return;
        }
        if (cmd == "card") {
            std::string file, word; int slot = 1;
            in >> file >> word >> slot;
            cards.emplace_back(file, slot);
            return;
        }
        if (cmd == "boot") { boot(); if (error.empty()) record(line); return; }
        if (!m) { error = "boot first"; return; }
        if (cmd == "keys") {
            std::vector<std::string> ids;
            std::string k;
            while (in >> k) ids.push_back(k);
            keys(ids);
        } else if (cmd == "type") {
            std::vector<std::string> ids;
            for (char c : rest()) ids.push_back(c == ' ' ? std::string("space") : std::string(1, c));
            keys(ids);
        }
        else if (cmd == "press" || cmd == "release") { std::string k; in >> k; key(k, cmd == "press"); }
        else if (cmd == "run") { double s = 0; in >> s; advance(s); }
        else if (cmd == "reset") { m->reset(); }
        else if (cmd == "allreset") { m->coldBoot(); sharptest::runUntilKeyReady(*m); }
        else if (cmd == "powercycle") {
            std::string word, p;
            in >> word >> p;
            if (!word.empty() && word != "attach") { error = "powercycle [attach <peripheral>]"; return; }
            powerCycle(p);
            if (!error.empty()) return;
        }
        else { error = "unknown command " + cmd; return; }
        record(line);
    }

    std::string finish() {
        if (!m) return out;
        auto* memory = calcu::machine::first<sharp::LoaderMemory>(*m);
        Fnv f;
        if (is1600()) {
            const std::vector<uint8_t> ram = memory->internalRam();
            f.add(ram.data(), ram.size());
            out += "= internal ram sum=" + f.hex() + "\n";
            Fnv z;
            for (uint32_t a = 0; a < 0x10000; ++a) { uint8_t b = memory->peek(static_cast<uint16_t>(a)); z.add(&b, 1); }
            out += "= z80 view sum=" + z.hex() + "\n";
        } else {
            for (uint32_t a = 0; a < 0x10000; ++a) { uint8_t b = memory->peek(static_cast<uint16_t>(a)); f.add(&b, 1); }
            out += "= me0 sum=" + f.hex() + "\n";
        }
        return out;
    }
};

std::string play(const std::string& script, std::string* error, std::vector<size_t>* statusWhileOff) {
    Run run;
    std::istringstream in(script);
    std::string line;
    while (std::getline(in, line)) {
        if (auto h = line.find(" #"); h != std::string::npos) line.erase(h);
        if (!line.empty() && line[0] == '#') continue;
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r')) line.pop_back();
        if (line.empty()) continue;
        run.command(line);
        if (!run.error.empty()) { *error = run.error; return {}; }
    }
    *statusWhileOff = run.statusWhileOff;
    return run.finish();
}

TEST(golden_runs_through_the_interface) {
    const fs::path dir = "tests/golden-runs";
    SKIP_UNLESS(testRomSet() != nullptr, "no full ROM set in roms/");
    const char* draft = std::getenv("CALCU1600_GOLDEN_RECORD");
    if (draft) fs::create_directories(draft);
    std::vector<fs::path> scripts;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".run") scripts.push_back(e.path());
    std::sort(scripts.begin(), scripts.end());
    CHECK(!scripts.empty());
    for (const auto& s : scripts) {
        std::string error;
        std::vector<size_t> skip;
        std::string got = play(readText(s), &error, &skip);
        if (!error.empty()) { std::fprintf(stderr, "FAIL %s: %s\n", s.filename().string().c_str(), error.c_str()); CHECK(false); continue; }
        fs::path golden = s;
        golden.replace_extension(".golden");
        if (draft) std::ofstream(fs::path(draft) / golden.filename(), std::ios::binary) << got;
        if (!fs::exists(golden)) { std::fprintf(stderr, "FAIL %s: no .golden\n", s.filename().string().c_str()); CHECK(false); continue; }
        const std::string want = readText(golden);
        std::istringstream a(want), b(got);
        std::string la, lb;
        for (size_t n = 0;; ++n) {
            const bool ea = !std::getline(a, la), eb = !std::getline(b, lb);
            if (ea && eb) break;
            const bool excused = !ea && !eb && la.rfind("  status", 0) == 0 && lb.rfind("  status", 0) == 0 &&
                                 std::find(skip.begin(), skip.end(), n) != skip.end();
            if (excused) continue;
            if (ea || eb || la != lb) {
                std::fprintf(stderr, "  %s:%zu differs\n    golden: %s\n    run:    %s\n", golden.filename().string().c_str(),
                             n + 1, la.c_str(), lb.c_str());
                CHECK(false);
                break;
            }
        }
    }
}

} // namespace
