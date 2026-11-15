// LIB-8 AC-39 (design §4.1, T12): the printouts of the reference runs of
// 2.3.0 (tests/reference/v2.3.0-examples/, M-9) turned once into the replies
// of `paper text` and `paper marks`, which C59-2 AC-11 (M-54) compares new
// runs with. CALCU59_PAPER_REPLIES=1 writes the reply files next to the
// references; the Frequent case checks that the committed files equal what
// the converter gives, so a change to the converter is seen.
//
// The rule (AC-25; the placement of 2.3.0's frontends/qt6/app/PrinterWidget.cpp:
// 52-62): per printed line, the text between the bars is a text line at
// (0, top). An empty line is a feed: 7.09 units, no mark. Any other line is
// split into the 20 strings of the PC-100C's table (longest match), each
// mapped to its code and drawn in the 5 × 7 font (app/PrinterFont.hpp
// kPC100CFont): the dot of column c, row r of character i at
// (i × 6.53 + c + 0.5, top + r + 0.5), full darkness (20 heating cycles);
// the line takes 10.18 units. The reach is 0 to the sum of the heights.
// The three printer-interrupt examples print lines cut short whose rows the
// M-9 files do not hold: they get a `.paper-text` only (D-13).
#include "TestHarness.hpp"
#include "../app/PrinterFont.hpp"
#include "paper/Replies.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace calcu::paper;

namespace {

const fs::path kDir = "tests/reference/v2.3.0-examples";

// The PC-100C's characters by code, as core/PC100C.cpp's PRN_CODE (a table
// of the core's own, not exported; copied here for the converter).
const char* const kCodes[64] = {
    " ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E",
    "-", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    ".", "U", "V", "W", "X", "Y", "Z", "+", "x", "*", "√", "π", "e", "(", ")", ",",
    "↑", "%", "⇄", "/", "=", "'", "ˣ", "x̄", "²", "?", "÷", "!", "Ⅱ", "▴", "∏", "∑"};

const std::set<std::string> kCutShort = {"debug_printer-interrupt-bits", "debug_printer-interrupt-sweep",
                                         "printer-interrupt"};

constexpr Coord kFeed = 70'900, kLine = 101'800, kCharStep = 65'300;
constexpr std::uint8_t kFull = 20;

// The 20 codes of a printed line, longest match first; empty when it does not split.
std::vector<int> split(const std::string& text) {
    std::vector<int> codes;
    std::size_t at = 0;
    while (at < text.size()) {
        int best = -1;
        std::size_t bestLen = 0;
        for (int c = 0; c < 64; ++c) {
            const std::string s = kCodes[c];
            if (s.size() > bestLen && text.compare(at, s.size(), s) == 0) {
                best = c;
                bestLen = s.size();
            }
        }
        if (best < 0) return {};
        codes.push_back(best);
        at += bestLen;
    }
    return codes.size() == 20 ? codes : std::vector<int>{};
}

struct Converted {
    std::vector<std::string> text, marks;
    std::string error;
};

Converted convert(const std::vector<std::string>& printed) {
    Converted out;
    std::vector<Mark> marks;
    std::vector<TextLine> lines;
    Coord top = 0;
    for (const std::string& raw : printed) {
        if (raw.size() < 2 || raw.front() != '|' || raw.back() != '|') {
            out.error = "not a printed line: " + raw;
            return out;
        }
        const std::string text = raw.substr(1, raw.size() - 2);
        lines.push_back(TextLine{text, Position{0, top}});
        if (text.empty()) {
            top += kFeed;
            continue;
        }
        const std::vector<int> codes = split(text);
        if (codes.empty()) {
            out.error = "does not split into the 20 characters of the PC-100C: |" + text + "|";
            return out;
        }
        for (std::size_t i = 0; i < codes.size(); ++i)
            for (int r = 0; r < 7; ++r)
                for (int c = 0; c < 5; ++c)
                    if (cu59::kPC100CFont[codes[i]][r] & (0x10 >> c))
                        marks.push_back(Dot{Position{static_cast<Coord>(i) * kCharStep + c * kPerUnit + 5'000,
                                                     top + r * kPerUnit + 5'000},
                                            kFull});
        top += kLine;
    }
    out.text = textReply(lines);
    out.marks = marksReply(Reach{0, top}, marks);
    return out;
}

// The printed lines of a reference file, or none.
std::vector<std::string> printedLines(const fs::path& file) {
    std::ifstream in(file);
    std::vector<std::string> out;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("--- printed (", 0) != 0) continue;
        const int n = std::atoi(line.c_str() + 13);
        for (int i = 0; i < n && std::getline(in, line); ++i) out.push_back(line);
        break;
    }
    return out;
}

std::string joined(const std::vector<std::string>& lines) {
    std::string s;
    for (const auto& l : lines) s += l + "\n";
    return s;
}

std::string readAll(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

} // namespace

TEST(lib8_ac39_ti_printouts_turned_into_paper_replies) {
    SKIP_UNLESS(fs::is_directory(kDir), "the reference runs are not here");
    const bool write = std::getenv("CALCU59_PAPER_REPLIES") && std::string(std::getenv("CALCU59_PAPER_REPLIES")) == "1";
    std::set<std::string> expected;
    int printing = 0;
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(kDir))
        if (e.path().extension() == ".txt") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const fs::path& f : files) {
        const std::vector<std::string> printed = printedLines(f);
        if (printed.empty()) continue;
        ++printing;
        const std::string stem = f.stem().string();   // characters.ti59
        const std::string base = stem.substr(0, stem.find('.'));
        const Converted c = convert(printed);
        CHECK_EQ(c.error, std::string());
        if (!c.error.empty()) {
            std::fprintf(stderr, "  in %s\n", f.filename().string().c_str());
            continue;
        }
        std::vector<std::pair<std::string, std::string>> replies{{stem + ".paper-text", joined(c.text)}};
        if (!kCutShort.count(base)) replies.push_back({stem + ".paper-marks", joined(c.marks)});
        for (const auto& [name, text] : replies) {
            expected.insert(name);
            if (write) {
                std::ofstream(kDir / name, std::ios::binary | std::ios::trunc) << text;
                continue;
            }
            const bool same = readAll(kDir / name) == text;
            CHECK(same);
            if (!same) std::fprintf(stderr, "  %s differs from the converter's output\n", name.c_str());
        }
    }
    CHECK(printing >= 30);   // the examples that print (32 at 2.3.0)
    // No reply file without its reference.
    for (const auto& e : fs::directory_iterator(kDir)) {
        const std::string ext = e.path().extension().string();
        if (ext != ".paper-text" && ext != ".paper-marks") continue;
        const bool known = expected.count(e.path().filename().string()) != 0;
        CHECK(known);
        if (!known) std::fprintf(stderr, "  %s has no reference\n", e.path().filename().string().c_str());
    }
}
