#include "TestHarness.hpp"
#include "settings/tests/TestProduct.hpp"
#include "settings/AtomicFile.hpp"
#include "settings/SettingsFile.hpp"
#include <algorithm>
#include <filesystem>
#include <set>

// The settings file: the declaration, defaults, the first start, versions,
// unreadable files, writing through and merging, the booleans.

using namespace calcu::settings;
using namespace calcu::settings::test;

namespace {
const std::string kVersioned = "format-version = 1\n";

std::string iniPath(const TempDir& dir) { return dir / "Test-Product/Test-Product.ini"; }
} // namespace

TEST(settings_declaration_names_kebab_case) {
    CHECK(checkDeclaration(testProduct()).empty());
    const auto keyIsKebab = [](const std::string& key) {
        ProductDecl d = testProduct();
        d.keys.push_back(testKey("test", key, ValueKind::Text, ""));
        return checkDeclaration(d).empty();
    };
    CHECK(keyIsKebab("max-file-size-mb") && keyIsKebab("startup-x") && keyIsKebab("a1"));
    CHECK(!keyIsKebab("maxFileSize") && !keyIsKebab("max_file") && !keyIsKebab("-a") && !keyIsKebab("a-") &&
          !keyIsKebab("a--b") && !keyIsKebab("") && !keyIsKebab("A"));
    // [preset] keys are model names as the glossary writes them; others are kebab-case.
    ProductDecl bad = testProduct();
    bad.keys.push_back(testKey("test", "camelCase", ValueKind::Text, ""));
    bad.keys.push_back(testKey("preset", "Model-C", ValueKind::Text, ""));  // not a model
    bad.keys.push_back(testKey("Window", "x", ValueKind::Integer, "0"));
    bad.keys.push_back(family::startupModel());                            // twice
    bad.keys.push_back(testKey("test", "choice", ValueKind::Choice, "three", {"one", "two"}));
    CHECK_EQ(checkDeclaration(bad).size(), std::size_t{5});
    // The file is <Product>.ini with [section] headers and key = value lines.
    const std::string text = SettingsFile::defaultText(testProduct());
    CHECK(text.find("[startup]\n") != std::string::npos);
    CHECK(text.find("\nmodel = last\n") != std::string::npos);
    CHECK(text.find("\nModel-A = \n") != std::string::npos || text.find("\nModel-A =\n") != std::string::npos);
}

TEST(settings_missing_key_has_default) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned + "[test]\nname = mine\n");
    const SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(f.outcome() == Outcome::Read);
    CHECK_EQ(f.text("test", "name"), std::string("mine"));
    CHECK_EQ(f.text("test", "style"), std::string("one"));
    CHECK_EQ(f.boolean("test", "flag"), false);
    CHECK_EQ(f.integer("test", "port"), 32168L);
    CHECK_EQ(f.text("startup", "model"), std::string("last"));
    CHECK_EQ(f.text("trace", "location"), std::string("app-support"));
    CHECK_EQ(f.integer("window", "width"), 900L);
    CHECK(!f.raw("test", "style"));
    // An undeclared key gives its text, or empty.
    CHECK_EQ(f.text("other", "thing"), std::string(""));
}

TEST(settings_booleans_true_false) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned + "[test]\nflag = true\n");
    CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).boolean("test", "flag"), true);
    for (const char* other : {"yes", "1", "on", "TRUE"}) {
        writeText(iniPath(dir), kVersioned + "[test]\nflag = " + other + "\n");
        CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).boolean("test", "flag"), false);
    }
}

