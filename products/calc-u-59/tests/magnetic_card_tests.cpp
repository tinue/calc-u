// C59-7 T1 to T4 (design §6): magnetic card format 3 read and written by the
// kind's codec (AC-2 to AC-5, AC-29, EC-3, EC-5), the content of a card and
// the bank an in-file side builds (AC-1, AC-19), the module cue cards from
// the module files (AC-22, AC-23).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/Bcd.hpp"
#include "../app/ModuleCatalog.hpp"
#include "../app/Settings.hpp"
#include "settings/Paths.hpp"
#include "../app/ModuleCueCards.hpp"
#include "../app/ModuleImage.hpp"
#include "../core/ModuleFile.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "../app/TextUtil.hpp"
#include "../core/TICatalogue.hpp"
#include "media/Shelf.hpp"
#include <set>

using namespace cu59;
using calcu::media::MediumCodec;
using calcu::media::ReadOutcome;

namespace {

const MediumCodec& codec() { return *magneticCardKind().codec; }

ReadOutcome read(const std::string& text) { return codec().read(text, {}); }

/// A bank as the ROM writes it: header, 30 rows of a pattern, checksum.
std::vector<std::uint8_t> bank(int number, std::uint8_t fill, bool program = true) {
    std::vector<std::uint8_t> d(kCardBankBytes, fill);
    d[0] = 0x16;
    d[1] = program ? 0x11 : 0x10;
    d[2] = pageByteOfBank(number);
    d[3] = 0x00;
    d[244] = d[245] = 0x07;
    return d;
}

std::shared_ptr<TIMagneticCardData> card(std::optional<std::vector<std::uint8_t>> s0, std::optional<std::vector<std::uint8_t>> s1) {
    auto c = std::make_shared<TIMagneticCardData>();
    if (s0) c->sides[0] = *s0;
    if (s1) c->sides[1] = *s1;
    return c;
}

std::string fileOf(const TIMagneticCardData& c, const std::string& name = "Prog", const std::string& description = "",
                   bool isTemplate = false, std::optional<CueCardContent> cue = std::nullopt) {
    return writeMagneticCardFile(c, name, description, isTemplate, cue, "2026-11-16T10:00:00Z");
}

std::vector<std::uint8_t> sideOf(const ReadOutcome& r, int side) {
    const auto c = magneticCardOf(r.info.content);
    if (!c) return {};
    const auto* b = std::get_if<std::vector<std::uint8_t>>(&c->sides[static_cast<std::size_t>(side)]);
    return b ? *b : std::vector<std::uint8_t>{};
}

std::string replaced(std::string s, const std::string& from, const std::string& to) {
    const auto at = s.find(from);
    if (at != std::string::npos) s.replace(at, from.size(), to);
    return s;
}

std::vector<std::uint8_t> hexFixture(const std::string& name) {
    std::vector<std::uint8_t> out;
    const std::string text = testroms::readFile("tests/fixtures/cards/" + name).value_or("");
    for (auto line : splitLines(text))
        for (auto t : splitWhitespace(stripInlineComment(line))) out.push_back(static_cast<std::uint8_t>(std::stoi(std::string(t), nullptr, 16)));
    return out;
}

RegisterNibbles value(const char* text) { return *parseDecimal(text); }

} // namespace

// ── T1 Codec, read ───────────────────────────────────────────────────────────

// LIB-7 AC-49 (ledger 59-59): the card file's comment line and `saved:`.
TEST(lib7_ac49_magnetic_card_comment_and_saved) {
    const std::string text = fileOf(*card(bank(1, 0x12), std::nullopt), "Mine");
    CHECK(text.rfind("# Calc-U-59 magnetic card\nformat: ti59-magnetic-card\n", 0) == 0);
    CHECK(text.find("\nsaved: 2026-11-16T10:00:00Z\n") != std::string::npos);
    CHECK(read(text).status == ReadOutcome::Status::Read);
}

