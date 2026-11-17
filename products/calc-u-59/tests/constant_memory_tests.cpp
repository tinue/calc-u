// C59-3 TI-58C constant memory (design §6, T1–T7): the product kind
// `c59.constant-memory`, the file ti58c.memory.yaml (constant memory format
// 1), when the memory comes back and when the file is written, in the app
// session as a native frontend hosts it (tests/AppRig.hpp; the save folder
// in memory, the rig's wall clock).

#include "TestHarness.hpp"
#include "AppRig.hpp"
#include "../app/Bcd.hpp"
#include "../app/ConstantMemoryFile.hpp"

using apprig::attach;
using apprig::detach;
using apprig::Files;
using apprig::kMemoryPath;
using apprig::Rig;
using apprig::Value;
namespace st = calcu::startup;

namespace {
const char* kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";
bool roms() { return testroms::romSet() != nullptr; }
cu59::RegisterNibbles num(const char* v) { return *cu59::parseDecimal(v); }
std::string head(const std::string& model = "TI-58C") { return "format-version: 2\nmodel: " + model + "\n"; }

std::string fileWith(int reg, const char* value, const std::string& card = "") {
    return "format: ti58c-constant-memory\nformat-version: 1\n" + card + "ram:\n  encoding: register-hex\n  bytes: |\n    R0" +
           std::to_string(reg) + ": " + cu59::registerLineHex(num(value)) + "\n";
}

std::optional<cu59::ConstantMemoryContents> written(Files& f) {
    auto it = f.files.find(kMemoryPath);
    if (it == f.files.end()) return std::nullopt;
    auto r = cu59::readConstantMemoryFile(it->second);
    if (auto* c = std::get_if<cu59::ConstantMemoryContents>(&r)) return *c;
    return std::nullopt;
}
} // namespace

// ── T1: the kind (AC-1) ─────────────────────────────────────────────────────

TEST(c59_3_ac1_listed_on_ti58c_only) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* m : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(m);
        const bool listed = cu59::constantMemoryOf(*r.machine()) != nullptr;
        CHECK(listed == (std::string(m) == "TI-58C"));
    }
}

TEST(c59_3_ac1_counter_changes_with_content_only) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-58C");
    cu59::ConstantMemory* k = cu59::constantMemoryOf(*r.machine());
    const auto c0 = k->changeCount();
    CHECK_EQ(k->changeCount(), c0);   // nothing changed
    r.memory().writeRegister(5, num("7"));
    const auto c1 = k->changeCount();
    CHECK(c1 != c0);
    r.memory().writeRegister(5, num("7"));   // the same value again
    CHECK_EQ(k->changeCount(), c1);
    CHECK(k->content()[static_cast<std::size_t>(59 - 5)] == num("7"));   // R05 is raw 54 at 60 registers
}

// ── T2: back as a switched-on TI-58C (AC-2) ─────────────────────────────────

TEST(c59_3_ac2_partition_and_display_format_come_back) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    {
        Rig r("TI-58C", files);
        r.keys({"2", "2nd", "9", "1", "7"});   // 2 Op 17: 20 data registers, 320 steps
        r.keys({"CLR", "2nd", "(", "2"});      // Fix 2
        r.runFor(std::chrono::milliseconds(800));
        CHECK_EQ(r.memory().partitionSteps(), 320);
        CHECK_EQ(testroms::displayText(*r.machine()), std::string("000"));   // 0.00 (the text has no points)
    }   // the product quits (AC-6: written at once)
    CHECK(written(*files).has_value());
    Rig r("TI-58C", files);   // started again: the memory is in place before the processor (row 59-15)
    CHECK_EQ(r.memory().partitionSteps(), 320);
    r.keys({"7", "+", "0", "="});
    r.runFor(std::chrono::milliseconds(300));
    CHECK_EQ(testroms::displayText(*r.machine()), std::string("700"));   // 7.00: Fix 2 came back
}

TEST(c59_3_ac2_memory_in_place_before_the_processor) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    Rig r("TI-58C", files);
    CHECK(r.memory().readRaw(55) == num("3.5"));
    CHECK(r.part().constantMemory().fromFile());
}

// ── T3: the file (AC-3, AC-4, EC-2, EC-3, EC-5) ─────────────────────────────

