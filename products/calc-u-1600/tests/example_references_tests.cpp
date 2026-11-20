// Each bundled example (every preset of examples/), loaded through the
// harness until the harness `wait` finds the machine idle, leaves what its
// reference run left: the rows of the display (`display-text`), the BASIC
// pointers (`c1600 inspect pointers`), the BASIC area and on the PC-1500
// models the machine-language area (`c1600 inspect dump-basic`,
// `dump-ml`), and each plotter's strokes (`paper marks`). A reference is
// tests/references/examples/<folder>_<name>.<ext>.reference, the replies
// of those commands, and <…>.<plotter>.paper-marks. The runs are made in
// a copy of examples/; the harness runs in emulated time only, so a run
// gives the same replies each time. Left out: the paper's reach (the
// strokes are compared). For make_diskworks_media also the files its
// `saveas:` wrote, against the ones of examples/dwx/, without their time
// stamps.
//
// Longer (`CalcU1600ReferenceRuns`, CALCU1600_REFERENCE_COMPARE=1).
// CALCU1600_REFERENCE_RECORD=<folder> writes the references of every
// example into that folder (a draft); it never writes into
// tests/references/. CALCU1600_REFERENCE_ONLY=<text> limits both to the
// examples whose name holds the text.
#include "../app/HarnessProduct.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace calcu::harness;

namespace {

const fs::path kRef = "tests/references/examples";
const char* const kPlotters[] = {"CE-150", "CE-1600P"};

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
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r')) s.pop_back();
    return s;
}

std::vector<std::string> body(const std::string& reply) {
    std::vector<std::string> l = linesOf(reply);
    if (!l.empty()) l.erase(l.begin());   // "done <n>"
    return l;
}

