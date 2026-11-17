// LIB-14 Calc-U-59 column Q1 (design §10.6, §10.9): what Calc-U-59 gives
// the shell, checked against each model's control state; the help address,
// the About texts and links (AC-32); the settings dialog per frontend and
// layout (AC-30); the faceplate's hit map (AC-24); ROMs missing (AC-19).
#include "../app/KeyIds.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../app/TIShellProduct.hpp"
#include "AppRig.hpp"
#include "TestHarness.hpp"
#include "control/ControlState.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ValueForms.hpp"
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

TEST(lib14_ac26_shell_product_complete) {
    SKIP_UNLESS(roms(), kSkip);
    const shell::ShellProduct p = cu59::shellProduct("3.0.0", "test");
    std::vector<Value> states;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(model);
        const auto* cs = r.fe.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        CHECK(cs != nullptr);
        if (!cs) continue;
        states.push_back(cs->toValue());
        // AC-26: no Edit menu; the card items on the TI-59 only (LIB-4 AC-12).
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
    CHECK(p.variantLabels.empty());
}

TEST(lib14_ac32_help_address_about_texts_and_links) {
    const shell::ShellProduct p = cu59::shellProduct("3.0.0", "abc123");
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/59/"));
    const shell::AboutModel a = shell::about(p);
    CHECK_EQ(a.title, std::string("About Calc-U-59"));
    CHECK_EQ(a.name, std::string("Calc-U-59"));
    CHECK_EQ(a.version, std::string("Version 3.0.0 (abc123)"));
    CHECK(a.lines.size() == 2 && a.lines[0] == "Calc-U-59 Calculator Emulation");
    CHECK(a.lines.size() == 2 && a.lines[1].find("presets") != std::string::npos);
    CHECK_EQ(a.links.size(), std::size_t{3});
    CHECK(a.links.size() == 3 && a.links[0].second == "https://github.com/tinue/Calc-U-59");
    CHECK(a.links.size() == 3 && a.links[1].second == "https://github.com/tinue/Calc-U-59/issues");
    CHECK(a.links.size() == 3 && a.links[2].second == "https://github.com/tinue/Calc-U-59/blob/main/PRIVACY.md");
    // The page's form carries the links and says there is no Copy Screen.
    const Value v = shell::toValue(p);
    CHECK(v.find("aboutLinks") != nullptr);
    CHECK(!*v.find("copyScreen")->boolean());
}

TEST(lib14_ac30_settings_spec_per_frontend_and_layout) {
    using F = cu59::ShellFrontend;
    CHECK_EQ(describe(cu59::settingsSpec(F::Qt6, "Version 3.0.0 (x)")),
             std::string("General: Startup model Version 3.0.0 (x);Startup presets: TI-59 TI-58 TI-58C;Storage: Save folder;"
                         "Folders: Preset folder;Tracing: Trace folder Maximum file size (MB);Display: Segment Style;"));
    CHECK_EQ(describe(cu59::settingsSpec(F::WebWide, "Version 3.0.0")),
             std::string("General: Startup model Version 3.0.0;Startup presets: TI-59 TI-58 TI-58C;"
                         "Tracing: Maximum file size (MB);"));
    CHECK_EQ(describe(cu59::settingsSpec(F::WebPortrait, "Version 3.0.0")),
             std::string("General: Startup model Version 3.0.0;Startup presets: TI-59 TI-58 TI-58C;Keyboard: Keyboard Feedback;"));
    // The labelled choices (S8): the Web offers Off and Click Sound of the three values.
    const auto qt = cu59::settingsSpec(F::Qt6, "");
    CHECK(qt.sections.back().rows[0].choices ==
          (std::vector<std::pair<std::string, std::string>>{{"modernized", "Modernized"}, {"classic", "Classic"}}));
    const auto portrait = cu59::settingsSpec(F::WebPortrait, "");
    CHECK(portrait.sections.back().rows[0].choices ==
          (std::vector<std::pair<std::string, std::string>>{{"off", "Off"}, {"click", "Click Sound"}}));
    // No Solid State Module row (row 59-9); the [folders] kinds asm-overlays and other get none (D-43).
    for (const auto& s : qt.sections)
        for (const auto& r : s.rows) {
            CHECK(r.key != "module");
            CHECK(r.key != "asm-overlays" && r.key != "other");
        }
}

TEST(lib14_ac24_faceplate_hit_map) {
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
    // The display: a press there commands full speed (AC-43).
    CHECK(shell::onDisplay(map, 1.0, cu59::kCanvasWidth / 2, (cu59::kDisplayRect.y + 0.01) * cu59::kCanvasHeight));
    const Value v = cu59::faceplateMapValue(calcu::machine::ModelId{"TI-58"});
    CHECK(v.find("map") && v.find("card") && v.find("canvas"));
}

TEST(lib14_ac19_roms_missing_topic_without_a_machine) {
    calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    cu59::TIAppSetup s;
    s.settings = &settings;
    s.saveFolder = "save";
    s.fileIO = std::make_shared<apprig::Files>();
    s.romsMissing = "TMC0583.txt: missing";
    auto app = cu59::makeAppSession(std::move(s), [&] { return now += std::chrono::milliseconds(1); }, 59u, {}, {}, false);
    calcu::sessions::test::TestFrontend fe;
    const auto me = app.session->listen(fe, {});
    const auto start = app.session->send(me, calcu::sessions::Command{calcu::sessions::kStartCommand, Value::object({})});
    calcu::testing::pumpFor(*app.session, now, std::chrono::milliseconds(200));
    const auto* missing = fe.mirror.state(calcu::sessions::TopicName{"roms-missing"});
    CHECK(missing != nullptr);
    CHECK(missing && shell::read::text((*missing)->toValue(), "text") == "TMC0583.txt: missing");
    CHECK(fe.reply(start) && !fe.reply(start)->done);   // no machine is built
    CHECK(app.session->machineSession().machine() == nullptr);
    app.session->close();
}
