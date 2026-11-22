// The C++ side of what the Swift frontend
// draws. The bridge's TIShell computes the library's shell models with the
// Swift frontend's traits over the topics of the real session;
// these cases do the same over Calc-U-59's session with the library's
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

// The Swift frontend's traits (the iPhone and the iPad): no menu bar to
// count on, no About, no Quit.
shell::Traits swiftTraits() {
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

    explicit SwiftRig(cu59::ModelId model = cu59::ModelId{cu59::kTI59}) {
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

// File, Machine, Help in that order with their items, as the iPad's menu
// bar shows them; the card items on the TI-59 only;
// an item that cannot be used is disabled, not left out.
TEST(menus_file_machine_help_in_order) {
    SKIP_UNLESS(roms(), kSkip);
    for (const auto model : {cu59::ModelId{cu59::kTI59}, cu59::ModelId{cu59::kTI58}, cu59::ModelId{cu59::kTI58C}}) {
        SwiftRig s(model);
        const shell::ShellInputs in = s.inputs();
        {
            const auto ms = shell::menus(in, product(), swiftTraits());
            CHECK_EQ(ms.size(), std::size_t{3});
            if (ms.size() != 3) continue;
            CHECK_EQ(ms[0].label, std::string("File"));
            CHECK_EQ(ms[1].label, std::string("Machine"));
            CHECK_EQ(ms[2].label, std::string("Help"));
            const auto file = ids(ms[0].items);
            CHECK(!file.empty() && file[0] == "load-preset");
            CHECK(has(file, "examples"));   // the bundle has examples
            const bool ti59 = model == cu59::ModelId{cu59::kTI59};
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

// The shortcuts of a hardware keyboard; no About and no Quit.
TEST(shortcuts_no_about_no_quit) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    const auto ms = shell::menus(s.inputs(), product(), swiftTraits());
    using S = shell::MenuItem::Shortcut;
    const auto* load = shell::findItem(ms, "load-preset");
    const auto* reset = shell::findItem(ms, "reset");
    const auto* cold = shell::findItem(ms, "cold-boot");
    CHECK(load && load->shortcut == S::LoadPreset);
    CHECK(reset && reset->shortcut == S::Reset);
    CHECK(cold && cold->shortcut == S::ColdBoot);
    CHECK(shell::findItem(ms, shell::actions::kAbout) == nullptr);
    CHECK(shell::findItem(ms, shell::actions::kQuit) == nullptr);
}

// The bar per model: the model picker first (59, 58, 58C), the
// picker of slot-1 with the 14 modules and None, the card controls on the
// TI-59 only; the button PC-100C on the paper panel.
TEST(control_bar_per_model) {
    SKIP_UNLESS(roms(), kSkip);
    using K = shell::BarControl::Kind;
    for (const auto model : {cu59::ModelId{cu59::kTI59}, cu59::ModelId{cu59::kTI58}, cu59::ModelId{cu59::kTI58C}}) {
        SwiftRig s(model);
        const auto bar = shell::bar(s.inputs(), product(), swiftTraits());
        const auto* picker = control(bar, K::ModelPicker);
        CHECK(picker != nullptr);
        if (picker) CHECK_EQ(picker->shown, model.value.substr(3));   // "TI-58C" → "58C"
        const auto* slot = control(bar, K::SlotPicker, "slot-1");
        CHECK(slot != nullptr);
        if (slot) {
            CHECK_EQ(slot->choices.size(), std::size_t{15});   // None and the 14 modules
            CHECK(!slot->choices.empty() && slot->choices[0].label == "None");
        }
        CHECK(control(bar, K::PlaceButton, "printer") == nullptr);
        const auto ps = shell::panels(s.inputs(), product());
        CHECK(ps.size() == 1 && ps[0].kind == "paper" && ps[0].places.size() == 1);
        if (ps.size() == 1 && ps[0].places.size() == 1)
            CHECK(ps[0].places[0].kind == K::PlaceButton && ps[0].places[0].label == "PC-100C");
        CHECK((control(bar, K::Media, "card-reader") != nullptr) == (model == cu59::ModelId{cu59::kTI59}));
    }
}

// Without a menu bar the bar keeps Preset and Settings, and More holds
// Examples, Reset, Cold boot, Resume and the help.
TEST(more_holds_examples_reset_cold_boot_resume_help) {
    SKIP_UNLESS(roms(), kSkip);
    using K = shell::BarControl::Kind;
    SwiftRig s;
    const auto in = s.inputs();
    const auto bar = shell::bar(in, product(), swiftTraits());
    CHECK(control(bar, K::Action, "reset") == nullptr);
    CHECK(control(bar, K::Action, shell::actions::kLoadPreset) != nullptr);
    CHECK(control(bar, K::Action, shell::actions::kSettings) != nullptr);
    const auto* more = control(bar, K::More);
    CHECK(more != nullptr);
    if (!more) return;
    CHECK((ids(more->choices) == std::vector<std::string>{"examples", "reset", "cold-boot", "resume", shell::actions::kHelp}));
    const auto more2 = shell::moreItems(shell::menus(in, product(), swiftTraits()), product());
    CHECK(ids(more2) == ids(more->choices));
}

// Cold boot is destructive there and asks first.
TEST(cold_boot_asks_first) {
    SKIP_UNLESS(roms(), kSkip);
    SwiftRig s;
    const auto more = shell::moreItems(shell::menus(s.inputs(), product(), swiftTraits()), product());
    const shell::MenuItem* cold = nullptr;
    for (const auto& i : more)
        if (i.id == "cold-boot") cold = &i;
    CHECK(cold != nullptr);
    if (!cold) return;
    CHECK(cold->destructive);
    CHECK_EQ(cold->confirm, std::string(shell::kColdBootQuestion));
    CHECK_EQ(cold->command.value, std::string("cold-boot"));
}

// The More items follow the control state: while an operation
// runs the examples are disabled; Cold boot stays available.
TEST(more_follows_the_control_state) {
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
        return shell::moreItems(shell::menus(in, product(), swiftTraits()), product());
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

// The settings dialog, drawn from the settings topic of SettingsPart (the
// [window] row included).
TEST(settings_spec) {
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
    CHECK_EQ(describe(cu59::ShellFrontend::SwiftIOS),
             std::string("General: Startup model Version 3.0.0 (test);Startup presets: TI-59 TI-58 TI-58C;"
                         "Display: Segment Style;Keyboard: Keyboard Feedback;Layout: Debug Page in Portrait;"
                         "Tracing: Save Location Maximum file size (MB);"));
    // The rows' choices: "Last used" first; the startup presets.
    const auto ios = shell::dialog(in.settings, in.presetList, cu59::settingsSpec(cu59::ShellFrontend::SwiftIOS, "v"));
    CHECK(!ios.empty() && !ios[0].rows.empty() && !ios[0].rows[0].choices.empty());
    if (!ios.empty() && !ios[0].rows.empty() && !ios[0].rows[0].choices.empty())
        CHECK_EQ(ios[0].rows[0].choices[0].second, std::string("Last used"));
    CHECK(ios.size() > 1 && !ios[1].rows.empty() && ios[1].rows[0].choices.size() >= 2);
    if (ios.size() > 1 && !ios[1].rows.empty() && !ios[1].rows[0].choices.empty())
        CHECK_EQ(ios[1].rows[0].choices[0].second, std::string("None"));
    // A change is written at once; the [window] row is set like any other.
    CHECK(s.r().send(shell::commands::kSetSetting,
                     Value::object({{"section", "window"}, {"key", "portrait-debug-page"}, {"value", "true"}})).done);
    CHECK_EQ(s.file.text("window", "portrait-debug-page"), std::string("true"));
    CHECK(s.r().send(shell::commands::kSetSetting,
                     Value::object({{"section", "display"}, {"key", "led-style"}, {"value", "classic"}})).done);
    CHECK_EQ(s.file.text("display", "led-style"), std::string("classic"));
    // The Solid State Module row is gone: no setting holds it.
    CHECK(describe(cu59::ShellFrontend::SwiftIOS).find("Module") == std::string::npos);
}

// Without a control state there is no bar and no Machine menu;
// Help stays.
TEST(no_bar_no_machine_menu_without_control_state) {
    shell::ShellInputs in;
    in.romsMissing = Value::object({{"text", "Calc-U-59 cannot start."}});
    {
        const shell::Traits t = swiftTraits();
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
