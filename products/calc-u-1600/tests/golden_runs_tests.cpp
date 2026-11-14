// Golden runs of the Sharp cores (first step 2 of docs/walkthroughs/first-steps.md,
// M-8 of docs/migration-plan.md): scripts of calls on PC1500Machine and
// PC1600Machine, recorded from the code as imported (Calc-U-1600 0.8.1),
// before the cores are changed. A run that differs later is a finding, not a
// new reference (C1600-1, Technical Requirements).
//
// Each tests/golden-runs/<name>.run is a script; <name>.golden is what the
// core did. CALCU1600_GOLDEN_RECORD=1 rewrites the .golden files (only when a
// reference is to be recorded, never to make a failing run pass).
//
// Script lines (# starts a comment):
//   model PC-1500|PC-1500A|PC-1600     rom A01|A03|A04 (PC-1500) or new|old (PC-1600)
//   attach ce150|ce158|ce1600p         a peripheral, before boot
//   card <file> [slot 1|2]             a bundled module definition, before boot
//   boot                               ROMs loaded, ALL RESET, the boot run to the prompt
//   keys <name> ...                    taps (key names of the keyboards): on the PC-1500
//                                      through its key queue, then 200 ms; on the PC-1600
//                                      80 ms down, 80 ms up
//   type <text>                        taps for each character (space = space)
//   press <name> / release <name>      one key down or up
//   run <seconds>                      advance emulated time
//   reset                              the machine's reset()
//   allreset                           the machine's allReset() and the boot run
// After every line the run records the display as text with its status
// symbols, the buzzer (samples, edges, a checksum), the plotters' paper
// (points and a checksum); at the end a checksum of the memory.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../core/PC1500/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500LcdText.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600LcdText.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/Resources/BundledRomCatalog.hpp"
#include "TestCards.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;

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

std::string plotSummary(const char* name, const std::vector<AlpsPlotterMechanism::FlatPoint>& pts) {
    Fnv f;
    for (const auto& p : pts) { f.add(&p.x, 4); f.add(&p.y, 4); f.add(&p.color, 1); f.add(&p.newStroke, 1); }
    return std::string("  paper ") + name + " points=" + std::to_string(pts.size()) + " sum=" + f.hex() + "\n";
}

struct Run {
    std::string model = "PC-1500A";
    std::string rom;  // empty: the default of the model
    bool ce150 = false, ce158 = false, ce1600p = false;
    std::vector<std::pair<std::string, int>> cards;
    std::unique_ptr<PC1500Machine> m15;
    std::unique_ptr<PC1600Machine> m16;
    uint64_t cycles = 0;
    std::string out, error;

    bool is1600() const { return model == "PC-1600"; }
    double hz() const { return is1600() ? PC1600Machine::kTStateHz : PC1500Machine::kCpuHz; }

    void advance(double seconds) {
        auto n = static_cast<uint64_t>(hz() * seconds);
        cycles += is1600() ? m16->runCycles(n) : m15->runCycles(n);
    }

    void boot() {
        std::string err;
        const std::vector<std::string> dirs = {"roms"};
        if (is1600()) {
            m16 = std::make_unique<PC1600Machine>();
            if (!BundledRoms::loadPC1600RomSet(*m16, dirs, rom.empty() ? "new" : rom, &err)) { error = err; return; }
            if (ce150 && !BundledRoms::attachCE150(*m16, dirs, &err)) { error = err; return; }
            if (ce158 && !BundledRoms::attachCE158(*m16, dirs, &err)) { error = err; return; }
            if (ce1600p && !BundledRoms::attachCE1600P(*m16, dirs, "new", &err)) { error = err; return; }
            for (auto& [file, slot] : cards) {
                auto c = bundledCard(file, slot == 2 ? CardHost::PC1600Slot2 : CardHost::PC1600Slot1);
                if (!c) { error = "card " + file; return; }
                if (slot == 2) m16->attachSlot2Card(std::move(c)); else m16->attachSlot1Card(std::move(c));
            }
            m16->allReset();
            runBootToPrompt(*m16);
        } else {
            m15 = std::make_unique<PC1500Machine>(model == "PC-1500" ? PC1500Variant::PC1500 : PC1500Variant::PC1500A);
            if (!BundledRoms::loadPC1500Rom(*m15, rom.empty() ? "A04" : rom, dirs, &err)) { error = err; return; }
            if (ce150 && !BundledRoms::attachCE150(*m15, dirs, &err)) { error = err; return; }
            if (ce158 && !BundledRoms::attachCE158(*m15, dirs, &err)) { error = err; return; }
            for (auto& [file, slot] : cards) {
                auto c = bundledCard(file, model == "PC-1500" ? CardHost::PC1500 : CardHost::PC1500A);
                if (!c) { error = "card " + file; return; }
                m15->attachExpansionCard(std::move(c));
            }
            m15->allReset();
            runBootToPrompt(*m15);
        }
    }

