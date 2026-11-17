// C59-8 (design §6, T1 to T6): the C++ side of what the Swift frontend
// draws. The bridge's TIShell computes the library's shell models with the
// Swift frontend's traits over the topics of the real session (design §3.1);
// these cases do the same over Calc-U-59's session with LIB-14's
// SettingsPart in front, as the bridge builds it.
#include "../app/TIShellProduct.hpp"
#include "SessionRig.hpp"
#include "TestHarness.hpp"
#include "control/ControlState.hpp"
#include "presets/PresetsPart.hpp"
#include "settings/Paths.hpp"
#include "shell/BarModel.hpp"
#include "shell/MenuModel.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/SettingsPart.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using testroms::SessionRig;
namespace shell = calcu::shell;
using calcu::sessions::Value;

namespace {

bool roms() { return std::filesystem::exists("roms/TMC0571B.txt"); }
const char* const kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";

// The traits of design §3.1.
shell::Traits macTraits() { return shell::Traits{}; }
shell::Traits iosTraits() {
    shell::Traits t;
    t.menuBar = false;
    t.about = false;
    t.hasQuit = false;
    return t;
}

// A session as the bridge builds it: SettingsPart (with the iOS row) first.
struct SwiftRig {
    calcu::settings::SettingsFile file =
        calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    std::unique_ptr<calcu::settings::Paths> paths;
    std::unique_ptr<shell::SettingsPart> settingsPart;
    std::unique_ptr<SessionRig> rig;

    explicit SwiftRig(cu59::Model model = cu59::Model::TI59) {
        calcu::settings::HostDirs dirs;
        dirs.frontend = calcu::settings::FrontendKind::Swift;
        paths = std::make_unique<calcu::settings::Paths>(file, dirs);
        shell::SettingsPartSetup s;
        s.file = &file;
        s.paths = paths.get();
        s.alsoShown = {cu59::kPortraitDebugPage};
        settingsPart = std::make_unique<shell::SettingsPart>(std::move(s));
        rig = std::make_unique<SessionRig>(
            testroms::RigOptions{.model = model, .before = {settingsPart.get()}, .settings = &file});
    }
    SessionRig& r() { return *rig; }

    Value topic(const calcu::sessions::TopicName& t) {
        const auto* p = r().fe.mirror.state(t);
        return p && *p ? (*p)->toValue() : Value();
    }
    Value presetList() {
        const auto id = r().session().ask(r().me, calcu::sessions::Request{calcu::presets::kPresetListRequest, Value::object({})});
        r().until([&] { return r().reply(id) != nullptr; });
        const auto* reply = r().reply(id);
        return reply && reply->result ? reply->result->toValue() : Value();
    }
    shell::ShellInputs inputs() {
        shell::ShellInputs in;
        const shell::InputNames n;
        in.controlState = topic(n.controlState);
        in.busy = topic(n.busy);
        in.runState = topic(n.runState);
        in.romsMissing = topic(n.romsMissing);
        in.settings = topic(n.settings);
        in.media = topic(n.media);
        in.presetList = presetList();
        return in;
    }
};

const shell::ShellProduct& product() {
    static const shell::ShellProduct p = cu59::shellProduct("3.0.0", "test");
    return p;
}

std::vector<std::string> ids(const std::vector<shell::MenuItem>& items) {
    std::vector<std::string> out;
    for (const auto& i : items)
        if (i.kind != shell::MenuItem::Kind::Separator) out.push_back(i.id);
    return out;
}

bool has(const std::vector<std::string>& xs, const std::string& x) {
    for (const auto& y : xs)
        if (y == x) return true;
    return false;
}

const shell::BarControl* control(const std::vector<shell::BarGroupModel>& bar, shell::BarControl::Kind k,
                                 const std::string& id = {}) {
    for (const auto& g : bar)
        for (const auto& c : g.controls)
            if (c.kind == k && (id.empty() || c.id == id)) return &c;
    return nullptr;
}

std::vector<shell::BarControl::Kind> kinds(const std::vector<shell::BarGroupModel>& bar) {
    std::vector<shell::BarControl::Kind> out;
    for (const auto& g : bar)
        for (const auto& c : g.controls) out.push_back(c.kind);
    return out;
}

} // namespace

