// The plots of the reference runs of 0.8.1
// (tests/reference/v0.8.1-examples/*.paper) turned once into the replies of
// `paper marks`, which the example reference test compares new runs with. CALCU1600_PAPER_REPLIES=1 writes the reply files next to the
// references (`<example>.<plotter>.paper-marks`, one per plotter block); the
// Frequent case checks that the committed files equal what the converter
// gives, so a change to the converter is seen.
//
// The rule: a stroke starts at each point with newStroke 1,
// in the pen's colour (black, blue (0, 0, 255), dark green (0, 128, 0), red
// (255, 0, 0)); its positions are (printableLeft + x, y) in units of
// 190/1920 mm, the margin rounded once to four decimals: 66.9474
// (CE-150), 101.0526 (CE-1600P). The reach is from the least to the
// greatest y of the points; the files hold no feeds. A plotter
// has no text layer, so there is no `.paper-text`.
#include "TestHarness.hpp"
#include "paper/Replies.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace calcu::paper;

namespace {

const fs::path kDir = "tests/reference/v0.8.1-examples";
constexpr Rgb kPens[4] = {{0, 0, 0}, {0, 0, 255}, {0, 128, 0}, {255, 0, 0}};

std::optional<Coord> printableLeft(const std::string& plotter) {
    if (plotter == "CE-150") return 669'474;
    if (plotter == "CE-1600P") return 1'010'526;
    return std::nullopt;
}

struct Block {
    std::string plotter;
    std::vector<std::string> lines;
    std::string error;
};

std::vector<Block> convert(const fs::path& file) {
    std::ifstream in(file);
    std::vector<Block> out;
    std::string line;
    while (std::getline(in, line)) {
        Block b;
        std::istringstream head(line);
        std::string word, points;
        long n = -1;
        head >> word >> b.plotter >> points >> n;
        const auto left = printableLeft(b.plotter);
        if (word != "plotter" || points != "points" || n < 0 || !left) {
            b.error = "not a plotter header: " + line;
            out.push_back(std::move(b));
            return out;
        }
        std::vector<Mark> marks;
        Coord lo = 0, hi = 0;
        for (long i = 0; i < n && std::getline(in, line); ++i) {
            std::istringstream p(line);
            long x = 0, y = 0, colour = 0, newStroke = 0;
            if (!(p >> x >> y >> colour >> newStroke)) {
                b.error = "not a point: " + line;
                break;
            }
            const Position at{*left + x * kPerUnit, y * kPerUnit};
            if (newStroke || marks.empty()) marks.push_back(Stroke{kPens[((colour % 4) + 4) % 4], {}});
            std::get<Stroke>(marks.back()).path.push_back(at);
            lo = i == 0 ? at.y : std::min(lo, at.y);
            hi = i == 0 ? at.y : std::max(hi, at.y);
        }
        b.lines = marksReply(Reach{lo, hi}, marks);
        out.push_back(std::move(b));
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

TEST(sharp_plots_turned_into_paper_replies) {
    SKIP_UNLESS(fs::is_directory(kDir), "the reference runs are not here");
    const char* env = std::getenv("CALCU1600_PAPER_REPLIES");
    const bool write = env && std::string(env) == "1";
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(kDir))
        if (e.path().extension() == ".paper") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    CHECK_EQ(files.size(), std::size_t(14));   // the 14 examples with a plotter
    std::set<std::string> expected;
    for (const fs::path& f : files) {
        for (const Block& b : convert(f)) {
            CHECK_EQ(b.error, std::string());
            if (!b.error.empty()) {
                std::fprintf(stderr, "  in %s\n", f.filename().string().c_str());
                continue;
            }
            const std::string name = f.stem().string() + "." + b.plotter + ".paper-marks";
            expected.insert(name);
            const std::string text = joined(b.lines);
            if (write) {
                std::ofstream(kDir / name, std::ios::binary | std::ios::trunc) << text;
                continue;
            }
            const bool same = readAll(kDir / name) == text;
            CHECK(same);
            if (!same) std::fprintf(stderr, "  %s differs from the converter's output\n", name.c_str());
        }
    }
    for (const auto& e : fs::directory_iterator(kDir)) {
        if (e.path().extension() != ".paper-marks") continue;
        const bool known = expected.count(e.path().filename().string()) != 0;
        CHECK(known);
        if (!known) std::fprintf(stderr, "  %s has no reference\n", e.path().filename().string().c_str());
    }
}
