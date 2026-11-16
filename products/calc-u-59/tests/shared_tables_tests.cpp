// Shared/ data tables: key names (against the Swift golden) (the keyboard
// map's rules moved to typing_tests.cpp, lib5_ac27_map_rules),
// 7-segment patterns, printer font, key layout and model info.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/KeyNames.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../core/PC100CPaper.hpp"
#include "../app/SevenSegment.hpp"
#include "../core/TIRomRanges.hpp"
#include "../core/TISegments.hpp"
#include "../app/TextUtil.hpp"

using namespace cu59;

TEST(keynames_match_swift_golden) {
    auto text = testroms::readFile("tests/golden/keynames.txt");
    CHECK(text.has_value());
    if (!text) return;
    int lines = 0;
    for (std::string_view line : splitLines(*text)) {
        if (line.empty()) continue;
        ++lines;
        if (line[0] == 'M') {
            auto w = splitWords(line);
            CHECK(w.size() == 4);
            if (w.size() != 4) continue;
            auto kc = static_cast<uint8_t>(*parseDec(w[1]));
            CHECK_EQ(mnemonic(kc), std::string(w[2]));
            CHECK_EQ(stepsAfter(kc), static_cast<int>(*parseDec(w[3])));
        } else if (line[0] == 'A') {
            size_t bar = line.find('|');
            std::vector<uint8_t> steps;
            for (auto t : splitWords(line.substr(2, bar - 2))) steps.push_back(static_cast<uint8_t>(*parseDec(t)));
            std::set<int> expected;
            for (auto t : splitWords(line.substr(bar + 1))) expected.insert(static_cast<int>(*parseDec(t)));
            CHECK(argumentSteps(steps) == expected);
        }
    }
    CHECK(lines > 400);
}

TEST(seven_segment_decode) {
    CHECK_EQ(segmentsFor(8, 0, false), 0x7F);
    CHECK_EQ(segmentsFor(7, 9, false), 0b0000111);  // 7 lights A B C only
    CHECK_EQ(segmentsFor(3, 0, true), 0);           // zero-suppressed
    CHECK_EQ(segmentsFor(0, 2, false), kSegmentsQuote);
    CHECK_EQ(segmentsFor(0, 4, false), kSegmentsApostrophe);
    CHECK_EQ(segmentsFor(0, 5, false), kSegmentsDegree);          // whatever the digit
    CHECK_EQ(segmentsFor(1, 5, false), kSegmentsDegree);
    CHECK_EQ(segmentsFor(4, 6, false), kSegmentsMinus);
    CHECK_EQ(segmentsFor(4, 7, false), 0);
    CHECK_EQ(segmentsFor(4, 3, false), 0);
    CHECK_EQ(segmentsFor(0xA, 0, false), kDigitSegments[0]);  // A–F digits wrap mod 10
    const uint8_t eight = segmentsFor(8, 0, false), minus = segmentsFor(0, 6, false);
    CHECK_EQ(ledCell(11, eight, 51, 0.5).segments, kSegmentsC);    // the brighter "C" covers cell 11
    CHECK_EQ(ledCell(11, eight, 51, 0.5).opacity, 0.5);
    CHECK_EQ(ledCell(11, eight, 0, 0.5).segments, kSegmentsC);     // dark: the C alone
    CHECK_EQ(ledCell(11, minus, 255, 0.06).segments, kSegmentsMinus);  // a faint C leaves a lit glyph
    CHECK_EQ(ledCell(11, minus, 255, 0.06).opacity, 1.0);
    CHECK_EQ(ledCell(10, eight, 51, 0.5).segments, 0x7F);         // other cells keep their glyph
    CHECK_EQ(ledCell(10, eight, 51, 0.5).opacity, 0.2);
    CHECK_EQ(ledCell(11, eight, 0, 0).segments, 0);                // dark: zero-suppressed
}

// LIB-8 AC-30: the shape of the font (was printer_font_shape; the font is
// the core's since M-53).
TEST(lib8_ac30_font_shape) {
    CHECK_EQ(pc100cpaper::kFont[0][3], 0x00);   // space
    CHECK_EQ(pc100cpaper::kFont[1][0], 0x0E);   // 0
    CHECK_EQ(pc100cpaper::kFont[63][6], 0x1F);  // Sigma
}