// T1, AC-18 (EC-6): File, Machine, Help in that order with their items, on
// macOS and in the iPad's menu bar; the card items on the TI-59 only;
// an item that cannot be used is disabled, not left out.
TEST(c59_8_ac18_menus_file_machine_help_in_order) {
    SKIP_UNLESS(roms(), kSkip);
    for (const auto model : {cu59::Model::TI59, cu59::Model::TI58, cu59::Model::TI58C}) {
        SwiftRig s(model);
        const shell::ShellInputs in = s.inputs();
        for (const shell::Traits t : {macTraits(), iosTraits()}) {
            const auto ms = shell::menus(in, product(), t);
            CHECK_EQ(ms.size(), std::size_t{3});
            if (ms.size() != 3) continue;
            CHECK_EQ(ms[0].label, std::string("File"));
            CHECK_EQ(ms[1].label, std::string("Machine"));
            CHECK_EQ(ms[2].label, std::string("Help"));
            const auto file = ids(ms[0].items);
            CHECK(!file.empty() && file[0] == "load-preset");
            CHECK(has(file, "examples"));   // the bundle has examples
            const bool ti59 = model == cu59::Model::TI59;
            CHECK(has(file, cu59::kItemArmCard) == ti59);
            CHECK(has(file, cu59::kItemArmFreshCard) == ti59);
            const auto machine = ids(ms[1].items);
            CHECK((machine == std::vector<std::string>{"model", "peripherals", "reset", "cold-boot", "resume"}));
            CHECK(!ids(ms[2].items).empty() && ids(ms[2].items)[0] == "help");
            CHECK_EQ(ms[2].items[0].label, std::string("Calc-U-59 Help"));
            // Resume is there while not paused, disabled with a reason.
            const auto* resume = shell::findItem(ms, "resume");
            CHECK(resume && !resume->enabled && !resume->reason.empty());
        }
    }
}

// T1, AC-1: the shortcuts and the items macOS puts in the application menu.
TEST(c59_8_ac1_shortcuts_and_roles) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    const auto ms = shell::menus(s.inputs(), product(), macTraits());
    using S = shell::MenuItem::Shortcut;
    using R = shell::MenuItem::Role;
    const auto* load = shell::findItem(ms, "load-preset");
    const auto* reset = shell::findItem(ms, "reset");
    const auto* cold = shell::findItem(ms, "cold-boot");
    CHECK(load && load->shortcut == S::LoadPreset);
    CHECK(reset && reset->shortcut == S::Reset);
    CHECK(cold && cold->shortcut == S::ColdBoot);
    // Settings…, Quit and About carry the roles the app menu takes them by.
    const auto* settings = shell::findItem(ms, shell::actions::kSettings);
    const auto* quit = shell::findItem(ms, shell::actions::kQuit);
    const auto* about = shell::findItem(ms, shell::actions::kAbout);
    CHECK(settings && settings->role == R::Preferences);
    CHECK(quit && quit->role == R::Quit);
    CHECK(about && about->role == R::About);
    // iOS: no About and no Quit (AC-11).
    const auto ios = shell::menus(s.inputs(), product(), iosTraits());
    CHECK(shell::findItem(ios, shell::actions::kAbout) == nullptr);
    CHECK(shell::findItem(ios, shell::actions::kQuit) == nullptr);
}

