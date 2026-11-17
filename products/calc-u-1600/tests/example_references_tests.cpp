// C1600-4 AC-16 (design §6 T8; LIB-6 §10.8): each of the 32 examples, in
// its version 2 file, loaded through the harness until the harness `wait`
// finds the machine idle, leaves what its reference run left: the display
// as text (`display-text`), the BASIC program and the machine-code areas the
// reference's inspector views list (`peek`), the BASIC pointers and the
// paper's strokes (`paper marks`; LIB-8 AC-39). The references are the
// records of M-4 (tests/reference/v0.8.1-examples/, made with b4ea8c8's
// CLIs from the version 1 files); they are never recorded anew. The runs
// are made in a copy of examples/. Left out: the cells of a real-time
// clock and the tick counts (none of the compared ranges holds them), the
// paper's reach (the converted references hold no feeds, LIB-8 D-14).
// The intended differences are named below with their reason; the presets
// that waited for LIB-7's media (C1600-4 design §1) are compared since M-47;
// for make_diskworks_media also the files its `saveas:` wrote, against the
// ones of examples/dwx/ it made with 0.8.1, without their time stamps.
//
// Longer (`CalcU1600ExampleReferences`, CALCU1600_REFERENCES=1): about
// 80 s of runs at full speed in emulated time.
#include "../app/HarnessProduct.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace calcu::harness;