TEST(c59_7_ac2_reads_format_3_both_sides_and_one_side) {
    const auto both = card(bank(1, 0x12), bank(2, 0x34, false));
    CueCardContent cue;
    cue.title = "My Program";
    cue.labels[5] = "run";
    const std::string text = fileOf(*both, "My Program", "Bank 1 and 2", false, cue);
    CHECK(text.rfind("# Calc-U-59 magnetic card\nformat: ti59-magnetic-card\nformat-version: 3\ncard-name: \"My Program\"\n", 0) == 0);
    CHECK(text.find("    encoding: register-hex\n    bytes: |\n      R000: 12 12") != std::string::npos);
    CHECK(text.find("      R030: 34 34") != std::string::npos);   // the row labels of bank 2
    const ReadOutcome r = read(text);
    CHECK(r.status == ReadOutcome::Status::Read);
    CHECK_EQ(r.info.name, std::string("My Program"));
    CHECK_EQ(r.info.description, std::string("Bank 1 and 2"));
    CHECK(sideOf(r, 0) == bank(1, 0x12));
    CHECK(sideOf(r, 1) == bank(2, 0x34, false));
    const auto c = magneticCardOf(r.info.content);
    CHECK(c && cardLabelOf(*c) && cardLabelOf(*c)->cueCard && cardLabelOf(*c)->cueCard->labels[5] == "run");
    // One side only: the other is fresh.
    const ReadOutcome one = read(fileOf(*card(std::nullopt, bank(4, 0x01))));
    CHECK(one.status == ReadOutcome::Status::Read);
    const auto oc = magneticCardOf(one.info.content);
    CHECK(oc && std::holds_alternative<std::monostate>(oc->sides[0]) && sideBank(oc->sides[1]) == 4);
    // A fresh card: `sides: {}`.
    const ReadOutcome fresh = read(fileOf(TIMagneticCardData{}));
    CHECK(fresh.status == ReadOutcome::Status::Read);
}

TEST(c59_7_ac3_refused_whole_missing_side_key_rows_unknown_key_cue_card_key) {
    const std::string good = fileOf(*card(bank(1, 0x12), std::nullopt));
    CHECK(read(good).status == ReadOutcome::Status::Read);
    const auto unreadable = [](const std::string& t) { return read(t).status == ReadOutcome::Status::Unreadable; };
    CHECK(unreadable(replaced(good, "    checksum: 0x07\n", "")));                      // a missing side key
    CHECK(unreadable(replaced(good, "      R029:", "      #R029:")));                   // 29 rows
    CHECK(unreadable(replaced(good, "      R000: 12 12 12 12 12 12 12 12\n", "      R000: 12 12 12 12 12 12 12 12\n      R000: 12 12 12 12 12 12 12 12\n")));   // 31
    CHECK(unreadable(good + "colour: red\n"));                                         // an unknown key at the top
    CHECK(unreadable(replaced(good, "    bank: 1\n", "    bank: 1\n    owner: me\n")));   // and in a side
    CHECK(unreadable(replaced(good, "  title: Prog\n", "  title: Prog\n  Template: MagnetCard\n")));   // the cue card's (cue-card.md §3)
    CHECK(unreadable(replaced(good, "sides:\n  0:", "sides:\n  2:")));
    CHECK(unreadable(replaced(good, "card-name: \"Prog\"\n", "")));
    const ReadOutcome r = read(replaced(good, "    checksum: 0x07\n", ""));
    CHECK(r.problem.find("checksum") != std::string::npos);   // one sentence why
}

