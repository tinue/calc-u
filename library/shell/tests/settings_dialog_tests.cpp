// SettingsPart (the `settings` topic, `set-setting`) and the dialog model,
// on the settings area's
// test product declaration (library/settings/tests/TestProduct.hpp) and an
// app session in virtual time. The model part is mirrored in
// shell_model.test.js.
#include "../../settings/tests/TestProduct.hpp"
#include "TestFrontend.hpp"
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestShellPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "settings/OpenFolders.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/SettingsPart.hpp"
#include <memory>
#include <optional>
#include <string>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;
namespace sett = calcu::settings;
using sessions::Value;

namespace {

sett::SettingsFile openFile(const sett::test::TempDir& home, bool writable) {
    const std::string path = home.str() + "/settings/Test-Product.ini";
    if (!writable) {
        std::filesystem::create_directories(home.str() + "/settings");
        sett::test::writeText(path, "[startup]\nmodel = Model-B\n");
    }
    return sett::SettingsFile::open(sett::test::testProduct(), path);
}

// A product's settings, its paths, a SettingsPart and a long operation in
// one session, a test frontend listening.
struct Rig {
    sett::test::TempDir home{"calcu-shell-home"};
    sett::SettingsFile file;
    std::unique_ptr<sett::Paths> paths;
    std::unique_ptr<SettingsPart> part;
    st::TestShellPart shellPart;
    std::unique_ptr<sessions::AppSession> session;
    sessions::test::TestFrontend fe;
    sessions::ListenerId id = 0;
    std::vector<std::string> changed;

    // Not writable: a file without a format version, which is left untouched
    // for the whole run.
    explicit Rig(bool writable = true, std::vector<std::string> alsoShown = {}) : file(openFile(home, writable)) {
        sett::HostDirs dirs;
        dirs.home = home.str();
        dirs.settingsBase = home.str() + "/settings";
        paths = std::make_unique<sett::Paths>(file, dirs);
        SettingsPartSetup s;
        s.file = &file;
        s.paths = paths.get();
        s.changed = [this](std::string_view sec, std::string_view key) { changed.push_back(std::string(sec) + "." + std::string(key)); };
        s.alsoShown = std::move(alsoShown);
        part = std::make_unique<SettingsPart>(std::move(s));
        sessions::AppSessionSetup a;
        a.clock = [] { return pacing::WallTime{0}; };   // a clock that stands: nothing runs unless called
        a.hostTime = [] { return std::chrono::system_clock::time_point{}; };
        a.parts = {part.get(), &shellPart};
        a.hosting = sessions::Hosting::Paced;
        session = std::make_unique<sessions::AppSession>(std::move(a));
        session->machineSession().setMachine(machine::test::buildTestMachine(machine::test::kSeg), pacing::PacingProfile{}, 0);
        id = session->listen(fe);
    }
    sessions::CallId set(const char* section, const char* key, Value v) {
        return session->send(id, {commands::kSetSetting, Value::object({{"section", section}, {"key", key}, {"value", std::move(v)}})});
    }
    Value topic() const {
        const auto* p = fe.mirror.state(kSettingsTopic);
        return p ? (*p)->toValue() : Value();
    }
    const Value* row(const char* section, const char* key) const {
        static Value kept;
        kept = topic();
        for (const auto& r : read::elements(kept, "rows"))
            if (read::text(r, "section") == section && read::text(r, "key") == key) return &r;
        return nullptr;
    }
    std::string rowText(const char* section, const char* key, const char* field) const {
        const Value* r = row(section, key);
        return r ? read::text(*r, field) : std::string("<none>");
    }
};

std::vector<ShownSection> shown(const Rig& r, const std::vector<std::string>& keys = {}) {
    return dialog(r.topic(), st::presetList(), familySections(sett::test::testProduct(), keys));
}

// A copy: the sections are often a temporary.
std::optional<ShownRow> find(const std::vector<ShownSection>& ss, const std::string& key) {
    for (const auto& s : ss)
        for (const auto& r : s.rows)
            if (r.row.section + "." + r.row.key == key) return r;
    return std::nullopt;
}

std::string titles(const std::vector<ShownSection>& ss) {
    std::string t;
    for (const auto& s : ss) t += (t.empty() ? "" : "|") + s.title;
    return t;
}

} // namespace