// T2, AC-3, AC-4, EC-3: the bar per model: the model picker first, the
// picker of slot-1 with the 14 modules and None, the button PC-100C, the
// card controls on the TI-59 only; on macOS no Reset and no Preset.
TEST(c59_8_ac3_bar_per_model) {
    SKIP_UNLESS(roms(), kSkip);
    using K = shell::BarControl::Kind;
    for (const auto model : {cu59::Model::TI59, cu59::Model::TI58, cu59::Model::TI58C}) {
        SwiftRig s(model);
        const auto bar = shell::bar(s.inputs(), product(), macTraits());
        const auto ks = kinds(bar);
        CHECK(!ks.empty() && ks[0] == K::ModelPicker);
        const auto* slot = control(bar, K::SlotPicker, "slot-1");
        CHECK(slot != nullptr);
        if (slot) {
            CHECK_EQ(slot->choices.size(), std::size_t{15});   // None and the 14 modules
            CHECK(!slot->choices.empty() && slot->choices[0].label == "None");
        }
        const auto* printer = control(bar, K::PlaceButton, "printer");
        CHECK(printer && printer->label == "PC-100C");
        CHECK((control(bar, K::Media, "card-reader") != nullptr) == (model == cu59::Model::TI59));
        CHECK(control(bar, K::Action) == nullptr);   // AC-4: Reset and Preset are menu items
        CHECK(control(bar, K::More) == nullptr);
    }
}

// T3, AC-5: without a menu bar the bar keeps Reset, Preset and Settings,
// and More holds Examples, Cold boot, Resume and the help.
TEST(c59_8_ac5_more_holds_examples_cold_boot_resume_help) {
    SKIP_UNLESS(roms(), kSkip);
    using K = shell::BarControl::Kind;
    SwiftRig s;
    const auto in = s.inputs();
    const auto bar = shell::bar(in, product(), iosTraits());
    CHECK(control(bar, K::Action, "reset") != nullptr);
    CHECK(control(bar, K::Action, shell::actions::kLoadPreset) != nullptr);
    CHECK(control(bar, K::Action, shell::actions::kSettings) != nullptr);
    const auto* more = control(bar, K::More);
    CHECK(more != nullptr);
    if (!more) return;
    CHECK((ids(more->choices) == std::vector<std::string>{"examples", "cold-boot", "resume", shell::actions::kHelp}));
    const auto more2 = shell::moreItems(shell::menus(in, product(), iosTraits()), product());
    CHECK(ids(more2) == ids(more->choices));
}

// T3, AC-5: Cold boot is destructive there and asks first.
TEST(c59_8_ac5_cold_boot_asks_first) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    const auto more = shell::moreItems(shell::menus(s.inputs(), product(), iosTraits()), product());
    const shell::MenuItem* cold = nullptr;
    for (const auto& i : more)
        if (i.id == "cold-boot") cold = &i;
    CHECK(cold != nullptr);
    if (!cold) return;
    CHECK(cold->destructive);
    CHECK_EQ(cold->confirm, std::string(shell::kColdBootQuestion));
    CHECK_EQ(cold->command.value, std::string("cold-boot"));
}

// T3, EC-1: the More items follow the control state: while an operation
// runs the examples are disabled; Cold boot stays available (LIB-4 AC-3).
TEST(c59_8_ec1_more_follows_the_control_state) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    auto find = [](const std::vector<shell::MenuItem>& items, const std::string& id) -> const shell::MenuItem* {
        for (const auto& i : items)
            if (i.id == id) return &i;
        return nullptr;
    };
    auto exampleEnabled = [&](const std::vector<shell::MenuItem>& more) {
        const auto* ex = find(more, "examples");
        if (!ex || ex->children.empty()) return false;
        const shell::MenuItem* first = &ex->children[0];
        while (first->kind == shell::MenuItem::Kind::Submenu && !first->children.empty()) first = &first->children[0];
        return first->enabled;
    };
    const Value list = s.presetList();
    auto more = [&] {
        shell::ShellInputs in = s.inputs();
        in.presetList = list;
        return shell::moreItems(shell::menus(in, product(), iosTraits()), product());
    };
    CHECK(exampleEnabled(more()));
    // An operation: a preset that waits a while.
    const auto id = s.r().post(calcu::presets::kLoadPresetCommand,
                               Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - wait: 5s\n"}, {"name", "w.ti59"}}));
    CHECK(s.r().until([&] { return s.r().busy(); }));
    const auto during = more();
    CHECK(!exampleEnabled(during));
    const auto* cold = find(during, "cold-boot");
    CHECK(cold && cold->enabled);
    CHECK(s.r().until([&] { return s.r().reply(id) != nullptr; }, std::chrono::seconds(30)));
    s.r().runFor(std::chrono::milliseconds(100));
    CHECK(exampleEnabled(more()));
}