TEST(c59_7_ac3_keys_exact_fixed_values_any_case) {
    const std::string good = fileOf(*card(bank(1, 0x12), std::nullopt));
    CHECK(read(replaced(good, "data-type: program", "data-type: PROGRAM")).status == ReadOutcome::Status::Read);
    CHECK(read(replaced(good, "encoding: register-hex", "encoding: Register-Hex")).status == ReadOutcome::Status::Read);
    CHECK(read(replaced(good, "protection: false", "protection: False")).status == ReadOutcome::Status::Read);
    CHECK(read(replaced(good, "format: ti59-magnetic-card", "format: TI59-Magnetic-Card")).status == ReadOutcome::Status::Read);
    CHECK(read(replaced(good, "    bank: 1\n", "    Bank: 1\n")).status == ReadOutcome::Status::Unreadable);   // keys exactly
    CHECK(read(replaced(good, "card-name:", "Card-Name:")).status == ReadOutcome::Status::Unreadable);
}

TEST(c59_7_ac4_older_version_outdated_with_hint) {
    const std::string good = fileOf(*card(bank(1, 0x12), std::nullopt));
    const ReadOutcome r = read(replaced(good, "format-version: 3", "format-version: 2"));
    CHECK(r.status == ReadOutcome::Status::Outdated);
    CHECK_EQ(r.info.formatVersion, 2);
    const std::string hint = calcu::media::Shelf::outdatedHint(magneticCardKind(), 2);
    CHECK(hint.find("Magnetic card format version 2 is outdated") != std::string::npos &&
          hint.find("current version is 3") != std::string::npos && hint.find("media.md") != std::string::npos);
    CHECK(read(replaced(good, "format-version: 3", "format-version: 4")).status == ReadOutcome::Status::Unreadable);
    CHECK(read("Calc-U-59 Card 2.0\nSIDE: 1\nEND-CARD\n").status == ReadOutcome::Status::Unreadable);   // EC-3 / LIB-7 EC-3
}

TEST(c59_7_ac4_u59_and_bin_not_listed_not_read_not_changed) {
    const auto& k = magneticCardKind();
    CHECK_EQ(k.suffix, std::string(".magnetcard.yaml"));
    calcu::testing::TempFolder folder("calcu59-cards");
    folder.write("old.u59", "Calc-U-59 Card 2.0\nEND-CARD\n");
    folder.write("old.bin", std::string(246, '\0'));
    folder.write("new.magnetcard.yaml", fileOf(*card(bank(1, 0x12), std::nullopt), "New"));
    calcu::settings::SettingsFile s = calcu::settings::SettingsFile::defaultsOnly(cu59::settings::settingsDecl());
    calcu::settings::HostDirs dirs;
    dirs.frontend = calcu::settings::FrontendKind::Given;   // the save folder as passed
    dirs.saveFolder = folder.str();
    calcu::settings::Paths paths(s, dirs);
    calcu::media::Shelf shelf({k}, &paths);
    const auto lines = shelf.reread();
    CHECK(lines.empty());   // nothing about the old files
    std::vector<std::string> names;
    for (const auto* e : shelf.entries(k))
        if (e->origin == calcu::media::Origin::Saved) names.push_back(e->fileName);
    CHECK(names == std::vector<std::string>{"new.magnetcard.yaml"});
    CHECK(!shelf.kindOfFile("old.u59") && !shelf.kindOfFile("old.bin"));
    CHECK_EQ(testroms::readFile(folder / "old.u59").value_or(""), std::string("Calc-U-59 Card 2.0\nEND-CARD\n"));
}

TEST(c59_7_ac29_info_name_description_template_no_switch) {
    const std::string t = fileOf(*card(bank(1, 0x12), std::nullopt), "Tpl", "A template", true);
    const ReadOutcome r = read(t);
    CHECK(r.status == ReadOutcome::Status::Read);
    CHECK(r.info.isTemplate && r.info.name == "Tpl" && r.info.description == "A template");
    CHECK(!r.info.hasWriteProtect && !r.info.writeProtected);
    CHECK(r.info.formatVersion == 3);
    const auto& k = magneticCardKind();
    CHECK(k.implicitTemplates.size() == 1 && k.implicitTemplates[0].name == "Fresh card");   // D-10: no file
    CHECK(k.words.insertStep == "card" && k.words.armStep == "arm" && k.words.inFileKey == "card" && k.words.ejectWord.empty());
}

