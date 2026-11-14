#include "TestHarness.hpp"
#include "settings/tests/TestProduct.hpp"
#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include <filesystem>
#include <map>

// LIB-9 AC-2, AC-12–18, AC-33 (paths), EC-2, EC-3, EC-4, EC-8, EC-9;
// design §6, T-7 to T-11. Every platform's places are checked on every host:
// the functions are pure and take what the caller passes.

using namespace calcu::settings;
using namespace calcu::settings::test;

namespace {
EnvLookup envOf(std::map<std::string, std::string> vars) {
    return [vars](const char* name) -> std::optional<std::string> {
        auto it = vars.find(name);
        if (it == vars.end()) return std::nullopt;
        return it->second;
    };
}

ProductDecl named(const std::string& name) {
    ProductDecl d = testProduct();
    d.productName = name;
    return d;
}

HostDirs qt6(const std::string& home, const std::string& base = "/cfg") {
    HostDirs d;
    d.frontend = FrontendKind::Qt6;
    d.home = home;
    d.settingsBase = base;
    return d;
}

SettingsFile withValues(const std::string& iniPath, const std::string& body) {
    writeText(iniPath, "format-version = 1\n" + body);
    return SettingsFile::open(testProduct(), iniPath);
}
} // namespace

TEST(ac2_settings_place_macos_ios) {
    const EnvLookup none = envOf({});
    // The Application Support folder the frontend found: inside the sandbox container ...
    const std::string sandboxed = "/Users/u/Library/Containers/app.id/Data/Library/Application Support";
    CHECK_EQ(settingsBaseFor(HostOs::MacOS, none, "/Users/u", sandboxed), sandboxed);
    CHECK_EQ(settingsBaseFor(HostOs::IOS, none, "/var/mobile/x", "/var/mobile/x/Library/Application Support"),
             std::string("/var/mobile/x/Library/Application Support"));
    // ... or ~/Library/Application Support for an app outside a sandbox.
    CHECK_EQ(settingsBaseFor(HostOs::MacOS, none, "/Users/u", ""),
             std::string("/Users/u/Library/Application Support"));
    HostDirs d = qt6("/Users/u", settingsBaseFor(HostOs::MacOS, none, "/Users/u", ""));
    CHECK_EQ(settingsFilePath(named("Calc-U-1600"), d),
             std::string("/Users/u/Library/Application Support/Calc-U-1600/Calc-U-1600.ini"));
    // The environment is not consulted on macOS.
    CHECK_EQ(settingsBaseFor(HostOs::MacOS, envOf({{"XDG_CONFIG_HOME", "/x"}}), "/Users/u", ""),
             std::string("/Users/u/Library/Application Support"));
}

TEST(ac2_settings_place_windows_appdata) {
    const std::string base =
        settingsBaseFor(HostOs::Windows, envOf({{"APPDATA", "C:\\Users\\u\\AppData\\Roaming"}}), "C:\\Users\\u", "");
    CHECK_EQ(base, std::string("C:\\Users\\u\\AppData\\Roaming"));
    HostDirs d = qt6("C:\\Users\\u", base);
    CHECK_EQ(settingsFilePath(named("Calc-U-59"), d),
             std::string("C:\\Users\\u\\AppData\\Roaming\\Calc-U-59\\Calc-U-59.ini"));
    // Without %APPDATA%: its usual place under the home folder.
    CHECK_EQ(settingsBaseFor(HostOs::Windows, envOf({}), "C:\\Users\\u", ""),
             std::string("C:\\Users\\u\\AppData\\Roaming"));
}

TEST(ac2_settings_place_linux_xdg_and_fallback) {
    CHECK_EQ(settingsBaseFor(HostOs::Linux, envOf({{"XDG_CONFIG_HOME", "/home/u/conf"}}), "/home/u", ""),
             std::string("/home/u/conf"));
    CHECK_EQ(settingsBaseFor(HostOs::Linux, envOf({}), "/home/u", ""), std::string("/home/u/.config"));
    CHECK_EQ(settingsBaseFor(HostOs::Linux, envOf({{"XDG_CONFIG_HOME", ""}}), "/home/u", ""),
             std::string("/home/u/.config"));
    HostDirs d = qt6("/home/u", settingsBaseFor(HostOs::Linux, envOf({}), "/home/u", ""));
    CHECK_EQ(settingsFilePath(named("Calc-U-1600"), d), std::string("/home/u/.config/Calc-U-1600/Calc-U-1600.ini"));
}

