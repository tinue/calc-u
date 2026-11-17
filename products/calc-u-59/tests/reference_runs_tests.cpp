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
// without a failure. The four examples with magnetic cards (C59-7 AC-30, M-55)
// also compare the banks of their in-file cards (`--- in-file cards`, from
// the media part's store) and of the card files they write (`--- cards
// written`, the sides of the run's save folder's `.magnetcard.yaml`); every
// card is fitted after it has passed, which is no difference of the machine.
// A difference is a finding (CLAUDE.md: never a
// new reference): named in kFindings with its reason, any other fails.
//
// CALCU59_REFERENCE_COMPARE=1 runs it (Longer: CalcU59ReferenceRuns);
// CALCU59_REFERENCE_EXAMPLES=<folder> compares the files of another folder
// (a draft of an upgrade) instead of examples/.

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
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const std::string kNeverStops = "historic/life.ti59";

// The differences found and named (CLAUDE.md: a finding, not a new
// reference): example → what differs and why.
const std::map<std::string, std::string> kFindings = {
    {"debug/example-brief.ti59",
     "C59-2 AC-10's correction of line 16 (`002 11. A` → `002 11 A`): the M-9 record was made from the "
     "uncorrected file (2.3.0 skipped the step), so step 002 holds 11 and the label A runs; design §8"},
    // The printed pictures: their `paper text` is the record's, line for line;
    // their `.paper-marks` (M-39, LIB-8 AC-39) were drawn from those text lines,
    // one character row per line, but these programs print with the printer
    // interrupt or plot rows, so the paper's dots and reach are not text rows
    // (as the three of LIB-8 D-13 that have no `.paper-marks`).
    {"historic/fast-grafik-3d-plot.ti59", "paper marks of a plot, not of text lines (the text is the record's)"},
    {"historic/printer-quirks.ti59", "paper marks of printer-interrupt rows, not of text lines (the text is the record's)"},
    {"historic/texas-print.ti59", "paper marks of a printer-interrupt picture, not of text lines (the text is the record's)"},
    {"screentest/test_texas.ti59", "paper marks of a printer-interrupt picture, not of text lines (the text is the record's)"},
    {"picard.ti59", "paper marks of a printer-interrupt picture, not of text lines (the text is the record's)"},
    {"globe-print.ti59", "paper marks of a printer-interrupt picture, not of text lines (the text is the record's; M-55)"},
    // M-55 (C59-7 AC-30): 2.3.0 recorded 2.7 s after its playback ended,
    // while the program was still printing the table at fast mode (one line
    // on paper, the display dark); the bare `wait` now runs to idle, so the
    // whole table is printed and the display shows 0. The card reads before
    // are the same: the program the card loads runs.
    {"historic/characters-fastmode.ti59",
     "the 2.3.0 record was taken while the table was still printing (2.7 s after the playback); the run now ends at idle"},
    {"switzerland.ti59", "paper marks of a printer-interrupt picture, not of text lines (the text is the record's)"},
    // A screen test that ends paused in the error blink: `wait: 400ms` now
    // counts emulated time (LIB-6 AC-20) and the pause lands by LIB-10's rules,
    // so the display is caught at another phase of the blink; RAM, partition
    // and paper are the record's.
    {"screentest/test_58_glyphs.ti58", "the display at the pause is another phase of the blink (RAM and partition are the record's)"},
};
// Only these parts may differ for a finding: the rest still has to match.
const std::map<std::string, std::string> kFindingParts = {
    {"debug/example-brief.ti59", "display ram"},
    {"historic/fast-grafik-3d-plot.ti59", "marks"},
    {"historic/printer-quirks.ti59", "marks"},
    {"historic/texas-print.ti59", "marks"},
    {"screentest/test_texas.ti59", "marks"},
    {"picard.ti59", "marks"},
    {"globe-print.ti59", "marks"},
    {"historic/characters-fastmode.ti59", "display ram text marks"},
    {"switzerland.ti59", "marks"},
    {"screentest/test_58_glyphs.ti58", "display"},
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

// A 2.3.0 card file (Calc-U-59 Card 2.0) of the record: its sides as banks.
// Its DATA rows hold the bank's bytes in order (2.3.0 CardFile.cpp).
void writtenCard(const std::string& name, const std::vector<std::string>& lines, Record& r) {
    int side = -1;
    std::vector<std::uint8_t> d;
    auto flush = [&] {
        if (side >= 0) r.written[name + ":" + std::to_string(side)] = hexBytes(d);
    };
    for (const std::string& l : lines) {
        if (l.rfind("SIDE: ", 0) == 0) {
            flush();
            side = std::stoi(l.substr(6)) - 1;
            d.assign(246, 0);
            continue;
        }
        if (side < 0) continue;
        const auto colon = l.find(':');
        if (colon == std::string::npos || colon + 2 > l.size()) continue;
        const std::string k = l.substr(0, colon), v = l.substr(colon + 2);
        if (k == "Partition") d[0] = static_cast<std::uint8_t>(std::stoi(v, nullptr, 16));
        else if (k == "DataType") d[1] = v == "program" ? 0x11 : 0x10;
        else if (k == "Bank") d[2] = cu59::pageByteOfBank(std::stoi(v));
        else if (k == "Protection") d[3] = v == "yes" ? 0x10 : 0x00;
        else if (k == "Checksum") d[244] = d[245] = static_cast<std::uint8_t>(std::stoi(v, nullptr, 16));
        else if (k.size() == 4 && k[0] == 'R') {
            const int row = std::stoi(k.substr(1)) % 30;
            std::istringstream in(v);
            for (int i = 0; i < 8; ++i) {
                std::string h;
                in >> h;
                d[static_cast<std::size_t>(4 + row * 8 + i)] = static_cast<std::uint8_t>(std::stoi(h, nullptr, 16));
            }
        }
    }
    flush();
}

Record fromReference(const std::string& text) {
    Record r;
    std::string part;
    std::string card;
    std::vector<std::string> cardLines;
    auto endCard = [&] {
        if (!card.empty()) writtenCard(card, cardLines, r);
        card.clear();
        cardLines.clear();
    };
    for (const std::string& l : linesOf(text)) {
        if (l.rfind("--- ", 0) == 0) {
            endCard();
            part = l;
            continue;
        }
        if (part.empty() && l.rfind("display: ", 0) == 0) r.display = l;
        else if (part.empty() && l.rfind("partition: ", 0) == 0) r.partition = l;
        else if (part.rfind("--- ram", 0) == 0 && !l.empty()) r.ram.push_back(l);
        else if (part.rfind("--- in-file cards", 0) == 0 && l.rfind("mem:", 0) == 0) {
            const auto sp = l.find(' ');
            r.inFile[l.substr(4, sp - 4)] = l.substr(sp + 1);
        } else if (part.rfind("--- cards written", 0) == 0) {
            if (l.rfind("card ", 0) == 0) {
                endCard();
                std::string f = l.substr(l.find_last_of('/') + 1);
                card = f.substr(0, f.rfind('.'));
            } else {
                cardLines.push_back(l);
            }
        }
    }
    endCard();
    return r;
}

struct Run {
    std::string failure;   // the preset's refusal or failure
    Record record;
    std::string paperText, paperMarks;
};

Run run(const fs::path& file, const Record& want) {
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
    // C59-7 AC-30: the in-file cards the record names, and the cards written.
    if (calcu::media::MediaPart* media = product->libraryParts().media()) {
        const auto& kind = cu59::magneticCardKind();
        for (const auto& [name, bank] : want.inFile) {
            const auto card = cu59::magneticCardOf(media->inFileContent(kind, name));
            const auto* b = card ? std::get_if<std::vector<std::uint8_t>>(&card->sides[0]) : nullptr;
            out.record.inFile[name] = b ? hexBytes(*b) : "(none)";
        }
        for (const auto& [key, bank] : want.written) {
            const std::string name = key.substr(0, key.rfind(':'));
            const int side = std::stoi(key.substr(key.rfind(':') + 1));
            const auto text = testroms::readFile((fs::path(server->setup().saveFolder()) / (name + kind.suffix)).string());
            const auto r = kind.codec->read(text.value_or(""), {});
            const auto card = cu59::magneticCardOf(r.info.content);
            const auto* b = card ? std::get_if<std::vector<std::uint8_t>>(&card->sides[static_cast<std::size_t>(side)]) : nullptr;
            out.record.written[key] = b ? hexBytes(*b) : "(none)";
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
        std::printf("  ... %s\n", rel.c_str());
        std::fflush(stdout);
        const Record recorded = fromReference(testroms::readFile((refDir / (referenceName(rel) + ".txt")).string()).value_or(""));
        const Run r = run(examples / rel, recorded);
        std::vector<std::string> diffs, parts;   // what differs, and which part (display, partition, ram, text, marks, load)
        if (!r.failure.empty()) parts.push_back("load"), diffs.push_back("the preset: " + r.failure);
        if (rel != kNeverStops && r.failure.empty()) {
            const std::string base = referenceName(rel);
            const Record want = fromReference(testroms::readFile((refDir / (base + ".txt")).string()).value_or(""));
            if (want.display != r.record.display) parts.push_back("display"), diffs.push_back("recorded " + want.display + ", now " + r.record.display);
            if (want.partition != r.record.partition) parts.push_back("partition"), diffs.push_back("recorded " + want.partition + ", now " + r.record.partition);
            if (want.ram != r.record.ram) parts.push_back("ram"), diffs.push_back("ram " + firstDifference(want.ram, r.record.ram));
            for (const auto& [name, bank] : want.inFile)
                if (r.record.inFile.count(name) == 0 || r.record.inFile.at(name) != bank)
                    parts.push_back("cards"), diffs.push_back("in-file card " + name + " differs");
            for (const auto& [key, bank] : want.written)
                if (r.record.written.count(key) == 0 || r.record.written.at(key) != bank)
                    parts.push_back("cards"), diffs.push_back("written card " + key + " differs");
            const auto text = testroms::readFile((refDir / (base + ".paper-text")).string());
            if (text ? *text != r.paperText : !r.paperText.empty())
                parts.push_back("text"), diffs.push_back("paper text " + firstDifference(linesOf(text.value_or("")), linesOf(r.paperText)));
            const auto marks = testroms::readFile((refDir / (base + ".paper-marks")).string());
            if (marks && *marks != r.paperMarks)
                parts.push_back("marks"), diffs.push_back("paper marks " + firstDifference(linesOf(*marks), linesOf(r.paperMarks)));
        }
        if (diffs.empty()) {
            std::printf("  same %s\n", rel.c_str());
            continue;
        }
        bool named = kFindings.count(rel) > 0;
        for (const std::string& part : parts)
            named = named && (" " + kFindingParts.at(rel) + " ").find(" " + part + " ") != std::string::npos;
        if (const auto it = kFindings.find(rel); named) {
            ++found;
            std::printf("  finding %s: %s\n", rel.c_str(), it->second.c_str());
            for (const auto& d : diffs) std::printf("    %s\n", d.c_str());
            continue;
        }
        ++differ;
        std::printf("  DIFFERS %s\n", rel.c_str());
        for (const auto& d : diffs) std::printf("    %s\n", d.c_str());
    }
    std::printf("  %d of %zu examples differ from the reference; %d named as findings\n", differ, all.size(), found);
    CHECK_EQ(differ, 0);
}