TEST(settings_sections_shown_for_keys_present) {
    Rig r;
    CHECK_EQ(std::string(kSettingsTitle), std::string("Settings"));
    const auto all = shown(r);
    CHECK_EQ(titles(all), std::string("General|Startup presets|Storage|Folders|Tracing"));
    CHECK_EQ(std::string(find(all, "startup.model")->row.label), std::string("Startup model"));
    CHECK_EQ(std::string(find(all, "storage.save-folder")->row.label), std::string("Save folder"));
    CHECK_EQ(std::string(find(all, "trace.folder")->row.label), std::string("Trace folder"));
    CHECK_EQ(std::string(find(all, "trace.max-file-size-mb")->row.label), std::string("Maximum file size (MB)"));
    CHECK_EQ(std::string(find(all, "folders.presets")->row.label), std::string("Presets folder"));
    CHECK(!find(all, "window.width"));           // never in the dialog
    CHECK(!find(all, "startup.last-model"));
    CHECK(r.row("window", "width") == nullptr);
    // A frontend with fewer keys (the Web: no folders) shows fewer sections.
    const auto web = shown(r, {"startup.model", "preset.Model-A", "preset.Model-B", "trace.max-file-size-mb"});
    CHECK_EQ(titles(web), std::string("General|Startup presets|Tracing"));
    // Startup model offers "Last used" and the product's models.
    auto sm = find(all, "startup.model");
    CHECK_EQ(sm->choices.size(), std::size_t{3});
    CHECK_EQ(sm->choices[0].second, std::string("Last used"));
    CHECK_EQ(sm->choices[1].first, std::string("Model-A"));
    // A product's own section follows the family's.
    auto spec = familySections(sett::test::testProduct());
    spec.sections.push_back(DialogSection{"Display", {{DialogRow::Widget::Choice, "test", "style", "Style"}}});
    const auto withProduct = dialog(r.topic(), st::presetList(), spec);
    CHECK_EQ(withProduct.back().title, std::string("Display"));
    CHECK_EQ(withProduct.back().rows[0].choices.size(), std::size_t{2});
}

// A frontend that shows a [window] key (Calc-U-59's
// Swift frontend on iOS) names it; it is then in the topic and set like any
// other; the rest of [window] stays out.
TEST(also_shown_key_in_topic_and_set) {
    Rig plain;
    CHECK(plain.row("window", "width") == nullptr);
    CHECK(!plain.fe.reply(plain.set("window", "width", "1000"))->done);
    Rig r(true, {"window.width"});
    CHECK(r.row("window", "width") != nullptr);
    const auto call = r.set("window", "width", "1000");
    CHECK(r.fe.reply(call) && r.fe.reply(call)->done);
    CHECK_EQ(r.file.integer("window", "width"), 1000L);
    CHECK_EQ(r.rowText("window", "width", "value"), std::string("1000"));
}

TEST(settings_change_written_at_once) {
    Rig r;
    const auto call = r.set("startup", "model", "Model-A");
    CHECK(r.fe.reply(call) && r.fe.reply(call)->done);
    // Written through at once, nothing waits for Close.
    CHECK(sett::test::readText(r.file.path()).find("model = Model-A") != std::string::npos);
    CHECK_EQ(r.rowText("startup", "model", "value"), std::string("Model-A"));
    CHECK_EQ(r.changed.size(), std::size_t{1});
    CHECK_EQ(r.changed[0], std::string("startup.model"));
    // A value the key does not accept is refused with the reason.
    const auto bad = r.set("trace", "max-file-size-mb", "nonsense");
    CHECK(r.fe.reply(bad) && !r.fe.reply(bad)->done);
    CHECK(r.fe.reply(bad)->reason.find("max-file-size-mb") != std::string::npos);
    // A page's whole numbers arrive as integers.
    const auto n = r.set("trace", "max-file-size-mb", Value(20));
    CHECK(r.fe.reply(n)->done);
    CHECK_EQ(r.file.integer("trace", "max-file-size-mb"), 20L);
    // A change from elsewhere shows at once.
    r.file.set("startup", "model", "Model-B");
    r.session->call(pacing::WallTime{0});
    CHECK_EQ(r.rowText("startup", "model", "value"), std::string("Model-B"));
}

TEST(startup_preset_choices) {
    Rig r;
    auto a = find(shown(r), "preset.Model-A");
    std::string labels;
    for (const auto& [v, l] : a->choices) labels += v + "=" + l + ";";
    CHECK_EQ(labels, std::string("=None;examples:startup/a.test=Start A — The A start;examples:startup/a2.test=Start A2;"));
    CHECK_EQ(a->row.label, std::string("Model-A"));
    const auto call = r.set("preset", "Model-A", "examples:startup/a2.test");
    CHECK(r.fe.reply(call)->done);
    CHECK_EQ(std::string(find(shown(r), "preset.Model-A")->value), std::string("examples:startup/a2.test"));
    CHECK_EQ(find(shown(r), "preset.Model-A")->choices.size(), std::size_t{3});   // one of them: not added again
    // A file set from a folder (Qt6's "Choose…") is added as it is set.
    r.set("preset", "Model-B", r.home.str() + "/mine.test");
    auto b = find(shown(r), "preset.Model-B");
    CHECK_EQ(b->choices.back().first, r.home.str() + "/mine.test");
    CHECK_EQ(b->choices.back().second, std::string("~/mine.test"));
}

