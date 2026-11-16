// Golden runs of the TI core (first step 2 of docs/walkthroughs/first-steps.md,
// M-8 of docs/migration-plan.md): scripts of calls on the machine, recorded
// from the code as imported (Calc-U-59 2.3.0), before the core was changed.
// A run that differs later is a finding, not a new reference (C59-1, C59-6,
// Technical Requirements).
//
// Since M-19 they replay through the machine interface (C59-1 AC-4; design
// §8, D-3): build(), advance(), keyDown/keyUp with the key IDs of AC-9,
// reset/powerCycle/coldBoot, readDisplay(), plus the adapters for what the
// interface does not have yet (card, RAM, partition, PC). Since M-25 (C59-6
// design §8) the module and the PC-100C are peripherals in their places:
// `module` and `printer` before the first command go into the build request
// (no power cycle is added), `trace`, `paper` and `print` are the PC-100C's
// controls. Since M-53 (LIB-8) the `print` lines are the text layer of a
// calcu::paper::Paper bound to its `printer/paper-output` capability
// (testroms::paperTap), and the `dots` lines its code lines
// (TIMachineTesting::printedCodeLines, adapter P-E3); both as before.
//
// Each tests/golden-runs/<name>.run is a script; <name>.golden is what the
// core did. A golden "display" line holds 2.3.0's control and digit nibbles;
// the replay compares it in segment space (core/TISegments.hpp segmentsFor;
// "--" is brightness 0, "." a decimal point with brightness above 0).
// CALCU59_GOLDEN_RECORD=1 writes a missing .golden with "segments" lines (the
// segment bits in hex) for a new reference — only for a new run, never to
// make a failing run pass.
//
// Script lines (# starts a comment):
//   model TI-59|TI-58|TI-58C     printer on|off     module <ID>|none
//   boot                         ROM start-up: run to key readiness
//   keys <k> <k> ...             taps (100 ms down, 100 ms up, then to key
//                                readiness); a second function stands for 2nd + key
//   press <k> / release <k>      one key down or up, nothing else
//   run <seconds>                advance emulated time
//   idle                         run to key readiness (at most 60 s)
//   partition <regs>             setPartitionProgramRegs
//   save-ram                     keep a copy of the RAM
//   app-poweron                  2.3.0's TI-58C start: start-up, then the saved RAM
//   app-reset                    Reset: Machine::reset() (TI-59/58: then to key readiness)
//   app-coldboot                 Clean: Machine::coldBoot()
//   core-reset                   2.3.0's core reset: Machine::powerCycle()
//   trace on|off                 the PC-100C's TRACE switch
//   paper / print                the PC-100C's ADV / PRINT button (a press)
//   card-blank / card-eject      a blank card into the reader / out of it
// After every line the run records the tick count since the last operation
// (in weighted instructions, tick count / 32) and the PC, the display
// (sampled over the next 30 ms of emulated time) and what was printed; at the
// end the RAM and the partition.
//
// Expected differences, which are not findings (design §8, D-3): after an
// app-reset on the TI-59/58, `t` is compared as the difference to the
// app-reset line, because Reset now ends at key readiness instead of 2.3.0's
// 300,000 instructions (C59-1 AC-6), and that line's own `t` is not compared;
// the two *-reset-partition runs are compared up to their app-reset line
// (behaviour change 59-22: Reset keeps the partition; machine_tests.cpp checks it).
// In ti59-fill-reset-coldboot the program counter after its app-reset is not
// compared either: the scan loop is reached at another point of its cycle, while
// the display, the RAM and the partition still match 2.3.0 (user, 2026-11-14).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../app/MachineModelInfo.hpp"
#include "../app/TITraceRecorder.hpp"
#include "Md5.hpp"
#include <limits>
#include <cstdlib>
#include <filesystem>
#include <map>

