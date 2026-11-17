// LIB-12 ROM lookup, the Calc-U-1600 column (M-27; design §6, Q1-Q3):
// the full set and its fetch list, the binary check, the CE-502B's module
// definition from the ROM repository, and a start without the full set.
// Run from products/calc-u-1600 (CoreTests' working directory); SKIP without
// the fetched ROMs in roms/.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../calc-u-59/tests/Md5.hpp"   // the test MD5 of LIB-12 (test code only)
#include "Files.hpp"
#include "TempFolder.hpp"
#include "../app/tests/TestSettings.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Connector/MemoryCardCatalog.hpp"
#include "Machine/SharpRoms.hpp"
#include "app/SharpSession.hpp"
#include "sessions/Listener.hpp"
#include "machine/Build.hpp"
#include "roms/Bundle.hpp"
#include "roms/Lookup.hpp"
#include "SharpTestSupport.hpp"
#include "TestHarness.hpp"
#include "startup/StartupPart.hpp"
#include "TestRoms.hpp"

namespace {

namespace fs = std::filesystem;
using calcu::roms::FileProblem;
using calcu::roms::Problem;
using calcu::roms::RomsMissing;
using Model = calcu::machine::ModelId;

// The table of AC-12, copied from the spec: file, bytes, md5, who needs it.
struct Row {
    const char* file;
    std::uint64_t bytes;
    const char* md5;
};
const std::vector<Row> kAc12 = {
    {"PC-1500_A01.ROM", 16384, "fbc55a9a8743e619b7709721ff5bcbff"},
    {"PC-1500_A03.ROM", 16384, "4bcf78a6d3d32e2a0349eb2d28987b8d"},
    {"PC-1500_A04.ROM", 16384, "8ebec8b0ef358645df14807c31df7d06"},
    {"CE-150.ROM", 8192, "eb9aa5156c6849890b137799efc50a4b"},
    {"CE-158.ROM", 16384, "aa952878fb29da4844791d95185649ca"},
    {"PC1600-LH5803-C000-FFFF-new.bin", 16384, "56168830b46d637b08529a74609bee3f"},
    {"PC1600-P0-B0-new.bin", 16384, "404bf6f2df489e09649167078acd9a25"},
    {"PC1600-P1-B0-new.bin", 16384, "bddbb8bbf0b2bd2d95038f67b8d002ac"},
    {"PC1600-P1-B3-new.bin", 16384, "6f6e1a9d46db7dc91d4322c93583ac81"},
    {"PC1600-P1-B3B-new.bin", 16384, "2483319acf35da4e848e59ab954abf46"},
    {"PC1600-P2-B6-new.bin", 16384, "86cb9036da284de2b04c7946d140a9fd"},
    {"PC1600-LH5803-C000-FFFF-old.bin", 16384, "6005b6420bd5e191e81a1562f3242ec9"},
    {"PC1600-P0-B0-old.bin", 16384, "5afcc22134e106bfd63b899febe9df7c"},
    {"PC1600-P1-B0-old.bin", 16384, "3bcb6b178f5967c7c8e32afe560a3e75"},
    {"PC1600-P1-B3-old.bin", 16384, "ded92d8280f8f83ce3498fb9cbfb9b3d"},
    {"PC1600-P1-B3B-old.bin", 16384, "2e8e075cac8f9696c5e833ceef130a4f"},
    {"PC1600-P2-B6-old.bin", 16384, "2c977fdd8c924c1492a2c23f67a20f23"},
    {"PC1600-P1-B4-CE1600P-new.bin", 16384, "05548a8dda3e572d50d4bd281a650ea8"},
    {"PC1600-P1-B5-CE1600P-OR-F-new.bin", 16384, "a675c6dbdf7dc4c10e8d96891e196f8f"},
    {"PC1600-P1-B4-CE1600P-old.bin", 16384, "df41b050acbc29c83214cbaaf29bee91"},
    {"PC1600-P1-B5-CE1600P-OR-F-old.bin", 16384, "33f3ef7207eac06587cc4c6d70c6cbd0"},
};
// AC-14: the CE-502B's module definition, md5 recorded when it was moved
// into PC-1500-ROM (roms/README.md).
constexpr const char* kCe502bFile = "CE-502B.card.yaml";
constexpr const char* kCe502bMd5 = "7c7af4b7adab69b5e75297bc1c3d7279";

using calcu::testing::readText;

const FileProblem* problemOf(const RomsMissing& m, const std::string& name) {
    for (const auto& f : m.files)
        if (f.fileName == name) return &f;
    return nullptr;
}

// A copy of roms/ in a folder of its own (the files of the full set only),
// for a bundle with one file taken away or changed.
struct BundleCopy {
    calcu::testing::TempFolder folder;
    fs::path dir = folder.path();
    explicit BundleCopy(const std::string& tag) : folder("calcu1600-roms-" + tag) {
        for (const auto& f : sharp::fullRomSet().files) fs::copy_file(fs::path("roms") / f.entry.fileName, dir / f.entry.fileName);
    }
    void remove(const std::string& file) const { folder.remove(file); }
};

// The library's app session with Calc-U-1600's part, its bundled resources
// folder `bundle`, on a scripted clock (M-31; 0.8.1's MachineSession before).
struct Session {
    TestSettings testSettings;
    fs::path home = fs::temp_directory_path() / "calcu1600-rom-lookup-home";
    cu1600::AppPaths paths;
    calcu::pacing::WallTime now{0};
    calcu::sessions::ProductSession<cu1600::SharpAppPart> app;
    cu1600::SharpAppPart& session;
    calcu::sessions::Listener host;
    explicit Session(const fs::path& bundle)
        : paths(testSettings.settings, testHostDirs(home.generic_string()), cu1600::BundleDirs{bundle.generic_string(), {}}),
          app(cu1600::makeSharpSession(testSettings.settings, paths, options())),
          session(*app.part) {
        host.listen(*app.session, calcu::sessions::HostServices{{calcu::sessions::HostService::FolderPicker}});
    }
    cu1600::SharpSessionOptions options() {
        cu1600::SharpSessionOptions o;
        o.clock = [this] { return now; };
        return o;
    }
    ~Session() {
        std::error_code ec;
        fs::remove_all(home, ec);
    }
    bool run(const calcu::sessions::CommandName& c, calcu::sessions::Value args = {}) {
        return host.runToEnd(c, std::move(args), [this] {
                       now += std::chrono::milliseconds(16);
                       host.session().call(now);
                   }).done;
    }
    // The start (the first machine) or a model switch; false when no machine
    // was built (the full set is missing).
    bool switchModel(Model m) {
        if (!session.hasLiveMachine()) {
            testSettings.settings.setStartupModelPreference(m.value.c_str());
            return run(cu1600::commands::kStart);
        }
        return run(calcu::startup::kModelCommand, calcu::sessions::Value::object({{"name", m.value.c_str()}}));
    }
    // LIB-12's `roms-missing` topic, or nullptr.
    const RomsMissing* missing() const {
        const auto* p = host.state<calcu::startup::RomsMissingState>(calcu::startup::kRomsMissingTopic);
        return p ? &p->missing : nullptr;
    }
    bool noNotices() { return host.mirror().takeAppended(calcu::sessions::kNoticesTopic).empty(); }
};

// ---- Q1: the full set and SharpRomReader ----

TEST(lib12_ac12_full_set_is_the_21_files_and_ce502b) {
    const auto& files = sharp::fullRomSet().files;
    CHECK(sharp::fullRomSet().catalogueErrors.empty());
    CHECK_EQ(files.size(), kAc12.size() + 1);
    for (const Row& r : kAc12) {
        const auto it = std::find_if(files.begin(), files.end(), [&](const auto& f) { return f.entry.fileName == r.file; });
        CHECK(it != files.end());
        if (it == files.end()) continue;
        CHECK(it->entry.encoding == calcu::machine::RomEncoding::Binary);
        CHECK(it->entry.unit == calcu::machine::ContentUnit::Bytes);
        CHECK_EQ(it->entry.decodedAmount, r.bytes);
        CHECK_EQ(it->entry.md5, std::string(r.md5));
    }
    const auto ce502b = std::find_if(files.begin(), files.end(), [](const auto& f) { return f.entry.fileName == kCe502bFile; });
    CHECK(ce502b != files.end());
    if (ce502b != files.end()) {
        CHECK(ce502b->entry.encoding == calcu::machine::RomEncoding::Text);
        CHECK_EQ(ce502b->entry.decodedAmount, std::uint64_t{16384});
        CHECK_EQ(ce502b->entry.md5, std::string(kCe502bMd5));
    }
    // The host drive's own ROM is not in the full set (roms.md Q-1).
    CHECK(!sharp::isFullSetFile("PC1600-P1-B7-HOSTDRIVE.bin"));

    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    // The fetched files are the ones of the table (fetch_roms.sh checked them).
    for (const Row& r : kAc12) CHECK_EQ(testroms::md5Hex(*testRomSet()->bytes(r.file)), std::string(r.md5));
    CHECK_EQ(testroms::md5Hex(*testRomSet()->bytes(kCe502bFile)), std::string(kCe502bMd5));
}

TEST(lib12_ac12_fetch_script_has_the_same_names_and_md5) {
    const std::string script = readText("roms/fetch_roms.sh");
    CHECK(!script.empty());
    std::vector<std::pair<std::string, std::string>> fetched;
    // fetch_if_needed "$ROMS_DIR/NAME" MD5 URL..., fetch_pc1600_calc / fetch_ce1600p VERSION BASE MD5
    const std::regex direct(R"re(^fetch_if_needed\s+"\$ROMS_DIR/([^"]+)"\s+([0-9a-f]{32})\s)re");
    const std::regex versioned(R"re(^fetch_(pc1600_calc|ce1600p)\s+(new|old)\s+(\S+)\s+([0-9a-f]{32})\s*$)re");
    std::istringstream in(script);
    for (std::string line; std::getline(in, line);) {
        std::smatch m;
        if (std::regex_search(line, m, direct)) fetched.emplace_back(m[1], m[2]);
        else if (std::regex_search(line, m, versioned)) fetched.emplace_back(std::string(m[3]) + "-" + std::string(m[2]) + ".bin", m[4]);
    }
    std::vector<std::pair<std::string, std::string>> want;
    for (const Row& r : kAc12) want.emplace_back(r.file, r.md5);
    want.emplace_back(kCe502bFile, kCe502bMd5);
    std::sort(fetched.begin(), fetched.end());
    std::sort(want.begin(), want.end());
    CHECK_EQ(fetched.size(), std::size_t{22});
    CHECK(fetched == want);
    // The copy into the Qt6 card resources went (AC-14).
    CHECK(script.find("resources/cards") == std::string::npos);
}

TEST(lib12_ac11_binary_dumps_checked_by_byte_size) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    BundleCopy copy("ac11");
    {   // a 16383-byte copy of a 16384-byte dump is bad
        std::vector<std::uint8_t> bytes = *testRomSet()->bytes("PC-1500_A01.ROM");
        bytes.pop_back();
        std::ofstream(copy.dir / "PC-1500_A01.ROM", std::ios::binary | std::ios::trunc)
            .write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const calcu::roms::LookupResult r = sharp::lookUpRoms(calcu::roms::DirectoryBundle(copy.dir));
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    const FileProblem* p = problemOf(r.missing, "PC-1500_A01.ROM");
    CHECK(p && p->problem == Problem::WrongAmount);
    CHECK(calcu::roms::describe(r.missing).find("PC-1500_A01.ROM: wrong size (16383 bytes, expected 16384 bytes)") !=
          std::string::npos);
    // The full set as fetched passes.
    CHECK(sharp::lookUpRoms(calcu::roms::DirectoryBundle("roms")).ok());
}

TEST(lib12_ec1_ce1600p_b5_file_once_in_set_and_message) {
    // The CE-1600P's second file is also the CE-1600F's ROM: one file of the
    // full set per variant, needed by the CE-1600P variant (the CE-1600F is
    // the CE-1600P's own; C1600-1 §3.4).
    const auto& files = sharp::fullRomSet().files;
    for (const char* f : {"PC1600-P1-B5-CE1600P-OR-F-new.bin", "PC1600-P1-B5-CE1600P-OR-F-old.bin"})
        CHECK_EQ(std::count_if(files.begin(), files.end(), [&](const auto& e) { return e.entry.fileName == f; }), 1);
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    BundleCopy copy("ec1");
    copy.remove("PC1600-P1-B5-CE1600P-OR-F-old.bin");
    const calcu::roms::LookupResult r = sharp::lookUpRoms(calcu::roms::DirectoryBundle(copy.dir));
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    const std::string text = calcu::roms::describe(r.missing);
    std::size_t n = 0;
    for (std::size_t at = text.find("PC1600-P1-B5-CE1600P-OR-F-old.bin"); at != std::string::npos;
         at = text.find("PC1600-P1-B5-CE1600P-OR-F-old.bin", at + 1))
        ++n;
    CHECK_EQ(n, std::size_t{1});
    CHECK(text.find("CE-1600P (variant Old)") != std::string::npos);
}

// ---- Q3: the CE-502B from the ROM repository ----

TEST(lib12_ac14_ce502b_definition_from_the_set) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    const std::string text = readText(fs::path("roms") / kCe502bFile);
    CHECK(text.find("format-version: 1") != std::string::npos);
    const auto m = sharp::romSetModule(*testRomSet(), "CE-502B");
    CHECK(m.has_value());
    if (!m) return;
    CHECK(m->fileName == kCe502bFile);
    CHECK(m->definition.moduleName == "CE-502B");
    CHECK(m->definition.isRom());
    // What the reader yields is the entry's amount (design §9: not verified then).
    std::size_t bytes = 0;
    for (const auto& r : m->definition.regions)
        for (const auto& b : r.initialContentByBank) bytes += b.second.size();
    CHECK_EQ(bytes, std::size_t{16384});
    // A definition that does not parse, or of another size, is bad.
    BundleCopy copy("ac14");
    std::ofstream(copy.dir / kCe502bFile, std::ios::trunc) << "format-version: 1\nmodule-name: \"CE-502B\"\n";
    const calcu::roms::LookupResult r = sharp::lookUpRoms(calcu::roms::DirectoryBundle(copy.dir));
    CHECK(!r.ok());
    CHECK(problemOf(r.missing, kCe502bFile) != nullptr);
    // A session lists it among the bundled ROM modules and adds it to its
    // catalogue (LIB-1 AC-3) from the set.
    Session s("roms");
    CHECK(s.session.catalogue().peripheral(calcu::machine::PeripheralId{"CE-502B"}) != nullptr);
}

TEST(lib12_ac14_ce502b_not_in_qt6_resources) {
    // The definition left the source repository's Qt6 resources: nothing
    // there names it, and the card scans pass over the ROM set's file.
    CHECK(!fs::exists("frontends/qt6/resources/cards/ce502b.card.yaml"));
    const std::string ignore = readText(".gitignore");
    CHECK(ignore.find("ce502b.card.yaml") == std::string::npos);
    CHECK(sharp::isFullSetFile(kCe502bFile));
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    for (const auto& e : scanMemoryCardDirectory("roms", nullptr)) CHECK(e.moduleName != "CE-502B");
}

// ---- Q2: no machine without the full set, no substitution (AC-13) ----

TEST(lib12_ac13_missing_old_set_does_not_start) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    BundleCopy copy("ac13-old");
    copy.remove("PC1600-P0-B0-old.bin");
    Session s(copy.dir);
    // Even a PC-1500A, which never needs the file, does not start.
    CHECK(!s.switchModel(calcu::machine::ModelId{"PC-1500A"}));
    CHECK(!s.session.hasLiveMachine());
    CHECK(s.missing() != nullptr);
    if (!s.missing()) return;
    CHECK(problemOf(*s.missing(), "PC1600-P0-B0-old.bin") != nullptr);
    CHECK(s.noNotices());   // no "Old ROM unavailable", no New in its place
    CHECK(s.session.romSet() == nullptr);
    const std::string text = calcu::roms::describe(*s.missing());
    CHECK(text.find("PC1600-P0-B0-old.bin: missing; needed by PC-1600 (variant Old)") != std::string::npos);
    CHECK(text.find(copy.dir.string()) != std::string::npos);
}

TEST(lib12_ac13_missing_ce1600p_old_file_does_not_start) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    BundleCopy copy("ac13-ce1600p");
    copy.remove("PC1600-P1-B4-CE1600P-old.bin");
    Session s(copy.dir);
    CHECK(!s.switchModel(calcu::machine::ModelId{"PC-1600"}));
    CHECK(!s.session.hasLiveMachine());
    CHECK(s.missing() && s.missing()->files.size() == 1);
    CHECK(s.missing() && problemOf(*s.missing(), "PC1600-P1-B4-CE1600P-old.bin") != nullptr);
    CHECK(s.noNotices());
    // A missing file of the New set alike.
    BundleCopy copy2("ac13-new");
    copy2.remove("PC1600-P2-B6-new.bin");
    Session s2(copy2.dir);
    CHECK(!s2.switchModel(calcu::machine::ModelId{"PC-1600"}));
    CHECK(s2.missing() && problemOf(*s2.missing(), "PC1600-P2-B6-new.bin") != nullptr);
}

