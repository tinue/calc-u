// TI preset content: the product kind
// "program memory", the sections partition:, program:, data:, cue-card:,
// the bundled examples and the bundle file. Read-time refusals through
// presets::readPreset with TIPresetRules; what a section does through the
// harness entry in virtual time; the fitted cue card through the app session
// (SessionRig). ROM-gated cases skip without the ROMs.

#include "TestHarness.hpp"
#include "TIPresetRig.hpp"
#include "SessionRig.hpp"
#include "../app/Bcd.hpp"
#include "../app/Keycodes.hpp"
#include "../app/TIPresetKeys.hpp"
#include "presets/BundleFile.hpp"
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;
using tirig::Calcu;
using tirig::PresetRead;

namespace {

const char* kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";

std::string head(const std::string& model = "TI-59") { return "format-version: 2\nmodel: " + model + "\n"; }

cu59::RegisterNibbles nibbles(const std::string& value) { return *cu59::parseDecimal(value); }

// The reply's lines without `done <n>`.
std::vector<std::string> body(const std::string& reply) {
    std::vector<std::string> out;
    std::istringstream in(reply);
    std::string l;
    std::getline(in, l);
    while (std::getline(in, l)) out.push_back(l);
    return out;
}

} // namespace

// ── The kind ────────────────────────────────────────────────────────────────

TEST(listed_on_every_model) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        CHECK_EQ(c(std::string("model ") + model), std::string("done\n"));
        CHECK(c("capabilities").find("RAM/c59.program-memory") != std::string::npos);
        CHECK(cu59::programMemoryOf(*c.machine()) != nullptr);
    }
}

TEST(step_register_extra_partition_read_back) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        c(std::string("model ") + model);
        cu59::ProgramMemory* pm = cu59::programMemoryOf(*c.machine());
        CHECK(pm != nullptr);
        if (!pm) return;
        pm->writeStep(7, 42);
        CHECK_EQ(int(pm->readStep(7)), 42);
        pm->writeRegister(5, nibbles("3.25"));
        CHECK(pm->readRegister(5) == nibbles("3.25"));
        pm->setPartition(160);
        CHECK_EQ(pm->partitionSteps(), 160);
        const bool c58 = std::string(model) == "TI-58C";
        CHECK_EQ(pm->extraCount(), c58 ? 4 : 0);
        if (c58) {
            pm->writeExtra(2, nibbles("7.77e22"));
            CHECK(pm->readExtra(2) == nibbles("7.77e22"));
        }
        CHECK_EQ(pm->lastStep(), std::string(model) == "TI-59" ? 959 : 479);
        CHECK_EQ(pm->dataRegisterCount(), std::string(model) == "TI-59" ? 100 : 60);
    }
}

// ── Partition: ──────────────────────────────────────────────────────────────

TEST(rounds_up_to_80_at_most_960) {
    CHECK(std::get<int>(cu59::partitionSteps("100", 959)) == 160);   // 101 steps → 160
    CHECK(std::get<int>(cu59::partitionSteps("79", 959)) == 80);
    CHECK(std::get<int>(cu59::partitionSteps("959", 959)) == 960);
    CHECK(std::get<int>(cu59::partitionSteps("0", 959)) == 80);
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "partition: 100\n"), std::string("done\n"));
    CHECK_EQ(c("c59 partition"), std::string("done 1\n159\n"));
}

TEST(point_part_not_read) {
    CHECK(std::get<int>(cu59::partitionSteps("479.59", 959)) == 480);
    CHECK(std::get<int>(cu59::partitionSteps("799.19", 959)) == 800);
    PresetRead r;
    CHECK_EQ(r.error(head() + "partition: 479.59\n"), std::string());
}

TEST(none_keeps_rom_partition) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    const std::pair<const char*, const char*> want[] = {{"TI-59", "479"}, {"TI-58", "239"}, {"TI-58C", "239"}};
    for (const auto& [model, last] : want) {
        CHECK_EQ(c.preset(head(model) + "data:\n  00: 1\n", "p.ti59"), std::string("done\n"));
        CHECK_EQ(c("c59 partition"), "done 1\n" + std::string(last) + "\n");
    }
}