// T5, AC-7: the settings dialog of macOS and of iOS, drawn from the
// settings topic of SettingsPart (the iOS [window] row included).
TEST(c59_8_ac7_settings_spec_macos_and_ios) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    const auto in = s.inputs();
    auto describe = [&](cu59::ShellFrontend f) {
        std::string out;
        for (const auto& sec : shell::dialog(in.settings, in.presetList, cu59::settingsSpec(f, "Version 3.0.0 (test)"))) {
            out += sec.title + ":";
            for (const auto& r : sec.rows) out += " " + r.row.label;
            out += ";";
        }
        return out;
    };
    CHECK_EQ(describe(cu59::ShellFrontend::SwiftMac),
             std::string("General: Startup model Version 3.0.0 (test);Startup presets: TI-59 TI-58 TI-58C;"
                         "Display: Segment Style;Tracing: Save Location Trace folder Maximum file size (MB);"));
    CHECK_EQ(describe(cu59::ShellFrontend::SwiftIOS),
             std::string("General: Startup model Version 3.0.0 (test);Startup presets: TI-59 TI-58 TI-58C;"
                         "Display: Segment Style;Keyboard: Keyboard Feedback;Layout: Debug Page in Portrait;"
                         "Tracing: Save Location Maximum file size (MB);"));
    // The rows' choices (S3, S4, S8): "Last used" first; the startup presets.
    const auto ios = shell::dialog(in.settings, in.presetList, cu59::settingsSpec(cu59::ShellFrontend::SwiftIOS, "v"));
    CHECK(!ios.empty() && !ios[0].rows.empty() && !ios[0].rows[0].choices.empty());
    if (!ios.empty() && !ios[0].rows.empty() && !ios[0].rows[0].choices.empty())
        CHECK_EQ(ios[0].rows[0].choices[0].second, std::string("Last used"));
    CHECK(ios.size() > 1 && !ios[1].rows.empty() && ios[1].rows[0].choices.size() >= 2);
    if (ios.size() > 1 && !ios[1].rows.empty() && !ios[1].rows[0].choices.empty())
        CHECK_EQ(ios[1].rows[0].choices[0].second, std::string("None"));
    // AC-8: a change is written at once; the [window] row is set like any other.
    CHECK(s.r().send(shell::commands::kSetSetting,
                     Value::object({{"section", "window"}, {"key", "portrait-debug-page"}, {"value", "true"}})).done);
    CHECK_EQ(s.file.text("window", "portrait-debug-page"), std::string("true"));
    CHECK(s.r().send(shell::commands::kSetSetting,
                     Value::object({{"section", "display"}, {"key", "led-style"}, {"value", "classic"}})).done);
    CHECK_EQ(s.file.text("display", "led-style"), std::string("classic"));
    // The Solid State Module row is gone (row 59-9): no setting holds it.
    CHECK(describe(cu59::ShellFrontend::SwiftMac).find("Module") == std::string::npos);
}

// T6, AC-19: without a control state there is no bar and no Machine menu;
// Help stays.
TEST(c59_8_ac19_no_bar_no_machine_menu_without_control_state) {
    shell::ShellInputs in;
    in.romsMissing = Value::object({{"text", "Calc-U-59 cannot start."}});
    for (const shell::Traits t : {macTraits(), iosTraits()}) {
        const auto ms = shell::menus(in, product(), t);
        CHECK(shell::findMenu(ms, "machine") == nullptr);
        CHECK(shell::findItem(ms, shell::actions::kHelp) != nullptr);
        CHECK(shell::findItem(ms, "load-preset") == nullptr);
        std::size_t controls = 0;
        for (const auto& g : shell::bar(in, product(), t))
            for (const auto& c : g.controls)
                if (c.kind != shell::BarControl::Kind::More && c.kind != shell::BarControl::Kind::Action) ++controls;
        CHECK_EQ(controls, std::size_t{0});
        CHECK(!in.started());
    }
}
