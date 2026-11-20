// app/Settings:
// the product's declaration and typed calls over the library's settings file.

#include "Files.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "../app/Settings.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/OpenFolders.hpp"
#include "settings/SettingsFile.hpp"
#include "settings/Startup.hpp"
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
using cu59::ModelId;
using cu59::kTI59;
using cu59::kTI58;
using cu59::kTI58C;

using calcu::testing::readText;
using TempDir = calcu::testing::TempFolder;

// Calc-U-59's settings over a file that is never read or written.
struct TestSettings {
    cs::SettingsFile file = cs::SettingsFile::defaultsOnly(s59::settingsDecl());
    s59::Settings settings{file};
    TestSettings() = default;
    TestSettings(const TestSettings&) = delete;
    TestSettings& operator=(const TestSettings&) = delete;
};

} // namespace

// The file is Calc-U-59.ini and holds these keys with these defaults.
TEST(settings_declaration) {
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
        {"folders.asm-overlays", ""},          {"window.x", ""},
        {"window.y", ""},                      {"window.width", "1280"},
        {"window.height", "860"},              {"window.calculator-width", ""},
        {"window.printer-width", ""},          {"window.debug-width", ""},
        {"window.portrait-debug-page", "false"},
    };
    std::vector<std::pair<std::string, std::string>> actual;
    for (const cs::KeyDecl& k : d.keys) actual.emplace_back(k.section + "." + k.key, k.defaultValue);
    CHECK(actual == expected);

    std::set<std::string> byApp;
    for (const cs::KeyDecl& k : d.keys)
        if (k.writtenByApp) byApp.insert(k.section + "." + k.key);
    CHECK((byApp == std::set<std::string>{"startup.last-model", "folders.presets", "folders.asm-overlays",
                                          "window.x", "window.y", "window.width", "window.height",
                                          "window.calculator-width", "window.printer-width",
                                          "window.debug-width", "window.portrait-debug-page"}));
    CHECK(d.find("folders", "other") == nullptr);  // removed, no dialog used it

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

    // The settings file reads and writes those keys by the declaration.
    TestSettings t;
    cs::SettingsFile& f = t.file;
    CHECK_EQ(f.text("startup", "model"), std::string("last"));
    CHECK(f.integer("trace", "max-file-size-mb") == 50);
    CHECK_EQ(f.text("display", "led-style"), std::string("modernized"));
    CHECK_EQ(f.text("keyboard", "feedback"), std::string("off"));
    CHECK_EQ(f.text("trace", "location"), std::string("app-support"));
    CHECK(f.text("storage", "save-folder").empty() && f.text("trace", "folder").empty());
    for (ModelId m : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) CHECK(cs::startupPreset(f, s59::modelName(m)).empty());
    CHECK(!f.boolean("window", "portrait-debug-page"));
    f.set("trace", "location", "folder");
    CHECK_EQ(f.text("trace", "location"), std::string("folder"));
    f.set("trace", "location", "icloud");
    CHECK_EQ(f.text("trace", "location"), std::string("icloud"));
    f.set("trace", "max-file-size-mb", "12");
    CHECK(f.integer("trace", "max-file-size-mb") == 12);
    f.set("trace", "max-file-size-mb", "0");  // a value (no minimum); the trace session takes it as 50
    CHECK(f.integer("trace", "max-file-size-mb") == 0);

    // A value its key does not accept reads as the default and stays.
    f.set("display", "led-style", "glowing");
    CHECK_EQ(f.text("display", "led-style"), std::string("modernized"));
    CHECK(f.raw("display", "led-style") == std::optional<std::string>("glowing"));

    // [folders]: the folder last used, or a fixed one that picks don't change.
    const std::string home = "/home/u";
    const std::string presets = s59::openFolderKind(s59::OpenFolder::Presets);
    CHECK_EQ(cs::openStartFolder(f, presets, home), home);
    cs::rememberOpenFolder(f, presets, "/data/presets");
    CHECK_EQ(cs::openStartFolder(f, presets, home), std::string("/data/presets"));
    CHECK(cs::fixedOpenFolder(f, presets).empty());
    cs::setFixedOpenFolder(f, presets, "/fixed");
    cs::rememberOpenFolder(f, presets, "/elsewhere");
    CHECK_EQ(cs::openStartFolder(f, presets, home), std::string("/fixed"));
    CHECK_EQ(cs::fixedOpenFolder(f, presets), std::string("/fixed"));
    cs::setFixedOpenFolder(f, presets, "");  // Reset: starts where the fixed folder was
    CHECK(cs::fixedOpenFolder(f, presets).empty());
    CHECK_EQ(cs::openStartFolder(f, presets, home), std::string("/fixed"));
    cs::rememberOpenFolder(f, s59::openFolderKind(s59::OpenFolder::AsmOverlays), "/asm");
    CHECK_EQ(f.text("folders", "asm-overlays"), std::string("/asm"));
}