TEST(not_a_number_or_above_last_step_refused) {
    PresetRead r;
    CHECK(r.error(head() + "partition: abc\n").find("line 3") != std::string::npos);
    CHECK(r.error(head() + "partition: 960\n").find("beyond the last program step") != std::string::npos);
    CHECK(r.error(head("TI-58") + "partition: 480\n", "t.ti58").find("479") != std::string::npos);
    CHECK_EQ(r.error(head("TI-58") + "partition: 479\n", "t.ti58"), std::string());
    CHECK(!r.error(head() + "partition: -1\n").empty());
}

// ── Program: ────────────────────────────────────────────────────────────────

TEST(compact_and_listing_notations) {
    const auto r = cu59::readKeycodes("76 11 42 29   # compact\n010 43 RCL\n011 00 00\n# ...\n110 42 STO\n92\n", 959);
    const auto* steps = std::get_if<std::vector<cu59::ProgramStep>>(&r);
    CHECK(steps != nullptr);
    if (!steps) return;
    const std::vector<cu59::ProgramStep> want = {{0, 76}, {1, 11}, {2, 42}, {3, 29}, {10, 43}, {11, 0}, {110, 42}, {111, 92}};
    CHECK(*steps == want);
}

TEST(mnemonic_not_read_even_a_number) {
    const auto r = cu59::readKeycodes("473 00 0\n474 05 5 7 9\n", 959);
    const auto* steps = std::get_if<std::vector<cu59::ProgramStep>>(&r);
    CHECK(steps && steps->size() == 2);
    if (steps) CHECK((*steps)[1].step == 474 && (*steps)[1].keycode == 5);
}

TEST(one_digit_or_word_refused_with_line) {
    PresetRead r;
    const std::string one = head() + "program:\n  keycodes: |\n    76 11\n    # a comment line\n    42 1\n";
    CHECK(r.error(one).find("line 7") != std::string::npos);   // the token's own line in the file
    const std::string word = head() + "program:\n  keycodes: |\n    76 LBL 11\n";
    CHECK(r.error(word).find("line 5") != std::string::npos);
    CHECK(r.error(word).find("LBL") != std::string::npos);
}

TEST(preset_step_beyond_last_refused) {
    PresetRead r;
    CHECK(!r.error(head("TI-58") + "program:\n  keycodes: |\n    480 76 LBL\n", "t.ti58").empty());
    CHECK_EQ(r.error(head("TI-58") + "program:\n  keycodes: |\n    479 76 LBL\n", "t.ti58"), std::string());
    CHECK(!r.error(head() + "program:\n  keycodes: |\n    959 76\n    11\n").empty());   // compact past 959
}

TEST(keystroke_program_file_read_by_same_rules) {
    PresetRead r;
    r.write("loan.keystrokes.yaml", "format: ti-keystroke-program\nformat-version: 1\nprogram-name: Loan\n"
                                    "description: Monthly payment\nkeycodes: |\n  000 76 LBL\n  001 11 A\n  92\n");
    CHECK_EQ(r.error(head() + "program:\n  file: loan.keystrokes.yaml\n"), std::string());
    const auto k = cu59::readKeystrokeProgramFile(
        "format: ti-keystroke-program\nformat-version: 1\nprogram-name: Loan\nkeycodes: |\n  000 76 LBL\n  92\n", 959);
    const auto* p = std::get_if<cu59::KeystrokeProgram>(&k);
    CHECK(p && p->name == "Loan" && p->steps.size() == 2 && p->steps[1].step == 1 && p->steps[1].keycode == 92);
}

TEST(file_breaking_its_format_refused) {
    PresetRead r;
    r.write("bad1.keystrokes.yaml", "format: ti-keystroke-program\nformat-version: 1\nkeycodes: |\n  76\n");   // no program-name
    r.write("bad2.keystrokes.yaml", "format: ti-keystroke-program\nformat-version: 2\nprogram-name: X\nkeycodes: |\n  76\n");
    r.write("bad3.keystrokes.yaml", "format: ti-keystroke-program\nformat-version: 1\nprogram-name: X\nkeycodes: |\n  76 A\n");
    r.write("bad4.keystrokes.yaml", "format: ti-keystroke-program\nformat-version: 1\nprogram-name: X\nextra: 1\nkeycodes: |\n  76\n");
    for (const char* f : {"bad1", "bad2", "bad3", "bad4"})
        CHECK(!r.error(head() + "program:\n  file: " + std::string(f) + ".keystrokes.yaml\n").empty());
    CHECK(!r.error(head() + "program:\n  file: missing.keystrokes.yaml\n").empty());
    CHECK(!r.error(head() + "program:\n  keycodes: |\n    76\n  file: x.keystrokes.yaml\n").empty());   // exactly one
}

