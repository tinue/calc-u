// app/Settings (LIB-9, Calc-U-59 column: AC-19 to AC-25, AC-34, AC-35):
// the product's declaration and typed calls over the library's settings file.

#include "TestHarness.hpp"
#include "../app/Settings.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;
namespace cs = calcu::settings;
namespace s59 = cu59::settings;
using cu59::Model;

std::string readAll(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A temporary folder of its own, removed afterwards.
struct TempDir {
    fs::path root;
    explicit TempDir(const char* name) : root(fs::temp_directory_path() / name) { fs::remove_all(root); }
    ~TempDir() { fs::remove_all(root); }
    std::string str() const { return root.string(); }
};

// Calc-U-59's settings over a file that is never read or written (AC-18).
struct TestSettings {
    cs::SettingsFile file = cs::SettingsFile::defaultsOnly(s59::settingsDecl());
    s59::Settings settings{file};
    TestSettings() = default;
    TestSettings(const TestSettings&) = delete;
    TestSettings& operator=(const TestSettings&) = delete;
};

} // namespace

// AC-19: the file is Calc-U-59.ini and holds these keys with these defaults.
TEST(lib9_ac19_declaration) {
    const cs::ProductDecl& d = s59::settingsDecl();
    CHECK_EQ(d.productName, std::string("Calc-U-59"));
    for (const std::string& p : cs::checkDeclaration(d)) std::fprintf(stderr, "  %s\n", p.c_str());
    CHECK(cs::checkDeclaration(d).empty());
    CHECK((d.models == std::vector<std::string>{"TI-59", "TI-58", "TI-58C"}));

    const std::vector<std::pair<std::string, std::string>> expected = {
        {"startup.model", "last"},             {"startup.last-model", ""},
        {"preset.TI-59", ""},                  {"preset.TI-58", ""},
        {"preset.TI-58C", ""},                 {"storage.save-folder", ""},
        {"trace.folder", ""},                  {"trace.max-file-size-mb", "50"},
        {"trace.location", "app-support"},     {"display.led-style", "modernized"},
        {"keyboard.feedback", "off"},          {"folders.presets", ""},
        {"folders.asm-overlays", ""},          {"folders.other", ""},
        {"window.x", ""},                      {"window.y", ""},
        {"window.width", "1280"},              {"window.height", "860"},
        {"window.calculator-width", ""},       {"window.printer-width", ""},
        {"window.debug-width", ""},            {"window.portrait-debug-page", "false"},
    };
    std::vector<std::pair<std::string, std::string>> actual;
    for (const cs::KeyDecl& k : d.keys) actual.emplace_back(k.section + "." + k.key, k.defaultValue);
    CHECK(actual == expected);

    std::set<std::string> byApp;
    for (const cs::KeyDecl& k : d.keys)
        if (k.writtenByApp) byApp.insert(k.section + "." + k.key);
    CHECK((byApp == std::set<std::string>{"startup.last-model", "folders.presets", "folders.asm-overlays",
                                          "folders.other", "window.x", "window.y", "window.width",
                                          "window.height", "window.calculator-width", "window.printer-width",
                                          "window.debug-width", "window.portrait-debug-page"}));

    // The accepted values.
    const auto accepts = [&](const char* sec, const char* key, const char* v) {
        const cs::KeyDecl* k = d.find(sec, key);
        return k && cs::accepts(d, *k, v);
    };
    for (const char* v : {"last", "TI-59", "TI-58", "TI-58C"}) CHECK(accepts("startup", "model", v));
    CHECK(!accepts("startup", "model", "TI59"));
    CHECK(accepts("display", "led-style", "modernized") && accepts("display", "led-style", "classic"));
    CHECK(!accepts("display", "led-style", "1"));
    for (const char* v : {"off", "haptic", "click"}) CHECK(accepts("keyboard", "feedback", v));
    for (const char* v : {"app-support", "icloud", "folder"}) CHECK(accepts("trace", "location", v));
    CHECK(!accepts("trace", "location", "local"));

    // The typed calls read and write those keys.
    TestSettings t;
    s59::Settings& s = t.settings;
    CHECK(!s.startupModelChoice());
    CHECK(s.traceMaxFileSizeMB() == 50);
    CHECK(s.ledStyle() == s59::LedStyle::Modernized);
    CHECK(s.keyboardFeedback() == s59::KeyboardFeedback::Off);
    CHECK(s.traceLocation() == s59::TraceLocation::AppSupport);
    CHECK(s.saveFolderOverride().empty() && s.traceFolder().empty());
    for (Model m : {Model::TI59, Model::TI58, Model::TI58C}) CHECK(s.startupPreset(m).empty());
    CHECK(!s.portraitDebugPage());
    s.setLedStyle(s59::LedStyle::Classic);
    CHECK_EQ(t.file.text("display", "led-style"), std::string("classic"));
    CHECK(s.ledStyle() == s59::LedStyle::Classic);
    s.setKeyboardFeedback(s59::KeyboardFeedback::Click);
    CHECK_EQ(t.file.text("keyboard", "feedback"), std::string("click"));
    s.setTraceLocation(s59::TraceLocation::Folder);
    CHECK_EQ(t.file.text("trace", "location"), std::string("folder"));
    s.setTraceMaxFileSizeMB(12);
    CHECK(s.traceMaxFileSizeMB() == 12);
    s.setTraceMaxFileSizeMB(0);  // AC-11
    CHECK(s.traceMaxFileSizeMB() == 50);
    s.setStartupModelChoice(Model::TI58C);
    CHECK_EQ(t.file.text("startup", "model"), std::string("TI-58C"));
    CHECK(s.startupModelChoice() == Model::TI58C);
    s.setStartupModelChoice(std::nullopt);
    CHECK_EQ(t.file.text("startup", "model"), std::string("last"));
    s.setPortraitDebugPage(true);
    CHECK_EQ(t.file.text("window", "portrait-debug-page"), std::string("true"));
    s.setSaveFolderOverride("/tmp/cards");
    CHECK_EQ(t.file.text("storage", "save-folder"), std::string("/tmp/cards"));

    // EC-5: a value its key does not accept reads as the default and stays.
    t.file.set("display", "led-style", "glowing");
    CHECK(s.ledStyle() == s59::LedStyle::Modernized);
    CHECK(t.file.raw("display", "led-style") == std::optional<std::string>("glowing"));

    // [folders]: the folder last used, or a fixed one that picks don't change (D-2).
    const std::string home = "/home/u";
    CHECK_EQ(s.openStartDir(s59::OpenFolder::Presets, home), home);
    s.rememberOpenFile(s59::OpenFolder::Presets, "/data/presets/a.ti59");
    CHECK_EQ(s.openStartDir(s59::OpenFolder::Presets, home), std::string("/data/presets"));
    CHECK(s.openDir(s59::OpenFolder::Presets).empty());
    s.setOpenDir(s59::OpenFolder::Presets, "/fixed");
    s.rememberOpenFile(s59::OpenFolder::Presets, "/elsewhere/b.ti59");
    CHECK_EQ(s.openStartDir(s59::OpenFolder::Presets, home), std::string("/fixed"));
    CHECK_EQ(s.openDir(s59::OpenFolder::Presets), std::string("/fixed"));
    s.setOpenDir(s59::OpenFolder::Presets, "");  // Reset: starts where the fixed folder was
    CHECK(s.openDir(s59::OpenFolder::Presets).empty());
    CHECK_EQ(s.openStartDir(s59::OpenFolder::Presets, home), std::string("/fixed"));
    s.rememberOpenFile(s59::OpenFolder::AsmOverlays, "/asm/x.asm");
    CHECK_EQ(t.file.text("folders", "asm-overlays"), std::string("/asm"));
    CHECK_EQ(s.openStartDir(s59::OpenFolder::Other, home), home);  // D-1: declared, no dialog yet
}

