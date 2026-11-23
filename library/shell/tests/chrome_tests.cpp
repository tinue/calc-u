// The chrome's values and their Value form (shell/Chrome).
#include "TestHarness.hpp"
#include "sessions/Value.hpp"
#include "shell/Chrome.hpp"

using namespace calcu;
using sessions::Value;

TEST(chrome_colour_css) {
    CHECK_EQ(shell::css(shell::Colour::rgb(0x1d1d1c)), std::string("#1d1d1c"));
    CHECK_EQ(shell::css(shell::Colour::grey(0.25)), std::string("#404040"));
    CHECK_EQ(shell::css(shell::Colour{0, 0, 0, 140}), std::string("rgba(0, 0, 0, 0.549)"));
}

TEST(chrome_text_scale) {
    shell::Chrome c;
    CHECK_EQ(c.textScale(900), 1.0);   // no rule: never scales
    c.sizes.textScaleWidth = 440;
    c.sizes.textScaleMax = 2;
    CHECK_EQ(c.textScale(300), 1.0);
    CHECK_EQ(c.textScale(660), 1.5);
    CHECK_EQ(c.textScale(2000), 2.0);
}

TEST(chrome_paper_row_compact) {
    shell::ChromeSizes s;
    CHECK(shell::paperRowCompact(s, 279));    // the narrow tape: short labels
    CHECK(!shell::paperRowCompact(s, 280));
    CHECK(!shell::paperRowCompact(s, 400));
    s.paperRowCompactWidth = 0;               // never
    CHECK(!shell::paperRowCompact(s, 100));
}

TEST(chrome_value_form) {
    shell::Chrome c;
    c.light.pill = shell::Colour::grey(0.25);
    c.light.more = {{"debug-header", shell::Colour::grey(0.07)}};
    c.sizes.panelHeaderHeight = 36;
    const Value v = shell::toValue(c);
    CHECK(v.find("dark") == nullptr);
    CHECK_EQ(*v.find("light")->find("pill")->string(), std::string("#404040"));
    CHECK_EQ(*v.find("light")->find("debug-header")->string(), std::string("#121212"));
    CHECK_EQ(*v.find("sizes")->find("panelHeaderHeight")->real(), 36.0);
    CHECK_EQ(*v.find("sizes")->find("pillRadius")->real(), 6.0);
    CHECK_EQ(*v.find("sizes")->find("paperRowCompactWidth")->real(), 280.0);
    c.dark = c.light;
    CHECK(shell::toValue(c).find("dark") != nullptr);
}

TEST(chrome_check_names) {
    shell::Chrome c;
    c.light.more = {{"a", {}}};
    c.dark = shell::ChromeColours{};
    c.dark->more = {{"b", {}}};
    const auto problems = shell::checkChrome(c, {"a"});
    CHECK_EQ(problems.size(), std::size_t{3});   // dark lacks a (twice: named, of light), light lacks b
    CHECK_EQ(c.light.named("x"), (shell::Colour{255, 0, 255, 255}));
}