TEST(only_listed_steps_written) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    const std::string text = head() + "program:\n  keycodes: |\n    11 12 13 14\nprogram:\n  keycodes: |\n    002 99\n";
    CHECK_EQ(c.preset(text), std::string("done\n"));
    CHECK_EQ(c("c59 program 000 count=5"), std::string("done 5\n000 11\n001 12\n002 99\n003 14\n004 00\n"));
}

TEST(steps_beyond_partition_written) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    const std::string text = head() + "partition: 159\nprogram:\n  keycodes: |\n    700 76 LBL\n";
    CHECK_EQ(c.preset(text), std::string("done\n"));
    CHECK_EQ(c("c59 program 700 count=1"), std::string("done 1\n700 76\n"));
}

// ── Data: ───────────────────────────────────────────────────────────────────

TEST(whole_number_digit_for_digit_cut_after_13) {
    CHECK(nibbles("12345678901234567") == nibbles("1234567890123e4"));
    CHECK_EQ(cu59::registerText(nibbles("12345678901234567")), std::string("1.234567890123e16"));
    CHECK_EQ(cu59::registerText(nibbles("99999999999999")), std::string("9.999999999999e13"));   // cut, not rounded
    CHECK_EQ(cu59::registerText(nibbles("-42")), std::string("-4.2e1"));
}

TEST(other_values_rounded_to_13) {
    CHECK_EQ(cu59::registerText(nibbles("1.23456789012345")), std::string("1.234567890123e0"));
    CHECK_EQ(cu59::registerText(nibbles("2.99999999999999")), std::string("3e0"));
    CHECK_EQ(cu59::registerText(nibbles("-1.5e-3")), std::string("-1.5e-3"));
    CHECK_EQ(cu59::registerText(nibbles(".5")), std::string("5e-1"));
}

TEST(no_number_refused) {
    PresetRead r;
    for (const char* v : {"abc", "1.2.3", "1e", "--1", "0x10", ""})
        CHECK(r.error(head() + "data:\n  05: " + std::string(v) + "\n").find("line 4") != std::string::npos);
    CHECK(!r.error(head() + "data:\n  05: 1\n  05: 2\n").empty());   // a key twice
}

TEST(registers_60_to_99_on_ti58_refused_h_off_ti58c_refused) {
    PresetRead r;
    CHECK(!r.error(head("TI-58") + "data:\n  60: 1\n", "t.ti58").empty());
    CHECK_EQ(r.error(head("TI-58") + "data:\n  59: 1\n", "t.ti58"), std::string());
    CHECK_EQ(r.error(head() + "data:\n  99: 1\n"), std::string());
    CHECK(!r.error(head() + "data:\n  H00: 1\n").empty());
    CHECK(!r.error(head("TI-58") + "data:\n  H00: 1\n", "t.ti58").empty());
    CHECK_EQ(r.error(head("TI-58C") + "data:\n  H03: 1\n  h01: 2\n", "t.ti58c"), std::string());
    CHECK(!r.error(head("TI-58C") + "data:\n  H04: 1\n", "t.ti58c").empty());
    CHECK(!r.error(head() + "data:\n  5: 1\n").empty());   // NN is two digits
}

TEST(h00_to_h03_are_not_60_to_63) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head("TI-58C") + "data:\n  H01: 7.77E22\n  59: 5\n", "t.ti58c"), std::string("done\n"));
    cu59::ProgramMemory* pm = cu59::programMemoryOf(*c.machine());
    CHECK(pm->readExtra(1) == nibbles("7.77e22"));
    CHECK(pm->readRaw(61) == nibbles("7.77e22"));   // rom/TI58C-ROM-Guide.md:37: H01 is RAM 61
    CHECK(pm->readRegister(59) == nibbles("5"));
    CHECK_EQ(c("c59 data-registers H01 59"), std::string("done 2\nH01: 7.77e22\n59: 5e0\n"));
}

