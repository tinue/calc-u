// app/KeyLayout/FaceplateLayout (the plotter paper's cases are
// in tests/plotter_paper_tests.cpp).

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#include "app/KeyLayout/FaceplateLayout.hpp"
#include "machine/Display.hpp"
#include "app/LcdLayout.hpp"
#include "TestHarness.hpp"

namespace {


TEST(test_faceplate_fit) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(calcu::machine::ModelId{"PC-1600"});
    CHECK(&KeyLayout::layoutFor(calcu::machine::ModelId{"PC-1500"}) == &KeyLayout::layoutFor(calcu::machine::ModelId{"PC-1500A"}));
    CHECK(!KeyLayout::fitLayout(l, 0, 100).art.valid());

    // A pane twice as wide as the art's aspect: letterboxed horizontally.
    const int h = 400;
    const int w = static_cast<int>(2 * h * l.imageWidth / l.imageHeight);
    const KeyLayout::FittedLayout f = KeyLayout::fitLayout(l, w, h);
    CHECK(f.art.h == h);
    CHECK(f.art.y == 0);
    CHECK(f.art.x > 0 && f.art.x + f.art.w <= w);
    CHECK(f.keys.size() == l.keyCount);
    CHECK(f.lcd.valid());

    // Every key's centre hits that key (or one listed earlier that overlaps).
    bool allHit = true;
    for (std::size_t i = 0; i < f.keys.size(); ++i) {
        const auto& r = f.keys[i];
        if (!r.valid()) continue;
        const int idx = KeyLayout::hitTest(f, r.x + r.w / 2, r.y + r.h / 2);
        if (idx < 0 || idx > static_cast<int>(i)) allHit = false;
    }
    CHECK(allHit);
    CHECK(KeyLayout::hitTest(f, 0, 0) == -1);  // letterbox margin
    CHECK(KeyLayout::keyIndex(l, "enter") >= 0);
    CHECK(KeyLayout::keyIndex(l, "no-such-key") == -1);
    CHECK(KeyLayout::heightForWidth(l, 1000) == static_cast<int>(1000 * (l.imageHeight / l.imageWidth)));
}

// The PC-1600's two SHIFT keys are one machine key: the one pressed is drawn
// pressed, the top one when no press was on either (typing).
TEST(test_faceplate_held_key_of_press) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(calcu::machine::ModelId{"PC-1600"});
    std::vector<int> shifts;
    for (std::size_t i = 0; i < l.keyCount; ++i)
        if (std::string(l.keys[i].name) == "shift") shifts.push_back(static_cast<int>(i));
    CHECK(shifts.size() == 2);
    if (shifts.size() != 2) return;
    const KeyLayout::KeyRect& bottom = l.keys[shifts[1]];
    CHECK(bottom.y > l.keys[shifts[0]].y);
    CHECK_EQ(KeyLayout::heldKeyIndex(l, "shift", bottom.x + bottom.w / 2, bottom.y + bottom.h / 2), shifts[1]);
    CHECK_EQ(KeyLayout::heldKeyIndex(l, "shift", -1, -1), shifts[0]);
    const KeyLayout::KeyRect& q = l.keys[KeyLayout::keyIndex(l, "q")];
    CHECK_EQ(KeyLayout::heldKeyIndex(l, "shift", q.x + 1, q.y + 1), shifts[0]);
    CHECK_EQ(KeyLayout::heldKeyIndex(l, "no-such-key", 0, 0), -1);
}

} // namespace

calcu::machine::DisplayOutput frameWith(std::initializer_list<const char*> lit) {
    calcu::machine::DisplayOutput frame;
    for (const char* name : {"BUSY", "SHIFT", "KANA", "S", "BATT", "DE", "G", "RAD", "I", "II", "III"}) {
        bool on = false;
        for (const char* l : lit) on = on || std::string(l) == name;
        frame.indicators.push_back({name, on, std::nullopt});
    }
    return frame;
}

TEST(test_lcd_indicators) {
    using cu1600::lcd::Indicator;
    // PC-1500: KANA reads JPN, DE+G is one word, the battery dot is always on.
    auto p15 = cu1600::lcd::indicators(calcu::machine::ModelId{"PC-1500A"}, frameWith({"KANA", "DE", "G"}));
    CHECK(p15.size() == 3);
    CHECK(p15[0].text == "JPN" && p15[0].x == cu1600::lcd::kP15JapX);
    CHECK(p15[1].text == "DEG" && p15[1].x == cu1600::lcd::kP15AngleX);
    CHECK(p15[2].style == Indicator::Style::Dot && p15[2].x == cu1600::lcd::kP15BatteryX);
    // PC-1500: I and II each have their own place.
    CHECK(cu1600::lcd::indicators(calcu::machine::ModelId{"PC-1500"}, frameWith({"I", "II"})).size() == 3);
    // PC-1600: S and BATT inverted, I/II/III share one place (first wins),
    // no battery dot.
    auto p16 = cu1600::lcd::indicators(calcu::machine::ModelId{"PC-1600"}, frameWith({"S", "BATT", "II", "III"}));
    CHECK(p16.size() == 3);
    CHECK(p16[0].text == "S" && p16[0].style == Indicator::Style::Inverted);
    CHECK(p16[1].text == "BATT" && p16[1].style == Indicator::Style::Inverted);
    CHECK(p16[2].text == "II");
    CHECK(p16[0].x > cu1600::lcd::kP16RowLeft && p16[1].x <= cu1600::lcd::kP16RowRight);
    CHECK(cu1600::lcd::indicators(calcu::machine::ModelId{"PC-1600"}, frameWith({})).empty());
}
