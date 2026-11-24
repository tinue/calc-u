// Help and About in the model; the drawn dialog, the log and a help address
// that cannot be opened are the Qt6 cases of
// library/qt6/tests/shell_widgets_tests.cpp.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;

TEST(help_opens_product_address) {
    const auto p = st::product();
    const auto ms = menus(st::inputs(), p, Traits{});
    const MenuItem* help = findItem(ms, actions::kHelp);
    CHECK(help != nullptr);
    CHECK_EQ(help->label, std::string("Test-Product Help"));
    CHECK(help->command.value.empty());   // a shell action: it opens p.helpUrl
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/test/"));
    // Help works also when the product could not start.
    CHECK(findItem(menus(ShellInputs{}, p, Traits{}), actions::kHelp) != nullptr);
}

TEST(about_shows_product_texts) {
    auto p = st::product();
    AboutModel a = about(p);
    CHECK_EQ(a.title, std::string("About Test-Product"));
    CHECK_EQ(a.name, std::string("Test-Product"));
    CHECK_EQ(a.version, std::string("Version 1.2.3 (abc123)"));
    CHECK_EQ(a.lines.size(), std::size_t{2});
    CHECK_EQ(a.lines[0], std::string("A product made for the tests."));
    CHECK_EQ(a.links.size(), std::size_t{2});
    CHECK_EQ(a.links[1].second, std::string("https://example.org/privacy"));
    p.aboutLabel = "About Test-Product…";   // a product's label with an ellipsis
    const auto ms = menus(st::inputs(), p, Traits{});
    CHECK_EQ(findItem(ms, actions::kAbout)->label, std::string("About Test-Product…"));
    CHECK_EQ(about(p).title, std::string("About Test-Product"));
    p.build.clear();
    CHECK_EQ(about(p).version, std::string("Version 1.2.3"));
}

TEST(about_support_rows_and_copied_text) {
    auto p = st::product();
    CHECK(about(p).sections.empty());   // no build time, no host rows, no display: no section
    CHECK_EQ(aboutText(about(p)),
             std::string("Test-Product\nVersion 1.2.3 (abc123)\nA product made for the tests.\nNo code of a product.\n"));
    p.buildTime = "2026-11-24T11:49:08Z";
    const AboutModel a = about(p, {{"OS", "Darwin arm64 27.2.0"}, {"Hardware", ""}, {"Browser", "Safari 27.2"}});
    CHECK_EQ(a.sections.size(), std::size_t{1});
    CHECK_EQ(a.sections[0].heading, std::string("Support Information"));
    const SupportRows& support = a.sections[0].rows;
    CHECK_EQ(support.size(), std::size_t{3});   // the empty Hardware is left out
    CHECK_EQ(support[0].first, std::string("Built"));
    CHECK_EQ(aboutText(a), std::string("Test-Product\nVersion 1.2.3 (abc123)\nA product made for the tests.\n"
                                       "No code of a product.\n\nSupport Information\nBuilt: 2026-11-24T11:49:08Z\n"
                                       "OS: Darwin arm64 27.2.0\nBrowser: Safari 27.2\n"));
    CHECK(aboutText(a).find("example.org") == std::string::npos);   // no links
    p.package = "DMG";   // first, before Built
    const AboutModel b = about(p, {{"OS", "Darwin arm64 27.2.0"}});
    CHECK_EQ(b.sections[0].rows.size(), std::size_t{3});
    CHECK_EQ(b.sections[0].rows[0].first + ": " + b.sections[0].rows[0].second, std::string("Package: DMG"));
    CHECK_EQ(b.sections[0].rows[1].first, std::string("Built"));
}

namespace {

std::string rowsText(const SupportRows& rows) {
    std::string out;
    for (const auto& [label, value] : rows) out += label + ": " + value + "\n";
    return out;
}

} // namespace

// A Mac's "looks like 2560 × 1440" mode on a 4K panel: drawn at 5120 × 2880,
// shown on 3840 × 2160.
TEST(display_rows_with_a_panel_of_its_own) {
    DisplayFacts d;
    d.screen = "EV2785";
    d.reported = {2560, 1440};
    d.scale = 2;
    d.panel = {3840, 2160};
    d.window = {1340, 757};
    CHECK_EQ(rowsText(displayRows(d)), std::string("Screen: EV2785\nReported: 2560 \xC3\x97 1440\nScale: 2\n"
                                                   "Actual: 5120 \xC3\x97 2880\nPanel: 3840 \xC3\x97 2160\n"
                                                   "Window: 1340 \xC3\x97 757\n"));
    d.panel = {5120, 2880};   // the panel draws what it is given: no Panel row
    CHECK(rowsText(displayRows(d)).find("Panel") == std::string::npos);
}

TEST(display_rows_fractional_scale_and_windowing) {
    DisplayFacts d;
    d.windowing = "X11 on Wayland";
    d.reported = {1920, 1080};
    d.scale = 1.25;
    d.window = {900, 700};
    CHECK_EQ(rowsText(displayRows(d)), std::string("Windowing: X11 on Wayland\nReported: 1920 \xC3\x97 1080\n"
                                                   "Scale: 1.25\nActual: 2400 \xC3\x97 1350\nWindow: 900 \xC3\x97 700\n"));
    d = {};
    d.reported = {1180, 820};
    d.scale = 1.5;   // one decimal, no trailing zero
    CHECK(rowsText(displayRows(d)).find("Scale: 1.5\n") != std::string::npos);
    CHECK(displayRows(DisplayFacts{}).empty());
}

// The Display section comes after Support Information, and Copy has it.
TEST(about_display_section) {
    auto p = st::product();
    DisplayFacts d;
    d.reported = {1180, 820};
    d.scale = 2;
    d.window = {1180, 735};
    const AboutModel a = about(p, {}, d);   // no support rows: only Display
    CHECK_EQ(a.sections.size(), std::size_t{1});
    CHECK_EQ(a.sections[0].heading, std::string("Display"));
    p.buildTime = "2026-11-24T11:49:08Z";
    const AboutModel b = about(p, {}, d);
    CHECK_EQ(b.sections.size(), std::size_t{2});
    CHECK_EQ(aboutText(b), std::string("Test-Product\nVersion 1.2.3 (abc123)\nA product made for the tests.\n"
                                       "No code of a product.\n\nSupport Information\nBuilt: 2026-11-24T11:49:08Z\n"
                                       "\nDisplay\nReported: 1180 \xC3\x97 820\nScale: 2\nActual: 2360 \xC3\x97 1640\n"
                                       "Window: 1180 \xC3\x97 735\n"));
}