TEST(data_before_partition_in_file_order) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    // Register 05 is written while the partition is the ROM's; then the
    // partition moves; what it then holds is the calculator's matter.
    CHECK_EQ(c.preset(head() + "data:\n  05: 3\npartition: 159\n"), std::string("done\n"));
    CHECK_EQ(c("c59 partition"), std::string("done 1\n159\n"));
    PresetRead r;
    const auto plan = r.read(head() + "data:\n  05: 3\npartition: 159\n");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p && p->items.size() == 2 && p->items[0].item->logText().rfind("data:", 0) == 0);
}

TEST(h_registers_written_after_cold_boot_else_as_startup_left) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head("TI-58C") + "data:\n  00: 1\n", "a.ti58c"), std::string("done\n"));
    cu59::ProgramMemory* pm = cu59::programMemoryOf(*c.machine());
    const cu59::RegisterNibbles h0 = pm->readExtra(0);   // as the start-up left it in zeroed memory
    c("c59 data-registers H00=5");
    CHECK_EQ(c.preset(head("TI-58C") + "data:\n  00: 1\n", "b.ti58c"), std::string("done\n"));
    pm = cu59::programMemoryOf(*c.machine());
    CHECK(pm->readExtra(0) == h0);   // a new preset: a cold boot, not what the last machine held
    CHECK_EQ(c.preset(head("TI-58C") + "data:\n  H00: 2.5\n", "c.ti58c"), std::string("done\n"));
    CHECK(cu59::programMemoryOf(*c.machine())->readExtra(0) == nibbles("2.5"));
}

// ── Cue-card: ───────────────────────────────────────────────────────────────

TEST(keys_defaults_tokens_prime_any_case) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    const std::string text = head() + "cue-card:\n  title: Loan \\to \\lambda\n  A: n\n  \"a'\": i\n  \"B′\": \\blank\n"
                                      "  row-2: Key it\n  row-2-align: center\n  pencil-color: Pencil\n  style: button\n";
    CHECK(r.load(text).done);
    r.run(std::chrono::milliseconds(100));
    const auto& card = r.part().ownCueCard();
    CHECK(card.has_value());
    if (!card) return;
    CHECK_EQ(card->title, std::string("Loan → λ"));
    CHECK_EQ(card->labels[5], std::string("n"));        // A
    CHECK_EQ(card->labels[0], std::string("i"));        // A′ (written a', any case)
    CHECK_EQ(card->labels[1], std::string(cu59::kCueCardInvisibleMarker));   // \blank
    CHECK(card->row2Align == cu59::TextAlign::Center);
    CHECK(card->row1Align == cu59::TextAlign::Center);  // the default
    CHECK(card->color == cu59::CueCardInkColor::Pencil);
    CHECK(card->style == cu59::CardButtonStyle::Button);
    CHECK(card->template_ == cu59::CueCardTemplate::CueCard);   // always the plain card
}

TEST(template_banks_id_refused) {
    PresetRead r;
    for (const char* key : {"Template: MagnetCard", "Banks: 1,2", "id: ML-01", "Title: x", "bank: 1"})
        CHECK(r.error(head() + "cue-card:\n  title: T\n  " + std::string(key) + "\n").find("line 5") != std::string::npos);
    CHECK(!r.error(head() + "cue-card:\n  row-1-align: middle\n").empty());
}

TEST(topic_says_fitted_card) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    CHECK(r.load(head() + "cue-card:\n  title: Sum 1..N\n  A: Sum\n").done);
    r.run(std::chrono::milliseconds(100));
    const auto* s = r.state<cu59::TICueCardState>(cu59::topics::kCueCard);
    CHECK(s && s->card && s->card->title == "Sum 1..N");
}

TEST(no_section_no_card) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    CHECK(r.load(head() + "cue-card:\n  title: First\n").done);
    CHECK(r.load(head() + "data:\n  00: 1\n").done);   // a new preset: a cold boot, no card until it fits one
    r.run(std::chrono::milliseconds(100));
    CHECK(!r.part().ownCueCard().has_value());
}

