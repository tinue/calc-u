// LIB-11 design T8, T17: the display as text (AC-9 lcd, AC-10) and as the
// machine gives it (AC-23).
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include "harness/DisplayText.hpp"
#include <string>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;
using namespace calcu::machine;
namespace mt = calcu::machine::test;

namespace {
// A frame of `cols` × 1 cells of 6 × 8 with the glyph columns drawn in.
DotMatrixFrame frameOf(const std::vector<std::array<std::uint8_t, 8>>& cells, int rows = 1) {
    DotMatrixFrame f{static_cast<int>(cells.size()) * 6 / rows, 8 * rows, {}};
    f.dots.assign(static_cast<std::size_t>(f.width * f.height), 0);
    const int perRow = static_cast<int>(cells.size()) / rows;
    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
        for (int c = 0; c < 6; ++c)
            for (int r = 0; r < 8; ++r)
                if (cells[std::size_t(i)][std::size_t(c)] & (1u << r))
                    f.dots[std::size_t(((i / perRow) * 8 + r) * f.width + (i % perRow) * 6 + c)] = 1;
    return f;
}
const std::vector<IndicatorDef> kDefs = {{"RUN", false}, {"BUSY", false}, {"DIM", true}};
} // namespace

TEST(ac10_dot_matrix_rows_indicators_and_cells_not_text) {
    const GlyphFont font = testFont();
    const auto a = font.glyphs[0].columns, b = font.glyphs[1].columns, c = font.glyphs[2].columns;
    const std::array<std::uint8_t, 8> blank{}, junk{0x01, 0, 0, 0, 0, 0x02, 0, 0};
    const DotMatrixFrame f = frameOf({a, blank, b, junk, c, blank, c, a}, 2);
    const std::vector<IndicatorOutput> ind = {{"RUN", true, std::nullopt}, {"BUSY", false, std::nullopt}, {"DIM", true, 0.25f}};
    const DisplayText t = readDotMatrix(f, font, ind, kDefs);
    CHECK(t.rows == (std::vector<std::string>{"A B" + std::string(kNotText), "C CA"}));
    CHECK_EQ(t.notText, 1);
    CHECK_EQ(t.indicators.size(), std::size_t{2});
    if (t.indicators.size() == 2) {
        CHECK(t.indicators[0].first == "RUN" && !t.indicators[0].second);
        CHECK(t.indicators[1].first == "DIM" && t.indicators[1].second == std::optional<Decimal>(Decimal{250, 3}));
    }
    CHECK(lcdLines(t) == (std::vector<std::string>{"A B" + std::string(kNotText), "C CA", "indicators: RUN DIM", "not text: 1"}));
    CHECK_EQ(writeJson(lcdValue(t)),
             std::string("{\"on\":true,\"rows\":[\"A B\xEF\xBF\xBD\",\"C CA\"],\"indicators\":[{\"name\":\"RUN\",\"brightness\":null},"
                         "{\"name\":\"DIM\",\"brightness\":0.250}],\"notText\":1}"));
}

TEST(ac10_inverted_glyph_is_text) {
    const GlyphFont font = testFont();
    std::array<std::uint8_t, 8> inv{};
    for (int i = 0; i < 6; ++i) inv[std::size_t(i)] = static_cast<std::uint8_t>(~font.glyphs[1].columns[std::size_t(i)]);
    const DisplayText t = readDotMatrix(frameOf({inv, font.glyphs[0].columns}), font, {}, kDefs);
    CHECK(t.rows == std::vector<std::string>{"BA"});
    CHECK_EQ(t.notText, 0);
}

TEST(ac10_segment_positions_left_to_right_by_product_rule) {
    SegmentLayout layout{{{"A", false}, {"B", false}}};
    SegmentFrame f;
    for (int i = 0; i < 3; ++i) f.positions.push_back(SegmentPosition{{i == 0, i == 1}, 1.0f, {}});
    SegmentRule rule = [](const SegmentPosition& p, const SegmentLayout&) {
        return p.lit[0] ? std::string("a") : p.lit[1] ? std::string("b") : std::string();
    };
    CHECK(readSegments(f, layout, rule, false, {}, {}).rows == std::vector<std::string>{"ab "});
    CHECK(readSegments(f, layout, rule, true, {}, {}).rows == std::vector<std::string>{" ba"});   // position 0 rightmost
    // The test product's rule on test-seg: 3 positions, the digits, the indicator C.
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 1ms");                // key ready: C on
    run.line("key-down 1");              // a press; a key down lights DP of position 0
    run.line("wait 1ms");                // the display topic is read when time passes
    CHECK_EQ(run.line("lcd"), std::string("done 2\n1.01\nindicators: C\n"));
}

