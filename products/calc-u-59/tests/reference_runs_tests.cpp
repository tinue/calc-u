// C59-2 AC-11 (design §6 T10, §8, D-16, D-17): every example's version 2
// file against the reference run of its 2.3.0 file
// (tests/reference/v2.3.0-examples/, recorded in M-9 by 2.3.0's session
// code before any file was upgraded or core code touched).
//
// Each example goes through the harness entry in virtual time (`calcu
// --product 59` in-process): `preset <file>`, then the bare `wait` (until
// the machine is idle or paused, at most an hour of emulated time). Then
// compared: the display (`display:` of the record), the partition
// (`partition:`), the raw RAM, 120 registers of 16 nibbles (`--- ram`:
// program steps, data registers and H00–H03 at once, D-16), and the paper
// (`paper text`, `paper marks` against `.paper-text`, `.paper-marks`; an
// example without a `.paper-text` prints nothing). Not compared (AC-11, row
// 59-23, LIB-6 AC-64): tick counts, `Elapsed:` lines, the log, the
// `playback:` line. `historic/life.ti59` never stops: it is only loaded
// without a failure. The four examples with magnetic cards wait for M-55
// (C59-7), named in kWaiting. A difference is a finding (CLAUDE.md: never a
// new reference): named in kFindings with its reason, any other fails.
//
// CALCU59_REFERENCE_COMPARE=1 runs it (Longer: CalcU59ReferenceRuns);
// CALCU59_REFERENCE_EXAMPLES=<folder> compares the files of another folder
// (a draft of an upgrade) instead of examples/.

#include "../app/HarnessProduct.hpp"
#include "../app/TIPresetKeys.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Run.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const std::set<std::string> kWaiting = {"globe-print.ti59", "historic/characters-fastmode.ti59",
                                        "historic/pdcm-card-tricks.ti59", "historic/pdcm-make-cards.ti59"};
const std::string kNeverStops = "historic/life.ti59";

// The differences found and named (CLAUDE.md: a finding, not a new
// reference): example → what differs and why.
const std::map<std::string, std::string> kFindings = {
    {"debug/example-brief.ti59",
     "C59-2 AC-10's correction of line 16 (`002 11. A` → `002 11 A`): the M-9 record was made from the "
     "uncorrected file (2.3.0 skipped the step), so step 002 holds 11 and the label A runs; design §8"},
    {"historic/fast-grafik-3d-plot.ti59",
     "the printed lines (`paper text`) are the record's; `paper marks` differ in their reach (1244.2 against "
     "1738.6): the `.paper-marks` of M-39 were drawn from the record's text lines (LIB-8 AC-39), and this "
     "example prints plot rows that are no text line (finding for the user)"},
};

std::string hexNibbles(const cu59::RegisterNibbles& n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (std::uint8_t b : n) {
        s += d[(b >> 4) & 15];
        s += d[b & 15];
    }
    return s;
}

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string l; std::getline(in, l);) out.push_back(l);
    return out;
}

// The reply's lines without its `done <n>` header.
std::string replyBody(const std::string& reply) {
    const std::size_t nl = reply.find('\n');
    return nl == std::string::npos ? std::string() : reply.substr(nl + 1);
}

struct Record {
    std::string display, partition;
    std::vector<std::string> ram;
};

Record fromReference(const std::string& text) {
    Record r;
    bool inRam = false;
    for (const std::string& l : linesOf(text)) {
        if (l.rfind("display: ", 0) == 0) r.display = l;
        else if (l.rfind("partition: ", 0) == 0) r.partition = l;
        else if (l.rfind("--- ram", 0) == 0) inRam = true;
        else if (inRam && !l.empty()) r.ram.push_back(l);
    }
    return r;
}

struct Run {
    std::string failure;   // the preset's refusal or failure
    Record record;
    std::string paperText, paperMarks;
};

Run run(const fs::path& file) {
    Run out;
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = calcu::harness::Server::start(*product, {}, &error);
    if (!server) {
        out.failure = "no start: " + error;
        return out;
    }
    const std::string loaded = server->line("preset " + file.generic_string()).text;
    if (loaded != "done\n") {
        out.failure = loaded;
        server->finish();
        return out;
    }
    if (file.generic_string().find(kNeverStops) == std::string::npos) server->line("wait");
    calcu::machine::Machine* m = server->host().session().context().machine();
    if (m) {
        out.record.display = "display: |" + testroms::displayText(*m) + "|";
        if (cu59::ProgramMemory* pm = cu59::programMemoryOf(*m)) {
            out.record.partition = "partition: " + std::to_string(pm->partitionSteps() / 8) + " program registers";
            for (int i = 0; i < 120; ++i) out.record.ram.push_back(std::to_string(i) + " " + hexNibbles(pm->readRaw(i)));
        }
    }
    out.paperText = replyBody(server->line("paper text").text);
    out.paperMarks = replyBody(server->line("paper marks").text);
    server->finish();
    return out;
}