TEST(settings_bad_value_reads_default_and_stays_in_file) {
    TempDir dir;
    const std::string original = kVersioned +
                                 "[test]\nport = many\nstyle = three\nflag = maybe\n"
                                 "[trace]\nfolder = relative/path\n[window]\nwidth = 9x\n";
    writeText(iniPath(dir), original);
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK_EQ(f.integer("test", "port"), 32168L);
    CHECK_EQ(f.text("test", "style"), std::string("one"));
    CHECK_EQ(f.boolean("test", "flag"), false);
    CHECK_EQ(f.text("trace", "folder"), std::string(""));  // a path is absolute
    CHECK_EQ(f.integer("window", "width"), 900L);
    CHECK(f.raw("test", "style") == std::optional<std::string>("three"));
    // Outside the range is not accepted either.
    writeText(iniPath(dir), kVersioned + "[test]\nport = 80\n");
    CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).integer("test", "port"), 32168L);
    // The bad value stays in the file when another setting is written ...
    writeText(iniPath(dir), original);
    SettingsFile g = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(g.set("test", "name", "changed").ok);
    const std::string onDisk = readText(iniPath(dir));
    CHECK(onDisk.find("port = many\n") != std::string::npos);
    CHECK(onDisk.find("style = three\n") != std::string::npos);
    // ... until that setting is changed.
    CHECK(g.set("test", "style", "two").ok);
    CHECK(readText(iniPath(dir)).find("style = two\n") != std::string::npos);
    CHECK_EQ(g.text("test", "style"), std::string("two"));
}

// The trace size: missing or not a number is the default 50; zero and
// negatives are values of the key (no minimum), which the trace session takes
// as 50 (debugger/tests/trace_tests.cpp).
TEST(trace_size_missing_zero_negative_is_50) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned);
    CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).integer("trace", "max-file-size-mb"), 50L);
    for (const char* v : {"", "ten"}) {
        writeText(iniPath(dir), kVersioned + "[trace]\nmax-file-size-mb = " + v + "\n");
        CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).integer("trace", "max-file-size-mb"), 50L);
    }
    for (const auto& [v, n] : {std::pair{"0", 0L}, std::pair{"-3", -3L}, std::pair{"7", 7L}}) {
        writeText(iniPath(dir), kVersioned + "[trace]\nmax-file-size-mb = " + v + "\n");
        CHECK_EQ(SettingsFile::open(testProduct(), iniPath(dir)).integer("trace", "max-file-size-mb"), n);
    }
    CHECK(!family::traceMaxFileSizeMb().min);   // the settings dialog takes 0 too
    CHECK_EQ(family::traceMaxFileSizeMb().help, std::string("The largest trace file in MiB; zero or less: 50."));
}

TEST(settings_declaration_holds_no_hardware) {
    // The family keys live in these sections and hold preferences, the
    // startup model and presets, folders and window state.
    const std::set<std::string> st4 = {"startup", "preset", "storage", "trace", "serial",
                                       "folders", "window", "debug", "display", "keyboard"};
    for (const KeyDecl& k : testProduct().keys)
        if (k.section != "test") CHECK(st4.count(k.section) == 1);
    // No key names a module, a peripheral or a medium.
    for (const KeyDecl& k : testProduct().keys)
        for (const char* word : {"module", "peripheral", "medium", "media", "card", "slot"})
            CHECK(k.key.find(word) == std::string::npos && k.section.find(word) == std::string::npos);
}

TEST(settings_first_start_writes_version_and_every_key) {
    TempDir dir;
    CHECK(!exists(iniPath(dir)));
    const SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(f.outcome() == Outcome::Created);
    CHECK(f.problem().empty());
    CHECK(f.writable());
    CHECK(exists(iniPath(dir)));  // the folder is created too
    const std::string text = readText(iniPath(dir));
    CHECK_EQ(text.substr(0, kVersioned.size()), kVersioned);
    CHECK_EQ(text, SettingsFile::defaultText(testProduct()));
    // Every declared key at its default, a help line above each.
    const ParseResult r = IniDocument::parse(text);
    CHECK(r.doc.has_value());
    if (!r.doc) return;
    for (const KeyDecl& k : testProduct().keys)
        CHECK(r.doc->get(k.section, k.key) == std::optional<std::string>(k.defaultValue));
    CHECK(text.find("# A boolean.\nflag = false\n") != std::string::npos);
    // Every setting has its default.
    CHECK_EQ(f.text("startup", "model"), std::string("last"));
    CHECK_EQ(f.integer("trace", "max-file-size-mb"), 50L);
    // It is read as written at the next start.
    CHECK(SettingsFile::open(testProduct(), iniPath(dir)).outcome() == Outcome::Read);
}

