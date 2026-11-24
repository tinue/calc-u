// The ROM lookup of Calc-U-59: the TI reader and its checks, the full set
// and the fetch list, the module files, the start without the full set and
// no ROM read while running.
#include "Md5.hpp"
#include "Files.hpp"
#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../app/ProductRoms.hpp"
#include "../app/TIAppPart.hpp"
#include "../core/ModuleFile.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TIRomReader.hpp"
#include "../core/TIRomText.hpp"
#include "roms/Bundle.hpp"
#include "sessions/Listener.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>

using namespace calcu::machine;
using namespace calcu::roms;
using testroms::TempDir;

namespace {

namespace fs = std::filesystem;

// The full set: each file with its md5.
const std::vector<std::pair<std::string, std::string>> kFullSet = {
    {"TMC0582.txt", "c1cb6d40e063aae1ba7b851b11b1c18a"},  {"TMC0583.txt", "2341a8e484c31ff24a16e70c14c1b3a3"},
    {"TMC0571B.txt", "599050d2b1c1900e64b56c089338d32e"}, {"TMC0582-CONST-K.txt", "bf4fb6005177ffdec8f8ceee7e7e16d6"},
    {"TMC0583-CONST-K.txt", "81724f14b2827410650f032ef352b1aa"}, {"CD2400.txt", "638c4d0c8359e00cf1f600884f43b789"},
    {"CD2401.txt", "3b0ec122cd34b214477780fd405ffdd8"},   {"TMC0573.txt", "6c0cb7db542f3b74a28613ff4ed14bce"},
    {"CD2400-CONST-K.txt", "dbaee54fe0f478c7659d17bff4b790d4"}, {"CD2401-CONST-K.txt", "ac81d5b45c736a0f45c3835f0d977b87"},
    {"ML.rom.yaml", "ab6db47478041bb1bd1b50b3114b8949"},  {"ST.rom.yaml", "6e7e4045c9e31720dba5fbee0ace248a"},
    {"RE.rom.yaml", "6ea276ed76b4b67df59141d73708e6ec"},  {"SY.rom.yaml", "d87b45b1f0fc6963db7dc3e537eeaf05"},
    {"NG.rom.yaml", "889d6f5c516afd62f7bd7680cecf47ed"},  {"AV.rom.yaml", "84823786f1b065a74ba7b36319c05ba4"},
    {"LE.rom.yaml", "f67efb0d1b10c46266d9a32891c9aa6c"},  {"SA.rom.yaml", "f26fd2f1cbe52ec93387c76fe26122e6"},
    {"BD.rom.yaml", "4ee8f14c5ce0f088cff3ea3f0c149f76"},  {"MU.rom.yaml", "81403a7ed013e53c0334a156a7ca8c0e"},
    {"EE.rom.yaml", "63fdc4324deb68025c9e98af817a91fc"},  {"SE.rom.yaml", "ea082fed55d93610d95f16cf2577f1e1"},
    {"AG.rom.yaml", "4903315a721f4ff41e60b40abbaeb284"},  {"RP.rom.yaml", "d62d3ea22600d0df8f24a6355ee0ea95"},
};

// The recorded md5 of each module's 5000-byte image, decoded from the module
// dumps of TI-59-ROM dumps/modules/ (90cd4a7), recorded on 2026-11-15.
const std::map<std::string, std::string> kModuleImageMd5 = {
    {"ML", "c2424949bcc52a0fac60345d33a07cde"}, {"ST", "bb7a258e79b60854fd759073b748fdce"},
    {"RE", "681f7bcf7c7be69037c3f382d265c349"}, {"SY", "e6014c604a7b40710d7f5e4a0aa5eba3"},
    {"NG", "9e0b92d73a52f203ce74a350cc19a9e1"}, {"AV", "57ee384f1f4531503fed1a5946225e78"},
    {"LE", "7b936ca9774214e8bf1948a3c2bbeb47"}, {"SA", "58148121ed86c89b63e41c0f900a9581"},
    {"BD", "5198cc7eefd1e0a55c41c2bf671d3f28"}, {"MU", "81e7b0b5c2aafcee3b92b723b49b5b9d"},
    {"EE", "a5961c031c6bc6f2071b0e4773d52c59"}, {"SE", "27c70a610f1ae7c2cb80c98edbafe6a6"},
    {"AG", "0828a17a145ff50a4112e3648dc70108"}, {"RP", "935deb6933a03fef9a13ba6db6eb03a7"},
};

using calcu::testing::readText;
using calcu::testing::writeText;
std::string_view textOf(const std::vector<std::uint8_t>& b) { return {reinterpret_cast<const char*>(b.data()), b.size()}; }

const FileProblem* problemOf(const RomsMissing& m, const std::string& name) {
    for (const auto& f : m.files)
        if (f.fileName == name) return &f;
    return nullptr;
}

std::vector<std::string> moduleIds() {
    std::vector<std::string> ids;
    for (const auto& m : cu59::allModuleMetadata(*testroms::romSet()))
        ids.push_back(m.id);
    return ids;
}

// Every line ending of `text` replaced by `eol`.
std::string withLineEndings(const std::string& text, const std::string& eol) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' || text[i] == '\n') {
            out += eol;
            if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
        } else {
            out += text[i];
        }
    }
    return out;
}

} // namespace