// `last` without a model used before starts the TI-59.
TEST(startup_last_without_last_model_is_ti59) {
    TestSettings t;
    const auto startup = [&t] { return cs::startupModel(t.file); };
    CHECK(t.file.text("startup", "last-model").empty());
    CHECK_EQ(startup(), std::string("TI-59"));
    t.file.set("startup", "last-model", "TI-58");
    CHECK_EQ(startup(), std::string("TI-58"));
    t.file.set("startup", "model", "TI-58C");
    CHECK_EQ(startup(), std::string("TI-58C"));
    t.file.set("startup", "model", "7");  // neither last nor a model: counts as last
    CHECK_EQ(startup(), std::string("TI-58"));
    t.file.set("startup", "model", "last");
    t.file.set("startup", "last-model", "PC-1500");  // not a model of this product
    CHECK_EQ(startup(), std::string("TI-59"));
    CHECK(s59::modelNamed("TI-58C") == ModelId{kTI58C} && !s59::modelNamed("ti58c"));
    CHECK_EQ(std::string(s59::modelName(ModelId{kTI58C})), std::string("TI-58C"));
}

// The first start has every default and writes the file then, from
// nothing of a platform store (the source check is CTest CalcU59NoPlatformSettings).
TEST(first_start_defaults_and_writes_file) {
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
        CHECK_EQ(cs::startupModel(f), std::string("TI-59"));
        CHECK_EQ(f.text("display", "led-style"), std::string("modernized"));
        CHECK(s59::Settings(f).windowState() == s59::WindowState{});
    }
    CHECK(fs::exists(path));
    const std::string text = readText(path);
    CHECK(text.rfind("format-version = 1", 0) == 0);
    for (const cs::KeyDecl& k : d.keys)
        CHECK(text.find(k.defaultValue.empty() ? k.key + " =\n" : k.key + " = " + k.defaultValue) != std::string::npos);
    // A change survives a new start.
    {
        cs::SettingsFile f = cs::SettingsFile::open(d, path);
        CHECK(f.outcome() == cs::Outcome::Read);
        f.set("keyboard", "feedback", "haptic");
    }
    cs::SettingsFile again = cs::SettingsFile::open(d, path);
    CHECK_EQ(again.text("keyboard", "feedback"), std::string("haptic"));
    // The Qt6 save folder is ~/Calc-U-59 unless [storage] save-folder names another.
    cs::Paths paths(again, dirs);
    CHECK_EQ(paths.saveFolder(), dirs.home + "/Calc-U-59");
    CHECK(paths.traceFolder() == paths.saveFolder());
    CHECK_EQ(paths.saveFolderFile("ti58c.mem"), dirs.home + "/Calc-U-59/ti58c.mem");
}

// No key holds the Solid State ROM module (or any hardware).
TEST(no_key_holds_a_module) {
    for (const cs::KeyDecl& k : s59::settingsDecl().keys) {
        const std::string name = cs::lowerAscii(k.section + "." + k.key);
        CHECK(name.find("module") == std::string::npos);
        CHECK(name.find("solid") == std::string::npos);
        CHECK(name.find("printer.") == std::string::npos);
    }
    // A key from 2.3.0's stores does not count, and is kept as an unknown key.
    TestSettings t;
    t.file.set("startup", "solid-state-module", "LE");
    CHECK(!s59::settingsDecl().find("startup", "solid-state-module"));
}

// Swift's trace location is Application Support (the default),
// iCloud (the save folder) or a chosen folder: the one the bookmark grants,
// passed in, else [trace] folder, else Application Support.
TEST(trace_location_app_support_icloud_folder) {
    TestSettings t;
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
        CHECK_EQ(p.saveFolder(), dirs.saveFolder);
    }
    t.file.set("trace", "location", "icloud");
    CHECK(cs::Paths(t.file, dirs).traceFolder() == dirs.saveFolder);
    t.file.set("trace", "location", "folder");
    CHECK(cs::Paths(t.file, dirs).traceFolder() == dirs.applicationSupport);  // no folder chosen
    t.file.set("trace", "folder", "/Users/u/Traces");
    CHECK(cs::Paths(t.file, dirs).traceFolder() == std::optional<std::string>("/Users/u/Traces"));
    dirs.chosenTraceFolder = [] { return std::string("/Volumes/Granted/Traces"); };  // the bookmark's folder
    CHECK(cs::Paths(t.file, dirs).traceFolder() == std::optional<std::string>("/Volumes/Granted/Traces"));
    // The save folder override is ignored outside Qt6.
    t.file.set("storage", "save-folder", "/elsewhere");
    CHECK_EQ(cs::Paths(t.file, dirs).saveFolder(), dirs.saveFolder);
}

// One declaration for Swift, Qt6 and Web; a key a frontend has no use
// for is kept in the file and ignored.
TEST(one_declaration_for_every_frontend) {
    TempDir tmp("calcu59-lib9-ac25");
    const cs::ProductDecl& d = s59::settingsDecl();
    const std::string path = tmp.str() + "/Calc-U-59/Calc-U-59.ini";
    fs::create_directories(tmp.path() / "Calc-U-59");
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
        f.set("display", "led-style", "classic");
    }
    const std::string text = readText(path);
    CHECK(text.find("feedback = haptic") != std::string::npos);
    CHECK(text.find("save-folder = /qt/cards") != std::string::npos);
    CHECK(text.find("note = kept") != std::string::npos);
    CHECK(text.find("led-style = classic") != std::string::npos);
}

// The Qt6 window's place, size and panel widths are plain numbers.
TEST(window_numbers_round_trip) {
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
    const std::string text = readText(path);
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