// LIB-7 AC-49 (ledger 59-59): the file's comment line and `saved:`.
TEST(lib7_ac49_constant_memory_comment_and_saved) {
    cu59::ConstantMemoryContents c;
    c.ram[0] = num("25");
    const std::string text = cu59::writeConstantMemoryFile(c, "2026-11-17T10:00:00Z");
    CHECK(text.rfind("# Calc-U-59 TI-58C constant memory\nformat: ti58c-constant-memory\nformat-version: 1\n"
                     "saved: 2026-11-17T10:00:00Z\n", 0) == 0);
    auto back = cu59::readConstantMemoryFile(text);
    CHECK(std::holds_alternative<cu59::ConstantMemoryContents>(back) && std::get<cu59::ConstantMemoryContents>(back) == c);
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    Rig r("TI-58C", files);
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(700));   // AC-5: written
    const std::string& saved = files->files[kMemoryPath];
    CHECK(saved.rfind("# Calc-U-59 TI-58C constant memory\n", 0) == 0);
    CHECK(saved.find("\nsaved: 20") != std::string::npos);
}

TEST(c59_3_ac3_file_at_root_of_save_folder_not_a_medium) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(700));
    CHECK(files->files.count("save/ti58c.memory.yaml") == 1);
    CHECK_EQ(cu59::TIFileAccess::kConstantMemoryFile, std::string("ti58c.memory.yaml"));
}

TEST(c59_3_ac4_format_1_r000_to_r063_h_as_r060_r063) {
    cu59::ConstantMemoryContents c;
    c.ram[0] = num("25");
    c.ram[60] = num("1");   // H00
    c.ram[63] = num("-2");  // H03
    const std::string text = cu59::writeConstantMemoryFile(c);
    CHECK(text.rfind("# Calc-U-59 TI-58C constant memory\nformat: ti58c-constant-memory\nformat-version: 1\n", 0) == 0);
    CHECK(text.find("  encoding: register-hex\n") != std::string::npos);
    CHECK(text.find("    R000: ") != std::string::npos);
    CHECK(text.find("    R060: ") != std::string::npos);
    CHECK(text.find("    R063: ") != std::string::npos);
    CHECK(text.find("R001") == std::string::npos);   // zero registers are left out
    CHECK(text.find("cue-card") == std::string::npos);
    auto back = cu59::readConstantMemoryFile(text);
    CHECK(std::holds_alternative<cu59::ConstantMemoryContents>(back) && std::get<cu59::ConstantMemoryContents>(back) == c);
}

TEST(c59_3_ac4_own_card_stored_module_card_not) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5", "cue-card:\n  title: Loan\n  A: n\n");
    Rig r("TI-58C", files);
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Loan");   // AC-7: fitted again
    r.keys({"2nd", "LRN", "0", "1"});   // Pgm 01: the module's cue card shows over the own card
    r.memory().writeRegister(5, num("9"));
    r.runFor(std::chrono::milliseconds(1500));
    const auto w = written(*files);
    CHECK(w && w->cueCard && w->cueCard->title == "Loan");   // the own card, not the module's
}

TEST(c59_3_ac4_register_above_r063_refused) {
    const auto r = cu59::readConstantMemoryFile(
        "format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n  bytes: |\n    R064: 00 00 00 00 00 00 00 01\n");
    const auto* p = std::get_if<cu59::ConstantMemoryProblem>(&r);
    CHECK(p && p->kind == cu59::ConstantMemoryProblem::Kind::Damaged && p->what.find("line 6") != std::string::npos);
}

TEST(c59_3_ec2_outdated_or_no_version_not_read_notice) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* text : {"format: ti58c-constant-memory\nram:\n  encoding: register-hex\n",
                             "format: ti58c-constant-memory\nformat-version: 0\nram:\n  encoding: register-hex\n",
                             "CUECARD:\nTitle: X\n── Registers (raw values) ──\nR055: 00 00 00 00 00 00 00 35\n"}) {
        auto files = std::make_shared<Files>();
        files->files[kMemoryPath] = text;
        Rig r("TI-58C", files);
        bool noticed = false;
        for (const auto& n : r.notices()) noticed = noticed || n.find("is outdated") != std::string::npos;
        CHECK(noticed);
        CHECK(r.part().constantMemory().fromFile());   // replaced at the first change
        r.memory().writeRegister(5, num("7"));
        r.runFor(std::chrono::milliseconds(700));
        CHECK(written(*files).has_value());
    }
}

TEST(c59_3_ec2_ti58c_mem_ignored_without_notice) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files["save/ti58c.mem"] = "── Registers (raw values) ──\nR055: 00 00 00 00 00 00 00 35\n";
    Rig r("TI-58C", files);
    CHECK(r.memory().readRaw(55) != num("3.5"));
    for (const auto& n : r.notices()) CHECK(n.find("ti58c.mem") == std::string::npos);
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(700));
    CHECK_EQ(files->writes["save/ti58c.mem"], 0);   // not changed
}