std::string referenceName(const std::string& rel) {
    std::string name = rel;
    std::replace(name.begin(), name.end(), '/', '_');
    return name;
}

std::string firstDifference(const std::vector<std::string>& want, const std::vector<std::string>& got) {
    for (std::size_t i = 0; i < std::max(want.size(), got.size()); ++i) {
        const std::string w = i < want.size() ? want[i] : "<end>", g = i < got.size() ? got[i] : "<end>";
        if (w != g) return "line " + std::to_string(i + 1) + ": recorded |" + w + "|, now |" + g + "|";
    }
    return {};
}

} // namespace

TEST(c59_2_ac11_reference_runs_of_the_examples) {
    if (!std::getenv("CALCU59_REFERENCE_COMPARE")) return;   // slow: Longer (CalcU59ReferenceRuns)
    SKIP_UNLESS(fs::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    const char* folder = std::getenv("CALCU59_REFERENCE_EXAMPLES");
    const fs::path examples = folder && *folder ? fs::path(folder) : fs::path("examples");
    const fs::path refDir = "tests/reference/v2.3.0-examples";
    std::vector<std::string> all;
    for (const auto& e : fs::recursive_directory_iterator(examples)) {
        const auto ext = e.path().extension().string();
        const std::string rel = fs::relative(e.path(), examples).generic_string();
        // The startup presets (C59-2 AC-13) are new in version 2: no reference.
        if ((ext == ".ti59" || ext == ".ti58" || ext == ".ti58c") && rel.rfind("startup/", 0) != 0) all.push_back(rel);
    }
    std::sort(all.begin(), all.end());
    CHECK_EQ(all.size(), std::size_t{57});
    int differ = 0, found = 0;
    for (const std::string& rel : all) {
        if (kWaiting.count(rel)) {
            std::printf("  waiting (M-55) %s\n", rel.c_str());
            continue;
        }
        std::printf("  ... %s\n", rel.c_str());
        std::fflush(stdout);
        const Run r = run(examples / rel);
        std::vector<std::string> diffs;
        if (!r.failure.empty()) diffs.push_back("the preset: " + r.failure);
        if (rel != kNeverStops && r.failure.empty()) {
            const std::string base = referenceName(rel);
            const Record want = fromReference(testroms::readFile((refDir / (base + ".txt")).string()).value_or(""));
            if (want.display != r.record.display) diffs.push_back("recorded " + want.display + ", now " + r.record.display);
            if (want.partition != r.record.partition) diffs.push_back("recorded " + want.partition + ", now " + r.record.partition);
            if (want.ram != r.record.ram) diffs.push_back("ram " + firstDifference(want.ram, r.record.ram));
            const auto text = testroms::readFile((refDir / (base + ".paper-text")).string());
            if (text ? *text != r.paperText : !r.paperText.empty())
                diffs.push_back("paper text " + firstDifference(linesOf(text.value_or("")), linesOf(r.paperText)));
            const auto marks = testroms::readFile((refDir / (base + ".paper-marks")).string());
            if (marks && *marks != r.paperMarks)
                diffs.push_back("paper marks " + firstDifference(linesOf(*marks), linesOf(r.paperMarks)));
        }
        if (diffs.empty()) {
            std::printf("  same %s\n", rel.c_str());
            continue;
        }
        if (const auto it = kFindings.find(rel); it != kFindings.end()) {
            ++found;
            std::printf("  finding %s: %s\n", rel.c_str(), it->second.c_str());
            for (const auto& d : diffs) std::printf("    %s\n", d.c_str());
            continue;
        }
        ++differ;
        std::printf("  DIFFERS %s\n", rel.c_str());
        for (const auto& d : diffs) std::printf("    %s\n", d.c_str());
    }
    std::printf("  %d of %zu examples differ from the reference; %d named as findings; %zu wait for M-55\n", differ,
                all.size(), found, kWaiting.size());
    CHECK_EQ(differ, 0);
}