// A key's first value goes into a new file (a fresh install's startup
// preset); a run without a file and a file without the key keep the default.
TEST(settings_first_value_only_in_a_new_file) {
    ProductDecl d = testProduct();
    for (KeyDecl& k : d.keys)
        if (k.section == "preset" && k.key == "Model-A") k.firstValue = "examples:startup/a.test";
    TempDir dir;
    const SettingsFile f = SettingsFile::open(d, iniPath(dir));
    CHECK(f.outcome() == Outcome::Created);
    CHECK_EQ(f.text("preset", "Model-A"), std::string("examples:startup/a.test"));
    CHECK_EQ(f.text("preset", "Model-B"), std::string());
    CHECK(readText(iniPath(dir)).find("Model-A = examples:startup/a.test\n") != std::string::npos);
    CHECK_EQ(SettingsFile::open(d, iniPath(dir)).text("preset", "Model-A"), std::string("examples:startup/a.test"));
    CHECK_EQ(SettingsFile::defaultsOnly(d).text("preset", "Model-A"), std::string());
    TempDir other;
    writeText(iniPath(other), "format-version = 1\n[preset]\nModel-B =\n");
    CHECK_EQ(SettingsFile::open(d, iniPath(other)).text("preset", "Model-A"), std::string());
}

TEST(settings_no_version_line_starts_with_defaults_and_says_so) {
    TempDir dir;
    const std::string noVersion = "[test]\nname = mine\n";
    writeText(iniPath(dir), noVersion);
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(f.outcome() == Outcome::NoVersion);
    CHECK(!f.problem().empty());
    CHECK(!f.writable());
    CHECK_EQ(f.text("test", "name"), std::string("plain"));  // the default, not read
    // Not read and not written in this run; a change lasts until quit.
    const SettingsFile::WriteResult w = f.set("test", "name", "new");
    CHECK(!w.ok && !w.problem.empty());
    CHECK_EQ(f.text("test", "name"), std::string("new"));
    CHECK_EQ(readText(iniPath(dir)), noVersion);
    // Another version is not read either.
    const std::string v2 = "format-version = 2\n[test]\nname = mine\n";
    writeText(iniPath(dir), v2);
    SettingsFile g = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(g.outcome() == Outcome::OtherVersion);
    CHECK(!g.problem().empty());
    CHECK_EQ(g.text("test", "name"), std::string("plain"));
    g.set("test", "name", "new");
    CHECK_EQ(readText(iniPath(dir)), v2);
    // The version is the top-level key: one inside a section does not count.
    writeText(iniPath(dir), "[test]\nformat-version = 1\n");
    CHECK(SettingsFile::open(testProduct(), iniPath(dir)).outcome() == Outcome::NoVersion);
}

TEST(settings_unreadable_file_untouched_defaults_logged) {
    TempDir dir;
    const std::string broken = kVersioned + "[test]\nname = mine\nthis is not ini\n";
    writeText(iniPath(dir), broken);
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(f.outcome() == Outcome::Unreadable);
    CHECK(!f.problem().empty());  // for the log, with the line
    CHECK(f.problem().find("line 4 ") != std::string::npos);  // the line that is not INI
    CHECK_EQ(f.text("test", "name"), std::string("plain"));
    CHECK(!f.set("test", "name", "other").ok);
    CHECK_EQ(readText(iniPath(dir)), broken);
    // A byte order mark makes the file unreadable too.
    const std::string bom = "\xEF\xBB\xBF" + kVersioned;
    writeText(iniPath(dir), bom);
    SettingsFile g = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(g.outcome() == Outcome::Unreadable);
    g.set("test", "name", "other");
    CHECK_EQ(readText(iniPath(dir)), bom);
}