TEST(ac10_display_off_says_so) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("key-down off");
    run.line("wait 1ms");
    CHECK_EQ(run.line("lcd"), std::string("done 1\ndisplay off\n"));
    RunOptions o;
    o.json = true;
    TestRun json(o, mt::kDot);
    CHECK(json.server != nullptr);
    if (!json.server) return;
    json.line("key-down off");
    json.line("wait 1ms");
    CHECK_EQ(json.line("lcd"), std::string("{\"done\":true,\"result\":{\"on\":false,\"rows\":[],\"indicators\":[],\"notText\":null},"
                                           "\"reason\":null}\n"));
}

TEST(ac9_lcd_reads_display_topic) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // test-dot's dots are no glyph of the test font: each lit cell is not text.
    run.line("wait 1ms");
    const std::string r = run.line("lcd");
    CHECK(r.rfind("done 3\n", 0) == 0);
    CHECK(r.find("indicators:") != std::string::npos && r.find("not text: ") != std::string::npos);
    // What lcd shows is the display topic's last state: unchanged without a wait.
    run.machine().advance(1000);   // behind the session's back
    CHECK_EQ(run.line("lcd"), r);
}

// ── display (T17) ───────────────────────────────────────────────────────

TEST(ac23_dot_matrix_rows_of_ones_and_zeros) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 64us");   // not a unit: refused, nothing passes
    run.line("wait 1ms");    // 1000 ticks: row 0's dot at (1000 / 16) % 12 = 2
    const std::string r = run.line("display");
    CHECK(r.rfind("done 11\n001000000000\n", 0) == 0);   // 8 rows, indicators, names, on
    CHECK(r.find("\n000000000000\n000000000000\n100000000000\n") != std::string::npos);   // row 3: no press yet
}

TEST(ac23_segment_positions) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("display"), std::string("done 7\nsegments: A B C D E F G DP\n0 11111100\n1 11111100\n2 11111100\n"
                                              "indicators: 0\nnames: C\non: true\n"));
}

TEST(ac23_indicator_bits_in_descriptor_order) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("key-down a");   // BUSY on; RUN on until key readiness
    run.line("wait 0.001ms");   // one instruction: the display is read
    const std::string r = run.line("display");
    CHECK(r.find("indicators: 11\nnames: RUN BUSY\non: true\n") != std::string::npos);
}

TEST(ac23_display_off) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("key-down off");
    run.line("wait 0.001ms");   // one instruction: the display is read
    CHECK(run.line("display").find("indicators: 00\nnames: RUN BUSY\non: false\n") != std::string::npos);
}

TEST(ac23_json_arrays) {
    RunOptions o;
    o.json = true;
    TestRun run(o);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("display"),
             std::string("{\"done\":true,\"result\":{\"kind\":\"segments\",\"segments\":[\"A\",\"B\",\"C\",\"D\",\"E\",\"F\",\"G\",\"DP\"],"
                         "\"positions\":[[1,1,1,1,1,1,0,0],[1,1,1,1,1,1,0,0],[1,1,1,1,1,1,0,0]],\"indicators\":[0],"
                         "\"indicatorNames\":[\"C\"],\"on\":true},\"reason\":null}\n"));
    TestRun dot(o, mt::kDot);
    CHECK(dot.server != nullptr);
    if (!dot.server) return;
    const std::string r = dot.line("display");
    CHECK(r.find("\"kind\":\"dots\",\"rows\":[[1,0,0,0,0,0,0,0,0,0,0,0],") != std::string::npos);
    CHECK(r.find("\"indicators\":[1,0],\"indicatorNames\":[\"RUN\",\"BUSY\"],\"on\":true") != std::string::npos);
}