namespace {

namespace fs = std::filesystem;
using calcu::machine::KeyId;
using testroms::dbg;
using testroms::mem;

// The scripts' key names (2.3.0's tools/headless/ti59.py names) to the key IDs
// of C59-1 AC-9; the replay's own table (design §8).
const std::map<std::string, std::string>& keyIds() {
    static const std::map<std::string, std::string> k = {
        {"A", "A"}, {"B", "B"}, {"C", "C"}, {"D", "D"}, {"E", "E"},
        {"2nd", "2nd"}, {"INV", "INV"}, {"lnx", "lnx"}, {"CE", "CE"}, {"CLR", "CLR"},
        {"LRN", "LRN"}, {"x<>t", "x⇄t"}, {"x2", "x²"}, {"sqrt", "√x"}, {"1/x", "1/x"},
        {"SST", "SST"}, {"STO", "STO"}, {"RCL", "RCL"}, {"SUM", "SUM"}, {"y^x", "yˣ"},
        {"BST", "BST"}, {"EE", "EE"}, {"(", "("}, {")", ")"}, {"/", "÷"},
        {"GTO", "GTO"}, {"7", "7"}, {"8", "8"}, {"9", "9"}, {"*", "×"},
        {"SBR", "SBR"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"-", "-"},
        {"RST", "RST"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"+", "+"},
        {"R/S", "R/S"}, {"0", "0"}, {".", "."}, {"+/-", "+/-"}, {"=", "="},
    };
    return k;
}

// Second functions: 2nd followed by the base key.
const std::map<std::string, std::string>& secondKeys() {
    static const std::map<std::string, std::string> k = {
        {"A'", "A"}, {"B'", "B"}, {"C'", "C"}, {"D'", "D"}, {"E'", "E"},
        {"log", "lnx"}, {"CP", "CE"}, {"Pgm", "LRN"}, {"P>R", "x⇄t"}, {"sin", "x²"}, {"cos", "√x"}, {"tan", "1/x"},
        {"Ins", "SST"}, {"CMs", "STO"}, {"Exc", "RCL"}, {"Prd", "SUM"}, {"Ind", "yˣ"},
        {"Del", "BST"}, {"Eng", "EE"}, {"Fix", "("}, {"Int", ")"}, {"|x|", "÷"},
        {"Pause", "GTO"}, {"x=t", "7"}, {"Nop", "8"}, {"Op", "9"}, {"Deg", "×"},
        {"Lbl", "SBR"}, {"x>=t", "4"}, {"S+", "5"}, {"mean", "6"}, {"Rad", "-"},
        {"StFlg", "RST"}, {"IfFlg", "1"}, {"D.MS", "2"}, {"pi", "3"}, {"Grad", "+"},
        {"Write", "R/S"}, {"Dsz", "0"}, {"Adv", "."}, {"Prt", "+/-"}, {"List", "="},
    };
    return k;
}

std::string hex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += d[p[i] >> 4]; s += d[p[i] & 15]; }
    return s;
}