TEST(settings_file_keeps_unknown_keys_on_write) {
    TempDir dir;
    const std::string hand = kVersioned +
                             "# my notes\n[startup]\nmodel = Model-A\nmine = kept\n\n"
                             "[my-section]\nanything = goes\n";
    writeText(iniPath(dir), hand);
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(f.set("startup", "model", "Model-B").ok);
    CHECK_EQ(readText(iniPath(dir)), kVersioned +
                                         "# my notes\n[startup]\nmodel = Model-B\nmine = kept\n\n"
                                         "[my-section]\nanything = goes\n");
}

TEST(settings_change_survives_reopen) {
    TempDir dir;
    {
        SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
        CHECK(f.set("test", "style", "two").ok);
        CHECK(f.set("trace", "max-file-size-mb", "12").ok);
    }
    const SettingsFile g = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK(g.outcome() == Outcome::Read);
    CHECK_EQ(g.text("test", "style"), std::string("two"));
    CHECK_EQ(g.integer("trace", "max-file-size-mb"), 12L);
}

TEST(settings_file_not_reread_while_running) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned + "[test]\nname = first\n");
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    writeText(iniPath(dir), kVersioned + "[test]\nname = edited\n");
    CHECK_EQ(f.text("test", "name"), std::string("first"));
    // An unchanged value is not written.
    writeText(iniPath(dir), kVersioned + "[test]\nname = edited\nstyle = two\n");
    CHECK(f.set("test", "name", "first").ok);
    CHECK_EQ(readText(iniPath(dir)), kVersioned + "[test]\nname = edited\nstyle = two\n");
}

TEST(settings_interrupted_write_leaves_old_or_new) {
    TempDir dir;
    const std::string old = kVersioned + "[test]\nname = old\n";
    writeText(iniPath(dir), old);
    SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    testing::failBeforeRename(true);
    const SettingsFile::WriteResult w = f.set("test", "name", "new");
    testing::failBeforeRename(false);
    CHECK(!w.ok && !w.problem.empty());
    CHECK_EQ(readText(iniPath(dir)), old);  // the old content, whole
    // Nothing is left beside the file.
    std::size_t files = 0;
    for (const auto& e : std::filesystem::directory_iterator(dir / "Test-Product")) {
        (void)e;
        ++files;
    }
    CHECK_EQ(files, std::size_t{1});
    // The next write gives the new content, whole.
    CHECK(f.set("test", "style", "two").ok);
    CHECK_EQ(readText(iniPath(dir)), kVersioned + "[test]\nname = new\nstyle = two\n");
}

TEST(settings_two_writers_last_wins_unknown_keys_kept) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned + "[test]\nname = start\n");
    SettingsFile a = SettingsFile::open(testProduct(), iniPath(dir));
    // B is a newer build of the product with a key A does not know.
    ProductDecl newer = testProduct();
    newer.keys.push_back(testKey("test", "newer", ValueKind::Text, ""));
    SettingsFile b = SettingsFile::open(newer, iniPath(dir));
    CHECK(b.set("test", "newer", "from-b").ok);
    CHECK(b.set("test", "style", "two").ok);
    CHECK(a.set("test", "flag", "true").ok);  // A writes after B
    const std::string afterA = readText(iniPath(dir));
    CHECK(afterA.find("newer = from-b\n") != std::string::npos);  // B's key not lost
    CHECK(afterA.find("style = two\n") != std::string::npos);
    CHECK(afterA.find("flag = true\n") != std::string::npos);
    // For a key both set, the last write wins.
    CHECK(a.set("test", "name", "from-a").ok);
    CHECK(b.set("test", "name", "from-b").ok);
    const SettingsFile c = SettingsFile::open(newer, iniPath(dir));
    CHECK_EQ(c.text("test", "name"), std::string("from-b"));
    CHECK_EQ(c.text("test", "newer"), std::string("from-b"));
    CHECK_EQ(c.boolean("test", "flag"), true);
    // A does not see B's values while it runs.
    CHECK_EQ(a.text("test", "style"), std::string("one"));
}