// ── The reader ──────────────────────────────────────────────────────────────

TEST(ti_entries_have_the_form) {
    const auto& set = cu59::fullRomSet();
    CHECK(set.catalogueErrors.empty());
    for (const auto& f : set.files) {
        const RomEntry& e = f.entry;
        CHECK(e.encoding == RomEncoding::Text);   // every TI file is a text dump or a module file
        const bool module = e.fileName.size() > 9 && e.fileName.substr(e.fileName.size() - 9) == ".rom.yaml";
        CHECK(module || e.fileName.substr(e.fileName.size() - 4) == ".txt");
        CHECK(e.unit == (module ? ContentUnit::Bytes
                                : e.fileName.find("CONST-K") != std::string::npos ? ContentUnit::Rows : ContentUnit::Words));
        CHECK_EQ(e.md5.size(), size_t{32});
        CHECK(std::all_of(e.md5.begin(), e.md5.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }));
        CHECK(!f.neededBy.empty());
    }
}

TEST(amounts_per_file_and_per_model) {
    const std::map<std::string, std::uint64_t> want = {
        {"TMC0582.txt", 2560}, {"TMC0583.txt", 2560}, {"TMC0571B.txt", 1024}, {"CD2400.txt", 2560},
        {"CD2401.txt", 2560},  {"TMC0573.txt", 1024}, {"TMC0582-CONST-K.txt", 32}, {"TMC0583-CONST-K.txt", 32},
        {"CD2400-CONST-K.txt", 32}, {"CD2401-CONST-K.txt", 32}};
    TIRomReader reader;
    auto set = testroms::romSet();
    for (const auto& f : cu59::fullRomSet().files) {
        const auto* b = set->bytes(f.entry.fileName);
        CHECK(b != nullptr);
        if (!b) continue;
        auto d = reader.decode(f.entry, *b);
        CHECK(d.error.empty());
        CHECK_EQ(d.amount, f.entry.decodedAmount);
        auto it = want.find(f.entry.fileName);
        CHECK_EQ(d.amount, it == want.end() ? std::uint64_t{5000} : it->second);   // a module file: 5000 bytes
    }
    for (const auto& m : tiCatalogue().models()) {
        std::uint64_t words = 0, rows = 0;
        for (const auto& e : m.romsFor(std::nullopt)) (e.unit == ContentUnit::Rows ? rows : words) += e.decodedAmount;
        CHECK_EQ(words, std::uint64_t{6144});
        CHECK_EQ(rows, std::uint64_t{64});
    }
    CHECK(reader.checkSet(*set).empty());
}

TEST(ti59_first_and_last_word) {
    auto set = testroms::romSet();
    std::string error;
    auto first = decodeRomText(textOf(*set->bytes("TMC0582.txt")), error);
    auto last = decodeRomText(textOf(*set->bytes("TMC0571B.txt")), error);
    CHECK(error.empty());
    CHECK(!first.empty() && first.front() == 0x0A01);
    CHECK(!last.empty() && last.back() == 0x1987);
}

TEST(wrong_sentinel_is_bad) {
    TempDir dir;
    dir.copyFrom("roms");
    std::string text = readText(dir.path() / "TMC0571B.txt");
    const auto at = text.rfind("1987");
    CHECK(at != std::string::npos);
    if (at == std::string::npos) return;
    text.replace(at, 4, "1986");
    writeText(dir.path() / "TMC0571B.txt", text);
    auto r = cu59::lookUpRoms(DirectoryBundle(dir.path()));
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), size_t{1});
    const auto* p = problemOf(r.missing, "TMC0571B.txt");
    CHECK(p && p->problem == Problem::ProductCheck);
    CHECK(p && p->detail.find("0x1986, expected 0x1987") != std::string::npos);
    CHECK(describe(r.missing).find("needed by TI-59, TI-58") != std::string::npos);
}

