// app/Settings (LIB-9, Calc-U-1600 column: AC-26 to AC-32), app/AppPaths.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Files.hpp"
#include "TempFolder.hpp"
#include "TestSettings.hpp"
#include "app/AppPaths.hpp"
#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
namespace cs = calcu::settings;
using Model = calcu::machine::ModelId;
using cu1600::OpenFolder;

using calcu::testing::readText;

using TempDir = calcu::testing::TempFolder;

// AC-26: the file is Calc-U-1600.ini and holds these keys with these defaults.
TEST(lib9_ac26_declaration) {
    const cs::ProductDecl& d = cu1600::settingsDecl();
    CHECK_EQ(d.productName, std::string("Calc-U-1600"));
    CHECK(cs::checkDeclaration(d).empty());
    for (const std::string& p : cs::checkDeclaration(d)) std::fprintf(stderr, "  %s\n", p.c_str());
    CHECK((d.models == std::vector<std::string>{"PC-1500", "PC-1500A", "PC-1600"}));

    // Every key, in order, with its default; nothing else (AC-8: no hardware).
    const std::vector<std::pair<std::string, std::string>> expected = {
        {"startup.model", "last"},          {"startup.last-model", ""},
        {"preset.PC-1500", ""},             {"preset.PC-1500A", ""},
        {"preset.PC-1600", ""},             {"storage.save-folder", ""},
        {"storage.cassette-folder", ""},        {"trace.folder", ""},
        {"trace.max-file-size-mb", "50"},   {"debug.dap-enabled", "false"},
        {"debug.dap-port", "32168"},        {"serial.link-folder", ""},
        {"folders.presets", ""},            {"folders.basic-programs", ""},
        {"folders.machine-code", ""},       {"folders.host-drive", ""},
        {"window.width", "900"},            {"window.height", "500"},
    };
    std::vector<std::pair<std::string, std::string>> actual;
    for (const cs::KeyDecl& k : d.keys) actual.emplace_back(k.section + "." + k.key, k.defaultValue);
    CHECK(actual == expected);

    // The app-written ones (AC-26: last-model, [folders], [window]).
    std::set<std::string> byApp;
    for (const cs::KeyDecl& k : d.keys)
        if (k.writtenByApp) byApp.insert(k.section + "." + k.key);
    CHECK((byApp == std::set<std::string>{"startup.last-model", "folders.presets", "folders.basic-programs",
                                          "folders.machine-code", "folders.host-drive", "window.width",
                                          "window.height"}));

    // [startup] model accepts last and the three models (AC-26, AC-9).
    const cs::KeyDecl* model = d.find("startup", "model");
    CHECK(model != nullptr);
    for (const char* v : {"last", "PC-1500", "PC-1500A", "PC-1600"}) CHECK(cs::accepts(d, *model, v));
    CHECK(!cs::accepts(d, *model, "PC1500"));

    // The typed calls read those keys.
    TestSettings t;
    cu1600::Settings& s = t.settings;
    CHECK(s.file().integer("trace", "max-file-size-mb") == 50);
    CHECK(!s.file().boolean("debug", "dap-enabled") && s.file().integer("debug", "dap-port") == 32168);
    CHECK(s.startupModelPreference() == "last");
    CHECK(s.file().text("storage", "save-folder").empty() && s.file().text("storage", "cassette-folder").empty() && s.file().text("trace", "folder").empty() &&
          s.file().text("serial", "link-folder").empty());
    for (Model m : {calcu::machine::ModelId{"PC-1500"}, calcu::machine::ModelId{"PC-1500A"}, calcu::machine::ModelId{"PC-1600"}}) CHECK(s.file().text("preset", m.value).empty());
    s.file().set("preset", "PC-1600", "examples:basic/hanoi.pc1600");
    CHECK(t.file.text("preset", "PC-1600") == "examples:basic/hanoi.pc1600");
    s.file().set("debug", "dap-enabled", "true");
    CHECK(t.file.raw("debug", "dap-enabled") == std::optional<std::string>("true") && s.file().boolean("debug", "dap-enabled"));
    s.file().set("trace", "max-file-size-mb", "7");
    CHECK(s.file().integer("trace", "max-file-size-mb") == 7);
    s.file().set("trace", "max-file-size-mb", "0");  // AC-11
    CHECK(s.file().integer("trace", "max-file-size-mb") == 50);
    s.setStartupModelPreference("PC-1600");
    CHECK(t.file.text("startup", "model") == "PC-1600");
}

// AC-27, EC-1: `last` without a model used before starts the PC-1500A.
TEST(lib9_ac27_startup_last_without_last_model_is_pc1500a) {
    TestSettings t;
    cu1600::Settings& s = t.settings;
    CHECK(!cu1600::sharpModel(s.file().text("startup", "last-model")));
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1500A"});
    s.file().set("startup", "last-model", "PC-1600");
    CHECK(t.file.text("startup", "last-model") == "PC-1600");
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1600"});
    s.setStartupModelPreference("PC-1500");
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1500"});
    s.setStartupModelPreference("PC1500");  // not a model's name: counts as last (AC-9)
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1600"});
    s.setStartupModelPreference("last");
    t.file.set("startup", "last-model", "TI-59");  // EC-1
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1500A"});
}