// The presets of examples/, by path, sorted.
std::vector<std::string> examplePresets() {
    std::vector<std::string> out;
    for (const auto& e : fs::recursive_directory_iterator("examples")) {
        const std::string ext = e.path().extension().string();
        if (ext == ".pc1500" || ext == ".pc1500a" || ext == ".pc1600") out.push_back(e.path().generic_string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

// examples/plotter/globus.pc1600 -> plotter_globus.pc1600
std::string baseOf(const std::string& preset) {
    std::string b = preset.substr(std::string("examples/").size());
    std::replace(b.begin(), b.end(), '/', '_');
    return b;
}

bool is1600(const std::string& base) { return base.size() > 7 && base.substr(base.size() - 7) == ".pc1600"; }

// The commands whose replies a reference holds, each a section `=== <command>`.
std::vector<std::string> commandsOf(const std::string& base) {
    std::vector<std::string> c = {"display-text", "c1600 inspect pointers", "c1600 inspect dump-basic"};
    if (!is1600(base)) c.push_back("c1600 inspect dump-ml");
    return c;
}

struct Range {
    unsigned address = 0;
    std::vector<int> bytes;   // -1: a row of 00/FF only
};

struct Reference {
    std::vector<std::string> lcd;
    std::vector<Range> ranges;
    std::vector<std::pair<unsigned, unsigned>> words;   // BASPRG_ST / BASPRG_END: address, value (high byte first)
};

Reference parse(const std::string& text) {
    Reference r;
    const std::regex row("^([0-9A-F]{4}): ((?:[0-9A-F]{2} {1,2}){15}[0-9A-F]{2})");
    const std::regex blank("^([0-9A-F]{4})[^0-9A-F:]+([0-9A-F]{4}): \\(00/FF only\\)");
    const std::regex word("^│ BASPRG_(?:ST|END) +│ ([0-9A-F]{4}) │ ([0-9A-F]{4})");
    std::string section;
    bool rows = true;
    for (const std::string& l : linesOf(text)) {
        std::smatch m;
        if (l.rfind("=== ", 0) == 0) {
            section = l.substr(4);
            continue;
        }
        if (section == "display-text") {
            if (l.rfind("indicators:", 0) == 0 || l.rfind("not text:", 0) == 0 || l == "display off") rows = false;
            if (rows) r.lcd.push_back(rtrim(l));
        } else if (section == "c1600 inspect pointers") {
            if (std::regex_search(l, m, word))
                r.words.push_back({static_cast<unsigned>(std::stoul(m[1], nullptr, 16)),
                                   static_cast<unsigned>(std::stoul(m[2], nullptr, 16))});
        } else if (section.rfind("c1600 inspect dump-", 0) == 0) {
            if (l.rfind("── ", 0) == 0) {   // a heading: a range of its own
                r.ranges.push_back(Range{});
                continue;
            }
            if (r.ranges.empty()) continue;
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
            }
        }
    }
    r.ranges.erase(std::remove_if(r.ranges.begin(), r.ranges.end(), [](const Range& x) { return x.bytes.empty(); }),
                   r.ranges.end());
    return r;
}

// The strokes of a paper-marks text, without its reach.
std::vector<std::string> strokes(const std::string& text) {
    std::vector<std::string> out;
    for (const std::string& l : linesOf(text))
        if (l.rfind("stroke ", 0) == 0) out.push_back(l);
    return out;
}

// A run of the example: the preset, the bare `wait`, then `then` with the server.
template <class F> std::string run(const std::string& preset, const fs::path& copy, F then) {
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    if (!server) return "cannot start: " + error;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    const std::string loaded = reply("preset " + (copy / preset).generic_string());
    if (loaded.rfind("done", 0) != 0) return "the preset was refused: " + loaded;
    reply("wait");
    then(reply);
    server->finish();
    return {};
}

// Writes the references of one example into `folder`.
std::string record(const std::string& preset, const fs::path& copy, const fs::path& folder) {
    const std::string base = baseOf(preset);
    return run(preset, copy, [&](auto& reply) {
        std::string text = "preset: " + preset + "\n";
        for (const std::string& c : commandsOf(base)) {
            text += "=== " + c + "\n";
            for (const std::string& l : body(reply(c))) text += rtrim(l) + "\n";
        }
        std::ofstream(folder / (base + ".reference"), std::ios::binary) << text;
        for (const char* plotter : kPlotters) {
            const std::string marks = reply(std::string("paper marks ") + plotter);
            if (marks.rfind("done", 0) != 0 || strokes(marks).empty()) continue;
            std::string t;
            for (const std::string& l : body(marks)) t += l + "\n";
            std::ofstream(folder / (base + "." + plotter + ".paper-marks"), std::ios::binary) << t;
        }
    });
}

// The differences of one example, one sentence each.
std::vector<std::string> compare(const std::string& preset, const fs::path& copy, std::size_t* bytes) {
    const std::string base = baseOf(preset);
    std::vector<std::string> d;
    if (!fs::exists(kRef / (base + ".reference"))) return {"no reference"};
    const Reference ref = parse(slurp(kRef / (base + ".reference")));
    const std::string failed = run(preset, copy, [&](auto& reply) {
        std::string text;
        for (const std::string& c : commandsOf(base)) {
            text += "=== " + c + "\n";
            for (const std::string& l : body(reply(c))) text += l + "\n";
        }
        const Reference now = parse(text);
        for (std::size_t i = 0; i < std::max(now.lcd.size(), ref.lcd.size()); ++i) {
            const std::string a = i < ref.lcd.size() ? ref.lcd[i] : std::string("<none>");
            const std::string b = i < now.lcd.size() ? now.lcd[i] : std::string("<none>");
            if (a != b) d.push_back("display row " + std::to_string(i + 1) + ": \"" + b + "\", the reference \"" + a + "\"");
        }
        for (const char* plotter : kPlotters) {
            const fs::path file = kRef / (base + "." + plotter + ".paper-marks");
            const std::vector<std::string> want = fs::exists(file) ? strokes(slurp(file)) : std::vector<std::string>{};
            const std::string marks = reply(std::string("paper marks ") + plotter);
            const std::vector<std::string> got = marks.rfind("done", 0) == 0 ? strokes(marks) : std::vector<std::string>{};
            if (got != want)
                d.push_back(std::string("paper of the ") + plotter + ": " + std::to_string(got.size()) +
                            " strokes, the reference " + std::to_string(want.size()));
        }
        if (now.ranges.size() != ref.ranges.size()) d.push_back("the areas differ in number");
        for (std::size_t k = 0; k < ref.ranges.size() && k < now.ranges.size(); ++k) {
            const Range& r = ref.ranges[k];
            const Range& g = now.ranges[k];
            *bytes += r.bytes.size();
            int diffs = 0;
            unsigned first = 0;
            for (std::size_t i = 0; i < r.bytes.size(); ++i) {
                const int got = i < g.bytes.size() && g.address == r.address ? g.bytes[i] : -2;
                if (got != r.bytes[i] && diffs++ == 0) first = r.address + static_cast<unsigned>(i);
            }
            if (g.bytes.size() != r.bytes.size()) diffs = std::max(diffs, 1);
            if (diffs) {
                char t[96];
                std::snprintf(t, sizeof t, "memory %04X-%04X: %d bytes differ, the first at %04X", r.address,
                              r.address + static_cast<unsigned>(r.bytes.size()) - 1, diffs, first);
                d.push_back(t);
            }
        }
        if (now.words != ref.words) d.push_back("the BASIC pointers differ");
    });
    if (!failed.empty()) return {failed};
    // The media make_diskworks_media saves next to itself: the module and
    // the disk as the ones of examples/dwx/, but their time stamps.
    if (base == "dwx_make_diskworks_media.pc1600")
        for (const char* file : {"CE-1601M - Progs.module.yaml", "Progs.floppy.yaml"}) {
            // Left out besides the time stamps: the comments (a module file
            // keeps those of the bundled CE-1601M it was spliced into) and the
            // disk's description, which a save keeps.
            auto stamped = [](const std::string& l) {
                return l.rfind("saved:", 0) == 0 || l.rfind("#", 0) == 0 || l.rfind("description:", 0) == 0;
            };
            std::vector<std::string> want, got;
            for (const std::string& l : linesOf(slurp(fs::path("examples/dwx") / file)))
                if (!stamped(l)) want.push_back(rtrim(l));
            for (const std::string& l : linesOf(slurp(copy / "examples/dwx" / file)))
                if (!stamped(l)) got.push_back(rtrim(l));
            // The content holds time stamps of its own (the directory entries
            // of the files SAVE wrote, by the harness's clock): rows of bytes
            // may differ, nothing else.
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

TEST(examples_match_their_references) {
    const char* draft = std::getenv("CALCU1600_REFERENCE_RECORD");
    SKIP_UNLESS(draft || std::getenv("CALCU1600_REFERENCE_COMPARE") != nullptr,
                "the reference runs are Longer (CalcU1600ReferenceRuns)");
    SKIP_UNLESS(fs::exists("roms/PC1600-P0-B0-new.bin") && fs::exists("roms/PC-1500_A04.ROM"),
                "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder tmp("c1600-references");
    const fs::path copy = tmp.path();
    fs::copy("examples", copy / "examples", fs::copy_options::recursive);
    if (draft) fs::create_directories(draft);
    const char* only = std::getenv("CALCU1600_REFERENCE_ONLY");
    int done = 0;
    for (const std::string& preset : examplePresets()) {
        const std::string base = baseOf(preset);
        if (only && base.find(only) == std::string::npos) continue;
        if (draft) {
            const std::string failed = record(preset, copy, draft);
            if (!failed.empty()) std::printf("  %s: %s\n", base.c_str(), failed.c_str());
            CHECK(failed.empty());
            ++done;
            continue;
        }
        std::size_t bytes = 0;
        const std::vector<std::string> d = compare(preset, copy, &bytes);
        std::printf("  %s: %zu bytes of memory compared\n", base.c_str(), bytes);
        for (const std::string& x : d) std::printf("  %s: %s\n", base.c_str(), x.c_str());
        CHECK(d.empty());
        ++done;
    }
    std::printf("  %d examples %s\n", done, draft ? "recorded" : "compared");
}