TEST(second_replaces_first) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    CHECK(r.load(head() + "cue-card:\n  title: First\ncue-card:\n  title: Second\n").done);
    r.run(std::chrono::milliseconds(100));
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Second");
}

TEST(model_printer_card_unknown_keys) {
    PresetRead r;
    for (const char* key : {"model: TI-58", "printer: PC-100C", "card: x"})
        CHECK(r.error(head() + "cue-card:\n  title: T\n  " + std::string(key) + "\n").find("no key") != std::string::npos);
}

// ── Sections in file order ──────────────────────────────────────────────────

TEST(first_stage_as_today) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    // sum.ti59's first stage (partition, program, cue card) and its keys give
    // the RAM of its reference run (the full comparison: reference_runs_tests.cpp).
    const fs::path refFile = "tests/references/examples/sum.ti59.reference";
    CHECK(fs::exists(refFile));
    Calcu c;
    CHECK_EQ(c("preset examples/sum.ti59"), std::string("done\n"));
    c("wait");
    std::ifstream ref(refFile);
    std::string line;
    bool inRam = false;
    int compared = 0;
    cu59::ProgramMemory* pm = cu59::programMemoryOf(*c.machine());
    while (std::getline(ref, line)) {
        if (line.rfind("--- ", 0) == 0) { inRam = line == "--- ram"; continue; }
        if (!inRam || line.empty()) continue;
        const int reg = std::stoi(line.substr(0, line.find(' ')));
        const std::string hex = line.substr(line.find(' ') + 1);
        std::string got;
        for (std::uint8_t n : pm->readRaw(reg)) {
            static const char* d = "0123456789abcdef";
            got += d[n >> 4];
            got += d[n & 15];
        }
        CHECK_EQ(got, hex);
        ++compared;
    }
    CHECK_EQ(compared, 120);
}

TEST(sections_after_keys_are_reloads) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    // The program counts R00 down from 5 (Lbl A … Dsz 0 …): a reload after
    // the keys changes only what it lists.
    const std::string text = head() +
                             "program:\n  keycodes: |\n    76 11 01 85 01 95 91\n"
                             "data:\n  00: 3\n  01: 4\n"
                             "keys:\n  - key: A\n"
                             "program:\n  keycodes: |\n    003 02\n"
                             "data:\n  01: 9\n";
    CHECK_EQ(c.preset(text), std::string("done\n"));
    CHECK_EQ(c("expect 2"), std::string("done\n"));   // A ran: 1 + 1 = 2
    CHECK_EQ(c("c59 program 000 count=7"),
             std::string("done 7\n000 76\n001 11\n002 01\n003 02\n004 01\n005 95\n006 91\n"));
    CHECK_EQ(c("c59 data-registers 00 01"), std::string("done 2\n00: 3e0\n01: 9e0\n"));
}

TEST(apply_state_ti58_default_partition_and_limits) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head("TI-58") + "program:\n  keycodes: |\n    479 92\n", "t.ti58"), std::string("done\n"));
    CHECK_EQ(c("c59 partition"), std::string("done 1\n239\n"));
    CHECK_EQ(c("c59 program 479 count=1"), std::string("done 1\n479 92\n"));
    CHECK(c("c59 program 480 count=1").rfind("refused", 0) == 0);
}

TEST(reload_changes_only_the_listed_steps_and_registers) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "program:\n  keycodes: |\n    11 12 13\ndata:\n  00: 1\n  02: 2\n"
                               "keys:\n  - key: CLR\nprogram:\n  keycodes: |\n    001 22\ndata:\n  02: 7\n"),
             std::string("done\n"));
    CHECK_EQ(c("c59 program 000 count=3"), std::string("done 3\n000 11\n001 22\n002 13\n"));
    CHECK_EQ(c("c59 data-registers 00 02"), std::string("done 2\n00: 1e0\n02: 7e0\n"));
}

TEST(data_registers_match_the_rom) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        CHECK_EQ(c.preset(head(model) + "data:\n  05: 1234.5\n  29: -2.5e-7\n", "x.ti59"), std::string("done\n"));
        c("key CLR RCL 0 5");
        c("wait");
        CHECK(c("display-text").find("1234.5") != std::string::npos);
        CHECK_EQ(c("expect 1234.5"), std::string("done\n"));
        c("key CLR RCL 2 9");
        c("wait");
        CHECK_EQ(c("expect -2.5e-7"), std::string("done\n"));
    }
}