// AC-28: a port outside 1024 to 65535 is 32168.
TEST(lib9_ac28_dap_port_outside_range_is_32168) {
    TestSettings t;
    cu1600::Settings& s = t.settings;
    for (int bad : {80, 1023, 65536, 0, -1}) {
        s.file().set("debug", "dap-port", std::to_string(bad));
        CHECK(s.file().integer("debug", "dap-port") == 32168);
    }
    t.file.set("debug", "dap-port", "abc");
    CHECK(s.file().integer("debug", "dap-port") == 32168);
    for (int good : {1024, 40000, 65535}) {
        s.file().set("debug", "dap-port", std::to_string(good));
        CHECK(s.file().integer("debug", "dap-port") == good);
    }
}

// AC-29: the four kinds of folder and their keys in [folders] (the rule --
// a fixed folder, else the one last used -- is the library's:
// library/settings/tests/startup_tests.cpp,
// ac29_open_folder_fixed_else_last_used_else_home).
TEST(lib9_ac29_folders_four_kinds) {
    const std::pair<OpenFolder, const char*> kinds[] = {{OpenFolder::Samples, "presets"},
                                                         {OpenFolder::Basic, "basic-programs"},
                                                         {OpenFolder::Assembly, "machine-code"},
                                                         {OpenFolder::HostDrive, "host-drive"}};
    for (const auto& [folder, key] : kinds) CHECK_EQ(std::string(cu1600::openFolderKind(folder)), std::string(key));
}

// AC-30: [window] keeps the size the user gave; without it 900 x 500.
TEST(lib9_ac30_window_size_default_900x500_and_kept) {
    TempDir tmp("calcu1600-lib9-window");
    const std::string path = (tmp.path() / "Calc-U-1600.ini").generic_string();
    {
        cs::SettingsFile file = cs::SettingsFile::open(cu1600::settingsDecl(), path);
        cu1600::Settings s(file);
        CHECK(s.windowWidth() == 900 && s.windowHeight() == 500);
        s.setWindowSize(1024, 640);
    }
    cs::SettingsFile again = cs::SettingsFile::open(cu1600::settingsDecl(), path);
    cu1600::Settings s(again);
    CHECK(s.windowWidth() == 1024 && s.windowHeight() == 640);
    const std::string text = readText(path);
    CHECK(text.find("width = 1024") != std::string::npos && text.find("height = 640") != std::string::npos);
}

// AC-31, AC-5: the first start has every default and writes the file at
// that moment, with its version and every key. Nothing is taken from a
// platform store (that no frontend uses one: CTest CalcU1600NoPlatformSettings).
TEST(lib9_ac31_first_start_defaults_and_writes_file) {
    TempDir tmp("calcu1600-lib9-first-start");
    cs::HostDirs dirs = testHostDirs((tmp.path() / "home").generic_string());
    const std::string path = cs::settingsFilePath(cu1600::settingsDecl(), dirs);
    CHECK(path == (tmp.path() / "home" / "settings" / "Calc-U-1600" / "Calc-U-1600.ini").generic_string());
    CHECK(!fs::exists(path));

    cs::SettingsFile file = cs::SettingsFile::open(cu1600::settingsDecl(), path);
    CHECK(file.outcome() == cs::Outcome::Created);
    CHECK(fs::exists(path));
    const std::string text = readText(path);
    CHECK(text == cs::SettingsFile::defaultText(cu1600::settingsDecl()));
    CHECK(text.rfind("format-version = 1\n", 0) == 0);

    cu1600::Settings s(file);
    CHECK(s.startupModel() == calcu::machine::ModelId{"PC-1500A"});
    CHECK(s.windowWidth() == 900 && s.file().integer("debug", "dap-port") == 32168 && s.file().integer("trace", "max-file-size-mb") == 50);

    // The save folder is ~/Calc-U-1600 on Qt6 (LIB-9 AC-13).
    cu1600::AppPaths paths(s, dirs, cu1600::BundleDirs{(tmp.path() / "bundle").generic_string(), {}});
    CHECK(paths.instanceDir() == (tmp.path() / "home" / "Calc-U-1600").generic_string());
    CHECK(paths.places().settingsFile() == path);
}

// AC-32: Qt6 and the Web use the one declaration.
TEST(lib9_ac32_web_and_qt6_share_declaration) {
    // Both frontends open their file through cu1600::settingsDecl(), one
    // declaration: one set of names, values and defaults (AC-26's table).
    // The Web's place: /persist/settings/Calc-U-1600/Calc-U-1600.ini (design §2).
    cs::HostDirs web;
    web.frontend = cs::FrontendKind::Web;
    web.home = "/persist";
    web.settingsBase = cs::settingsBaseFor(cs::HostOs::Web, {}, "/persist", "/persist/settings");
    web.saveFolder = "/persist/Calc-U-1600";
    CHECK(cs::settingsFilePath(cu1600::settingsDecl(), web) == "/persist/settings/Calc-U-1600/Calc-U-1600.ini");
}