// AC-20, EC-1: `last` without a model used before starts the TI-59.
TEST(lib9_ac20_startup_last_without_last_model_is_ti59) {
    TestSettings t;
    s59::Settings& s = t.settings;
    CHECK(!s.lastUsedModel());
    CHECK(s.startupModel() == Model::TI59);
    s.setLastUsedModel(Model::TI58);
    CHECK_EQ(t.file.text("startup", "last-model"), std::string("TI-58"));
    CHECK(s.startupModel() == Model::TI58);
    s.setStartupModelChoice(Model::TI58C);
    CHECK(s.startupModel() == Model::TI58C);
    t.file.set("startup", "model", "7");  // neither last nor a model: counts as last (AC-9)
    CHECK(s.startupModel() == Model::TI58);
    t.file.set("startup", "model", "last");
    t.file.set("startup", "last-model", "PC-1500");  // EC-1
    CHECK(s.startupModel() == Model::TI59);
    CHECK(s59::modelNamed("TI-58C") == Model::TI58C && !s59::modelNamed("ti58c"));
    CHECK_EQ(std::string(s59::modelName(Model::TI58C)), std::string("TI-58C"));
}

// AC-21: the first start has every default and writes the file then, from
// nothing of a platform store (the source check is CTest CalcU59NoPlatformSettings).
TEST(lib9_ac21_first_start_defaults_and_writes_file) {
    TempDir tmp("calcu59-lib9-ac21");
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Qt6;
    dirs.home = tmp.str() + "/home";
    dirs.settingsBase = tmp.str() + "/settings";
    const cs::ProductDecl& d = s59::settingsDecl();
    const std::string path = cs::settingsFilePath(d, dirs);
    CHECK_EQ(path, tmp.str() + "/settings/Calc-U-59/Calc-U-59.ini");
    CHECK(!fs::exists(path));
    {
        cs::SettingsFile f = cs::SettingsFile::open(d, path);
        CHECK(f.outcome() == cs::Outcome::Created);
        s59::Settings s(f);
        CHECK(s.startupModel() == Model::TI59);
        CHECK(s.ledStyle() == s59::LedStyle::Modernized);
        CHECK(s.windowState() == s59::WindowState{});
    }
    CHECK(fs::exists(path));
    const std::string text = readAll(path);
    CHECK(text.rfind("format-version = 1", 0) == 0);
    for (const cs::KeyDecl& k : d.keys)
        CHECK(text.find(k.defaultValue.empty() ? k.key + " =\n" : k.key + " = " + k.defaultValue) != std::string::npos);
    // AC-6: a change survives a new start.
    {
        cs::SettingsFile f = cs::SettingsFile::open(d, path);
        CHECK(f.outcome() == cs::Outcome::Read);
        s59::Settings(f).setKeyboardFeedback(s59::KeyboardFeedback::Haptic);
    }
    cs::SettingsFile again = cs::SettingsFile::open(d, path);
    CHECK(s59::Settings(again).keyboardFeedback() == s59::KeyboardFeedback::Haptic);
    // The Qt6 save folder is ~/Calc-U-59 unless [storage] save-folder names another (AC-13).
    cs::Paths paths(again, dirs);
    CHECK_EQ(paths.saveFolder(), dirs.home + "/Calc-U-59");
    CHECK(paths.traceFolder() == paths.saveFolder());
    CHECK_EQ(paths.saveFolderFile("ti58c.mem"), dirs.home + "/Calc-U-59/ti58c.mem");
}