// The display in segment space, position 13 (leftmost) first: the segment
// bits A–G in hex when driven, "--" when not, then "." when its decimal point
// is driven.
std::string segmentState(calcu::machine::Machine& m) {
    const auto out = m.readDisplay();
    std::string s;
    for (int i = 11; i >= 0; --i) {
        const auto& p = testroms::positions(out)[static_cast<size_t>(i)];
        if (p.brightness.value_or(0) > 0) {
            s += "0123456789abcdef"[testroms::segmentBits(p) >> 4];
            s += "0123456789abcdef"[testroms::segmentBits(p) & 15];
        } else {
            s += "--";
        }
        s += p.segmentBrightness[kTIDecimalPoint].value_or(0) > 0 ? '.' : ' ';
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

// A golden "display" line (2.3.0's "<ctrl><digit>" per position) in segment space.
std::string segmentsOfGolden(const std::string& display) {
    std::string s;
    for (size_t i = 0; i + 1 < display.size() || i < display.size(); i += 3) {
        std::string tok = display.substr(i, 2);
        if (tok == "--") s += "--";
        else {
            auto nib = [](char c) { return static_cast<uint8_t>(c <= '9' ? c - '0' : c - 'A' + 10); };
            const uint8_t seg = segmentsFor(nib(tok[1]), nib(tok[0]), false);
            s += "0123456789abcdef"[seg >> 4];
            s += "0123456789abcdef"[seg & 15];
        }
        s += (i + 2 < display.size() && display[i + 2] == '.') ? '.' : ' ';
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

struct Run {
    cu59::Model model = cu59::Model::TI59;
    std::unique_ptr<calcu::machine::Machine> m;
    std::string module = "ML";
    bool printer = false;
    std::vector<uint8_t> savedRam;
    calcu::machine::Ticks base = 0;   // the tick count at the last operation
    std::string out;
    std::string error;

    double hz() const { return testroms::instructionHz(model); }
    void steps(double seconds) { testroms::run(*m, static_cast<uint32_t>(hz() * seconds)); }
    void idle() { testroms::runToScanLoop(*m, model, static_cast<uint32_t>(hz() * 60)); }
    void operation() { base = m->tickCount(); }

    void build() {
        if (module != "none" && !cu59::moduleRomFile(module)) { error = "module " + module + " not found"; return; }
        cu59::MachinePeripherals places;
        if (module != "none") places.module = module;
        places.printer = printer;
        m = cu59::buildMachine(model, *testroms::romSet(), places);
        testroms::freshMachine(*m);   // the PC-100C's code lines kept from the build (P-E3)
        operation();
    }

    void key(const std::string& id, bool down) {
        if (down) m->keyDown(KeyId{id}); else m->keyUp(KeyId{id});
    }

    void tap(const std::string& id) {
        key(id, true);
        steps(0.1);
        key(id, false);
        steps(0.1);
        idle();
    }

    bool keyName(const std::string& k) {
        if (auto it = keyIds().find(k); it != keyIds().end()) { tap(it->second); return true; }
        if (auto it = secondKeys().find(k); it != secondKeys().end()) { tap("2nd"); tap(it->second); return true; }
        return false;
    }

    void record(const std::string& line) {
        out += "> " + line + "\n";
        if (!m) return;
        out += "  t=" + std::to_string((m->tickCount() - base) / 32) + " pc=" + std::to_string(dbg(*m).pc()) + "\n";
        // The display as it is now: what was shown before is dropped, then
        // 30 ms of emulated time are sampled (part of every recorded step).
        m->readDisplay();
        steps(0.03);
        out += "  segments " + segmentState(*m) + "\n";
        for (const auto& l : testroms::printedLines(*m)) out += "  print |" + l + "|\n";
        for (const auto& c : testroms::printedCodeLines(*m))
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
        if (cmd == "printer") { printer = rest() == "on"; if (m) testroms::setPrinter(*m, printer); record(line); return; }
        if (!m) build();
        if (!error.empty()) return;
        if (cmd == "boot" || cmd == "idle") idle();
        else if (cmd == "keys") {
            std::string k;
            while (in >> k) if (!keyName(k)) { error = "unknown key " + k; return; }
        } else if (cmd == "press" || cmd == "release") {
            std::string k; in >> k;
            auto it = keyIds().find(k);
            if (it == keyIds().end()) { error = "unknown key " + k; return; }
            key(it->second, cmd == "press");
        } else if (cmd == "run") { double s = 0; in >> s; steps(s); }
        else if (cmd == "partition") { int r = 0; in >> r; mem(*m).setPartitionProgramRegs(r); }
        else if (cmd == "save-ram") { savedRam.assign(120 * 16, 0); mem(*m).serialiseRAM(savedRam.data()); }
        else if (cmd == "app-poweron") {
            m->powerCycle();
            operation();
            // 2.3.0's StateApplier::powerOn (gone in M-54), as it was recorded: on
            // the TI-58C the ROM's start-up for 300,000 instructions, then the RAM.
            if (cu59::info(model).hasConstantMemory) {
                mem(*m).runInstructions(300'000);
                if (savedRam.size() >= 120 * 16) mem(*m).deserialiseRAM(savedRam.data());
            }
        }
        else if (cmd == "app-reset") {
            m->reset();
            operation();
            // 2.3.0 ran the start-up inside softReset on the TI-59/58; on the TI-58C it did not.
            if (!cu59::info(model).hasConstantMemory) idle();
        }
        else if (cmd == "app-coldboot") { m->coldBoot(); operation(); }
        else if (cmd == "core-reset") { m->powerCycle(); operation(); }
        else if (cmd == "trace") testroms::printerControl(*m, PC100C::kTrace, rest() == "on");
        else if (cmd == "paper" || cmd == "print") {
            auto press = [&](bool d) { testroms::printerControl(*m, cmd == "paper" ? PC100C::kAdv : PC100C::kPrint, d); };
            press(true); steps(0.1); press(false); steps(0.1);
            idle();
        }
        else if (cmd == "card-blank") testroms::card(*m).insertCard(nullptr, 0);
        else if (cmd == "card-eject") { auto b = testroms::card(*m).cardEject(); out += "  card " + hex(b.data(), b.size()) + "\n"; }
        else { error = "unknown command " + cmd; return; }
        record(line);
    }

    std::string finish() {
        if (!m) return out;
        std::vector<uint8_t> ram(120 * 16);
        mem(*m).serialiseRAM(ram.data());
        out += "= ram\n";
        for (int r = 0; r < 120; ++r) out += "  " + std::to_string(r) + " " + hex(&ram[r * 16], 16) + "\n";
        out += "= partition " + std::to_string(mem(*m).partitionProgramRegs()) + "\n";
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

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> v;
    std::istringstream in(text);
    for (std::string l; std::getline(in, l);) v.push_back(l);
    return v;
}

bool parseT(const std::string& l, long& t, std::string& pc) {
    if (l.rfind("  t=", 0) != 0) return false;
    t = std::stol(l.substr(4));
    pc = l.substr(l.find(" pc="));
    return true;
}

// The golden against the replay under the rules of design §8 / D-3. Returns
// the first difference ("" when none).
std::string compare(const std::string& name, const std::string& golden, const std::string& got) {
    const auto want = lines(golden), have = lines(got);
    const bool stopAtReset = name.find("-reset-partition") != std::string::npos;
    const bool sixty = name.rfind("ti58c-", 0) != 0;   // the TI-59/58: Reset ends at key readiness
    const bool pcAfterReset = name != "ti59-fill-reset-coldboot.golden";   // see the header comment
    bool afterReset = false;
    long wantResetT = 0, haveResetT = 0;
    for (size_t i = 0; i < std::max(want.size(), have.size()); ++i) {
        const std::string w = i < want.size() ? want[i] : "<end>", h = i < have.size() ? have[i] : "<end>";
        auto diff = [&] { return name + ":" + std::to_string(i + 1) + " differs\n    golden: " + w + "\n    run:    " + h; };
        if (w == "> app-reset" && stopAtReset) return h == w ? "" : diff();
        long tw, th;
        std::string pw, ph;
        if (parseT(w, tw, pw) && parseT(h, th, ph)) {
            const bool resetLine = i > 0 && want[i - 1] == "> app-reset" && sixty;
            if (resetLine) {
                afterReset = true;
                wantResetT = tw;
                haveResetT = th;
                if (pcAfterReset && pw != ph) return diff();
                continue;
            }
            if (afterReset ? (tw - wantResetT != th - haveResetT || (pcAfterReset && pw != ph)) : w != h) return diff();
            if (w.rfind("> app-coldboot", 0) == 0 || w.rfind("> core-reset", 0) == 0) afterReset = false;
            continue;
        }
        if (w.rfind("  display ", 0) == 0 && h.rfind("  segments ", 0) == 0) {
            if (segmentsOfGolden(w.substr(10)) != h.substr(11)) return diff();
            continue;
        }
        if (w.rfind("> ", 0) == 0 && (w == "> app-coldboot" || w == "> core-reset" || w == "> app-poweron")) afterReset = false;
        if (w != h) return diff();
    }
    return "";
}

} // namespace

TEST(c59_1_ac4_golden_runs_through_the_machine_interface) {
    const fs::path dir = "tests/golden-runs";
    const bool recording = std::getenv("CALCU59_GOLDEN_RECORD") != nullptr;
    std::vector<fs::path> scripts;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".run") scripts.push_back(e.path());
    std::sort(scripts.begin(), scripts.end());
    CHECK_EQ(scripts.size(), size_t{18});
    for (const auto& s : scripts) {
        std::string error;
        std::string got = play(testroms::readFile(s.string()).value_or(""), &error);
        if (!error.empty()) { std::printf("  %s: %s\n", s.filename().c_str(), error.c_str()); CHECK(false); continue; }
        fs::path golden = s; golden.replace_extension(".golden");
        auto want = testroms::readFile(golden.string());
        if (!want && recording) {
            std::ofstream(golden, std::ios::binary) << got;
            continue;
        }
        if (!want) { std::printf("  %s: no .golden (record with CALCU59_GOLDEN_RECORD=1)\n", s.filename().c_str()); CHECK(false); continue; }
        const std::string d = compare(golden.filename().string(), *want, got);
        if (!d.empty()) std::printf("  %s\n", d.c_str());
        CHECK(d.empty());
    }
}

// LIB-10 AC-33 (design §12.9, E10-6): before the core was touched, M-53
// recorded on 2.3.0's path (TraceWriter, the frame ring drained every 256
// instructions) a TI-59 running ti59-calculation.run with the trace on from
// its first 256 instructions. It is kept as its length and MD5 with the
// SESSION_START's wall-clock seconds zeroed (5.5 MB; no key or card record
// was in it: the script presses keys on the machine, not through typing).
// The trace recorder's file of the same run must equal it byte for byte.
TEST(lib10_ac33_bytes_equal_the_recorded_reference) {
    constexpr std::size_t kLength = 5'570'343;
    const std::string kMd5 = "921b0aeb81afd89a74ff695f69803509";
    std::unique_ptr<calcu::debugger::TraceRecorder> rec;
    std::vector<uint8_t> out;
    testroms::afterRun() = [&](calcu::machine::Machine& m) {
        if (!rec) {
            rec = cu59::makeTITraceRecorder(m, [] { return std::uint64_t{0}; });
            rec->begin(out);
        }
        rec->take(out, std::numeric_limits<std::uint64_t>::max());
    };
    std::string error;
    play(testroms::readFile("tests/golden-runs/ti59-calculation.run").value_or(""), &error);
    testroms::afterRun() = nullptr;
    CHECK(error.empty());
    CHECK(rec != nullptr);
    if (!rec) return;
    rec->end(out);
    CHECK_EQ(out.size(), kLength);
    CHECK_EQ(testroms::md5Hex(out), kMd5);
}