// ── The bundled presets ─────────────────────────────────────────────────────

TEST(every_bundled_preset_is_format_2) {
    // The four examples with magnetic cards too.
    int seen = 0;
    for (const auto& e : fs::recursive_directory_iterator("examples")) {
        const auto ext = e.path().extension().string();
        if (ext != ".ti59" && ext != ".ti58" && ext != ".ti58c") continue;
        const std::string rel = fs::relative(e.path(), "examples").generic_string();
        std::ifstream in(e.path());
        std::stringstream ss;
        ss << in.rdbuf();
        const bool v2 = ss.str().find("\nformat-version: 2\n") != std::string::npos || ss.str().rfind("format-version: 2\n", 0) == 0;
        if (!v2) std::fprintf(stderr, "  not version 2: %s\n", rel.c_str());
        CHECK(v2);
        ++seen;
    }
    CHECK_EQ(seen, 57 + 3);   // the 57 examples and the three startup presets
}

TEST(names_short_described_once_unique_per_model) {
    PresetRead r;
    calcu::presets::DirectoryPresetBundle bundle("examples");
    const std::vector<std::string> problems = calcu::presets::checkBundle(bundle, r.setup());
    for (const auto& p : problems) std::fprintf(stderr, "  bundle.yaml: %s\n", p.c_str());
    CHECK(problems.empty());
    std::vector<std::string> left;
    const auto list = calcu::presets::presetList(bundle, r.rules, tiCatalogue(), &left);
    CHECK(left.empty());
    std::set<std::string> seen;
    for (const auto& e : list) {
        CHECK(!e.name.empty() && e.name.size() <= 18);
        CHECK(!e.description.empty());
        CHECK(seen.insert(e.model.value + "/" + e.name).second);
    }
    CHECK_EQ(list.size(), std::size_t{14});   // three startup presets and eleven examples
}

TEST(trace_quirk_keeps_extension_and_model) {
    PresetRead r;
    std::ifstream in("examples/historic/trace-quirk.ti59");
    std::stringstream ss;
    ss << in.rdbuf();
    const auto plan = r.read(ss.str(), "trace-quirk.ti59");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p && p->model.value == "TI-58");
}

// ── The bundle file ─────────────────────────────────────────────────────────

TEST(bundle_lists_three_startup_and_todays_examples_in_groups) {
    calcu::presets::DirectoryPresetBundle bundle("examples");
    const auto read = calcu::presets::readBundleFile(bundle);
    const auto* entries = std::get_if<std::vector<calcu::presets::BundleEntry>>(&read);
    CHECK(entries != nullptr);
    if (!entries) return;
    std::vector<std::string> got;
    for (const auto& e : *entries) got.push_back((e.startup ? "startup " : e.group + " ") + e.preset);
    const std::vector<std::string> want = {
        "startup startup/default-ti59.ti59", "startup startup/default-ti58.ti58", "startup startup/default-ti58c.ti58c",
        "Diagnostics diag.ti59", "Diagnostics diag.ti58", "Diagnostics ram_test.ti59",
        "Programs sum.ti59", "Programs characters.ti59", "Programs fastmode-sst.ti59",
        "Programs historic/life.ti59",
        "Printer pictures printer-interrupt.ti59", "Printer pictures picard.ti59", "Printer pictures switzerland.ti59",
        "Printer pictures globe-print.ti59"};
    CHECK(got == want);
    // Each startup preset: the Master Library module, the PC-100C, nothing else: a startup preset builds
    // the machine from a cold boot; settings hold no hardware.
    PresetRead r;
    for (const char* f : {"startup/default-ti59.ti59", "startup/default-ti58.ti58", "startup/default-ti58c.ti58c"}) {
        const auto source = calcu::presets::bundleSource(bundle, f);
        const auto plan = calcu::presets::readPreset(*source, r.setup());
        const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
        CHECK(p && p->places.size() == 2 && p->items.empty());
        if (!p) continue;
        std::set<std::string> plugged;
        for (const auto& pl : p->places) plugged.insert(pl.request.peripheral.value);
        CHECK(plugged == (std::set<std::string>{"ML", "PC-100C"}));
    }
}