// AC-22: no key holds the Solid State ROM module (or any hardware, AC-8).
TEST(lib9_ac22_no_key_holds_a_module) {
    for (const cs::KeyDecl& k : s59::settingsDecl().keys) {
        const std::string name = cs::lowerAscii(k.section + "." + k.key);
        CHECK(name.find("module") == std::string::npos);
        CHECK(name.find("solid") == std::string::npos);
        CHECK(name.find("printer.") == std::string::npos);
    }
    // A key from 2.3.0's stores does not count, and is kept as an unknown key (AC-4).
    TestSettings t;
    t.file.set("startup", "solid-state-module", "LE");
    CHECK(!s59::settingsDecl().find("startup", "solid-state-module"));
}

// AC-24, AC-34: Swift's trace location is Application Support (the default),
// iCloud (the save folder) or a chosen folder: the one the bookmark grants,
// passed in, else [trace] folder, else Application Support (D-8).
TEST(lib9_ac24_trace_location_app_support_icloud_folder) {
    TestSettings t;
    s59::Settings& s = t.settings;
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Swift;
    dirs.home = "/Users/u";
    dirs.settingsBase = "/Users/u/Library/Application Support";
    dirs.applicationSupport = "/Users/u/Library/Application Support/Calc-U-59";
    dirs.saveFolder = "/Users/u/Library/Mobile Documents/iCloud~calcu59/Documents";
    {
        cs::Paths p(t.file, dirs);
        CHECK(p.traceFolder() == dirs.applicationSupport);
        CHECK(p.screenshotFolder() == dirs.saveFolder);
        CHECK_EQ(p.saveFolder(), dirs.saveFolder);  // AC-14
    }
    s.setTraceLocation(s59::TraceLocation::ICloud);
    CHECK(cs::Paths(t.file, dirs).traceFolder() == dirs.saveFolder);
    s.setTraceLocation(s59::TraceLocation::Folder);
    CHECK(cs::Paths(t.file, dirs).traceFolder() == dirs.applicationSupport);  // no folder chosen
    s.setTraceFolder("/Users/u/Traces");
    CHECK_EQ(t.file.text("trace", "folder"), std::string("/Users/u/Traces"));
    CHECK(cs::Paths(t.file, dirs).traceFolder() == std::optional<std::string>("/Users/u/Traces"));
    dirs.chosenTraceFolder = "/Volumes/Granted/Traces";  // the bookmark's folder (AC-34)
    CHECK(cs::Paths(t.file, dirs).traceFolder() == dirs.chosenTraceFolder);
    // The save folder override is ignored outside Qt6 (AC-25).
    s.setSaveFolderOverride("/elsewhere");
    CHECK_EQ(cs::Paths(t.file, dirs).saveFolder(), dirs.saveFolder);
}

