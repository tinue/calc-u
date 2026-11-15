// Golden runs of the Sharp cores (first step 2 of docs/walkthroughs/first-steps.md,
// M-8 of docs/migration-plan.md), replayed through the machine interface
// (C1600-1 AC-4, design §8, T10). The .golden files were recorded from the
// code as imported (Calc-U-1600 0.8.1) by a driver that called PC1500Machine
// and PC1600Machine directly, before the cores were changed (the power cycle
// runs in M-26 T0, before the locks went). A run that differs is a finding,
// not a new reference (C1600-1, Technical Requirements).
//
// Each tests/golden-runs/<name>.run is a script; <name>.golden is what the
// core did. CALCU1600_GOLDEN_RECORD=1 writes the .golden of a script that has
// none yet; it never overwrites one.
//
// Script lines (# starts a comment), and what each is on the interface:
//   model PC-1500|PC-1500A|PC-1600     rom A01|A03|A04 (PC-1500) or new|old (PC-1600)
//   attach ce150|ce158|ce1600p         a peripheral of the BuildRequest
//   card <file> [slot 1|2]             a bundled module, a peripheral of the BuildRequest
//   boot                               build(), coldBoot(), the boot run (LegacyCalls::runToPrompt)
//   keys <name> ...                    taps: PC-1500 through the typing engine with the
//                                      PC-1500 profile (LIB-5, M-36; 4 frames down, 4 up),
//                                      the 200 ms in all from the tap's start;
//                                      PC-1600 keyDown, 80 ms, keyUp, 80 ms
//   type <text>                        taps for each character (space = space)
//   press <name> / release <name>      keyDown / keyUp
//   run <seconds>                      advance(tick rate × seconds)
//   reset                              reset()
//   allreset                           coldBoot() and the boot run
//   powercycle [attach <peripheral>]   attach (it waits for the off point), powerCycle(),
//                                      the boot run (C1600-1 §3.3)
// After every line the run records the display as text with its status
// words (displayText of readDisplay()), the sound (takeSound: samples,
// non-zero samples, a checksum), the plotters' paper (c1600.plotter-paper);
// at the end a checksum of the memory (c1600.loader-memory).
//
// Finding (M-26, for the user's ruling): the status words of a record taken
// while the display is off are not compared. The old text readers read the
// status line from its RAM even then (`> reset` in the two *-reset-allreset
// runs: blank rows, `status DEG PRO I`); through the interface a display
// that is off shows no indicator (LIB-1 AC-43), so the run has `status`
// there. Every other line, these runs' included, is compared exactly.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../core/Connector/MemoryCardDefinition.hpp"
#include "../core/Display/DisplayOutputText.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "../core/Machine/SharpBuilder.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpMachine.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "../app/SharpTyping.hpp"
#include "SharpTestSupport.hpp"
#include "typing/TypingEngine.hpp"
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