TEST(module_without_data_is_bad) {
    TempDir dir;
    dir.copyFrom("roms");
    std::string text = readText(dir.path() / "ST.rom.yaml");
    const auto a = text.find("  bytes: |\n");
    const auto b = text.find("\ncue-cards:");
    CHECK(a != std::string::npos && b != std::string::npos);
    if (a == std::string::npos || b == std::string::npos) return;
    text = text.substr(0, a) + "  bytes: |\n" + text.substr(b);   // a ROM without a row of data
    writeText(dir.path() / "ST.rom.yaml", text);
    auto r = cu59::lookUpRoms(DirectoryBundle(dir.path()));
    CHECK(!r.ok());
    const auto* p = problemOf(r.missing, "ST.rom.yaml");
    CHECK(p && p->problem == Problem::Unreadable);
    CHECK(describe(r.missing).find("needed by ST") != std::string::npos);
    // A truncated instruction dump is bad too: its amount.
    TempDir dir2;
    dir2.copyFrom("roms");
    std::string rom = readText(dir2.path() / "CD2401.txt");
    writeText(dir2.path() / "CD2401.txt", rom.substr(0, rom.size() / 2));
    r = cu59::lookUpRoms(DirectoryBundle(dir2.path()));
    CHECK(problemOf(r.missing, "CD2401.txt") && problemOf(r.missing, "CD2401.txt")->problem == Problem::WrongAmount);
}

TEST(crlf_and_cr_dumps_check_alike) {
    auto lf = testroms::romSet();
    for (const char* eol : {"\r\n", "\r"}) {
        TempDir dir;
        dir.copyFrom("roms");
        for (const auto& name : lf->fileNames()) writeText(dir.path() / name, withLineEndings(readText(dir.path() / name), eol));
        CHECK(readText(dir.path() / "TMC0582.txt") != readText("roms/TMC0582.txt"));
        auto r = cu59::lookUpRoms(DirectoryBundle(dir.path()));
        CHECK(r.ok());
        if (!r.ok()) {
            std::fprintf(stderr, "%s\n", describe(r.missing).c_str());
            continue;
        }
        // Decoded alike: the same instruction words and module images.
        std::string e1, e2;
        CHECK(decodeRomText(textOf(*r.set->bytes("TMC0583.txt")), e1) == decodeRomText(textOf(*lf->bytes("TMC0583.txt")), e2));
        CHECK(decodeConstantsText(textOf(*r.set->bytes("CD2400-CONST-K.txt")), e1) ==
              decodeConstantsText(textOf(*lf->bytes("CD2400-CONST-K.txt")), e2));
        CHECK(cu59::moduleImage(*r.set, "LE.rom.yaml") == cu59::moduleImage(*lf, "LE.rom.yaml"));
        CHECK(cu59::buildMachine(cu59::ModelId{cu59::kTI58C}, *r.set) != nullptr);
    }
}

// ── The full set and the fetch list ─────────────────────────────────────────

TEST(full_set_is_the_24_files_with_their_md5) {
    const auto& set = cu59::fullRomSet();
    CHECK_EQ(set.files.size(), size_t{24});
    std::vector<std::pair<std::string, std::string>> got;
    for (const auto& f : set.files) got.emplace_back(f.entry.fileName, f.entry.md5);
    auto want = kFullSet;
    std::sort(got.begin(), got.end());
    std::sort(want.begin(), want.end());
    CHECK(got == want);
    // The fetched files are the ones of the table (fetch_roms.sh checked them).
    for (const auto& [name, md5] : kFullSet) CHECK_EQ(testroms::md5Hex(*testroms::romSet()->bytes(name)), md5);
}