// AC-25: one declaration for Swift, Qt6 and Web; a key a frontend has no use
// for is kept in the file and ignored.
TEST(lib9_ac25_one_declaration_for_every_frontend) {
    TempDir tmp("calcu59-lib9-ac25");
    const cs::ProductDecl& d = s59::settingsDecl();
    CHECK(&d == &s59::settingsDecl());
    const std::string path = tmp.str() + "/Calc-U-59/Calc-U-59.ini";
    fs::create_directories(tmp.root / "Calc-U-59");
    std::ofstream(path, std::ios::binary) << "format-version = 1\n[keyboard]\nfeedback = haptic\n"
                                             "[storage]\nsave-folder = /qt/cards\n[mine]\nnote = kept\n";
    {
        cs::SettingsFile f = cs::SettingsFile::open(d, path);
        CHECK(f.outcome() == cs::Outcome::Read);
        // Qt6 ignores [keyboard] feedback, Web the save folder; a write keeps both.
        cs::HostDirs web;
        web.frontend = cs::FrontendKind::Web;
        web.saveFolder = "/persist-calc-u-59/Calc-U-59";
        CHECK_EQ(cs::Paths(f, web).saveFolder(), web.saveFolder);
        CHECK(!cs::Paths(f, web).traceFolder());
        s59::Settings(f).setLedStyle(s59::LedStyle::Classic);
    }
    const std::string text = readAll(path);
    CHECK(text.find("feedback = haptic") != std::string::npos);
    CHECK(text.find("save-folder = /qt/cards") != std::string::npos);
    CHECK(text.find("note = kept") != std::string::npos);
    CHECK(text.find("led-style = classic") != std::string::npos);

    // The UI tests' launch values (design §4): in a file never written.
    TestSettings t;
    CHECK(s59::applyLaunchSetting(t.file, "display.led-style=classic"));
    CHECK(s59::applyLaunchSetting(t.file, "window.portrait-debug-page=true"));
    CHECK(s59::applyLaunchSetting(t.file, "startup.last-model=TI-59"));
    CHECK(!s59::applyLaunchSetting(t.file, "display.led-style"));
    CHECK(!s59::applyLaunchSetting(t.file, "nothing.here=1"));
    CHECK(t.settings.ledStyle() == s59::LedStyle::Classic && t.settings.portraitDebugPage());
    CHECK(t.settings.lastUsedModel() == Model::TI59);
}

// AC-35: the Qt6 window's place, size and panel widths are plain numbers.
TEST(lib9_ac35_window_numbers_round_trip) {
    TempDir tmp("calcu59-lib9-ac35");
    const std::string path = tmp.str() + "/Calc-U-59/Calc-U-59.ini";
    s59::WindowState w;
    w.x = -40;  // a second screen left of the first
    w.y = 25;
    w.width = 1500;
    w.height = 900;
    w.calculatorWidth = 420;
    w.printerWidth = 300;
    w.debugWidth = 780;
    {
        cs::SettingsFile f = cs::SettingsFile::open(s59::settingsDecl(), path);
        s59::Settings s(f);
        CHECK(s.windowState() == s59::WindowState{});
        s.setWindowState(w);
    }
    const std::string text = readAll(path);
    for (const char* line : {"x = -40", "y = 25", "width = 1500", "height = 900", "calculator-width = 420",
                             "printer-width = 300", "debug-width = 780"})
        CHECK(text.find(line) != std::string::npos);
    cs::SettingsFile f = cs::SettingsFile::open(s59::settingsDecl(), path);
    CHECK(s59::Settings(f).windowState() == w);
    // A value that is not a number counts as not set.
    f.set("window", "width", "wide");
    f.set("window", "x", "left");
    const s59::WindowState back = s59::Settings(f).windowState();
    CHECK(back.width == 1280 && !back.x && back.y == 25);
}