TEST(c59_3_ec3_damaged_line_nothing_used_notice) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = "format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n  bytes: |\n"
                                "    R055: 00 00 00 00 00 00 00 35\n    R056: zz\n";
    Rig r("TI-58C", files);
    CHECK(r.memory().readRaw(55) != num("3.5"));   // nothing of it used
    bool noticed = false;
    for (const auto& n : r.notices()) noticed = noticed || (n.find("cannot be read") != std::string::npos && n.find("line 7") != std::string::npos);
    CHECK(noticed);
}

TEST(c59_3_ec5_no_register_lines_valid) {
    const auto r = cu59::readConstantMemoryFile("format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n");
    CHECK(std::holds_alternative<cu59::ConstantMemoryContents>(r));
}

// ── T4: writing (AC-5, AC-6, EC-4) ──────────────────────────────────────────

TEST(c59_3_ac5_written_within_half_a_second_wall_time) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    r.runFor(std::chrono::milliseconds(700));
    const int before = files->writes[kMemoryPath];
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(520));
    CHECK_EQ(files->writes[kMemoryPath], before + 1);
    CHECK(written(*files) && written(*files)->ram[54] == num("7"));
}

TEST(c59_3_ac5_not_written_without_change) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    r.runFor(std::chrono::seconds(1));
    const int before = files->writes[kMemoryPath];
    r.runFor(std::chrono::seconds(3));
    CHECK_EQ(files->writes[kMemoryPath], before);
}

TEST(c59_3_ac6_written_at_once_before_replace_on_suspend_on_quit) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    {
        Rig r("TI-58C", files);
        r.runFor(std::chrono::milliseconds(700));
        r.memory().writeRegister(5, num("1"));
        r.send(calcu::sessions::kSuspendCommand);   // suspend: at once
        CHECK(written(*files) && written(*files)->ram[54] == num("1"));
        r.send(calcu::sessions::kContinueCommand);
        r.memory().writeRegister(5, num("2"));
        CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-59"}})).done);   // before it is replaced
        CHECK(written(*files) && written(*files)->ram[54] == num("2"));
        CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58C"}})).done);
        CHECK(r.memory().readRaw(54) == num("2"));   // AC-14: away and back keeps it
        r.memory().writeRegister(5, num("3"));
    }   // quit
    CHECK(written(*files) && written(*files)->ram[54] == num("3"));
}

TEST(c59_3_ec4_failed_write_notice_once_next_change_retries) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->failWrites = true;
    Rig r("TI-58C", files);
    r.memory().writeRegister(5, num("1"));
    r.runFor(std::chrono::milliseconds(700));
    r.memory().writeRegister(5, num("2"));
    r.runFor(std::chrono::milliseconds(700));
    int notices = 0;
    for (const auto& n : r.notices()) notices += n.find("could not be saved") != std::string::npos;
    CHECK_EQ(notices, 1);
    CHECK(r.machine() != nullptr);   // runs on
    files->failWrites = false;
    r.memory().writeRegister(5, num("3"));
    r.runFor(std::chrono::milliseconds(700));
    CHECK(written(*files) && written(*files)->ram[54] == num("3"));
}

// ── T5: when it comes back (AC-7, AC-8, EC-1, EC-6, EC-7) ───────────────────

TEST(c59_3_ac7_start_and_model_switch_get_the_file) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    {
        Rig r("TI-58C", files);
        CHECK(r.memory().readRaw(55) == num("3.5"));
    }
    Rig r("TI-58", files);
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58C"}})).done);
    CHECK(r.memory().readRaw(55) == num("3.5"));
}

TEST(c59_3_ac7_cue_card_of_the_file_fitted_again) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5", "cue-card:\n  title: Loan\n");
    Rig r("TI-58C", files);
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Loan");
    const auto* s = r.fe.state<cu59::TICueCardState>(cu59::topics::kCueCard);
    CHECK(s && s->card && s->card->title == "Loan");
}

TEST(c59_3_ac8_preset_or_startup_preset_zeroed_preset_card) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5", "cue-card:\n  title: Loan\n");
    Rig r("TI-58C", files);
    CHECK(r.load(head() + "cue-card:\n  title: Preset\n", "p.ti58c").done);
    CHECK(r.memory().readRaw(55) != num("3.5"));
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Preset");
    CHECK(!r.part().constantMemory().fromFile());
    Rig s("TI-58C", files, [](calcu::settings::SettingsFile& f) { f.set("preset", "TI-58C", "examples:startup/default-ti58c.ti58c"); });
    CHECK(s.memory().readRaw(55) != num("3.5"));
    CHECK(!s.part().ownCueCard().has_value());
    CHECK(!s.part().constantMemory().fromFile());
}