TEST(ac2_settings_place_web) {
    const std::string base = settingsBaseFor(HostOs::Web, envOf({{"HOME", "/home/web_user"}}), "/home/web_user",
                                             "/persist/settings");
    CHECK_EQ(base, std::string("/persist/settings"));
    HostDirs d;
    d.frontend = FrontendKind::Web;
    d.settingsBase = base;
    d.saveFolder = "/persist/Calc-U-1600";
    CHECK_EQ(settingsFilePath(named("Calc-U-1600"), d), std::string("/persist/settings/Calc-U-1600/Calc-U-1600.ini"));
}

TEST(ac2_product_name_as_written) {
    // The same spelling on every platform, also on Linux (not lower-cased).
    for (const char* product : {"Calc-U-59", "Calc-U-1600"}) {
        const std::string p = product;
        CHECK_EQ(settingsFilePath(named(p), qt6("/home/u", "/home/u/.config")),
                 "/home/u/.config/" + p + "/" + p + ".ini");
    }
    CHECK_EQ(joinPath("/a/", "b"), std::string("/a/b"));
    CHECK_EQ(joinPath("C:\\a\\", "b"), std::string("C:\\a\\b"));
    CHECK_EQ(joinPath("C:/a", "b"), std::string("C:/a/b"));
}

TEST(ec4_two_settings_bases_two_files) {
    // The Swift app (sandboxed) and a Qt6 build of the same product on one Mac.
    HostDirs swift;
    swift.frontend = FrontendKind::Swift;
    swift.home = "/Users/u";
    swift.settingsBase = settingsBaseFor(HostOs::MacOS, envOf({}), "/Users/u",
                                         "/Users/u/Library/Containers/app.id/Data/Library/Application Support");
    HostDirs qt = qt6("/Users/u", settingsBaseFor(HostOs::MacOS, envOf({}), "/Users/u", ""));
    CHECK(settingsFilePath(named("Calc-U-59"), swift) != settingsFilePath(named("Calc-U-59"), qt));
    // A change in one does not show in the other.
    TempDir dir;
    const std::string a = dir / "a/Test-Product/Test-Product.ini";
    const std::string b = dir / "b/Test-Product/Test-Product.ini";
    SettingsFile fa = SettingsFile::open(testProduct(), a);
    CHECK(fa.set("test", "style", "two").ok);
    CHECK_EQ(SettingsFile::open(testProduct(), b).text("test", "style"), std::string("one"));
}

TEST(ac12_every_place_from_the_passed_dirs) {
    TempDir dir;
    HostDirs d = qt6(dir / "home", dir / "config");
    const SettingsFile f = SettingsFile::open(testProduct(), settingsFilePath(testProduct(), d));
    const Paths p(f, d);
    const std::string save = dir / "home/Test-Product";
    CHECK_EQ(p.settingsFile(), dir / "config/Test-Product/Test-Product.ini");
    CHECK_EQ(p.saveFolder(), save);
    CHECK_EQ(p.tapeFolder(), save + "/Tapes");
    CHECK(p.traceFolder() == std::optional<std::string>(save));
    CHECK(p.screenshotFolder() == std::optional<std::string>(save));
    CHECK(p.serialLinkFolder() == std::optional<std::string>(save));
    CHECK_EQ(p.saveFolderFile("ti58c.memory.yaml"), save + "/ti58c.memory.yaml");
    CHECK(p.folder(Folder::Tapes) == std::optional<std::string>(save + "/Tapes"));
    // A folder the user chose through the platform is passed in like a host directory.
    HostDirs s;
    s.frontend = FrontendKind::Swift;
    s.home = "/Users/u";
    s.saveFolder = "/icloud/Documents";
    s.applicationSupport = "/app-support";
    s.chosenTraceFolder = "/Volumes/Traces";
    const SettingsFile sf = SettingsFile::defaultsOnly(testProduct());
    SettingsFile chosen = SettingsFile::defaultsOnly(testProduct());
    chosen.set("trace", "location", "folder");
    CHECK(Paths(chosen, s).traceFolder() == std::optional<std::string>("/Volumes/Traces"));
    CHECK(Paths(sf, s).traceFolder() == std::optional<std::string>("/app-support"));
}