TEST(lib12_ac13_no_rom_read_during_session) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/");
    auto copy = std::make_unique<BundleCopy>("ac13-session");
    Session s(copy->dir);
    copy.reset();   // the bundle folder is gone after the start
    CHECK(!fs::exists(s.paths.bundledResourcesDir()));
    CHECK(s.switchModel(calcu::machine::ModelId{"PC-1500"}));
    for (auto rev : {PC1500RomRevision::A01, PC1500RomRevision::A03, PC1500RomRevision::A04}) {
        CHECK(s.run(calcu::startup::kVariantCommand,
                    calcu::sessions::Value::object({{"name", rev == PC1500RomRevision::A01 ? "A01" : rev == PC1500RomRevision::A03 ? "A03" : "A04"}})));
        CHECK(s.session.hasLiveMachine());
    }
    auto attach = [&](const char* place, const char* peripheral) {
        return s.run(calcu::startup::kAttachCommand, calcu::sessions::Value::object({{"place", place}, {"peripheral", peripheral}}));
    };
    CHECK(attach("printer", "CE-150"));
    CHECK(s.switchModel(calcu::machine::ModelId{"PC-1500A"}));
    CHECK(attach("interface", "CE-158"));
    CHECK(s.switchModel(calcu::machine::ModelId{"PC-1600"}));
    CHECK(s.run(calcu::startup::kVariantCommand, calcu::sessions::Value::object({{"name", "Old"}})));
    CHECK(s.session.machine()->variant() && s.session.machine()->variant()->value == "Old");
    CHECK(s.run(cu1600::commands::kCE1600PRom, calcu::sessions::Value::object({{"version", "old"}})));
    CHECK(attach("printer", "CE-1600P"));
    CHECK(s.session.ce1600pAttached());
    CHECK(s.run(cu1600::commands::kCE1600PRom, calcu::sessions::Value::object({{"version", "new"}})));
    CHECK(s.run(calcu::startup::kVariantCommand, calcu::sessions::Value::object({{"name", "New"}})));
    CHECK(s.session.hasLiveMachine());
    CHECK(s.missing() == nullptr);
    CHECK(s.noNotices());
}

} // namespace
