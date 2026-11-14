// Golden runs of the TI core (first step 2 of docs/walkthroughs/first-steps.md,
// M-8 of docs/migration-plan.md): scripts of calls on TI59Machine, recorded
// from the code as imported (Calc-U-59 2.3.0), before the core is changed.
// A run that differs later is a finding, not a new reference (C59-1, C59-6,
// Technical Requirements).
//
// Each tests/golden-runs/<name>.run is a script; <name>.golden is what the
// core did. CALCU59_GOLDEN_RECORD=1 rewrites the .golden files (only when a
// reference is to be recorded, never to make a failing run pass).
//
// Script lines (# starts a comment):
//   model TI-59|TI-58|TI-58C     printer on|off     module <ID>|none
//   boot                         ROM start-up: run to the keyboard scan loop
//   keys <k> <k> ...             taps (100 ms down, 100 ms up, then to the scan
//                                loop); a second function stands for 2nd + key
//   press <k> / release <k>      one key down or up, nothing else
//   run <seconds>                advance emulated time
//   idle                         run to the scan loop (at most 60 s)
//   partition <regs>             setPartitionProgramRegs
//   save-ram                     keep a copy of the RAM
//   app-poweron                  2.3.0's TI-58C start: start-up, then the saved RAM
//   app-reset                    2.3.0's Reset (StateApplier softReset, saved RAM)
//   app-coldboot                 2.3.0's Clean (StateApplier cleanReset)
//   core-reset                   TI59Machine::reset only
//   trace on|off                 the PC-100C's TRACE switch
//   paper / print                the PC-100C's ADV / PRINT button (a press)
//   card-blank / card-eject      a blank card into the reader / out of it
// After every line the run records the tick count, the display (sampled over
// the next 30 ms of emulated time) and what was printed; at the end the RAM.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../app/StateApplier.hpp"
#include <cstdlib>
#include <filesystem>
#include <map>

namespace {

namespace fs = std::filesystem;

const std::map<std::string, int>& keyCodes() {
    static const std::map<std::string, int> k = {
        {"A", 11}, {"B", 12}, {"C", 13}, {"D", 14}, {"E", 15},
        {"2nd", 21}, {"INV", 22}, {"lnx", 23}, {"CE", 24}, {"CLR", 25},
        {"LRN", 31}, {"x<>t", 32}, {"x2", 33}, {"sqrt", 34}, {"1/x", 35},
        {"SST", 41}, {"STO", 42}, {"RCL", 43}, {"SUM", 44}, {"y^x", 45},
        {"BST", 51}, {"EE", 52}, {"(", 53}, {")", 54}, {"/", 55},
        {"GTO", 61}, {"7", 62}, {"8", 63}, {"9", 64}, {"*", 65},
        {"SBR", 71}, {"4", 72}, {"5", 73}, {"6", 74}, {"-", 75},
        {"RST", 81}, {"1", 82}, {"2", 83}, {"3", 84}, {"+", 85},
        {"R/S", 91}, {"0", 92}, {".", 93}, {"+/-", 94}, {"=", 95},
    };
    return k;
}

// Second functions: 2nd followed by the base key's matrix code.
const std::map<std::string, int>& secondCodes() {
    static const std::map<std::string, int> k = {
        {"A'", 11}, {"B'", 12}, {"C'", 13}, {"D'", 14}, {"E'", 15},
        {"log", 23}, {"CP", 24}, {"Pgm", 31}, {"P>R", 32}, {"sin", 33}, {"cos", 34}, {"tan", 35},
        {"Ins", 41}, {"CMs", 42}, {"Exc", 43}, {"Prd", 44}, {"Ind", 45},
        {"Del", 51}, {"Eng", 52}, {"Fix", 53}, {"Int", 54}, {"|x|", 55},
        {"Pause", 61}, {"x=t", 62}, {"Nop", 63}, {"Op", 64}, {"Deg", 65},
        {"Lbl", 71}, {"x>=t", 72}, {"S+", 73}, {"mean", 74}, {"Rad", 75},
        {"StFlg", 81}, {"IfFlg", 82}, {"D.MS", 83}, {"pi", 84}, {"Grad", 85},
        {"Write", 91}, {"Dsz", 92}, {"Adv", 93}, {"Prt", 94}, {"List", 95},
    };
    return k;
}

std::string hex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += d[p[i] >> 4]; s += d[p[i] & 15]; }
    return s;
}