TEST(ac13_qt6_save_folder_home_product_or_override) {
    TempDir dir;
    const std::string ini = dir / "cfg/Test-Product/Test-Product.ini";
    const HostDirs d = qt6("/home/u");
    CHECK_EQ(Paths(SettingsFile::open(testProduct(), ini), d).saveFolder(), std::string("/home/u/Test-Product"));
    const SettingsFile over = withValues(ini, "[storage]\nsave-folder = /data/mine\n");
    CHECK_EQ(Paths(over, d).saveFolder(), std::string("/data/mine"));
    const SettingsFile tilde = withValues(ini, "[storage]\nsave-folder = ~/elsewhere\n");
    CHECK_EQ(Paths(tilde, d).saveFolder(), std::string("/home/u/elsewhere"));
}

TEST(ac33_paths_absolute_tilde_is_home) {
    CHECK_EQ(expandHome("~", "/home/u"), std::string("/home/u"));
    CHECK_EQ(expandHome("~/a/b", "/home/u"), std::string("/home/u/a/b"));
    CHECK_EQ(expandHome("/abs/~x", "/home/u"), std::string("/abs/~x"));
    CHECK_EQ(expandHome("~other/x", "/home/u"), std::string("~other/x"));  // only `~` and `~/`
    CHECK_EQ(expandHome("~\\x", "C:\\Users\\u"), std::string("C:\\Users\\u\\x"));
    // A relative path is not accepted: the setting has its default (EC-5).
    TempDir dir;
    const std::string ini = dir / "cfg/Test-Product/Test-Product.ini";
    const SettingsFile rel = withValues(ini, "[storage]\nsave-folder = data/mine\n");
    CHECK_EQ(Paths(rel, qt6("/home/u")).saveFolder(), std::string("/home/u/Test-Product"));
    // The stored text keeps the `~`.
    const SettingsFile tilde = withValues(ini, "[storage]\nsave-folder = ~/x\n");
    CHECK_EQ(tilde.text("storage", "save-folder"), std::string("~/x"));
    // Windows paths are absolute too.
    const SettingsFile win = withValues(ini, "[storage]\nsave-folder = D:\\Calc\n");
    CHECK_EQ(Paths(win, qt6("C:\\Users\\u")).saveFolder(), std::string("D:\\Calc"));
}

TEST(ac14_swift_save_folder_is_passed_folder) {
    HostDirs s;
    s.frontend = FrontendKind::Swift;
    s.home = "/Users/u";
    s.saveFolder = "/Users/u/Library/Mobile Documents/iCloud~app/Documents";
    TempDir dir;
    const std::string ini = dir / "cfg/Test-Product/Test-Product.ini";
    // [storage] save-folder is used by Qt6 only.
    const SettingsFile f = withValues(ini, "[storage]\nsave-folder = /data/mine\n");
    CHECK_EQ(Paths(f, s).saveFolder(), s.saveFolder);
    CHECK_EQ(Paths(f, s).saveFolderFile("ti58c.memory.yaml"), s.saveFolder + "/ti58c.memory.yaml");
}

TEST(ec2_swift_save_folder_follows_each_start) {
    const SettingsFile f = SettingsFile::defaultsOnly(testProduct());
    HostDirs s;
    s.frontend = FrontendKind::Swift;
    s.home = "/Users/u";
    s.saveFolder = "/icloud/Documents";  // iCloud available at this start
    CHECK_EQ(Paths(f, s).saveFolder(), std::string("/icloud/Documents"));
    s.saveFolder = "/local/Documents";   // not at the next
    CHECK_EQ(Paths(f, s).saveFolder(), std::string("/local/Documents"));
    CHECK_EQ(Paths(f, s).tapeFolder(), std::string("/local/Documents/Tapes"));
}

TEST(ac15_web_save_folder_inside_passed_fs) {
    HostDirs w;
    w.frontend = FrontendKind::Web;
    w.home = "/home/web_user";
    w.settingsBase = "/persist/settings";
    w.saveFolder = "/persist/Test-Product";
    TempDir dir;
    const SettingsFile f = withValues(dir / "x/Test-Product.ini", "[storage]\nsave-folder = /elsewhere\n");
    const Paths p(f, w);
    CHECK_EQ(p.saveFolder(), std::string("/persist/Test-Product"));
    CHECK_EQ(p.settingsFile(), std::string("/persist/settings/Test-Product/Test-Product.ini"));
    CHECK_EQ(p.tapeFolder(), std::string("/persist/Test-Product/Tapes"));
}

