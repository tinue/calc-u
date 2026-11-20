// Every example (every preset of examples/) against its reference run
// (tests/references/examples/<folder>_<name>.<ext>.reference, with
// `.paper-text` and `.paper-marks` beside it when it printed).
//
// Each example goes through the harness entry in virtual time (`calcu
// --product c59` in-process): `preset <file>`, then the bare `wait` (until
// the machine is idle or paused, at most an hour of emulated time). Then
// compared: the display (`display:`), the partition (`partition:`), the raw
// RAM, 120 registers of 16 nibbles (`--- ram`: program steps, data
// registers and H00–H03 at once), the in-file magnetic cards (`--- in-file
// cards`, the banks of the media part's store, by the picker's names) and
// the cards written (`--- cards written`, the sides of the run's save
// folder's `.magnetcard.yaml`), and the paper (`paper text`, `paper
// marks`). Not compared: tick counts, the log. `historic/life.ti59` never
// stops: it is only loaded without a failure. A difference is a finding
// (CLAUDE.md: never a new reference).
//
// CALCU59_REFERENCE_COMPARE=1 runs it (Longer: CalcU59ReferenceRuns);
// CALCU59_REFERENCE_EXAMPLES=<folder> compares the files of another folder
// (a draft of an upgrade) instead of examples/. CALCU59_REFERENCE_RECORD=<folder>
// writes the references of every example into that folder (a draft); it
// never writes into tests/references/.

#include "../app/HarnessProduct.hpp"
#include "../app/TIPresetKeys.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "media/MediaPart.hpp"
#include "parts/LibraryParts.hpp"
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
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const std::string kNeverStops = "historic/life.ti59";

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
    std::map<std::string, std::string> inFile;    // mem:<name> → the bank, hex
    std::map<std::string, std::string> written;   // <name>:<side 0|1> → the bank, hex
};

std::string hexBytes(const std::vector<std::uint8_t>& b) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (std::uint8_t x : b) {
        s += d[x >> 4];
        s += d[x & 15];
    }
    return s;
}

Record fromReference(const std::string& text) {
    Record r;
    std::string part;
    for (const std::string& l : linesOf(text)) {
        if (l.rfind("--- ", 0) == 0) {
            part = l;
            continue;
        }
        if (part.empty() && l.rfind("display: ", 0) == 0) r.display = l;
        else if (part.empty() && l.rfind("partition: ", 0) == 0) r.partition = l;
        else if (part == "--- ram" && !l.empty()) r.ram.push_back(l);
        else if (part == "--- in-file cards" && l.rfind("mem:", 0) == 0) {
            const auto sp = l.find(' ');
            r.inFile[l.substr(4, sp - 4)] = l.substr(sp + 1);
        } else if (part == "--- cards written" && !l.empty()) {
            const auto sp = l.find(' ');
            r.written[l.substr(0, sp)] = l.substr(sp + 1);
        }
    }
    return r;
}

std::string toReference(const std::string& rel, const Record& r) {
    std::string t = "example: " + rel + "\n" + r.display + "\n" + r.partition + "\n--- ram\n";
    for (const std::string& l : r.ram) t += l + "\n";
    t += "--- in-file cards\n";
    for (const auto& [name, bank] : r.inFile) t += "mem:" + name + " " + bank + "\n";
    t += "--- cards written\n";
    for (const auto& [key, bank] : r.written) t += key + " " + bank + "\n";
    return t;
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
    // The in-file cards (every name the preset gives with `mem:`), and the
    // cards written (the save folder's card files).
    if (calcu::media::MediaPart* media = &product->libraryParts().media()) {
        const auto& kind = cu59::magneticCardKind();
        std::set<std::string> names;
        const std::string presetText = testroms::readFile(file.string()).value_or("");
        const std::regex mem("mem:\\s*([A-Za-z0-9_+-]+)");
        for (auto it = std::sregex_iterator(presetText.begin(), presetText.end(), mem); it != std::sregex_iterator(); ++it)
            names.insert((*it)[1].str());
        for (const std::string& name : names) {
            const auto card = cu59::magneticCardOf(media->inFileContent(kind, name));
            const auto* b = card ? std::get_if<std::vector<std::uint8_t>>(&card->sides[0]) : nullptr;
            if (b) out.record.inFile[name] = hexBytes(*b);   // a name of a card the run never wrote: none
        }
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(server->setup().saveFolder(), ec)) {
            const std::string f = e.path().filename().string();
            if (f.size() <= kind.suffix.size() || f.substr(f.size() - kind.suffix.size()) != kind.suffix) continue;
            const std::string name = f.substr(0, f.size() - kind.suffix.size());
            const auto r = kind.codec->read(testroms::readFile(e.path().string()).value_or(""), {});
            const auto card = cu59::magneticCardOf(r.info.content);
            for (std::size_t side = 0; card && side < card->sides.size(); ++side)
                if (const auto* b = std::get_if<std::vector<std::uint8_t>>(&card->sides[side]))
                    out.record.written[name + ":" + std::to_string(side)] = hexBytes(*b);
        }
    }
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