// AC-32: the keys the Web has no use for are kept in the file and ignored.
TEST(lib9_ac32_web_keeps_and_ignores_dap_serial_window_folders) {
    TempDir tmp("calcu1600-lib9-web-keeps");
    const fs::path ini = tmp.path() / "settings" / "Calc-U-1600" / "Calc-U-1600.ini";
    std::string text = cs::SettingsFile::defaultText(cu1600::settingsDecl());
    auto put = [&](const std::string& from, const std::string& to) {
        const auto at = text.find(from);
        CHECK(at != std::string::npos);
        if (at != std::string::npos) text.replace(at, from.size(), to);
    };
    put("dap-enabled = false", "dap-enabled = true");
    put("link-folder =", "link-folder = /links");
    put("save-folder =", "save-folder = /elsewhere");
    put("width = 900", "width = 1200");
    put("host-drive =", "host-drive = fixed:/drive");
    text += "\n[unknown]\nkept = yes\n";
    calcu::testing::writeText(ini, text);

    cs::HostDirs web;
    web.frontend = cs::FrontendKind::Web;
    web.home = (tmp.path() / "persist").generic_string();
    web.settingsBase = (tmp.path() / "settings").generic_string();
    web.saveFolder = (tmp.path() / "persist" / "Calc-U-1600").generic_string();
    cs::SettingsFile file = cs::SettingsFile::open(cu1600::settingsDecl(), ini.generic_string());
    CHECK(file.outcome() == cs::Outcome::Read);
    cu1600::Settings s(file);
    cu1600::AppPaths paths(s, web, cu1600::BundleDirs{(tmp.path() / "bundle").generic_string(), {}});
    // Ignored: the save folder override, the serial link folder (Qt6 only).
    CHECK(paths.instanceDir() == web.saveFolder);
    CHECK(paths.serialLinkDir() == web.saveFolder);
    CHECK(paths.traceDir().empty());  // no trace folder on the Web (LIB-10 D-10; A-3 went in M-45)

    // A change the Web makes keeps them.
    s.setStartupModelPreference("PC-1600");
    s.file().set("preset", "PC-1500", "examples:x.pc1500");
    const std::string after = readText(ini);
    for (const char* line : {"dap-enabled = true", "link-folder = /links", "save-folder = /elsewhere",
                             "width = 1200", "host-drive = fixed:/drive", "kept = yes", "model = PC-1600",
                             "PC-1500 = examples:x.pc1500"})
        CHECK(after.find(line) != std::string::npos);
}

TEST(test_app_paths) {
    TempDir tmp("calcu-shared-paths-test");
    const fs::path& root = tmp.path();
    const std::string home = (root / "home").generic_string();
    TestSettings t;
    cu1600::Settings& s = t.settings;
    cu1600::AppPaths paths(s, testHostDirs(home), cu1600::BundleDirs{(root / "bundle").generic_string(), {}});

    CHECK(paths.instanceDir() == home + "/Calc-U-1600");
    CHECK(fs::is_directory(home + "/Calc-U-1600"));
    CHECK(paths.cassetteDir() == home + "/Calc-U-1600/Cassettes");
    CHECK(paths.traceDir() == home + "/Calc-U-1600");
    CHECK(paths.serialLinkDir() == home + "/Calc-U-1600");
    CHECK(paths.displayPath(home + "/Calc-U-1600/x.module.yaml") == "~/Calc-U-1600/x.module.yaml");
    CHECK(paths.displayPath(home + "/Documents/y") == "~/Documents/y");
    CHECK(paths.displayPath(home + "-other/y") == home + "-other/y");
    CHECK(paths.displayPath("/elsewhere") == "/elsewhere");

    t.file.set("storage", "save-folder", (root / "saves").generic_string());
    CHECK(paths.instanceDir() == (root / "saves").generic_string());
    CHECK(fs::is_directory(root / "saves"));
    CHECK(paths.cassetteDir() == (root / "saves" / "Cassettes").generic_string());  // LIB-9 EC-3
    t.file.set("storage", "cassette-folder", (root / "tapes").generic_string());
    CHECK(paths.cassetteDir() == (root / "tapes").generic_string());
    t.file.set("trace", "folder", (root / "traces").generic_string());
    CHECK(paths.traceDir() == (root / "traces").generic_string());


    std::ofstream((root / "saves" / "f.txt").string()) << "x";
    CHECK(cu1600::isUnderDir((root / "saves" / "f.txt").generic_string(), (root / "saves").generic_string()));
    CHECK(cu1600::isUnderDir((root / "saves").generic_string(), (root / "saves").generic_string()));
    fs::create_directories(root / "saves-backup");
    CHECK(!cu1600::isUnderDir((root / "saves-backup").generic_string(), (root / "saves").generic_string()));
    CHECK(!cu1600::isUnderDir((root / "missing").generic_string(), (root / "saves").generic_string()));
}

} // namespace
