// Reference runs of the 57 examples (first step 6 of
// docs/walkthroughs/first-steps.md, M-9 of docs/migration-plan.md): what each
// example's 2.3.0 file leaves behind, recorded before any file is upgraded
// and before core code is touched (C59-2, Technical Requirements; C59-2
// AC-11) by 2.3.0's own SessionController in its cooperative mode on a
// clock the driver supplied (60 Hz ticks): tests/reference/v2.3.0-examples/.
//
// Since M-34 the driver runs the product as every frontend hosts it: the
// library's app session with cu59::TIAppPart (SessionRig.hpp, PartRig), called at 60 Hz
// on the driver's clock; the example goes in as the frontends load a preset
// (the command c59.load-state, an operation). SessionController went in M-34.
//
// CALCU59_REFERENCE_COMPARE=1: each example is run and compared with its
// recorded file; a difference is a finding (CLAUDE.md: never a new
// reference), printed with the first lines that differ. Longer group (CTest
// CalcU59ReferenceRuns). CALCU59_REFERENCE_RECORD=1 (or =<folder>) writes
// what the runs give to build/reference-runs/ (or that folder), never into
// the references; both are slow. Each example: the model its extension names is started, the file is
// loaded, the run goes on until the playback has ended and then one more
// second (at most 30 minutes of emulated time); then the display, the model,
// the partition, the printed lines, the cards written (files and in-file
// `mem:` cards), the log and the whole RAM (program steps and registers).

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace cu59;
namespace fs = std::filesystem;

namespace {

std::string hexBytes(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += d[p[i] >> 4]; s += d[p[i] & 15]; }
    return s;
}

std::string modelName(Model m) { return m == Model::TI58 ? "TI-58" : m == Model::TI58C ? "TI-58C" : "TI-59"; }

std::string record(const fs::path& example) {
    using namespace std::chrono_literals;
    const std::string ext = example.extension().string();
    const Model start = ext == ".ti58" ? Model::TI58 : ext == ".ti58c" ? Model::TI58C : Model::TI59;
    testroms::PartRig r(start);
    TIAppPart& c = r.part();
    std::string out = "example: " + example.generic_string() + "\nstarted: " + modelName(start) + "\n";
    r.run(1s);
    auto text = testroms::readFile(example.string());
    calcu::sessions::Value args = calcu::sessions::Value::object({});
    if (text) args = calcu::sessions::Value::object({{"bytes", calcu::sessions::Value(calcu::sessions::Value::Bytes(text->begin(), text->end()))}});
    r.post(commands::kLoadState, std::move(args));
    const auto loadedAt = r.now;
    bool started = false, ended = false;
    while (r.now - loadedAt < 30min) {
        r.run(100ms);
        if (c.keystrokesPlaying()) started = true;
        if (!c.keystrokesPlaying() && !r.busy() && (started || r.now - loadedAt > 2s)) { ended = true; break; }
    }
    r.run(1s);
    const double secs = std::chrono::duration<double>(r.now - loadedAt).count();
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.1f", secs);
    out += std::string("playback: ") + (ended ? "ended" : "still running after 30 minutes") + ", recorded after " + buf + " s\n";
    calcu::machine::Machine& m = c.emulator()->machine();
    out += "model: " + modelName(c.model()) + "\n";
    out += "display: |" + testroms::displayText(m) + "|\n";
    out += "partition: " + std::to_string(testroms::mem(m).partitionProgramRegs()) + " program registers\n";
    for (const auto& e : r.errors()) out += "error: " + e + "\n";
    out += "--- printed (" + std::to_string(c.printerLines().size()) + " lines)\n";
    for (const auto& l : c.printerLines()) out += "|" + l + "|\n";
    std::map<std::string, std::string> cards = r.p.cards;
    cards.erase("cards/ti58c.mem");   // 2.3.0's Platform kept the constant memory apart from the cards
    out += "--- cards written (" + std::to_string(cards.size()) + ")\n";
    for (const auto& [path, content] : cards) out += "card " + path + "\n" + content + (content.empty() || content.back() != '\n' ? "\n" : "");
    const auto memCards = c.memoryCardBanks();
    out += "--- in-file cards (" + std::to_string(memCards.size()) + ")\n";
    for (const auto& [name, bank] : memCards) out += "mem:" + name + " " + hexBytes(bank.data(), bank.size()) + "\n";
    out += "--- log\n" + c.logText() + (c.logText().empty() || c.logText().back() == '\n' ? "" : "\n");
    std::vector<uint8_t> ram(120 * 16);
    testroms::mem(m).serialiseRAM(ram.data());
    out += "--- ram (register: 16 nibbles)\n";
    for (int i = 0; i < 120; ++i) out += std::to_string(i) + " " + hexBytes(&ram[i * 16], 16) + "\n";
    return out;
}

std::vector<fs::path> examples() {
    std::vector<fs::path> v;
    for (const auto& e : fs::recursive_directory_iterator("examples")) {
        const auto ext = e.path().extension().string();
        if (ext == ".ti59" || ext == ".ti58" || ext == ".ti58c") v.push_back(e.path());
    }
    std::sort(v.begin(), v.end());
    return v;
}

std::string referenceName(const fs::path& ex) {
    std::string name = fs::relative(ex, "examples").generic_string();
    std::replace(name.begin(), name.end(), '/', '_');
    return name + ".txt";
}

/// The first lines where `a` and `b` differ.
std::string firstDifference(const std::string& want, const std::string& got) {
    std::istringstream w(want), g(got);
    std::string lw, lg, out;
    int line = 0, shown = 0;
    while (shown < 4) {
        const bool hw = static_cast<bool>(std::getline(w, lw)), hg = static_cast<bool>(std::getline(g, lg));
        if (!hw && !hg) break;
        ++line;
        if (hw && hg && lw == lg) continue;
        out += "    line " + std::to_string(line) + ": recorded |" + (hw ? lw : "<end>") + "|, now |" + (hg ? lg : "<end>") + "|\n";
        ++shown;
    }
    return out;
}

} // namespace

TEST(reference_runs_of_the_examples) {
    const bool compare = std::getenv("CALCU59_REFERENCE_COMPARE") != nullptr;
    const char* recordTo = std::getenv("CALCU59_REFERENCE_RECORD");
    if (!compare && !recordTo) return;   // slow: on request (Longer: CalcU59ReferenceRuns)
    const fs::path refDir = "tests/reference/v2.3.0-examples";
    // Never into the references: build/reference-runs/, or the folder named.
    const fs::path outDir = recordTo && *recordTo && std::string(recordTo) != "1" ? fs::path(recordTo) : fs::path("build/reference-runs");
    const auto all = examples();
    CHECK(all.size() == 57);
    int differ = 0;
    for (const auto& ex : all) {
        const std::string name = referenceName(ex);
        const std::string got = record(ex);
        if (recordTo) {
            fs::create_directories(outDir);
            std::ofstream(outDir / name, std::ios::binary) << got;
        }
        if (!compare) continue;
        const std::string want = testroms::readFile((refDir / name).string()).value_or("");
        if (got == want) {
            std::printf("  same %s\n", name.c_str());
            continue;
        }
        ++differ;
        std::printf("  DIFFERS %s\n%s", name.c_str(), firstDifference(want, got).c_str());
    }
    if (compare) std::printf("  %d of %zu examples differ from the reference\n", differ, all.size());
    CHECK_EQ(differ, 0);
}
