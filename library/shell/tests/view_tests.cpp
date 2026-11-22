// The view rule (shell/View), the same cases under the same names in
// library/web/tests/shell_model.test.js. The faceplates are the products':
// the TI-59's tall one (2064 × 4075) with its column's margins and bar,
// the PC-1500's wide one (4000 × 1760) with its bar; a desktop window has
// the Web's menu bar above them.
#include "TestHarness.hpp"
#include "shell/View.hpp"

using namespace calcu;
using shell::ViewSide;

namespace {
constexpr double kMenu = 26;

shell::ViewInputs tall(double w, double h, bool mobile) {
    shell::ViewInputs in;
    in.width = w;
    in.height = h;
    in.faceplateAspect = 2064.0 / 4075.0;
    in.fixedWidth = 16;
    in.fixedHeight = 50 + 16 + (mobile ? 0 : kMenu);
    in.panelsMinWidth = 256;
    in.panelsMinHeight = 0;
    in.mobile = mobile;
    return in;
}

shell::ViewInputs wide(double w, double h, bool mobile) {
    shell::ViewInputs in;
    in.width = w;
    in.height = h;
    in.faceplateAspect = 4000.0 / 1760.0;
    in.fixedHeight = 40 + (mobile ? 0 : kMenu);
    in.panelsMinWidth = 0;
    in.panelsMinHeight = 200;
    in.mobile = mobile;
    in.panelsAloneOnMobileBelow = true;
    return in;
}
} // namespace

// The table of the rule: TI-59 (tall faceplate).
TEST(view_tall_desktop_ordinary) {
    const auto v = shell::view(tall(1400, 900, false));
    CHECK(v.side == ViewSide::Beside);
    CHECK(v.panelsFit);
    CHECK(!v.panelsAlone);
}

TEST(view_tall_desktop_narrow) {
    const auto v = shell::view(tall(400, 900, false));
    CHECK(v.side == ViewSide::Below);
    CHECK(!v.panelsFit);   // no panels below: the pages
}

TEST(view_tall_phone_portrait) {
    const auto v = shell::view(tall(393, 852, true));
    CHECK(v.side == ViewSide::Below);
    CHECK(!v.panelsFit);
    CHECK(!v.panelsAlone);
}

TEST(view_tall_phone_landscape) {
    const auto v = shell::view(tall(852, 393, true));
    CHECK(v.side == ViewSide::Beside);
    CHECK(v.panelsFit);
}

// PC-1500/1600 (wide faceplate).
TEST(view_wide_desktop_ordinary) {
    const auto v = shell::view(wide(1400, 900, false));
    CHECK(v.side == ViewSide::Below);
    CHECK(v.panelsFit);
    CHECK(!v.panelsAlone);   // desktop: never the panels alone
}

TEST(view_wide_desktop_narrow) {
    const auto v = shell::view(wide(600, 900, false));
    CHECK(v.side == ViewSide::Below);
    CHECK(v.panelsFit);
}

TEST(view_wide_phone_portrait) {
    const auto v = shell::view(wide(393, 852, true));
    CHECK(v.side == ViewSide::Below);
    CHECK(v.panelsAlone);
}

TEST(view_wide_phone_landscape) {
    const auto v = shell::view(wide(852, 393, true));
    CHECK(v.side == ViewSide::Beside);
    CHECK(!v.panelsFit);   // no panels beside: the calculator alone
    CHECK(!v.panelsAlone);
}

// A phone in landscape whose safe area narrows the window: the room is
// below, but only a sliver of it; the calculator stays.
TEST(view_wide_phone_landscape_safe_area) {
    const auto v = shell::view(wide(838, 440, true));
    CHECK(v.side == ViewSide::Below);
    CHECK(!v.panelsFit);
    CHECK(!v.panelsAlone);
}

// The faceplate fills the box less the fixed parts in one direction, its
// proportions kept; the room is what is left in the other.
TEST(view_faceplate_fitted) {
    shell::ViewInputs in;
    in.width = 1000;
    in.height = 540;
    in.faceplateAspect = 2;
    in.fixedWidth = 0;
    in.fixedHeight = 40;
    auto v = shell::view(in);
    CHECK_EQ(v.faceplateWidth, 1000.0);
    CHECK_EQ(v.faceplateHeight, 500.0);
    CHECK(v.side == ViewSide::Below);
    CHECK_EQ(v.room, 0.0);
    in.width = 1300;
    v = shell::view(in);
    CHECK(v.side == ViewSide::Beside);
    CHECK_EQ(v.faceplateWidth, 1000.0);
    CHECK_EQ(v.room, 300.0);
}

// A minimum of 0: no panels on that side, however much room.
TEST(view_no_panels_on_a_side) {
    shell::ViewInputs in;
    in.width = 4000;
    in.height = 500;
    in.faceplateAspect = 2;
    in.panelsMinWidth = 0;
    in.panelsMinHeight = 100;
    CHECK(!shell::view(in).panelsFit);
    in.panelsMinWidth = 100;
    CHECK(shell::view(in).panelsFit);
}

// The debugger beside the panels: beside the TI-59 where the room takes
// both and the window is high enough; below the PC-1500 across the
// window's width; never in the panels alone.
TEST(view_debugger_fits) {
    auto in = tall(1400, 900, false);
    in.debuggerMinWidth = 220;
    in.debuggerMinHeight = 560;
    CHECK(shell::view(in).debuggerFits);
    in.height = 500;   // too low
    CHECK(!shell::view(in).debuggerFits);
    in = tall(800, 900, false);   // room for the printer only
    in.debuggerMinWidth = 220;
    CHECK(shell::view(in).panelsFit);
    CHECK(!shell::view(in).debuggerFits);
    auto w = wide(1000, 900, false);
    w.debuggerMinWidth = 700;
    CHECK(shell::view(w).debuggerFits);
    w.width = 600;
    CHECK(!shell::view(w).debuggerFits);
    w = wide(1000, 1400, true);   // mobile, the panels alone
    w.debuggerMinWidth = 700;
    CHECK(shell::view(w).panelsAlone);
    CHECK(!shell::view(w).debuggerFits);
}