TEST(c59_7_ec5_bom_or_not_utf8_unreadable) {
    const std::string good = fileOf(*card(bank(1, 0x12), std::nullopt));
    const ReadOutcome bom = read("\xEF\xBB\xBF" + good);
    CHECK(bom.status == ReadOutcome::Status::Unreadable && bom.problem.find("UTF-8") != std::string::npos);
    const ReadOutcome latin = read(replaced(good, "card-name: \"Prog\"", "card-name: \"Pr\xE9g\""));
    CHECK(latin.status == ReadOutcome::Status::Unreadable && latin.problem.find("UTF-8") != std::string::npos);
    CHECK(latin.problem.find("size") == std::string::npos);
}

// ── T2 Codec, write ──────────────────────────────────────────────────────────

namespace {
std::string write(const MediumCodec::Write& w) {
    std::string problem;
    auto files = codec().write(w, problem);
    return files.empty() ? "ERROR " + problem : files.front().bytes;
}
} // namespace

TEST(c59_7_ac5_written_whole_in_key_order_only_written_side) {
    CueCardContent cue;
    cue.title = "Kept title";
    const std::string source = fileOf(*card(bank(1, 0x11), bank(2, 0x22)), "Shared", "kept", false, cue);
    auto written = card(bank(3, 0x33), bank(4, 0x44));   // what the place has: side 1 written now
    written->lastWritten = 1;
    MediumCodec::Write w;
    w.name = "Shared";
    w.content = written;
    w.source = source;
    w.savedAt = "2026-11-17T08:00:00Z";
    const std::string out = write(w);
    const ReadOutcome r = read(out);
    CHECK(r.status == ReadOutcome::Status::Read);
    CHECK(sideOf(r, 0) == bank(1, 0x11));   // the side not written: as the file holds it
    CHECK(sideOf(r, 1) == bank(4, 0x44));
    CHECK_EQ(r.info.description, std::string("kept"));
    CHECK(out.find("title: Kept title") != std::string::npos);
    // The key order of media.md §1.1.
    std::vector<std::size_t> at;
    for (const char* k : {"format:", "format-version:", "card-name:", "description:", "saved:", "cue-card:", "sides:"})
        at.push_back(out.find(std::string("\n") + k) == std::string::npos ? out.find(k) : out.find(std::string("\n") + k));
    CHECK(std::is_sorted(at.begin(), at.end()));
}

TEST(c59_7_ac5_saved_set_comments_dropped) {
    const std::string source = "# a comment\n" + fileOf(*card(bank(1, 0x11), std::nullopt));
    CHECK(read(source).status == ReadOutcome::Status::Read);   // comments are allowed
    auto c = card(bank(1, 0x55), std::nullopt);
    c->lastWritten = 0;
    MediumCodec::Write w;
    w.name = "Prog";
    w.content = c;
    w.source = source;
    w.savedAt = "2026-11-17T09:30:00Z";
    const std::string out = write(w);
    CHECK(out.find("saved: 2026-11-17T09:30:00Z\n") != std::string::npos);
    CHECK(out.find("# a comment") == std::string::npos);
}

TEST(c59_7_ac5_template_never_written) {
    // The kind's template is never handed to the codec: the library writes an
    // instance only (LIB-7 A6, AC-15); here, the fresh card started by a name.
    calcu::testing::TempFolder folder("calcu59-template");
    const auto fresh = magneticCardKind().implicitTemplates[0].make();
    const auto c = magneticCardOf(fresh);
    CHECK(c && std::holds_alternative<std::monostate>(c->sides[0]) && std::holds_alternative<std::monostate>(c->sides[1]));
    // A first write without a source: both sides as the content has them, a cue card titled with the name.
    auto first = card(bank(1, 0x66), std::nullopt);
    first->lastWritten = 0;
    MediumCodec::Write w;
    w.name = "Started";
    w.content = first;
    const std::string out = write(w);
    CHECK(out.find("cue-card:\n  title: Started\n") != std::string::npos);
    CHECK(sideOf(read(out), 0) == bank(1, 0x66));
}

