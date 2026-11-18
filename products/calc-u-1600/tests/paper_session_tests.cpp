// The paper in the Calc-U-1600 app session: the papers of the two plotters in SharpAppPart's PaperPart, kept
// through what changes the machine, and the plotters' Copy.

#include <algorithm>

#include "control/ControlState.hpp"
#include <cstdint>
#include <cstdio>
#include <string>
#include <variant>
#include <vector>

#include "../app/tests/SessionFixture.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "support/RawPlotPoints.hpp"
#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "paper/Picture.hpp"
#include "paper/PaperTopics.hpp"

namespace {

using namespace sessionfixture;
using sessions::Value;
namespace c = cu1600::commands;

const calcu::paper::Paper* paperOf(SessionFixture& f, const std::string& owner) {
    for (const auto& [name, paper] : f.part().paperPart().papers())
        if (name == owner) return paper;
    return nullptr;
}
std::size_t points(SessionFixture& f, const std::string& owner) {
    const calcu::paper::Paper* p = paperOf(f, owner);
    return p ? sharp::rawPlotPoints(*p).size() : 0;
}
Value toggle(const char* p) { return Value::object({{"peripheral", p}}); }

// Types a line and gives the plotter time to draw it.
void plot(SessionFixture& f, const std::string& line, double seconds) {
    f.typeLine(line);
    f.runUntilTyped(10);
    f.runFor(seconds);
}

// ── The papers topic, and the papers kept ────────────────────────────────

TEST(papers_topic_lists_both_plotters_without_any_plugged) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    const auto* list = f.state<calcu::paper::PaperList>(calcu::paper::kPapersTopic);
    CHECK(list != nullptr);
    if (!list) return;
    CHECK_EQ(list->papers.size(), std::size_t{2});
    CHECK(list->papers.size() == 2 && list->papers[0].owner == "CE-150" && list->papers[1].owner == "CE-1600P");
    const auto* s = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    CHECK(s && s->panels.size() == 1 && !s->panels[0].owner);   // the panel shows no name without a plotter
}

TEST(ce150_paper_kept_through_detach_rom_module_and_model_switch) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    CHECK(f.toggle("CE-150"));
    plot(f, "LPRINT 123", 4);
    const std::size_t drawn = points(f, "CE-150");
    CHECK(drawn > 0);
    const calcu::paper::Reach first = paperOf(f, "CE-150")->reach();

    CHECK(f.toggle("CE-150"));   // a detach
    CHECK_EQ(points(f, "CE-150"), drawn);
    CHECK(f.ok(calcu::startup::kVariantCommand, Value::object({{"name", "A03"}})));   // a change of ROM
    CHECK_EQ(points(f, "CE-150"), drawn);
    CHECK(f.choose("slot-1", "CE-161").done);   // a module change
    CHECK_EQ(points(f, "CE-150"), drawn);
    CHECK(f.model("PC-1600"));   // a model switch
    CHECK_EQ(points(f, "CE-150"), drawn);

    // The CE-150 on the PC-1600 (MODE 1) prints on, below what it drew.
    const std::size_t marksBefore = paperOf(f, "CE-150")->marks().size();
    CHECK(f.toggle("CE-150"));
    plot(f, "MODE 1", 1);
    plot(f, "LPRINT 456", 4);
    const calcu::paper::Paper* p = paperOf(f, "CE-150");
    CHECK(points(f, "CE-150") > drawn);
    calcu::paper::Coord least = INT64_MAX;
    for (std::size_t i = marksBefore; i < p->marks().size(); ++i)
        if (const auto* s = std::get_if<calcu::paper::Stroke>(&p->marks()[i]))
            for (const auto& at : s->path) least = std::min(least, at.y);
    if (least < first.to)
        std::fprintf(stderr, "  new marks from y=%lld, the first printout reached %lld\n", static_cast<long long>(least),
                     static_cast<long long>(first.to));
    CHECK(least >= first.to);
}

TEST(ce1600p_paper_kept_through_detach_and_its_rom_variant) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.toggle("CE-1600P"));
    plot(f, "LPRINT 123", 8);
    const std::vector<sharp::RawPlotPoint> drawn = sharp::rawPlotPoints(*paperOf(f, "CE-1600P"));
    CHECK(!drawn.empty());
    // What was drawn stays as it was; the plotter's own start after the
    // rebuild may put a dot below it (its pen goes down once as it homes).
    auto kept = [&] {
        const std::vector<sharp::RawPlotPoint> now = sharp::rawPlotPoints(*paperOf(f, "CE-1600P"));
        if (now.size() < drawn.size()) return false;
        for (std::size_t i = 0; i < drawn.size(); ++i)
            if (now[i].x != drawn[i].x || now[i].y != drawn[i].y || now[i].color != drawn[i].color) return false;
        return true;
    };
    CHECK(f.ok(c::kCE1600PVariant, Value::object({{"version", "old"}})));   // its ROM variant
    CHECK(kept());
    CHECK(f.toggle("CE-1600P"));   // a detach
    CHECK(kept());
    CHECK_EQ(points(f, "CE-150"), std::size_t{0});
}

// A preset the user loads starts the paper of the plotter it plugs in
// empty.
TEST(user_preset_naming_a_plotter_starts_it_empty) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.toggle("CE-150"));
    plot(f, "LPRINT 123", 4);
    const std::size_t drawn = points(f, "CE-150");
    CHECK(drawn > 0);
    // A preset with the CE-150 that draws a little and ends.
    const fs::path preset = f.root / "short.pc1500a";
    {
        std::FILE* out = std::fopen(preset.string().c_str(), "wb");
        CHECK(out != nullptr);
        if (!out) return;
        std::fputs("format-version: 2\nmodel: PC-1500A\nprinter: CE-150\nkeys:\n  - key: cl\n  - type: LPRINT 7\n  - wait: 3s\n", out);
        std::fclose(out);
    }
    CHECK(f.ok(calcu::presets::kLoadPresetCommand, Value::object({{"ref", preset.generic_string()}})));
    const std::size_t after = points(f, "CE-150");
    CHECK(after > 0);
    CHECK(after < drawn);   // only the preset's own printout
}

// ── Copy ─────────────────────────────────────────────────────────────────

TEST(copy_of_a_plotter_is_1200_dpi) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.toggle("CE-1600P"));
    plot(f, "LPRINT 1", 4);
    const calcu::paper::Paper* p = paperOf(f, "CE-1600P");
    CHECK(p && !p->empty());
    if (!p) return;
    const calcu::paper::PictureSize s = calcu::paper::pictureSize(*p);
    CHECK_EQ(s.width, 9921);
    CHECK(s.dotsPerInch == 1200.0);
}

TEST(long_roll_lowered_never_cropped) {
    // A CE-150 roll longer than 16000 pixels at 1200 dots per inch (339 mm):
    // the picture is lowered, never cropped.
    sharp::PlotterPaperOutput out(sharp::ce150PaperDescription());
    calcu::paper::Paper paper(out.description());
    paper.bind(&out);
    out.penDown(AlpsPlotterMechanism::PenColor::Black, 0, 0);
    out.penTo(0, 4000);   // 4000 raw steps: 396 mm
    out.penUp();
    const calcu::paper::PictureSize s = calcu::paper::pictureSize(paper);
    CHECK(s.dotsPerInch < 1200.0);
    CHECK(s.height <= calcu::paper::kMaxPictureSide);
    CHECK(s.top <= 0 && s.bottom >= 4000 * calcu::paper::kPerUnit);
    paper.bind(nullptr);
}

} // namespace