TEST(c59_3_ec1_no_file_zeroed_written_at_first_change) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    CHECK(!r.part().ownCueCard().has_value());
    CHECK(r.part().constantMemory().fromFile());
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(700));
    CHECK(written(*files).has_value());
}

TEST(c59_3_ec6_startup_preset_not_loadable_gets_the_file) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    Rig r("TI-58C", files, [](calcu::settings::SettingsFile& f) { f.set("preset", "TI-58C", "examples:nothere.ti58c"); });
    CHECK(r.memory().readRaw(55) == num("3.5"));
    CHECK(r.part().constantMemory().fromFile());
}

TEST(c59_3_ec7_failed_preset_writes_nothing) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    Rig r("TI-58C", files);
    const auto reply = r.load(head() + "data:\n  00: 1\nkeys:\n  - key: 1\n  - expect: 2\n", "f.ti58c");
    CHECK(!reply.done);   // failed while carried out
    const int before = files->writes[kMemoryPath];   // the machine from the file was written before it was replaced (AC-6)
    r.memory().writeRegister(5, num("9"));
    r.runFor(std::chrono::seconds(1));
    CHECK_EQ(files->writes[kMemoryPath], before);
}

// ── T6: Reset, Cold boot, other models (AC-9 to AC-12) ──────────────────────

TEST(c59_3_ac9_cold_boot_zeroes_then_startup) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    Rig r("TI-58C", files);
    CHECK(r.send(st::kColdBootCommand).done);
    CHECK(r.memory().readRaw(55) != num("3.5"));
}

TEST(c59_3_ac10_preset_machine_never_writes) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    {
        Rig r("TI-59", files);
        CHECK(r.load(head() + "data:\n  05: 1\n", "p.ti58c").done);
        r.memory().writeRegister(5, num("4"));
        r.runFor(std::chrono::seconds(1));
        r.send(calcu::sessions::kSuspendCommand);
        r.send(calcu::sessions::kContinueCommand);
        CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-59"}})).done);
    }
    CHECK_EQ(files->writes[kMemoryPath], 0);
    CHECK(written(*files) && written(*files)->ram[55] == num("3.5"));
}

TEST(c59_3_ac10_cold_boot_of_file_machine_file_follows_card_kept) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5", "cue-card:\n  title: Loan\n");
    Rig r("TI-58C", files);
    CHECK(r.send(st::kColdBootCommand).done);
    r.runFor(std::chrono::milliseconds(600));
    const auto w = written(*files);
    CHECK(w && w->ram[55] != num("3.5"));
    CHECK(w && w->cueCard && w->cueCard->title == "Loan");
}

TEST(c59_3_ac11_reset_power_cycle_memory_and_card_stay_file_not_read) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5", "cue-card:\n  title: Loan\n");
    Rig r("TI-58C", files);
    files->files[kMemoryPath] = fileWith(55, "9");   // changed behind its back: not read again
    r.memory().writeRegister(5, num("7"));
    CHECK(r.send(st::kResetCommand).done);
    CHECK(r.memory().readRegister(5) == num("7"));
    CHECK(r.memory().readRaw(55) == num("3.5"));
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);   // a power cycle
    CHECK(r.memory().readRegister(5) == num("7"));
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Loan");
}

TEST(c59_3_ac12_ti59_ti58_neither_read_nor_write) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    files->files[kMemoryPath] = fileWith(55, "3.5");
    for (const char* m : {"TI-59", "TI-58"}) {
        Rig r(m, files);
        CHECK(r.memory().readRaw(55) != num("3.5"));
        r.memory().writeRegister(5, num("4"));
        r.runFor(std::chrono::seconds(1));
    }
    CHECK_EQ(files->writes[kMemoryPath], 0);
}

// ── T7: frontends, today's tests (AC-13, AC-14) ─────────────────────────────

TEST(c59_3_ac14_model_switch_away_and_back_keeps_memory) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    r.memory().writeRegister(5, num("6.5"));
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58"}})).done);
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58C"}})).done);
    CHECK(r.memory().readRegister(5) == num("6.5"));
}

TEST(c59_3_ac13_the_host_folder_is_the_one_written) {
    SKIP_UNLESS(roms(), kSkip);
    // Each host passes TIAppSetup::saveFolder and its file access (Qt6: plain
    // files in LIB-9's save folder; Swift: the bridge's coordinated I/O; the
    // Web: the persistent file system): the file goes where the host says.
    auto files = std::make_shared<Files>();
    Rig r("TI-58C", files);
    r.memory().writeRegister(5, num("7"));
    r.runFor(std::chrono::milliseconds(700));
    CHECK(files->files.count(kMemoryPath) == 1);
    for (const auto& [path, text] : files->files) CHECK(path.rfind("save/", 0) == 0);
}