TEST(c59_7_ac5_opened_card_written_back_where_it_lies) {
    // C59-7 AC-5: an opened card's binding path is its own file; the library
    // writes there (LIB-7 AC-1); the codec keeps its keys (above). Checked
    // through the session in card_flows_tests.cpp (c59_7_ac27_dropped_card_*).
    auto c = card(bank(1, 0x10), std::nullopt);
    c->lastWritten = 0;
    MediumCodec::Write w;
    w.name = "Opened";
    w.content = c;
    w.source = fileOf(*card(bank(2, 0x20), bank(3, 0x30)), "Opened", "elsewhere");
    const ReadOutcome r = read(write(w));
    CHECK(r.info.description == "elsewhere" && sideOf(r, 0) == bank(1, 0x10) && sideOf(r, 1) == bank(3, 0x30));
}

TEST(c59_7_ec3_other_side_kept_as_the_file_holds_it) {
    // Fitted with side 1 fresh; another device wrote side 1 since; side 0 written here.
    auto fitted = card(bank(1, 0x01), std::nullopt);
    fitted->sides[0] = bank(1, 0x02);
    fitted->lastWritten = 0;
    MediumCodec::Write w;
    w.name = "Two";
    w.content = fitted;
    w.source = fileOf(*card(bank(1, 0x01), bank(2, 0x77)), "Two");   // the file now (re-read, D-3)
    const ReadOutcome r = read(write(w));
    CHECK(sideOf(r, 0) == bank(1, 0x02));
    CHECK(sideOf(r, 1) == bank(2, 0x77));
    // A source that cannot be read: no write.
    w.source = "Calc-U-59 Card 2.0\nbroken";
    CHECK(write(w).rfind("ERROR ", 0) == 0);
}

// ── T3 Content, bank builder ─────────────────────────────────────────────────

TEST(c59_7_ac1_side_holds_any_bank_either_side) {
    auto c = card(bank(4, 0x01), bank(4, 0x02));   // both sides the same bank
    CHECK(sideBank(c->sides[0]) == 4 && sideBank(c->sides[1]) == 4);
    TICardSideDef def;
    def.bank = 2;
    c->sides[0] = def;
    CHECK(sideBank(c->sides[0]) == 2);
    CHECK(!sideBank(TIMagneticCardData::Side{}));
    const ReadOutcome r = read(fileOf(*card(bank(3, 0x09), bank(1, 0x08))));
    CHECK(sideBank(magneticCardOf(r.info.content)->sides[0]) == 3 && sideBank(magneticCardOf(r.info.content)->sides[1]) == 1);
    // 30 registers, a checksum, the header of the bank.
    CHECK_EQ(kCardBankBytes, std::size_t{246});
}

TEST(c59_7_ac19_built_bank_is_the_roms_bank) {
    // The fixtures were written by the TI-59 ROM (n Write, read back with the
    // headless reader): a data bank and a protected program bank.
    TICardSideDef data;
    data.bank = 4;
    data.registers = {{0, value("1.5")}, {29, value("2.25e-7")}};
    CHECK(buildCardBank(data, 60) == hexFixture("bank4-data.hex"));
    TICardSideDef program;
    program.bank = 1;
    program.isProtected = true;
    program.steps = {{0, 1}, {1, 94}, {2, 96}, {3, 91}};   // 1 +/- Write R/S
    CHECK(buildCardBank(program, 60) == hexFixture("bank1-protected.hex"));
}

