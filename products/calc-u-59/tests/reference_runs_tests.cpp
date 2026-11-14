// Reference runs of the 57 examples (first step 6 of
// docs/walkthroughs/first-steps.md, M-9 of docs/migration-plan.md): what each
// example's 2.3.0 file leaves behind, played by 2.3.0's own SessionController
// in its cooperative mode on a clock this driver supplies (60 Hz ticks),
// recorded before any file is upgraded and before core code is touched
// (C59-2, Technical Requirements; C59-2 AC-11).
//
// Runs only with CALCU59_REFERENCE_RECORD=1: it is slow, and its output is
// reference data (tests/reference/v2.3.0-examples/), not a test of today's
// code. Each example: the model its extension names is started, the file is
// loaded as 2.3.0 loads it, the run goes on until the playback has ended and
// then one more second (at most 30 minutes of emulated time); then the
// display, the model, the partition, the printed lines, the cards written
// (files and in-file `mem:` cards),
// the log and the whole RAM (program steps and registers) are written.

#include "TestHarness.hpp"
#include "FakePlatform.hpp"
#include "TestRoms.hpp"
#include "../app/SessionController.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace cu59;
using Clock = std::chrono::steady_clock;
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
    testroms::FakePlatform p;
    SessionController c{p, testroms::reader(), testroms::readFile("roms/solid-state/cuecards.txt").value_or(""), "ML",
                        Threading::Cooperative};
    Clock::time_point t = Clock::time_point{} + 500ms;
    auto run = [&](Clock::duration d) {
        for (auto end = t + d; t < end; t += std::chrono::microseconds(16'667)) { c.tick(t); p.pump(); }
    };
    std::string out = "example: " + example.generic_string() + "\nstarted: " + modelName(start) + "\n";
    if (!c.startMachine(start)) return out + "error: the machine did not start\n";
    run(1s);
    auto text = testroms::readFile(example.string());
    c.loadStateFile(text ? std::optional<std::vector<uint8_t>>(std::vector<uint8_t>(text->begin(), text->end())) : std::nullopt);
    const auto loadedAt = t;
    bool started = false, ended = false;
    while (t - loadedAt < 30min) {
        run(100ms);
        if (c.keystrokesPlaying()) started = true;
        if (!c.keystrokesPlaying() && (started || t - loadedAt > 2s)) { ended = true; break; }
    }
    run(1s);
    const double secs = std::chrono::duration<double>(t - loadedAt).count();
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.1f", secs);
    out += std::string("playback: ") + (ended ? "ended" : "still running after 30 minutes") + ", recorded after " + buf + " s\n";
    TI59Machine& m = c.session()->machine();
    out += "model: " + modelName(c.model()) + "\n";
    out += "display: |" + testroms::displayText(m) + "|\n";
    out += "partition: " + std::to_string(m.partitionProgramRegs()) + " program registers\n";
    for (const auto& e : p.errors) out += "error: " + e + "\n";
    out += "--- printed (" + std::to_string(c.printerLines().size()) + " lines)\n";
    for (const auto& l : c.printerLines()) out += "|" + l + "|\n";
    out += "--- cards written (" + std::to_string(p.cards.size()) + ")\n";
    for (const auto& [path, content] : p.cards) out += "card " + path + "\n" + content + (content.empty() || content.back() != '\n' ? "\n" : "");
    const auto memCards = c.memoryCardBanks();
    out += "--- in-file cards (" + std::to_string(memCards.size()) + ")\n";
    for (const auto& [name, bank] : memCards) out += "mem:" + name + " " + hexBytes(bank.data(), bank.size()) + "\n";
    out += "--- log\n" + c.logText() + (c.logText().empty() || c.logText().back() == '\n' ? "" : "\n");
    std::vector<uint8_t> ram(120 * 16);
    m.serialiseRAM(ram.data());
    out += "--- ram (register: 16 nibbles)\n";
    for (int r = 0; r < 120; ++r) out += std::to_string(r) + " " + hexBytes(&ram[r * 16], 16) + "\n";
    return out;
}

} // namespace

TEST(reference_runs_of_the_examples) {
    if (!std::getenv("CALCU59_REFERENCE_RECORD")) return;  // reference data, recorded on request
    const fs::path outDir = "tests/reference/v2.3.0-examples";
    fs::create_directories(outDir);
    std::vector<fs::path> examples;
    for (const auto& e : fs::recursive_directory_iterator("examples")) {
        const auto ext = e.path().extension().string();
        if (ext == ".ti59" || ext == ".ti58" || ext == ".ti58c") examples.push_back(e.path());
    }
    std::sort(examples.begin(), examples.end());
    for (const auto& ex : examples) {
        std::string name = fs::relative(ex, "examples").generic_string();
        std::replace(name.begin(), name.end(), '/', '_');
        std::ofstream(outDir / (name + ".txt"), std::ios::binary) << record(ex);
        std::printf("  recorded %s\n", name.c_str());
    }
    CHECK(examples.size() == 57);
}