namespace {

const fs::path kRef = "tests/reference/v0.8.1-examples";

// The presets that wait for a later step (none since M-47, LIB-7 1600).
const std::map<std::string, std::string> kWaiting = {
};

// The intended differences (AC-16: "excepted by name"), what each leaves
// out of the comparison, and why.
struct Excepted {
    bool lcd = false;         // the display is not compared
    int lcdRow = -1;          // only this row of the display is not compared
    bool paperPrefix = false; // the reference's strokes are the start of the new ones
    bool memory = false;      // the memory is not compared
    const char* why;
};
const std::map<std::string, Excepted> kExcepted = {
    // M-47 finding: the last item, BLOAD, is not waited for (LIB-6 AC-60)
    // and the harness's `wait` finds the PC-1600 idle (key readiness) while
    // DWX.BIN still comes from the host drive, before DiskWorks draws the
    // banner of the reference; seconds later it shows its file list.
    {"dwx_DiskWorks.pc1600", {true, -1, false, false, "DiskWorks: idle before its banner is drawn (M-47 finding)"}},
    // conversion check §4: the program never returns to BASIC; version 1
    // ran it at full speed up to an hour, version 2 hands it over at once.
    {"machine-code_Calculat.pc1600", {true, -1, false, true, "Calculat: its program never returns to BASIC (conversion check §4)"}},
    // conversion check §4, row 1600-30: the last item is not waited for,
    // version 1 ended its load while the plot ran; `wait` lets it finish.
    {"plotter_ce150_demo.pc1500a",
     {true, -1, true, true, "ce150_demo: version 1 ended the load in the middle of the plot (conversion check §4)"}},
    // The INPUT cursor blinks; the reference caught it off.
    {"plotter_biorhythmus_1600.pc1600", {false, 1, false, false, "biorhythmus_1600: the INPUT cursor's blink phase"}},
};

struct Range {
    std::string space;
    unsigned address = 0;
    std::vector<int> bytes;   // -1: not known (a row of 00/FF only)
};

struct Reference {
    std::string preset;
    bool pc1600 = false;
    std::vector<std::string> lcd;
    std::vector<Range> ranges;
    std::vector<std::pair<unsigned, unsigned>> words;   // address, value (PC-1600 BASIC pointers, stored high byte first)
};

std::string slurp(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
}

std::vector<std::string> linesOf(const std::string& t) {
    std::vector<std::string> out;
    std::istringstream in(t);
    for (std::string l; std::getline(in, l);) out.push_back(l);
    return out;
}

std::string rtrim(std::string s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

Reference readReference(const std::string& base) {
    Reference r;
    const std::vector<std::string> meta = linesOf(slurp(kRef / (base + ".meta")));
    r.preset = meta.empty() ? std::string() : meta[0].substr(meta[0].find(": ") + 2);
    r.pc1600 = base.size() > 7 && base.substr(base.size() - 7) == ".pc1600";
    const std::vector<std::string> out = linesOf(slurp(kRef / (base + ".out")));
    for (std::size_t i = 0; i < out.size(); ++i)
        if (out[i].rfind("status:", 0) == 0) {
            const std::size_t n = r.pc1600 ? 4 : 1;
            for (std::size_t k = i - n; k < i; ++k) r.lcd.push_back(out[k]);
            break;
        }
    const std::regex row("^([0-9A-F]{4}): ((?:[0-9A-F]{2} {1,2}){15}[0-9A-F]{2})");
    const std::regex blank("^([0-9A-F]{4})[^0-9A-F:]+([0-9A-F]{4}): \\(00/FF only\\)");
    const std::regex word("^BASPRG_(?:ST|END) +\\$([0-9A-F]{4}) = \\$([0-9A-F]{4})");
    bool in = false;
    for (const std::string& l : out) {
        std::smatch m;
        if (l.rfind("── ", 0) == 0 || l.rfind("--- ", 0) == 0) {
            // The areas the reference lists: on the PC-1500 models the BASIC
            // area and the machine-language area (not the system RAM), on
            // the PC-1600 the program area.
            in = r.pc1600 ? l.rfind("--- program area", 0) == 0
                          : l.find("BASIC area ") != std::string::npos || l.find("Machine-language area") != std::string::npos;
            if (in) r.ranges.push_back(Range{r.pc1600 ? "memory" : "me0", 0, {}});
            continue;
        }
        if (r.pc1600 && std::regex_search(l, m, word))
            r.words.push_back({static_cast<unsigned>(std::stoul(m[1], nullptr, 16)),
                               static_cast<unsigned>(std::stoul(m[2], nullptr, 16))});
        if (!in) continue;
        Range& cur = r.ranges.back();
        const auto put = [&](unsigned at, int value) {
            if (cur.bytes.empty()) cur.address = at;
            while (cur.address + cur.bytes.size() < at) cur.bytes.push_back(-1);
            cur.bytes.push_back(value);
        };
        if (std::regex_search(l, m, row)) {
            const unsigned at = static_cast<unsigned>(std::stoul(m[1], nullptr, 16));
            std::istringstream hex(m[2].str());
            std::string b;
            unsigned k = 0;
            while (hex >> b) put(at + k++, static_cast<int>(std::stoul(b, nullptr, 16)));
        } else if (std::regex_search(l, m, blank)) {
            const unsigned from = static_cast<unsigned>(std::stoul(m[1], nullptr, 16));
            const unsigned to = static_cast<unsigned>(std::stoul(m[2], nullptr, 16));
            for (unsigned a = from; a <= to; ++a) put(a, -1);
        } else if (!cur.bytes.empty()) {
            in = false;
        }
    }
    return r;
}

// The strokes of a paper-marks text, without its reach.
std::vector<std::string> strokes(const std::string& text) {
    std::vector<std::string> out;
    for (const std::string& l : linesOf(text))
        if (l.rfind("stroke ", 0) == 0) out.push_back(l);
    return out;
}

std::vector<std::string> body(const std::string& reply) {
    std::vector<std::string> l = linesOf(reply);
    if (!l.empty()) l.erase(l.begin());   // "done <n>"
    return l;
}

// The differences of one example, one sentence each.
std::vector<std::string> compare(const std::string& base, const fs::path& copy, std::size_t* bytes) {
    std::vector<std::string> d;
    const Reference ref = readReference(base);
    const auto ex = kExcepted.find(base);
    const Excepted none{false, -1, false, false, ""};
    const Excepted& e = ex == kExcepted.end() ? none : ex->second;

    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    if (!server) return {"cannot start: " + error};
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    const fs::path preset = copy / ref.preset;
    const std::string loaded = reply("preset " + preset.generic_string());
    if (loaded.rfind("done", 0) != 0) return {"the preset was refused: " + loaded};
    reply("wait");

    if (!e.lcd) {
        std::vector<std::string> rows;
        for (const std::string& l : body(reply("display-text"))) {
            if (l.rfind("indicators:", 0) == 0 || l.rfind("not text:", 0) == 0) break;
            rows.push_back(l);
        }
        for (std::size_t i = 0; i < std::max(rows.size(), ref.lcd.size()); ++i) {
            if (static_cast<int>(i) == e.lcdRow) continue;
            const std::string a = i < ref.lcd.size() ? rtrim(ref.lcd[i]) : std::string("<none>");
            const std::string b = i < rows.size() ? rtrim(rows[i]) : std::string("<none>");
            if (a != b) d.push_back("display row " + std::to_string(i + 1) + ": \"" + b + "\", the reference \"" + a + "\"");
        }
    }
    for (const auto& entry : fs::directory_iterator(kRef)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind(base + ".", 0) != 0 || name.size() < 12 || name.substr(name.size() - 12) != ".paper-marks") continue;
        const std::string plotter = name.substr(base.size() + 1, name.size() - base.size() - 13);
        const std::vector<std::string> want = strokes(slurp(entry.path()));
        const std::vector<std::string> got = strokes(reply("paper marks " + plotter));
        // A plot cut off at the end of version 1's load: its strokes start
        // the new ones, the last of them cut short.
        bool same = got == want;
        if (e.paperPrefix && !same && !want.empty() && got.size() >= want.size())
            same = std::equal(want.begin(), want.end() - 1, got.begin()) &&
                   got[want.size() - 1].rfind(want.back(), 0) == 0;
        if (!same)
            d.push_back("paper of the " + plotter + ": " + std::to_string(got.size()) + " strokes, the reference " +
                        std::to_string(want.size()));
    }
    if (!e.memory) {
        const char* target = ref.pc1600 ? "SC7852" : "LH5801";
        auto peek = [&](const std::string& space, unsigned at, std::size_t n) {
            char a[16];
            std::snprintf(a, sizeof a, "&%04X", at);
            std::vector<int> v;
            for (const std::string& l : body(reply(std::string("peek ") + target + " " + space + " " + a + " " + std::to_string(n))))
                v.push_back(static_cast<int>(std::stoul(l.substr(l.find(": ") + 2), nullptr, 16)));
            return v;
        };
        for (const Range& r : ref.ranges) {
            if (r.bytes.empty()) continue;
            *bytes += r.bytes.size();
            const std::vector<int> got = peek(r.space, r.address, r.bytes.size());
            int diffs = 0;
            unsigned first = 0;
            for (std::size_t i = 0; i < r.bytes.size() && i < got.size(); ++i) {
                const int want = r.bytes[i];
                const bool ok = want < 0 ? (got[i] == 0x00 || got[i] == 0xFF) : got[i] == want;
                if (!ok && diffs++ == 0) first = r.address + static_cast<unsigned>(i);
            }
            if (got.size() != r.bytes.size()) diffs = static_cast<int>(r.bytes.size());
            if (diffs) {
                char t[96];
                std::snprintf(t, sizeof t, "memory %04X-%04X: %d bytes differ, the first at %04X", r.address,
                              r.address + static_cast<unsigned>(r.bytes.size()) - 1, diffs, first);
                d.push_back(t);
            }
        }
        for (const auto& [at, value] : ref.words) {
            const std::vector<int> w = peek("memory", at, 2);
            const unsigned got = w.size() == 2 ? static_cast<unsigned>((w[0] << 8) | w[1]) : 0x10000u;
            if (got != value) {
                char t[64];
                std::snprintf(t, sizeof t, "BASIC pointer at %04X: %04X, the reference %04X", at, got, value);
                d.push_back(t);
            }
        }
    }
    server->finish();
    // The media make_diskworks_media saves next to itself (LIB-7 1600, M-47):
    // the module and the disk as the ones of examples/dwx/ (made with 0.8.1,
    // the disk since upgraded to version 2), but their time stamps.
    if (base == "dwx_make_diskworks_media.pc1600")
        for (const char* file : {"CE-1601M - Progs.module.yaml", "Progs.floppy.yaml"}) {
            // Left out besides the time stamps: the comments (a module file
            // keeps those of the bundled CE-1601M it was spliced into, which
            // were edited since) and the disk's description, which a save now
            // keeps (LIB-7 AC-41; 0.8.1's preset save dropped it).
            auto stamped = [](const std::string& l) {
                return l.rfind("saved:", 0) == 0 || l.rfind("#", 0) == 0 || l.rfind("description:", 0) == 0;
            };
            std::vector<std::string> want, got;
            for (const std::string& l : linesOf(slurp(fs::path("examples/dwx") / file)))
                if (!stamped(l)) want.push_back(rtrim(l));
            for (const std::string& l : linesOf(slurp(copy / "examples/dwx" / file)))
                if (!stamped(l)) got.push_back(rtrim(l));
            // The content holds time stamps of its own (the directory entries
            // of the files SAVE wrote: the harness's clock is not the CLI's of
            // 0.8.1): rows of bytes may differ, nothing else.
            const auto isRow = [](const std::string& l) { return l.find("$") != std::string::npos && l.find(": ") != std::string::npos; };
            int rows = 0;
            bool same = got.size() == want.size();
            for (std::size_t i = 0; same && i < got.size(); ++i)
                if (got[i] != want[i]) {
                    same = isRow(got[i]) && isRow(want[i]) && got[i].substr(0, got[i].find(": ")) == want[i].substr(0, want[i].find(": "));
                    ++rows;
                }
            std::printf("  %s: saved %s, %d rows of bytes differ (time stamps)\n", base.c_str(), file, rows);
            if (!same) d.push_back(std::string("saved ") + file + ": differs from examples/dwx beyond rows of bytes");
        }
    return d;
}

} // namespace