TEST(reference_runs_of_the_examples) {
    const char* draft = std::getenv("CALCU59_REFERENCE_RECORD");
    if (!draft && !std::getenv("CALCU59_REFERENCE_COMPARE")) return;   // slow: Longer (CalcU59ReferenceRuns)
    SKIP_UNLESS(fs::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    const fs::path refDir = "tests/references/examples";
    if (draft) fs::create_directories(draft);
    const char* folder = std::getenv("CALCU59_REFERENCE_EXAMPLES");
    const fs::path examples = folder && *folder ? fs::path(folder) : fs::path("examples");
    std::vector<std::string> all;
    for (const auto& e : fs::recursive_directory_iterator(examples)) {
        const auto ext = e.path().extension().string();
        if (ext == ".ti59" || ext == ".ti58" || ext == ".ti58c") all.push_back(fs::relative(e.path(), examples).generic_string());
    }
    std::sort(all.begin(), all.end());
    CHECK(!all.empty());
    int differ = 0;
    for (const std::string& rel : all) {
        std::printf("  ... %s\n", rel.c_str());
        std::fflush(stdout);
        const Run r = run(examples / rel);
        const std::string base = referenceName(rel);
        if (draft) {
            if (!r.failure.empty()) std::printf("  %s: the preset: %s\n", rel.c_str(), r.failure.c_str());
            CHECK(r.failure.empty());
            if (rel == kNeverStops) continue;
            std::ofstream(fs::path(draft) / (base + ".reference"), std::ios::binary) << toReference(rel, r.record);
            if (!r.paperText.empty()) std::ofstream(fs::path(draft) / (base + ".paper-text"), std::ios::binary) << r.paperText;
            if (!r.paperMarks.empty())
                std::ofstream(fs::path(draft) / (base + ".paper-marks"), std::ios::binary) << r.paperMarks;
            continue;
        }
        std::vector<std::string> diffs;
        if (!r.failure.empty()) diffs.push_back("the preset: " + r.failure);
        if (rel != kNeverStops && r.failure.empty()) {
            const auto file = testroms::readFile((refDir / (base + ".reference")).string());
            if (!file) diffs.push_back("no reference");
            const Record want = fromReference(file.value_or(""));
            if (want.display != r.record.display) diffs.push_back("recorded " + want.display + ", now " + r.record.display);
            if (want.partition != r.record.partition) diffs.push_back("recorded " + want.partition + ", now " + r.record.partition);
            if (want.ram != r.record.ram) diffs.push_back("ram " + firstDifference(want.ram, r.record.ram));
            if (want.inFile != r.record.inFile) diffs.push_back("the in-file cards differ");
            if (want.written != r.record.written) diffs.push_back("the cards written differ");
            const auto text = testroms::readFile((refDir / (base + ".paper-text")).string());
            if (text ? *text != r.paperText : !r.paperText.empty())
                diffs.push_back("paper text " + firstDifference(linesOf(text.value_or("")), linesOf(r.paperText)));
            const auto marks = testroms::readFile((refDir / (base + ".paper-marks")).string());
            if (marks.value_or("") != r.paperMarks)
                diffs.push_back("paper marks " + firstDifference(linesOf(marks.value_or("")), linesOf(r.paperMarks)));
        }
        if (diffs.empty()) {
            std::printf("  same %s\n", rel.c_str());
            continue;
        }
        ++differ;
        std::printf("  DIFFERS %s\n", rel.c_str());
        for (const auto& d : diffs) std::printf("    %s\n", d.c_str());
    }
    if (draft) std::printf("  %zu examples recorded\n", all.size());
    else std::printf("  %d of %zu examples differ from the reference\n", differ, all.size());
    CHECK_EQ(differ, 0);
}
