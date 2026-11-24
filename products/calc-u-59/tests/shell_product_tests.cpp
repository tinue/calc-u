// What Calc-U-59 gives the shell, checked against each model's control
// state; the help address, the About texts and links; the settings dialog
// per frontend and layout; the faceplate's hit map; ROMs missing.
#include "../app/KeyIds.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../app/TIChrome.hpp"
#include "../app/TIShellProduct.hpp"
#include "AppRig.hpp"
#include "TestHarness.hpp"
#include "control/ControlState.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ValueForms.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using apprig::Rig;
namespace shell = calcu::shell;
using calcu::sessions::Value;

namespace {

bool roms() { return std::filesystem::exists("roms/TMC0571B.txt"); }
const char* const kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";

std::string describe(const shell::DialogSpec& spec) {
    std::string s;
    for (const auto& sec : spec.sections) {
        s += sec.title + ":";
        for (const auto& r : sec.rows) s += " " + r.label;
        s += ";";
    }
    return s;
}

} // namespace

TEST(shell_product_complete) {
    SKIP_UNLESS(roms(), kSkip);
    const shell::ShellProduct p = cu59::shellProduct("3.0.0", "test");
    std::vector<Value> states;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(model);
        const auto* cs = r.fe.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        CHECK(cs != nullptr);
        if (!cs) continue;
        states.push_back(cs->toValue());
        // No Edit menu; the card items on the TI-59 only.
        shell::ShellInputs in;
        in.controlState = cs->toValue();
        const auto ms = shell::menus(in, p, shell::Traits{});
        CHECK(shell::findMenu(ms, "edit") == nullptr);
        const bool ti59 = std::string(model) == "TI-59";
        CHECK((shell::findItem(ms, cu59::kItemArmCard) != nullptr) == ti59);
        CHECK((shell::findItem(ms, cu59::kItemArmFreshCard) != nullptr) == ti59);
    }
    const auto problems = shell::checkShellProduct(p, states);
    for (const auto& s : problems) std::fprintf(stderr, "  %s\n", s.c_str());
    CHECK(problems.empty());
    CHECK(!p.copyScreen);
    CHECK(!p.pasteText);
    CHECK_EQ(p.presetFilter, std::string("Presets (*.ti59 *.ti58 *.ti58c)"));
    CHECK_EQ(p.presetUploads, std::string(".ti59,.ti58,.ti58c,.yaml"));
    CHECK(p.variantLabels.empty());
}

TEST(help_address_about_texts_and_links) {
    const shell::ShellProduct p = cu59::shellProduct("3.0.0", "abc123");
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/59/"));
    const shell::AboutModel a = shell::about(p);
    CHECK_EQ(a.title, std::string("About Calc-U-59"));
    CHECK_EQ(a.name, std::string("Calc-U-59"));
    CHECK_EQ(a.version, std::string("Version 3.0.0 (abc123)"));
    CHECK(a.lines.size() == 2 && a.lines[0] == "Calc-U-59 Calculator Emulation");
    CHECK(a.lines.size() == 2 && a.lines[1].find("presets") != std::string::npos);
    CHECK_EQ(a.links.size(), std::size_t{3});
    CHECK(a.links.size() == 3 && a.links[0].second == "https://github.com/tinue/calc-u");
    CHECK(a.links.size() == 3 && a.links[1].second == "https://github.com/tinue/calc-u/issues");
    CHECK(a.links.size() == 3 && a.links[2].second == "https://github.com/tinue/calc-u/blob/main/products/calc-u-59/PRIVACY.md");
    // The page's form carries the links.
    CHECK(shell::toValue(a).find("links")->elements()->size() == 3);
}

TEST(settings_spec_per_frontend_and_layout) {
    using F = cu59::ShellFrontend;
    CHECK_EQ(describe(cu59::settingsSpec(F::Qt6, "Version 3.0.0 (x)")),
             std::string("General: Startup model Version 3.0.0 (x);Startup presets: TI-59 TI-58 TI-58C;Storage: Save folder;"
                         "Folders: Presets folder ASM overlay folder;Tracing: Trace folder Maximum file size (MB);"
                         "App log: App log level;Display: Segment Style;"));
    CHECK_EQ(describe(cu59::settingsSpec(F::WebWide, "Version 3.0.0")),
             std::string("General: Startup model Version 3.0.0;Startup presets: TI-59 TI-58 TI-58C;"
                         "Tracing: Maximum file size (MB);App log: App log level;"));
    CHECK_EQ(describe(cu59::settingsSpec(F::WebPortrait, "Version 3.0.0")),
             std::string("General: Startup model Version 3.0.0;Startup presets: TI-59 TI-58 TI-58C;"
                         "App log: App log level;Keyboard: Keyboard Feedback;"));
    // The labelled choices: the Web offers Off and Click Sound of the three values.
    const auto qt = cu59::settingsSpec(F::Qt6, "");
    CHECK(!qt.sections.empty() && qt.sections.back().rows[0].choices ==
          (std::vector<std::pair<std::string, std::string>>{{"modernized", "Modernized"}, {"classic", "Classic"}}));
    const auto portrait = cu59::settingsSpec(F::WebPortrait, "");
    CHECK(!portrait.sections.empty() && portrait.sections.back().rows[0].choices ==
          (std::vector<std::pair<std::string, std::string>>{{"off", "Off"}, {"click", "Click Sound"}}));
    // No Solid State Module row.
    for (const auto& s : qt.sections)
        for (const auto& r : s.rows) CHECK(r.key != "module");
    // The desktop's file dialogs start in the loaders' folders; the iOS
    // importer takes no start folder, the Web has no folders.
    auto folders = [](const shell::DialogSpec& spec) {
        std::string out;
        for (const auto& s : spec.sections)
            if (s.title == "Folders")
                for (const auto& r : s.rows) out += r.key + " ";
        return out;
    };
    CHECK_EQ(folders(qt), std::string("presets asm-overlays "));
    CHECK_EQ(folders(cu59::settingsSpec(F::SwiftIOS, "")), std::string());
    CHECK_EQ(folders(cu59::settingsSpec(F::WebWide, "")), std::string());
}