TEST(c1600_4_ac16_examples_match_their_references) {
    SKIP_UNLESS(std::getenv("CALCU1600_REFERENCES") != nullptr, "the reference runs are Longer (CalcU1600ExampleReferences)");
    SKIP_UNLESS(fs::exists("roms/PC1600-P0-B0-new.bin") && fs::exists("roms/PC-1500_A04.ROM"),
                "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder tmp("c1600-references");
    const fs::path copy = tmp.path();
    fs::copy("examples", copy / "examples", fs::copy_options::recursive);
    const char* only = std::getenv("CALCU1600_REFERENCES_ONLY");
    int compared = 0;
    for (const auto& entry : fs::directory_iterator(kRef)) {
        const std::string name = entry.path().filename().string();
        if (name.size() < 5 || name.substr(name.size() - 5) != ".meta") continue;
        const std::string base = name.substr(0, name.size() - 5);
        if (only && base.find(only) == std::string::npos) continue;
        if (const auto w = kWaiting.find(base); w != kWaiting.end()) {
            std::printf("  %s: waits: %s\n", base.c_str(), w->second.c_str());
            continue;
        }
        std::size_t bytes = 0;
        const std::vector<std::string> d = compare(base, copy, &bytes);
        std::printf("  %s: %zu bytes of memory compared\n", base.c_str(), bytes);
        if (const auto e = kExcepted.find(base); e != kExcepted.end()) std::printf("  %s: excepted: %s\n", base.c_str(), e->second.why);
        for (const std::string& x : d) std::printf("  %s: %s\n", base.c_str(), x.c_str());
        CHECK(d.empty());
        ++compared;
    }
    std::printf("  %d examples compared\n", compared);
}