// Position 13 (leftmost) first: "<ctrl><digit>" when driven, "--" when not,
// then "." when its decimal point is driven.
std::string displayState(TI59Machine& m) {
    DisplaySnapshot d = m.takeDisplay();
    std::string s;
    for (int i = 11; i >= 0; --i) {
        if (d.digitDuty[i] > 0) {
            s += "0123456789ABCDEF"[d.ctrl[i] & 15];
            s += "0123456789ABCDEF"[d.digits[i] & 15];
        } else {
            s += "--";
        }
        s += d.dpDuty[i] > 0 ? '.' : ' ';
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

struct Run {
    cu59::Model model = cu59::Model::TI59;
    std::unique_ptr<TI59Machine> m;
    std::string module = "ML";
    bool printer = false;
    std::vector<uint8_t> savedRam;
    std::string out;
    std::string error;

    double hz() const { return cu59::info(model).instructionHz; }
    void steps(double seconds) { m->stepN(static_cast<uint32_t>(hz() * seconds), false); }

    void build() {
        m = std::make_unique<TI59Machine>(cu59::toVariant(model));
        auto words = cu59::loadRom(model, testroms::reader());
        m->loadROM(words.data(), words.size());
        if (module != "none") {
            auto cue = testroms::readFile("roms/solid-state/cuecards.txt");
            auto file = cue ? cu59::moduleRomFile(*cue, module) : std::nullopt;
            auto lib = file ? cu59::loadModule(*file, testroms::reader()) : std::nullopt;
            if (!lib) { error = "module " + module + " not found"; return; }
            m->loadLibrary(lib->data(), lib->size());
        }
        auto k = cu59::loadConstants(model, testroms::reader());
        m->loadConstants(k.data(), k.size());
        m->setPrinterConnected(printer);
        m->reset();
    }

    void tap(int code) {
        cu59::pressMatrix(*m, code);
        steps(0.1);
        cu59::releaseMatrix(*m, code);
        steps(0.1);
        testroms::runToScanLoop(*m, model, static_cast<uint32_t>(hz() * 60));
    }

    bool keyName(const std::string& k) {
        if (auto it = keyCodes().find(k); it != keyCodes().end()) { tap(it->second); return true; }
        if (auto it = secondCodes().find(k); it != secondCodes().end()) { tap(21); tap(it->second); return true; }
        return false;
    }

    int code(const std::string& k) {
        auto it = keyCodes().find(k);
        return it == keyCodes().end() ? -1 : it->second;
    }

    void record(const std::string& line) {
        out += "> " + line + "\n";
        if (!m) return;
        out += "  t=" + std::to_string(m->elapsedTicks()) + " pc=" + std::to_string(m->pc()) + "\n";
        // The display as it is now: what was shown before is dropped, then
        // 30 ms of emulated time are sampled (part of every recorded step).
        m->takeDisplay();
        steps(0.03);
        out += "  display " + displayState(*m) + "\n";
        for (const auto& l : m->drainPrinterLines()) out += "  print |" + l + "|\n";
        for (const auto& c : m->drainPrinterCodeLines())
            out += "  dots " + hex(c.codes.data(), c.codes.size()) + " rows=" + std::to_string(c.rowCount) +
                   " partial=" + std::to_string(c.partialColumns) + "/" + std::to_string(c.partialHeat) + "\n";
    }

    void command(const std::string& line) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        auto rest = [&] { std::string a; std::getline(in, a); return a.empty() ? a : a.substr(a.find_first_not_of(' ')); };
        if (cmd == "model") {
            std::string name = rest();
            model = name == "TI-58" ? cu59::Model::TI58 : name == "TI-58C" ? cu59::Model::TI58C : cu59::Model::TI59;
            return;
        }
        if (cmd == "module") { module = rest(); return; }
        if (cmd == "printer") { printer = rest() == "on"; if (m) m->setPrinterConnected(printer); record(line); return; }
        if (!m) build();
        if (!error.empty()) return;
        if (cmd == "boot" || cmd == "idle") testroms::runToScanLoop(*m, model, static_cast<uint32_t>(hz() * 60));
        else if (cmd == "keys") {
            std::string k;
            while (in >> k) if (!keyName(k)) { error = "unknown key " + k; return; }
        } else if (cmd == "press" || cmd == "release") {
            std::string k; in >> k;
            int c = code(k);
            if (c < 0) { error = "unknown key " + k; return; }
            if (cmd == "press") cu59::pressMatrix(*m, c); else cu59::releaseMatrix(*m, c);
        } else if (cmd == "run") { double s = 0; in >> s; steps(s); }
        else if (cmd == "partition") { int r = 0; in >> r; m->setPartitionProgramRegs(r); }
        else if (cmd == "save-ram") { savedRam.assign(120 * 16, 0); m->serialiseRAM(savedRam.data()); }
        else if (cmd == "app-poweron") { m->reset(); cu59::powerOn(*m, model, savedRam.empty() ? nullptr : &savedRam); }
        else if (cmd == "app-reset") {
            std::vector<uint8_t> ram(120 * 16);
            m->serialiseRAM(ram.data());
            cu59::softReset(*m, model, &ram);
        }
        else if (cmd == "app-coldboot") cu59::cleanReset(*m, model);
        else if (cmd == "core-reset") m->reset();
        else if (cmd == "trace") m->setPrinterTrace(rest() == "on");
        else if (cmd == "paper" || cmd == "print") {
            auto press = [&](bool d) { if (cmd == "paper") m->pressPrinterAdv(d); else m->pressPrinterPrint(d); };
            press(true); steps(0.1); press(false); steps(0.1);
            testroms::runToScanLoop(*m, model, static_cast<uint32_t>(hz() * 60));
        }
        else if (cmd == "card-blank") m->insertCard(nullptr, 0);
        else if (cmd == "card-eject") { auto b = m->cardEject(); out += "  card " + hex(b.data(), b.size()) + "\n"; }
        else { error = "unknown command " + cmd; return; }
        record(line);
    }

    std::string finish() {
        if (!m) return out;
        std::vector<uint8_t> ram(120 * 16);
        m->serialiseRAM(ram.data());
        out += "= ram\n";
        for (int r = 0; r < 120; ++r) out += "  " + std::to_string(r) + " " + hex(&ram[r * 16], 16) + "\n";
        out += "= partition " + std::to_string(m->partitionProgramRegs()) + "\n";
        return out;
    }
};

std::string play(const std::string& script, std::string* error) {
    Run run;
    std::istringstream in(script);
    std::string line;
    while (std::getline(in, line)) {
        if (auto h = line.find('#'); h != std::string::npos && !(h > 0 && line[h - 1] != ' ')) line.erase(h);
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r')) line.pop_back();
        if (line.empty()) continue;
        run.command(line);
        if (!run.error.empty()) { *error = run.error; return {}; }
    }
    return run.finish();
}

} // namespace