TEST(faceplate_hit_map) {
    const shell::HitMap map = cu59::faceplateHitMap(calcu::machine::ModelId{"TI-59"});
    CHECK_EQ(map.keys.size(), std::size_t{45});
    CHECK_EQ(map.distance, 0.0);
    CHECK(map.display.has_value());
    // At scale 1 (base image pixels): each key's centre is that key.
    for (int row = 0; row < 9; ++row)
        for (int col = 0; col < 5; ++col) {
            const cu59::NRect& k = cu59::kKeyRects[row][col];
            const double x = (k.x + k.w / 2) * cu59::kCanvasWidth;
            const double y = (cu59::kKeyboardYStart + (k.y + k.h / 2) * cu59::kKeyboardYScale) * cu59::kCanvasHeight;
            const auto hit = shell::keyAt(map, 1.0, x, y);
            CHECK(hit && *hit == *cu59::keyIdAt(cu59::matrixCode(row, col)));
        }
    // A point in the widened area (just left of 2nd's key rectangle) is that key.
    const cu59::NRect& second = cu59::kKeyRects[0][0];
    const double wideX = (second.x - second.w * 0.1) * cu59::kCanvasWidth;
    const double midY = (cu59::kKeyboardYStart + (second.y + second.h / 2) * cu59::kKeyboardYScale) * cu59::kCanvasHeight;
    CHECK(shell::keyAt(map, 1.0, wideX + 2064 * 0.0, midY).has_value());
    // A point in a gap between two columns is no key (distance 0).
    const double gapX = (cu59::kKeyRects[0][0].x + cu59::kKeyRects[0][0].w * 1.3) * cu59::kCanvasWidth;
    CHECK(!shell::keyAt(map, 1.0, gapX, midY).has_value());
    // The display: a press there commands full speed.
    CHECK(shell::onDisplay(map, 1.0, cu59::kCanvasWidth / 2, (cu59::kDisplayRect.y + 0.01) * cu59::kCanvasHeight));
    const Value v = cu59::faceplateMapValue(calcu::machine::ModelId{"TI-58"});
    CHECK(v.find("map") && v.find("card") && v.find("canvas"));
}

TEST(roms_missing_topic_without_a_machine) {
    calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    const auto paths = testroms::rigPaths(settings, "save", "");
    cu59::TIAppSetup s;
    s.settings = &settings;
    s.paths = paths.get();
    s.files = std::make_shared<apprig::Files>();
    s.roms.missing.files = {calcu::roms::FileProblem{"TMC0583.txt", calcu::roms::Problem::Missing, {}, {}}};
    const std::string expected = calcu::roms::describe(s.roms.missing);
    auto app = cu59::makeAppSession(std::move(s), testroms::hostSetup(now));
    calcu::sessions::test::TestFrontend fe;
    const auto me = app.session->listen(fe, {});
    const auto start = app.session->send(me, calcu::sessions::Command{calcu::sessions::kStartCommand, Value::object({})});
    calcu::testing::pumpFor(*app.session, now, std::chrono::milliseconds(200));
    const auto* missing = fe.mirror.state(calcu::sessions::TopicName{"roms-missing"});
    CHECK(missing != nullptr);
    CHECK(missing && shell::read::text((*missing)->toValue(), "text") == expected);
    CHECK(fe.reply(start) && !fe.reply(start)->done);   // no machine is built
    CHECK(app.session->machineSession().machine() == nullptr);
    app.session->close();
}

// The chrome: every colour the GUIs name is there; the dark set changes
// only the Web's menus and dialogs (the window is dark in both); the
// printer's width range and the text scale as the GUIs use them.
TEST(ti_chrome_complete) {
    const shell::Chrome& c = cu59::tiChrome();
    const auto problems = shell::checkChrome(c, cu59::tiChromeNames());
    for (const auto& p : problems) std::printf("  %s\n", p.c_str());
    CHECK(problems.empty());
    CHECK(c.dark.has_value());
    if (c.dark) {
        const std::vector<std::string_view> webOnly{shell::colour::kMenu, shell::colour::kMenuText,
                                                    shell::colour::kMenuBorder, shell::colour::kLink,
                                                    shell::colour::kAlert};
        CHECK_EQ(shell::css(c.dark->window), shell::css(c.light.window));
        CHECK_EQ(shell::css(c.dark->text), shell::css(c.light.text));
        for (const auto& [name, colour] : c.light.more) {
            const bool differs = shell::css(c.dark->named(name)) != shell::css(colour);
            const bool allowed = std::find(webOnly.begin(), webOnly.end(), name) != webOnly.end();
            if (differs && !allowed) std::printf("  dark changes %s\n", name.c_str());
            CHECK(!differs || allowed);
        }
    }
    CHECK(c.sizes.printerMinWidth < c.sizes.printerMaxWidth);
    CHECK_EQ(c.textScale(880), 2.0);
    CHECK(shell::toValue(c).find("sizes") != nullptr);
}