    void key(const std::string& name, bool down) {
        if (is1600()) { if (down) m16->pressKey(name); else m16->releaseKey(name); }
        else { if (down) m15->pressKey(name); else m15->releaseKey(name); }
    }
    // A tap as 0.8.1 types it: on the PC-1500 through its key queue (4 frames
    // down, 4 up), on the PC-1600 as a press held for 80 ms.
    void tap(const std::string& name) {
        if (is1600()) { key(name, true); advance(0.08); key(name, false); advance(0.08); }
        else { m15->enqueueKey(name); advance(0.2); }
    }

    void record(const std::string& line) {
        out += "> " + line + "\n";
        if (!m15 && !m16) return;
        LcdText t = is1600() ? pc1600LcdText(*m16) : pc1500LcdText(*m15);
        out += "  cycles " + std::to_string(cycles) + "\n";
        for (const auto& r : t.rows) out += "  lcd |" + r + "|\n";
        out += "  status";
        for (const auto& s : t.status) out += " " + s;
        out += "\n";
        std::vector<int16_t> audio;
        int16_t chunk[4096];
        size_t n;
        while ((n = is1600() ? m16->drainAudio(chunk, 4096) : m15->drainAudio(chunk, 4096)) > 0) audio.insert(audio.end(), chunk, chunk + n);
        size_t loud = 0;
        for (int16_t s : audio) if (s != 0) ++loud;
        Fnv f;
        f.add(audio.data(), audio.size() * sizeof(int16_t));
        out += "  audio samples=" + std::to_string(audio.size()) + " nonzero=" + std::to_string(loud) + " sum=" + f.hex() + "\n";
        if (is1600()) {
            if (m16->ce150Attached()) out += plotSummary("CE-150", m16->ce150PlotPoints());
            if (m16->ce1600pAttached()) out += plotSummary("CE-1600P", m16->ce1600pPlotPoints());
        } else if (m15->ce150Attached()) {
            out += plotSummary("CE-150", m15->ce150PlotPoints());
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
        if (!m15 && !m16) { error = "boot first"; return; }
        if (cmd == "keys") { std::string k; while (in >> k) tap(k); }
        else if (cmd == "type") {
            for (char c : rest()) tap(c == ' ' ? std::string("space") : std::string(1, c));
        }
        else if (cmd == "press" || cmd == "release") { std::string k; in >> k; key(k, cmd == "press"); }
        else if (cmd == "run") { double s = 0; in >> s; advance(s); }
        else if (cmd == "reset") { if (is1600()) m16->reset(); else m15->reset(); }
        else if (cmd == "allreset") {
            if (is1600()) { m16->allReset(); runBootToPrompt(*m16); } else { m15->allReset(); runBootToPrompt(*m15); }
        }
        else { error = "unknown command " + cmd; return; }
        record(line);
    }

    std::string finish() {
        if (!m15 && !m16) return out;
        Fnv f;
        if (is1600()) {
            std::vector<uint8_t> ram(PC1600Machine::kInternalRamSize);
            m16->debugCopyInternalRam(ram.data());
            f.add(ram.data(), ram.size());
            out += "= internal ram sum=" + f.hex() + "\n";
            Fnv z;
            for (uint32_t a = 0; a < 0x10000; ++a) { uint8_t b = m16->memory().peek(static_cast<uint16_t>(a)); z.add(&b, 1); }
            out += "= z80 view sum=" + z.hex() + "\n";
        } else {
            for (uint32_t a = 0; a < 0x10000; ++a) { uint8_t b = m15->memory().peek(static_cast<uint16_t>(a)); f.add(&b, 1); }
            out += "= me0 sum=" + f.hex() + "\n";
        }
        return out;
    }
};

std::string play(const std::string& script, std::string* error) {
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
    return run.finish();
}

TEST(test_golden_runs_of_the_sharp_cores) {
    const fs::path dir = "tests/golden-runs";
    const bool recording = std::getenv("CALCU1600_GOLDEN_RECORD") != nullptr;
    std::vector<fs::path> scripts;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".run") scripts.push_back(e.path());
    std::sort(scripts.begin(), scripts.end());
    CHECK(!scripts.empty());
    for (const auto& s : scripts) {
        std::string error;
        std::string got = play(readText(s), &error);
        if (!error.empty()) { std::fprintf(stderr, "FAIL %s: %s\n", s.filename().c_str(), error.c_str()); CHECK(false); continue; }
        fs::path golden = s;
        golden.replace_extension(".golden");
        if (recording) { std::ofstream(golden, std::ios::binary) << got; continue; }
        if (!fs::exists(golden)) { std::fprintf(stderr, "FAIL %s: no .golden\n", s.filename().c_str()); CHECK(false); continue; }
        std::string want = readText(golden);
        if (want != got) {
            std::istringstream a(want), b(got);
            std::string la, lb;
            int n = 0;
            while (true) {
                ++n;
                const bool ea = !std::getline(a, la), eb = !std::getline(b, lb);
                if (ea && eb) break;
                if (ea || eb || la != lb) {
                    std::fprintf(stderr, "  %s:%d differs\n    golden: %s\n    run:    %s\n", golden.filename().c_str(), n, la.c_str(), lb.c_str());
                    break;
                }
            }
        }
        CHECK(want == got);
    }
}

} // namespace