TEST(folder_row_home_change_reset) {
    Rig r;
    auto save = find(shown(r), "storage.save-folder");
    CHECK_EQ(save->shown, std::string("~/Test-Product"));   // the default in use, with ~ for home
    CHECK(!save->resettable);
    r.set("storage", "save-folder", r.home.str() + "/Elsewhere");
    save = find(shown(r), "storage.save-folder");
    CHECK_EQ(save->shown, std::string("~/Elsewhere"));
    CHECK(save->resettable);   // "Reset" only while it differs
    r.set("storage", "save-folder", r.home.str() + "y/a");   // a name that only starts like home: as written
    CHECK_EQ(std::string(find(shown(r), "storage.save-folder")->shown), r.home.str() + "y/a");
    const auto reset = r.set("storage", "save-folder", Value());   // Reset: null is the default
    CHECK(r.fe.reply(reset)->done);
    save = find(shown(r), "storage.save-folder");
    CHECK_EQ(save->shown, std::string("~/Test-Product"));
    CHECK(!save->resettable);
    // A [folders] row: the folder a dialog starts in; Change… fixes it.
    CHECK_EQ(std::string(find(shown(r), "folders.presets")->shown), std::string("~"));
    r.set("folders", "presets", r.home.str() + "/presets");
    CHECK(sett::hasFixedOpenFolder(r.file, "presets"));
    CHECK_EQ(std::string(find(shown(r), "folders.presets")->shown), std::string("~/presets"));
    CHECK(find(shown(r), "folders.presets")->resettable);
    r.set("folders", "presets", Value());
    CHECK(!sett::hasFixedOpenFolder(r.file, "presets"));
    // remember-folder: a pick remembers the folder last used.
    r.session->send(r.id, {commands::kRememberFolder, Value::object({{"kind", "programs"}, {"folder", r.home.str() + "/p"}})});
    CHECK_EQ(std::string(find(shown(r), "folders.programs")->shown), std::string("~/p"));
    CHECK(!find(shown(r), "folders.programs")->resettable);
}

TEST(set_setting_while_operation_runs) {
    Rig r;
    r.session->send(r.id, {sessions::CommandName{"tsp.op"}, Value::object({{"ticks", 1'000'000}})});
    CHECK(r.session->busy());
    const auto call = r.set("startup", "model", "Model-B");
    CHECK(r.fe.reply(call) && r.fe.reply(call)->done);   // not held back
    CHECK(r.session->busy());   // and it starts nothing, stops nothing
    CHECK_EQ(r.rowText("startup", "model", "value"), std::string("Model-B"));
}

TEST(missing_startup_preset_still_shown) {
    Rig r;
    const std::string gone = r.home.str() + "/gone/start.test";
    r.set("preset", "Model-A", gone);
    auto a = find(shown(r), "preset.Model-A");
    CHECK_EQ(a->value, gone);
    CHECK_EQ(a->choices.back().first, gone);   // shown as set, though the file is not there
    CHECK_EQ(a->choices.back().second, std::string("~/gone/start.test"));
}

TEST(set_setting_unwritable_gives_notice) {
    Rig r(false);   // the file is not writable in this run
    const auto call = r.set("startup", "model", "Model-A");
    CHECK(r.fe.reply(call)->done);
    const auto notices = r.fe.noticeSentences();
    CHECK_EQ(notices.size(), std::size_t{1});
    CHECK(notices[0].find("not saved") != std::string::npos);
    CHECK_EQ(r.file.text("startup", "model"), std::string("Model-A"));   // kept for this run
    CHECK_EQ(r.rowText("startup", "model", "value"), std::string("Model-A"));
}

// A choice row with labels.
namespace {

DialogSpec styleSpec() {
    DialogSpec spec;
    DialogRow row{DialogRow::Widget::Choice, "test", "style", "Style"};
    row.choices = {{"one", "The first"}};   // "two" is declared and not offered
    spec.sections.push_back({"Look", {row}});
    return spec;
}

std::string choiceText(const ShownRow& r) {
    std::string s;
    for (const auto& [v, l] : r.choices) s += v + "=" + l + ";";
    return s;
}

} // namespace

TEST(choice_row_with_labels_offers_only_those) {
    Rig r;
    const auto ss = dialog(r.topic(), st::presetList(), styleSpec());
    const auto style = find(ss, "test.style");
    CHECK(style.has_value());
    CHECK_EQ(style->value, std::string("one"));
    CHECK_EQ(choiceText(*style), std::string("one=The first;"));
    // Without labels: every declared value as written (as before).
    DialogSpec plain;
    plain.sections.push_back({"Look", {{DialogRow::Widget::Choice, "test", "style", "Style"}}});
    CHECK_EQ(choiceText(*find(dialog(r.topic(), st::presetList(), plain), "test.style")), std::string("one=one;two=two;"));
}

TEST(choice_value_not_listed_shown_as_written) {
    Rig r;
    CHECK(r.fe.reply(r.set("test", "style", "two"))->done);
    const auto style = find(dialog(r.topic(), st::presetList(), styleSpec()), "test.style");
    CHECK_EQ(style->value, std::string("two"));
    CHECK_EQ(choiceText(*style), std::string("one=The first;two=two;"));
}