TEST(golden_runs_of_the_ti_core) {
    const fs::path dir = "tests/golden-runs";
    const bool recording = std::getenv("CALCU59_GOLDEN_RECORD") != nullptr;
    std::vector<fs::path> scripts;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".run") scripts.push_back(e.path());
    std::sort(scripts.begin(), scripts.end());
    CHECK(!scripts.empty());
    for (const auto& s : scripts) {
        std::string error;
        std::string got = play(testroms::readFile(s.string()).value_or(""), &error);
        if (!error.empty()) { std::printf("  %s: %s\n", s.filename().c_str(), error.c_str()); CHECK(false); continue; }
        fs::path golden = s; golden.replace_extension(".golden");
        if (recording) {
            std::ofstream(golden, std::ios::binary) << got;
            continue;
        }
        auto want = testroms::readFile(golden.string());
        if (!want) { std::printf("  %s: no .golden (record with CALCU59_GOLDEN_RECORD=1)\n", s.filename().c_str()); CHECK(false); continue; }
        if (*want != got) {
            std::istringstream a(*want), b(got);
            std::string la, lb; int n = 0;
            while (true) {
                ++n;
                bool ea = !std::getline(a, la), eb = !std::getline(b, lb);
                if (ea && eb) break;
                if (ea || eb || la != lb) {
                    std::printf("  %s:%d differs\n    golden: %s\n    run:    %s\n", golden.filename().c_str(), n, la.c_str(), lb.c_str());
                    break;
                }
            }
        }
        CHECK(*want == got);
    }
}