TEST(keyboard_layout_hit_test) {
    KeyHit hit{};
    const NRect& r = kKeyRects[8][4];  // '=' key
    CHECK(keyAt(r.x + r.w / 2, r.y + r.h / 2, hit));
    CHECK(hit.row == 8 && hit.col == 4);
    // Just above a key, inside the 35% vertical expansion.
    const NRect& k = kKeyRects[3][2];
    CHECK(keyAt(k.x + k.w / 2, k.y - k.h * 0.3, hit));
    CHECK(hit.row == 3 && hit.col == 2);
    CHECK(!keyAt(0.18, 0.0, hit));  // gap between columns, above row 0's expansion
}

TEST(pointer_tracker) {
    // A key's centre in canvas space.
    auto at = [](int row, int col) {
        const NRect& k = kKeyRects[row][col];
        return std::pair{k.x + k.w / 2, kKeyboardYStart + (k.y + k.h / 2) * kKeyboardYScale};
    };
    PointerTracker p;
    auto [x0, y0] = at(8, 4);
    auto c = p.move(x0, y0);
    CHECK(c.press == 44 && c.release == -1 && !c.fullSpeed);
    CHECK(p.move(x0, y0).press == -1);  // same key: nothing
    auto [x1, y1] = at(8, 3);
    c = p.move(x1, y1);  // drag to the next key
    CHECK(c.release == 44 && c.press == 43);
    c = p.move(0.5, kDisplayRect.y + kDisplayRect.h / 2);  // onto the display
    CHECK(c.fullSpeed == std::optional<bool>(true) && c.release == 43 && p.key() == -1);
    c = p.up();
    CHECK(c.fullSpeed == std::optional<bool>(false) && c.release == -1);
    p.move(x0, y0);
    c = p.move(0.5, 0.0);  // off the keyboard
    CHECK(c.release == 44 && c.press == -1 && !c.fullSpeed);
}

TEST(model_info) {
    CHECK_EQ(testroms::instructionHz(Model::TI59), 14218.75);
    CHECK_EQ(testroms::instructionHz(Model::TI58C), 12312.5);
    CHECK(info(Model::TI59).hasCardReader && !info(Model::TI58).hasCardReader);
    const auto c = romRanges(MachineVariant::TI58C), t = romRanges(MachineVariant::TI59);
    CHECK(c.scanLoop.contains(0x0A2E) && !c.scanLoop.contains(0x065A));
    CHECK(t.scanLoop.contains(0x0660));  // printer line at D0: confirm scan every pass
    CHECK(!t.idleAndKeyDetect.contains(0x0661));
}

// LIB-8 AC-30: the heights of a feed, a complete and a cut-short line, and
// the dot rules of a line cut short (was printer_strip_geometry; on the
// core's paper since M-53: the heights are what the paper's reach grows by).
TEST(lib8_ac30_feed_complete_and_cut_short_heights) {
    namespace P = pc100cpaper;
    using calcu::paper::kPerUnit;
    CHECK_EQ(P::kFeed, 70'900);   // 7.09 units
    CHECK_EQ(P::lineHeight(7, 0), P::kLine);          // complete: 10.18 units
    CHECK_EQ(P::lineHeight(3, 0), 3 * kPerUnit);      // cut short: rows only, no advance
    CHECK_EQ(P::lineHeight(3, 2), 4 * kPerUnit);      // plus the row in progress
    CHECK_EQ(P::lineHeight(6, 5), 7 * kPerUnit);      // 7 rows, but no advance
    CHECK_EQ(P::lineHeight(0, 0), 1 * kPerUnit);      // at least 1 row
    CHECK_EQ(P::drawnRows(4, 2), 5);
    // The head fires right to left: 2 columns of row 4 are its 2 rightmost.
    CHECK_EQ(P::inkedColumns(4, 2, 3), 0x1F);
    CHECK_EQ(P::inkedColumns(4, 2, 4), 0x03);
    CHECK_EQ(P::inkedColumns(4, 2, 5), 0);
    CHECK_EQ(P::inkedColumns(4, 0, 4), 0);
    CHECK_EQ(P::faintColumns(4, 2, 5, 4), 0x02);
    CHECK_EQ(P::faintColumns(4, 2, 20, 4), 0);
    CHECK_EQ(P::faintColumns(4, 2, 5, 3), 0);
    // Ink grows with the heating time: faint at first, full from 20 cycles.
    CHECK(P::inkAlpha(0) < 0.1 && P::inkAlpha(4) < 0.1);
    CHECK(P::inkAlpha(12) > 0.25 && P::inkAlpha(12) < 0.35);
    CHECK(P::inkAlpha(16) > P::inkAlpha(12));
    CHECK_EQ(P::inkAlpha(20), 1.0);
}