std::string plotSummary(const std::string& name, const std::vector<AlpsPlotterMechanism::FlatPoint>& pts) {
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
    uint64_t cycles = 0;
    std::string out, error;
    std::vector<size_t> statusWhileOff;   // 0-based lines of `out` (see the header)

    bool is1600() const { return model == "PC-1600"; }

    void advance(double seconds) {
        cycles += m->advance(static_cast<uint64_t>(static_cast<double>(m->descriptor().tickRate) * seconds));
    }

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
        sharp::legacy::runToPrompt(*m);
    }

    void key(const std::string& name, bool down) {
        // An ID the machine lacks was ignored by the old classes too.
        if (down) m->keyDown(KeyId{name}); else m->keyUp(KeyId{name});
    }
    // A tap as 0.8.1 types it: on the PC-1500 (its key queue until M-36) a
    // `key` of the typing engine with the model's profile, 4 frames down and
    // 4 up, the machine advanced in pieces cut where the engine is due, as the
    // machine session does (LIB-3 §3.7); on the PC-1600 a press held 80 ms.
    void tap(const std::string& name) {
        if (is1600()) { key(name, true); advance(0.08); key(name, false); advance(0.08); return; }
        // 0.8.1's queue pressed the key after the first instruction of the
        // 200 ms advance; the engine presses where it is told to.
        const Ticks end = m->tickCount() + static_cast<Ticks>(static_cast<double>(m->descriptor().tickRate) * 0.2);
        cycles += m->advance(1);
        // The PC-1500 profile as 0.8.1's queue typed: without the wait for key
        // readiness of AC-31 (row 1600-8). With it, pc1500a-ce150 differs in
        // two cycle counts and one audio sample count after `type lprint 123`
        // (display, paper and memory the same): a press waited for readiness
        // there. Finding of M-36, LIB-5 qa-report; the core is unchanged.
        static const calcu::typing::TypingProfile asQueued = [] {
            calcu::typing::TypingProfile p = *shared::typingProfile(ModelId{sharp::kPC1500A});   // the PC-1500's is the same
            p.queuedWaitsForReadiness = false;
            return p;
        }();
        calcu::typing::TypingEngine engine;
        engine.setMachine(m.get(), &asQueued, nullptr);
        engine.typeKeys({calcu::typing::Press{KeyId{name}}}, calcu::typing::Origin::Typed);
        for (Ticks now = m->tickCount(); now < end; now = m->tickCount()) {
            if (engine.nextDue(now) <= now) engine.act(*m, now, {});
            Ticks piece = end - now;
            const Ticks due = engine.nextDue(now);
            if (due != calcu::sessions::kNever && due > now) piece = std::min(piece, due - now);
            cycles += m->advance(piece);
        }
        if (engine.nextDue(m->tickCount()) <= m->tickCount()) engine.act(*m, m->tickCount(), {});
    }

    void powerCycle(const std::string& attach) {
        bool cycle = m->readDisplay().on;   // a machine that is off only gets the change, as 0.8.1
        if (!attach.empty()) {
            std::string err;
            const char* id = attach == "ce150" ? sharp::kCE150 : attach == "ce158" ? sharp::kCE158 : sharp::kCE1600P;
            const char* place = attach == "ce158" ? sharp::kInterface : sharp::kPrinter;
            const ChangeResult r = m->attach(busRequest(catalogue, id, place, &err), catalogue);
            if (!err.empty() || r.refused) { error = "attach " + attach + ": " + (r.refused ? r.refused->reason : err); return; }
            cycle = r.follows == AfterChange::PowerCycle;
        }
        m->powerCycle();
        if (cycle) sharp::legacy::runToPrompt(*m);
    }

    void record(const std::string& line) {
        out += "> " + line + "\n";
        if (!m) return;
        LcdText t = displayText(*m);
        out += "  cycles " + std::to_string(cycles) + "\n";
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
        for (const auto& e : m->capabilities(sharp::PlotterPaper::kKind))
            if (auto* paper = capabilityAs<sharp::PlotterPaper>(*m, e.id))
                out += plotSummary(std::get<PeripheralOwner>(e.owner).peripheral.value, paper->points());
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
        if (cmd == "keys") { std::string k; while (in >> k) tap(k); }
        else if (cmd == "type") {
            for (char c : rest()) tap(c == ' ' ? std::string("space") : std::string(1, c));
        }
        else if (cmd == "press" || cmd == "release") { std::string k; in >> k; key(k, cmd == "press"); }
        else if (cmd == "run") { double s = 0; in >> s; advance(s); }
        else if (cmd == "reset") { m->reset(); }
        else if (cmd == "allreset") { m->coldBoot(); sharp::legacy::runToPrompt(*m); }
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
        auto* memory = sharp::firstOf<sharp::LoaderMemory>(*m);
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

TEST(c1600_1_ac4_golden_runs_through_the_interface) {
    const fs::path dir = "tests/golden-runs";
    SKIP_UNLESS(testRomSet() != nullptr, "no full ROM set in roms/");
    const bool recording = std::getenv("CALCU1600_GOLDEN_RECORD") != nullptr;
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
        if (recording && !fs::exists(golden)) { std::ofstream(golden, std::ios::binary) << got; continue; }
        if (!fs::exists(golden)) { std::fprintf(stderr, "FAIL %s: no .golden\n", s.filename().string().c_str()); CHECK(false); continue; }
        const std::string want = readText(golden);
        std::istringstream a(want), b(got);
        std::string la, lb;
        bool same = true;
        for (size_t n = 0;; ++n) {
            const bool ea = !std::getline(a, la), eb = !std::getline(b, lb);
            if (ea && eb) break;
            const bool excused = !ea && !eb && la.rfind("  status", 0) == 0 && lb.rfind("  status", 0) == 0 &&
                                 std::find(skip.begin(), skip.end(), n) != skip.end();
            if (excused) continue;
            if (ea || eb || la != lb) {
                std::fprintf(stderr, "  %s:%zu differs\n    golden: %s\n    run:    %s\n", golden.filename().string().c_str(), n + 1,
                             la.c_str(), lb.c_str());
                same = false;
                break;
            }
        }
        CHECK(same);
    }
}

} // namespace