TEST(ac16_files_at_root_tapes_in_Tapes) {
    const SettingsFile f = SettingsFile::defaultsOnly(testProduct());
    const Paths p(f, qt6("/home/u"));
    CHECK_EQ(p.saveFolderFile("ti58c.memory.yaml"), std::string("/home/u/Test-Product/ti58c.memory.yaml"));
    CHECK_EQ(p.saveFolderFile("card.yaml"), std::string("/home/u/Test-Product/card.yaml"));
    CHECK_EQ(p.tapeFolder(), std::string("/home/u/Test-Product/Tapes"));
}

TEST(ac16_tape_override) {
    TempDir dir;
    const SettingsFile f = withValues(dir / "c/Test-Product.ini", "[storage]\ntape-folder = /tapes/here\n");
    CHECK_EQ(Paths(f, qt6("/home/u")).tapeFolder(), std::string("/tapes/here"));
    CHECK_EQ(Paths(f, qt6("/home/u")).saveFolder(), std::string("/home/u/Test-Product"));
}

TEST(ec3_override_save_folder_keeps_Tapes_inside) {
    TempDir dir;
    const SettingsFile f =
        withValues(dir / "c/Test-Product.ini", "[storage]\nsave-folder = /data/mine\ntape-folder =\n");
    CHECK_EQ(Paths(f, qt6("/home/u")).tapeFolder(), std::string("/data/mine/Tapes"));
}

TEST(ac17_qt6_traces_screenshots_links_default_to_save_folder) {
    TempDir dir;
    const std::string ini = dir / "c/Test-Product.ini";
    const SettingsFile plain = withValues(ini, "");
    const Paths p(plain, qt6("/home/u"));
    CHECK(p.traceFolder() == std::optional<std::string>("/home/u/Test-Product"));
    CHECK(p.screenshotFolder() == std::optional<std::string>("/home/u/Test-Product"));
    CHECK(p.serialLinkFolder() == std::optional<std::string>("/home/u/Test-Product"));
    const SettingsFile over = withValues(ini, "[trace]\nfolder = /t\n[serial]\nlink-folder = ~/links\n");
    const Paths q(over, qt6("/home/u"));
    CHECK(q.traceFolder() == std::optional<std::string>("/t"));
    CHECK(q.serialLinkFolder() == std::optional<std::string>("/home/u/links"));
    CHECK(q.screenshotFolder() == std::optional<std::string>("/home/u/Test-Product"));
    // [trace] location is the Swift frontend's: Qt6 ignores it.
    const SettingsFile loc = withValues(ini, "[trace]\nlocation = icloud\n");
    CHECK(Paths(loc, qt6("/home/u")).traceFolder() == std::optional<std::string>("/home/u/Test-Product"));
}

TEST(ac17_swift_screenshots_save_folder_traces_by_location) {
    HostDirs s;
    s.frontend = FrontendKind::Swift;
    s.home = "/Users/u";
    s.saveFolder = "/icloud/Documents";
    s.applicationSupport = "/app-support";
    TempDir dir;
    const std::string ini = dir / "c/Test-Product.ini";
    const SettingsFile def = withValues(ini, "");
    CHECK(Paths(def, s).screenshotFolder() == std::optional<std::string>("/icloud/Documents"));
    CHECK(Paths(def, s).traceFolder() == std::optional<std::string>("/app-support"));  // the default
    CHECK(!Paths(def, s).serialLinkFolder());
    const SettingsFile icloud = withValues(ini, "[trace]\nlocation = icloud\n");
    CHECK(Paths(icloud, s).traceFolder() == std::optional<std::string>("/icloud/Documents"));
    // With `folder`: the folder the bookmark grants, passed in (AC-34) ...
    const SettingsFile folder = withValues(ini, "[trace]\nlocation = folder\nfolder = /plain/path\n");
    s.chosenTraceFolder = "/granted";
    CHECK(Paths(folder, s).traceFolder() == std::optional<std::string>("/granted"));
    // ... without a usable grant, the plain path of [trace] folder, then Application Support (D-8).
    s.chosenTraceFolder.clear();
    CHECK(Paths(folder, s).traceFolder() == std::optional<std::string>("/plain/path"));
    const SettingsFile bare = withValues(ini, "[trace]\nlocation = folder\n");
    CHECK(Paths(bare, s).traceFolder() == std::optional<std::string>("/app-support"));
    // An unknown location has the default.
    const SettingsFile odd = withValues(ini, "[trace]\nlocation = moon\n");
    CHECK(Paths(odd, s).traceFolder() == std::optional<std::string>("/app-support"));
}