TEST(c59_7_ac19_partition_of_the_item_or_the_calculators) {
    TICardSideDef p;
    p.bank = 1;
    p.steps = {{0, 1}};
    const auto withMachine = buildCardBank(p, 60);
    CHECK_EQ(int(withMachine[0]), 0x16);   // 60 program registers
    CHECK_EQ(int(withMachine[1]), 0x11);   // bank 1 entirely in program memory
    p.partitionSteps = 160;                // the item's: 20 registers
    const auto own = buildCardBank(p, 60);
    CHECK_EQ(int(own[0]), 0x12);
    CHECK_EQ(int(own[1]), 0x10);
    CHECK(registerInBank(0, 4) && registerInBank(29, 4) && !registerInBank(30, 4) && registerInBank(90, 1) && !registerInBank(89, 1));
    CHECK(stepInBank(480, 3) && !stepInBank(479, 3) && firstStepOfBank(2) == 240);
}

// ── T4 Module cue cards ──────────────────────────────────────────────────────

TEST(c59_7_ac23_ml_25_and_le_21_preprinted) {
    SKIP_UNLESS(testroms::romSet() != nullptr, "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (const auto& m : allModuleMetadata(*testroms::romSet())) {
        const auto cards = moduleCueCards(*testroms::romSet(), m.id);
        int preprinted = 0;
        for (const auto& [n, c] : cards) {
            CHECK(c.template_ == CueCardTemplate::SolidState);
            if (c.title != m.menuTitle || c.id != m.id) ++preprinted;
        }
        const int want = m.id == "ML" ? 25 : m.id == "LE" ? 21 : 0;
        if (preprinted != want) std::printf("  %s: %d pre-printed\n", m.id.c_str(), preprinted);
        CHECK_EQ(preprinted, want);
    }
    const auto ml = moduleCueCards(*testroms::romSet(), "ML");
    CHECK(ml.count(1) && ml.at(1).id == "ML-01" && ml.at(1).title == "MASTER LIBRARY DIAGNOSTIC");
    CHECK(ml.count(2) && ml.at(2).labels[3] == kCueCardInvisibleMarker && ml.at(2).labels[1] == "→ A⁻¹");
}

TEST(c59_7_ac23_every_program_of_every_module_has_a_card) {
    SKIP_UNLESS(testroms::romSet() != nullptr, "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (const auto& m : tiModules()) {
        // The number of programs: the module's ROM header (app/ModuleImage.hpp).
        const auto* bytes = testroms::romSet()->bytes(std::string(m.id) + ".rom.yaml");
        CHECK(bytes != nullptr);
        if (!bytes) continue;
        std::string error;
        const TIModuleFile f = readModuleFile({reinterpret_cast<const char*>(bytes->data()), bytes->size()}, error);
        CHECK(error.empty() && f.rom.size() > 1);
        if (f.rom.size() < 2) continue;
        const int programs = (f.rom[0] >> 4) * 10 + (f.rom[0] & 0xF);   // byte 0: the program count (BCD)
        const auto cards = moduleCueCards(*testroms::romSet(), m.id);
        std::set<int> want, have;
        for (int i = 1; i <= programs; ++i) want.insert(i);
        for (const auto& [n, c] : cards) have.insert(n);
        if (want != have) std::printf("  %s: %d programs, %zu cards\n", m.id, programs, cards.size());
        CHECK(want == have);
    }
}

TEST(c59_7_ac22_no_stand_in_made_at_run_time) {
    // A module file without `cue-cards:` gives none: nothing is made up.
    const auto* ml = testroms::romSet() ? testroms::romSet()->bytes("ML.rom.yaml") : nullptr;
    SKIP_UNLESS(ml != nullptr, "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::string text(ml->begin(), ml->end());
    text = text.substr(0, text.find("cue-cards:"));
    CHECK(moduleCueCardsOf(text, "ML.rom.yaml").empty());
    std::string problem;
    CHECK(moduleCueCards(*testroms::romSet(), "XX", [&](const std::string& p) { problem = p; }).empty());
    CHECK(problem.find("XX.rom.yaml") != std::string::npos);
}