TEST(settings_defaults_only_reads_and_writes_nothing) {
    SettingsFile f = SettingsFile::defaultsOnly(testProduct());
    CHECK(f.outcome() == Outcome::DefaultsOnly);
    CHECK(f.path().empty());
    CHECK(!f.writable());
    CHECK_EQ(f.text("startup", "model"), std::string("last"));
    const SettingsFile::WriteResult w = f.set("test", "style", "two");
    CHECK(w.ok);  // nothing is meant to be written
    CHECK_EQ(f.text("test", "style"), std::string("two"));
}

// A read or a write that fails carries the system's reason, for the app log.
TEST(settings_failed_read_and_write_carry_the_system_reason) {
    TempDir dir;
    std::string why;
    CHECK(!readWholeFile(dir / "none.ini", &why));
    CHECK_EQ(why, std::string("No such file or directory"));
    namespace fs = std::filesystem;
    const std::string locked = dir / "locked.ini";
    writeText(locked, kVersioned);
    fs::permissions(locked, fs::perms::none);
    why.clear();
    if (!readWholeFile(locked)) {   // not where the user may read anything (root)
        CHECK(!readWholeFile(locked, &why));
        CHECK_EQ(why, std::string("Permission denied"));
        const SettingsFile f = SettingsFile::open(testProduct(), locked);
        CHECK(f.outcome() == Outcome::Unreadable);
        CHECK(f.problem().find("cannot be read (Permission denied)") != std::string::npos);
    }
    fs::permissions(locked, fs::perms::owner_all);
    const std::string folder = dir / "read-only";
    fs::create_directories(folder);
    fs::permissions(folder, fs::perms::owner_read | fs::perms::owner_exec);
    const FileWrite w = writeFileAtomically(folder + "/x.ini", "x");
    if (!w.ok) CHECK(w.problem.find("cannot write beside " + folder + "/x.ini: Permission denied") != std::string::npos);
    fs::permissions(folder, fs::perms::owner_all);
}

// A stored value the key does not accept: its default, and named for the app log.
TEST(settings_values_not_accepted_are_named) {
    TempDir dir;
    writeText(iniPath(dir), kVersioned + "[test]\nflag = maybe\nstyle = two\nport = many\n");
    const SettingsFile f = SettingsFile::open(testProduct(), iniPath(dir));
    CHECK_EQ(f.text("test", "flag"), std::string("false"));
    const std::vector<std::string> bad = f.notAccepted();
    CHECK(std::find(bad.begin(), bad.end(), "[test] flag = \"maybe\"") != bad.end());
    CHECK(std::find(bad.begin(), bad.end(), "[test] port = \"many\"") != bad.end());
    CHECK_EQ(bad.size(), std::size_t{2});
}

// A number without a default ([window] x) is empty until the app writes it:
// accepted, not a value to warn about.
TEST(integer_without_default_accepts_empty) {
    TempDir tmp("calcu-settings-optional-number");
    SettingsFile f = SettingsFile::open(testProduct(), tmp.str() + "/Test-Product.ini");
    CHECK(f.notAccepted().empty());
    const KeyDecl* x = testProduct().find("window", "paper-x");
    CHECK(x && accepts(testProduct(), *x, "") && accepts(testProduct(), *x, "-20") && !accepts(testProduct(), *x, "left"));
    const KeyDecl* width = testProduct().find("window", "paper-width");
    CHECK(width && !accepts(testProduct(), *width, ""));
}