TEST(ac17_web_has_no_trace_or_screenshot_folder) {
    HostDirs w;
    w.frontend = FrontendKind::Web;
    w.settingsBase = "/persist/settings";
    w.saveFolder = "/persist/Test-Product";
    TempDir dir;
    const SettingsFile f = withValues(dir / "c/Test-Product.ini", "[trace]\nfolder = /t\n");
    const Paths p(f, w);
    CHECK(!p.traceFolder());       // a trace that has ended is offered as a download
    CHECK(!p.screenshotFolder());  // and so is a screenshot
    CHECK(!p.serialLinkFolder());
    const Paths::Ensured e = p.ensure(Folder::Traces);
    CHECK(!e.ok && !e.problem.empty());
}

TEST(ec8_missing_folder_created_on_first_write) {
    TempDir dir;
    const HostDirs d = qt6(dir / "home", dir / "cfg");
    const SettingsFile f = SettingsFile::defaultsOnly(testProduct());
    const Paths p(f, d);
    CHECK(!exists(p.saveFolder()));
    const Paths::Ensured tapes = p.ensure(Folder::Tapes);  // creates the save folder too
    CHECK(tapes.ok);
    CHECK_EQ(tapes.path, dir / "home/Test-Product/Tapes");
    CHECK(std::filesystem::is_directory(dir / "home/Test-Product/Tapes"));
    const Paths::Ensured save = p.ensure(Folder::Save);
    CHECK(save.ok && save.problem.empty());
    // An existing folder is fine as it is; nothing is left behind by the check.
    writeText(dir / "home/Test-Product/kept.txt", "x");
    CHECK(p.ensure(Folder::Save).ok);
    std::size_t entries = 0;
    for (const auto& e : std::filesystem::directory_iterator(dir / "home/Test-Product")) {
        (void)e;
        ++entries;
    }
    CHECK_EQ(entries, std::size_t{2});  // Tapes and kept.txt
}

TEST(ec9_unusable_override_reported_no_fallback) {
    TempDir dir;
    writeText(dir / "a-file", "not a folder");
    const std::string ini = dir / "cfg/Test-Product/Test-Product.ini";
    const SettingsFile f = withValues(ini, "[storage]\nsave-folder = " + (dir / "a-file/save") + "\n");
    const HostDirs d = qt6(dir / "home", dir / "cfg");
    const Paths p(f, d);
    const Paths::Ensured e = p.ensure(Folder::Save);
    CHECK(!e.ok);
    CHECK(!e.problem.empty());
    CHECK_EQ(e.path, dir / "a-file/save");  // the folder the user named, reported
    CHECK(!p.ensure(Folder::Tapes).ok);
    // Nothing is written to another folder instead.
    CHECK(!exists(dir / "home/Test-Product"));
    CHECK_EQ(p.saveFolder(), dir / "a-file/save");
}

TEST(ac18_own_host_dirs_touch_nothing_else) {
    // The "user's" home with a settings file and a save folder of its own.
    TempDir user;
    const std::string userIni = user / "config/Test-Product/Test-Product.ini";
    const std::string userText = "format-version = 1\n[test]\nstyle = two\n";
    writeText(userIni, userText);
    writeText(user / "Test-Product/card.yaml", "mine");
    // A run with host directories of its own.
    TempDir run;
    const HostDirs d = qt6(run / "home", run / "config");
    SettingsFile f = SettingsFile::open(testProduct(), settingsFilePath(testProduct(), d));
    CHECK(f.outcome() == Outcome::Created);
    CHECK_EQ(f.text("test", "style"), std::string("one"));  // not the user's value
    CHECK(f.set("test", "name", "run").ok);
    const Paths p(f, d);
    for (Folder which : {Folder::Save, Folder::Tapes, Folder::Traces, Folder::Screenshots, Folder::SerialLinks})
        CHECK(p.ensure(which).ok);
    CHECK_EQ(p.saveFolder().rfind(run.str(), 0), std::size_t{0});
    CHECK_EQ(p.settingsFile().rfind(run.str(), 0), std::size_t{0});
    // The user's files are as they were.
    CHECK_EQ(readText(userIni), userText);
    CHECK_EQ(readText(user / "Test-Product/card.yaml"), std::string("mine"));
    std::size_t entries = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(user.str())) {
        (void)e;
        ++entries;
    }
    CHECK_EQ(entries, std::size_t{5});  // config, config/Test-Product, the ini, Test-Product, card.yaml
}
