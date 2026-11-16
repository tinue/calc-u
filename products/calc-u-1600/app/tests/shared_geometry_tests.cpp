// Shared/KeyLayout/FaceplateLayout (the plotter paper's cases are
// tests/plotter_paper_tests.cpp's lib8_ac37_ cases since M-45).

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/DisplayFrame.hpp"
#include "app/LcdLayout.hpp"
#include "TestHarness.hpp"

namespace {


TEST(test_faceplate_fit) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(shared::Model::PC1600);
    CHECK(&KeyLayout::layoutFor(shared::Model::PC1500) == &KeyLayout::layoutFor(shared::Model::PC1500A));
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

} // namespace

shared::DisplayFrame frameWith(std::initializer_list<const char*> lit) {
    shared::DisplayFrame frame;
    for (const char* name : {"BUSY", "SHIFT", "KANA", "S", "BATT", "DE", "G", "RAD", "I", "II", "III"}) {
        bool on = false;
        for (const char* l : lit) on = on || std::string(l) == name;
        frame.statusSymbols.emplace_back(name, on);
    }
    return frame;
}

TEST(test_lcd_indicators) {
    using shared::lcd::Indicator;
    using shared::Model;
    // PC-1500: KANA reads JPN, DE+G is one word, the battery dot is always on.
    auto p15 = shared::lcd::indicators(Model::PC1500A, frameWith({"KANA", "DE", "G"}));
    CHECK(p15.size() == 3);
    CHECK(p15[0].text == "JPN" && p15[0].x == shared::lcd::kP15JapX);
    CHECK(p15[1].text == "DEG" && p15[1].x == shared::lcd::kP15AngleX);
    CHECK(p15[2].style == Indicator::Style::Dot && p15[2].x == shared::lcd::kP15BatteryX);
    // PC-1500: I and II each have their own place.
    CHECK(shared::lcd::indicators(Model::PC1500, frameWith({"I", "II"})).size() == 3);
    // PC-1600: S and BATT inverted, I/II/III share one place (first wins),
    // no battery dot.
    auto p16 = shared::lcd::indicators(Model::PC1600, frameWith({"S", "BATT", "II", "III"}));
    CHECK(p16.size() == 3);
    CHECK(p16[0].text == "S" && p16[0].style == Indicator::Style::Inverted);
    CHECK(p16[1].text == "BATT" && p16[1].style == Indicator::Style::Inverted);
    CHECK(p16[2].text == "II");
    CHECK(p16[0].x > shared::lcd::kP16RowLeft && p16[1].x <= shared::lcd::kP16RowRight);
    CHECK(shared::lcd::indicators(Model::PC1600, frameWith({})).empty());
}
