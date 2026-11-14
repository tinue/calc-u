#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <type_traits>
#include <variant>

// LIB-1 AC-15, AC-16, AC-17, AC-18, AC-43 (design §6, T9).

using namespace calcu::machine;
using namespace calcu::machine::test;

TEST(ac15_segments_and_brightness_per_position_and_dp) {
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    runUntilKeyReady(*m);
    m->keyDown(KeyId{"1"});   // position 0 shows the presses so far, 1; the DP is lit while a key is down
    const DisplayOutput out = m->readDisplay();
    const auto* f = std::get_if<SegmentFrame>(&out.frame);
    CHECK(f != nullptr);
    if (!f) return;
    const auto& layout = std::get<SegmentGeometry>(m->descriptor().display.geometry).layout;
    CHECK_EQ(f->positions.size(), std::size_t{3});
    int wrongShape = 0;
    for (const auto& p : f->positions) {
        if (p.lit.size() != layout.segments.size() || p.segmentBrightness.size() != layout.segments.size()) ++wrongShape;
        if (!p.brightness || *p.brightness < 0.0f || *p.brightness > 1.0f) ++wrongShape;
        for (std::size_t s = 0; s < layout.segments.size(); ++s)
            if (p.segmentBrightness[s].has_value() != layout.segments[s].ownBrightness) ++wrongShape;
    }
    CHECK_EQ(wrongShape, 0);
    // A B C D E F G DP: "1" lights B and C, and the DP is a segment.
    const auto& p0 = f->positions[0];
    CHECK(p0.lit == std::vector<bool>({false, true, true, false, false, false, false, true}));
    CHECK(p0.brightness == 1.0f);
    CHECK(p0.segmentBrightness[7] == 0.5f);   // the DP's own brightness, driven separately
    m->keyUp(KeyId{"1"});
    const auto f2 = std::get<SegmentFrame>(m->readDisplay().frame);
    CHECK(!f2.positions[0].lit[7] && f2.positions[0].segmentBrightness[7] == 0.0f);
}

TEST(ac16_dots_of_descriptor_size_in_cells) {
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    m->advance(16 * 5);   // the moving dot is at column 5 of row 0
    const DisplayOutput out = m->readDisplay();
    const auto* f = std::get_if<DotMatrixFrame>(&out.frame);
    CHECK(f != nullptr);
    if (!f) return;
    const auto& g = std::get<DotMatrixGeometry>(m->descriptor().display.geometry);
    CHECK(f->width == g.width && f->height == g.height);
    CHECK_EQ(f->dots.size(), std::size_t(g.width * g.height));
    CHECK(g.cells && g.cells->cellWidth * g.cells->columns == g.width && g.cells->cellHeight * g.cells->rows == g.height);
    CHECK_EQ(f->dots[0 * 12 + 5], std::uint8_t{1});
    CHECK_EQ(f->dots[0 * 12 + 4], std::uint8_t{0});
}

TEST(ac17_indicators_by_name_with_state) {
    auto seg = buildTestMachine(kSeg);
    auto dot = buildTestMachine(kDot);
    CHECK(seg && dot);
    if (!seg || !dot) return;
    auto names = [](const DisplayOutput& o, const MachineDescriptor& d) {
        if (o.indicators.size() != d.display.indicators.size()) return false;
        for (std::size_t i = 0; i < o.indicators.size(); ++i)
            if (o.indicators[i].name != d.display.indicators[i].name) return false;
        return true;
    };
    // test-dot: RUN while the start-up runs, BUSY while a key is down; no brightness.
    DisplayOutput o = dot->readDisplay();
    CHECK(names(o, dot->descriptor()));
    CHECK(o.indicators[0].name == "RUN" && o.indicators[0].on && !o.indicators[0].brightness);
    CHECK(o.indicators[1].name == "BUSY" && !o.indicators[1].on);
    runUntilKeyReady(*dot);
    dot->keyDown(KeyId{"a"});
    o = dot->readDisplay();
    CHECK(!o.indicators[0].on && o.indicators[1].on);
    // test-seg: C, with a brightness of its own, on at key readiness.
    o = seg->readDisplay();
    CHECK(names(o, seg->descriptor()));
    CHECK(o.indicators[0].name == "C" && !o.indicators[0].on && o.indicators[0].brightness == 0.0f);
    runUntilKeyReady(*seg);
    o = seg->readDisplay();
    CHECK(o.indicators[0].on && o.indicators[0].brightness == 1.0f);
}

TEST(ac18_output_has_no_host_pixels_glyphs_colours) {
    // The shape of the types: segments are flags and duties, dots are 0 or 1
    // in the descriptor's own geometry, indicators are names with a state.
    // No member carries a host pixel format, a glyph style or a colour (and
    // review checks that none is added).
    static_assert(std::is_same_v<decltype(DotMatrixFrame::dots), std::vector<std::uint8_t>>);
    static_assert(std::is_same_v<decltype(SegmentPosition::lit), std::vector<bool>>);
    static_assert(std::is_same_v<decltype(SegmentPosition::brightness), std::optional<float>>);
    static_assert(std::is_same_v<decltype(IndicatorOutput::on), bool>);
    static_assert(std::variant_size_v<decltype(DisplayOutput::frame)> == 2);
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    m->keyDown(KeyId{"a"});
    m->advance(5'000);
    const auto f = std::get<DotMatrixFrame>(m->readDisplay().frame);
    int other = 0;
    for (auto d : f.dots) if (d > 1) ++other;
    CHECK_EQ(other, 0);
}

TEST(ac43_display_off_shows_nothing) {
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    runUntilKeyReady(*m);
    m->keyDown(KeyId{"a"});
    CHECK(m->readDisplay().on);
    m->keyDown(KeyId{"off"});   // the key `off` switches the display off
    const DisplayOutput off = m->readDisplay();
    CHECK(!off.on);
    const auto f = std::get<DotMatrixFrame>(off.frame);
    int lit = 0;
    for (auto d : f.dots) lit += d;
    for (const auto& i : off.indicators) if (i.on) ++lit;
    CHECK_EQ(lit, 0);
    CHECK_EQ(f.dots.size(), std::size_t{96});   // the shape stays
    m->keyUp(KeyId{"off"});
    m->keyDown(KeyId{"on"});
    CHECK(m->readDisplay().on);
    // A segment display that is never switched off says so too.
    auto seg = buildTestMachine(kSeg);
    CHECK(seg && seg->readDisplay().on);
}