TEST(fetch_script_has_the_same_names_and_md5) {
    const std::string script = readText("roms/fetch_roms.sh");
    CHECK(!script.empty());
    std::vector<std::pair<std::string, std::string>> fetched;
    // fetch_if_needed "$ROMS_DIR/NAME" MD5 URL / fetch_module ID MD5
    const std::regex dump(R"re(^fetch_if_needed\s+"\$ROMS_DIR/([^"]+)"\s+([0-9a-f]{32})\s)re");
    const std::regex module(R"re(^fetch_module\s+([A-Z]{2})\s+([0-9a-f]{32})\s*$)re");
    std::istringstream in(script);
    for (std::string line; std::getline(in, line);) {
        std::smatch m;
        if (std::regex_search(line, m, dump)) fetched.emplace_back(m[1], m[2]);
        else if (std::regex_search(line, m, module)) fetched.emplace_back(std::string(m[1]) + ".rom.yaml", m[2]);
    }
    auto want = kFullSet;
    std::sort(fetched.begin(), fetched.end());
    std::sort(want.begin(), want.end());
    CHECK_EQ(fetched.size(), size_t{24});
    CHECK(fetched == want);
    CHECK(script.find("solid-state/TMC05") == std::string::npos);   // the 14 dumps left the fetch list
    CHECK(script.find("calculator/") == std::string::npos);         // flat roms/
}

TEST(every_model_and_module_needs_only_set_files) {
    auto set = testroms::romSet();
    const auto names = set->fileNames();
    CHECK_EQ(names.size(), size_t{24});
    auto inSet = [&](const std::string& n) { return std::find(names.begin(), names.end(), n) != names.end(); };
    for (const auto& m : tiCatalogue().models())
        for (const auto& e : m.romsFor(std::nullopt)) CHECK(inSet(e.fileName));
    const auto ids = moduleIds();
    CHECK_EQ(ids.size(), size_t{14});
    for (const auto& id : ids) {
        auto file = cu59::moduleRomFile(id);
        CHECK(file && *file == id + ".rom.yaml");
        CHECK(file && inSet(*file));
    }
    CHECK(!cu59::moduleRomFile("ZZ"));
}

// ── The module files ────────────────────────────────────────────────────────

TEST(module_rom_from_rom_yaml) {
    auto set = testroms::romSet();
    for (const auto& id : moduleIds()) {
        const auto* b = set->bytes(id + ".rom.yaml");
        CHECK(b != nullptr);
        if (!b) continue;
        CHECK(textOf(*b).find("format-version: 1\n") != std::string_view::npos);
        std::string error;
        TIModuleFile m = readModuleFile(textOf(*b), error);
        CHECK(error.empty());
        CHECK_EQ(m.moduleName, id);
        CHECK(!m.description.empty());
        CHECK_EQ(m.rom.size(), size_t{5000});
        CHECK(cu59::moduleImage(*set, id + ".rom.yaml") == m.rom);
    }
}

TEST(module_images_have_their_recorded_md5) {
    auto set = testroms::romSet();
    for (const auto& [id, md5] : kModuleImageMd5) {
        auto image = cu59::moduleImage(*set, id + ".rom.yaml");
        CHECK(image.has_value());
        if (image) CHECK_EQ(testroms::md5Hex(*image), md5);
    }
}

TEST(unknown_format_version_is_bad) {
    TempDir dir;
    dir.copyFrom("roms");
    std::string text = readText(dir.path() / "SE.rom.yaml");
    text.replace(text.find("format-version: 1"), 17, "format-version: 2");
    writeText(dir.path() / "SE.rom.yaml", text);
    auto r = cu59::lookUpRoms(DirectoryBundle(dir.path()));
    CHECK(!r.ok());
    const auto* p = problemOf(r.missing, "SE.rom.yaml");
    CHECK(p && p->problem == Problem::Unreadable);
    CHECK(p && p->detail.find("unsupported") != std::string::npos);
    // A file whose module-name is not its file name is bad as well.
    TempDir dir2;
    dir2.copyFrom("roms");
    fs::copy_file(dir2.path() / "AG.rom.yaml", dir2.path() / "RP.rom.yaml", fs::copy_options::overwrite_existing);
    r = cu59::lookUpRoms(DirectoryBundle(dir2.path()));
    CHECK(problemOf(r.missing, "RP.rom.yaml") && problemOf(r.missing, "RP.rom.yaml")->detail.find("module-name") != std::string::npos);
}

// ── The start ───────────────────────────────────────────────────────────────

// A host's start (Qt6, Swift, the Web's makeWebAppSession): the lookup over
// its bundle, and the app session only with the full set.
namespace {
// Host calls until no operation runs and there is a machine (the library's start,
// model, attach are operations).
void settle(calcu::sessions::AppSession& s) {
    calcu::pacing::WallTime now = std::chrono::hours(2);
    for (int i = 0; i < 6000; ++i) {
        s.call(now += std::chrono::milliseconds(16));
        if (s.machineSession().machine() && !s.context().busy()) return;
    }
}
void command(calcu::sessions::AppSession& s, const calcu::sessions::CommandName& name, calcu::sessions::Value args) {
    calcu::sessions::Listener l;
    l.listen(s);
    l.send(name, std::move(args));
    settle(s);
}
std::optional<calcu::sessions::ProductSession<cu59::TIAppPart>> startFrom(const Bundle& bundle,
                                                                       std::vector<std::string>& missingTexts) {
    auto roms = cu59::lookUpRoms(bundle);
    if (!roms.ok()) {
        missingTexts.push_back(describe(roms.missing));
        return std::nullopt;
    }
    static const calcu::testing::TempFolder folder("calcu59-romlookup");
    static calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    static const auto paths = testroms::rigPaths(settings, folder.path().string(), folder.path().string());
    cu59::TIAppSetup s = testroms::appSetup(settings, cu59::ModelId{cu59::kTI59}, *paths);
    s.roms = roms;
    auto t = std::make_shared<calcu::pacing::WallTime>(std::chrono::hours(1));
    calcu::shell::HostSessionSetup host = testroms::hostSetup([t] { return *t += std::chrono::milliseconds(1); });
    host.start = true;
    auto app = cu59::makeAppSession(std::move(s), std::move(host));
    settle(*app.session);   // the library's start
    return app;
}
} // namespace

TEST(missing_file_no_machine_and_files_named) {
    TempDir dir;
    dir.copyFrom("roms");
    fs::remove(dir.path() / "TMC0583.txt");
    fs::remove(dir.path() / "MU.rom.yaml");
    std::vector<std::string> missing;
    auto c = startFrom(DirectoryBundle(dir.path()), missing);
    CHECK(!c);   // no session, so no machine
    CHECK_EQ(missing.size(), size_t{1});
    if (missing.empty()) return;
    const std::string& text = missing.front();
    CHECK(text.find("2 ROM files are missing or bad") != std::string::npos);
    CHECK(text.find("TMC0583.txt: missing; needed by TI-59, TI-58") != std::string::npos);
    CHECK(text.find("MU.rom.yaml: missing; needed by MU") != std::string::npos);
    CHECK(text.find("The full set of ROM files is needed") != std::string::npos);
    CHECK(text.find(dir.path().string()) != std::string::npos);
}

TEST(bad_file_no_machine) {
    TempDir dir;
    dir.copyFrom("roms");
    std::string text = readText(dir.path() / "CD2400-CONST-K.txt");
    writeText(dir.path() / "CD2400-CONST-K.txt", text.substr(0, text.size() / 2));
    std::vector<std::string> missing;
    auto c = startFrom(DirectoryBundle(dir.path()), missing);
    CHECK(!c);
    CHECK_EQ(missing.size(), size_t{1});
    if (!missing.empty()) CHECK(missing.front().find("CD2400-CONST-K.txt: wrong size") != std::string::npos);
    // With the full set the same start gives a session and no message.
    std::vector<std::string> none;
    auto good = startFrom(DirectoryBundle("roms"), none);
    CHECK(good.has_value());
    CHECK(none.empty());
}

TEST(model_and_module_changes_read_no_file) {
    std::vector<std::string> missing;
    std::optional<calcu::sessions::ProductSession<cu59::TIAppPart>> app;
    {
        TempDir dir;
        dir.copyFrom("roms");
        app = startFrom(DirectoryBundle(dir.path()), missing);
    }   // the bundle folder is gone
    CHECK(app.has_value());
    if (!app) return;
    cu59::TIAppPart& c = *app->part;
    for (auto model : {cu59::ModelId{cu59::kTI59}, cu59::ModelId{cu59::kTI58}, cu59::ModelId{cu59::kTI58C}, cu59::ModelId{cu59::kTI59}}) {
        command(*app->session, calcu::startup::kModelCommand, calcu::sessions::Value::object({{"name", model.value}}));
        CHECK(c.hasMachine() && c.model() == model);
        CHECK_EQ(c.moduleID(), std::string("ML"));
    }
    for (const auto& id : moduleIds()) {
        command(*app->session, calcu::startup::kAttachCommand,
                calcu::sessions::Value::object({{"place", "slot-1"}, {"peripheral", id}}));
        CHECK_EQ(c.moduleID(), id);
        CHECK(cu59::moduleImage(*testroms::romSet(), id + ".rom.yaml").has_value());
    }
    CHECK(missing.empty());
}
