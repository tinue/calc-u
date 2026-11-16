// LIB-6's Calc-U-59 column (design §11.6, P1–P6, P10): the TI rules of the
// library's preset loader (app/TIPresetRules), and LIB-5's "in a preset"
// clauses (design §10.6). Read-time cases through presets::readPreset with
// TIPresetRules; run cases through the harness entry in virtual time or the
// app session (SessionRig).

#include "TestHarness.hpp"
#include "TIPresetRig.hpp"
#include "SessionRig.hpp"
#include "../app/Bcd.hpp"
#include "../app/TIDisplayPicture.hpp"
#include "../app/TIPresetKeys.hpp"
#include "../app/KeyNames.hpp"
#include "presets/Expect.hpp"
#include "presets/IdleWatch.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using tirig::Calcu;
using tirig::PresetRead;

namespace {

const char* kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";
std::string head(const std::string& model = "TI-59") { return "format-version: 2\nmodel: " + model + "\n"; }

std::string fileText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

// ── P1: models, places (AC-35, AC-37) ───────────────────────────────────────

TEST(lib6_ac35_extensions_models_any_case_no_variant) {
    PresetRead r;
    for (const char* m : {"TI-59", "ti-58", "Ti-58c"}) CHECK_EQ(r.error(head(m) + "data:\n  00: 1\n"), std::string());
    CHECK(!r.error(head("TI-59:A") + "data:\n  00: 1\n").empty());   // no variant
    CHECK(!r.error(head("TI-57") + "data:\n  00: 1\n").empty());
    CHECK(!r.error("format-version: 2\ndata:\n  00: 1\n").empty());   // the model only from model:
}

TEST(lib6_ac35_slot1_by_name_only_file_refused_with_notice) {
    PresetRead r;
    CHECK_EQ(r.error(head() + "slot-1: { name: ML }\n"), std::string());
    CHECK(r.error(head() + "slot-1: { file: ml.rom.yaml }\n").find("A Solid State ROM module is named by name: in this release.") !=
          std::string::npos);
    CHECK(!r.error(head() + "slot-1: ML\n").empty());   // the bare form is not taken
    CHECK(!r.error(head() + "slot-1: { name: XX }\n").empty());
}

TEST(lib6_ac35_printer_pc100c) {
    PresetRead r;
    CHECK_EQ(r.error(head() + "printer: PC-100C\n"), std::string());
    CHECK_EQ(r.error(head("TI-58C") + "printer: pc-100c\n", "t.ti58c"), std::string());
    CHECK(!r.error(head() + "printer: CE-150\n").empty());
}

TEST(lib6_ac37_no_slot1_no_printer_means_none) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "data:\n  00: 1\n"), std::string("done\n"));
    const std::string caps = c("capabilities");
    CHECK(caps.find("place slot-1 empty") != std::string::npos);
    CHECK(caps.find("place printer empty") != std::string::npos);
    CHECK_EQ(c.preset(head() + "slot-1: { name: ST }\nprinter: PC-100C\n"), std::string("done\n"));
    CHECK(c("capabilities").find("place slot-1 ST") != std::string::npos);
    CHECK(c("capabilities").find("place printer PC-100C") != std::string::npos);
}

// ── P2: idle (AC-36, AC-53) ─────────────────────────────────────────────────

TEST(lib6_ac53_ti_idle_rule_values) {
    cu59::TIPresetRules rules;
    for (const char* m : {"TI-59", "TI-58", "TI-58C"}) {
        const calcu::presets::IdleRule r = rules.forModel(calcu::machine::ModelId{m}).idle();
        CHECK(r.readyNow != nullptr);
        CHECK(r.stretch.count() == 0.1);
        const auto rate = tiCatalogue().model(calcu::machine::ModelId{m})->tickRate;
        CHECK_EQ(r.sample, static_cast<calcu::machine::Ticks>(rate / 1000));
    }
}

TEST(lib6_ac36_running_program_not_idle_each_model_with_and_without_printer) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    // A loop of about two seconds (999 Dsz): the next item starts only after it.
    const std::string program = "program:\n  keycodes: |\n    76 11 09 09 09 42 00 97 00 00 07 25 91\n";
    for (const char* m : {"TI-59", "TI-58", "TI-58C"})
        for (bool printer : {false, true}) {
            Calcu c;
            const std::string text = head(m) + (printer ? "printer: PC-100C\n" : "") + program +
                                     "keys:\n  - key: A\n  - elapsed-time: report\n";
            const std::string reply = c.preset(text, "p.ti59");
            CHECK_EQ(reply, std::string("done\n"));
            // The loop ran to its end (CLR) before the next item: the report counts it.
            CHECK(c("elapsed-time report").find("Elapsed: 0:0") == std::string::npos);   // longer than 10 s
            CHECK_EQ(c("expect 0"), std::string("done\n"));
        }
}

TEST(lib6_ac36_idle_after_100ms_of_key_readiness) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "keys:\n  - elapsed-time: start\n  - key: 5\n  - elapsed-time: report\n"), std::string("done\n"));
    const std::string t = c("elapsed-time report");
    // A press of 100 ms and the readiness without a break for 100 ms: at least 0.2 s.
    CHECK(t.find("0:00.") != std::string::npos);
    const auto at = t.find("0:00.");
    if (at != std::string::npos) CHECK(std::stoi(t.substr(at + 5, 3)) >= 200);
}

TEST(lib6_ac36_card_wait_after_write_is_idle) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    // `1 2nd Write` makes the TI-59 wait for a card: the item finishes (idle), the next one runs.
    CHECK_EQ(c.preset(head() + "keys:\n  - key: 1 Write\n  - elapsed-time: report\n"), std::string("done\n"));
    cu59::TIPresetRules rules;
    CHECK(rules.forModel(calcu::machine::ModelId{"TI-59"}).idle().readyNow(*c.machine()));
    CHECK(!c.machine()->isKeyReady());
}

// ── P3: outdated, all or nothing (AC-38) ────────────────────────────────────

TEST(lib6_ac38_2_3_0_file_refused_as_outdated_with_hint) {
    PresetRead r;
    // An example as 2.3.0 shipped it (kept as a fixture since M-55 upgraded the example).
    const std::string old = fileText("tests/fixtures/v2.3.0/globe-print.ti59");
    const std::string e = r.error(old, "globe-print.ti59");
    CHECK(e.find("Calc-U-59") != std::string::npos);
    CHECK(e.find("the preset format in the User Guide, chapter Presets") != std::string::npos);
    const std::string tiny = r.error("MODEL: TI-59\nPROGRAM:\n000 76 LBL\n");
    CHECK(tiny.find("the preset format in the User Guide") != std::string::npos);
}

TEST(lib6_ac38_any_error_refuses_whole_machine_untouched) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    c("ti59 registers 00=42");
    const std::string reply = c.preset(head("TI-58") + "data:\n  00: 1\n  70: 2\n", "t.ti58");
    CHECK(reply.rfind("refused: ", 0) == 0);
    CHECK(reply.find("line 5") != std::string::npos);   // the first error only, with its line (row 59-42)
    CHECK_EQ(c.machine()->model().value, std::string("TI-59"));   // nothing built
    CHECK_EQ(c("ti59 registers 00"), std::string("done 1\n00: 4.2e1\n"));
}

// ── P4: speed, input (AC-39) ────────────────────────────────────────────────

TEST(lib6_ac39_full_speed_unless_regular_speed_first) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    // A loop of 168 s of emulated time (999 Dsz). Paced on the rig's wall
    // clock: at full speed the preset is over long before 168 s of wall time;
    // with `- regular-speed:` first it takes them (LIB-6 AC-16; row 59-5).
    const std::string loop = "program:\n  keycodes: |\n    76 11 09 09 09 42 00 97 00 00 07 91\n";
    auto wallFor = [&](const std::string& keys) {
        testroms::PartRig r;
        const auto start = r.now;
        const auto id = r.post(calcu::presets::kLoadPresetCommand,
                               calcu::sessions::Value::object({{"content", head() + loop + keys}, {"name", "f.ti59"}}));
        r.until([&] { return r.reply(id) != nullptr; }, std::chrono::seconds(400));
        CHECK(r.reply(id) && r.reply(id)->done);
        CHECK(r.session().context().commandedSpeed() == calcu::pacing::Speed::Regular);   // ends at regular speed
        return std::chrono::duration<double>(r.now - start).count();
    };
    const double full = wallFor("keys:\n  - key: A\n  - key: 1\n");
    const double regular = wallFor("keys:\n  - regular-speed:\n  - key: A\n  - key: 1\n");
    CHECK(full < 100);
    CHECK(regular > 160);
}

TEST(lib6_ac39_user_keys_dropped_second_load_refused) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    const std::string loop = "program:\n  keycodes: |\n    76 11 09 09 42 00 97 00 00 07 91\n";
    const auto id = r.post(calcu::presets::kLoadPresetCommand,
                           calcu::sessions::Value::object({{"content", head() + loop + "keys:\n  - regular-speed:\n  - key: A\n  - key: 1\n"}, {"name", "f.ti59"}}));
    r.runFor(std::chrono::milliseconds(300));
    CHECK(r.busy());
    const auto second = r.send(calcu::presets::kLoadPresetCommand, calcu::sessions::Value::object({{"content", head()}, {"name", "x.ti59"}}));
    CHECK(!second.done);
    CHECK_EQ(second.reason, std::string(calcu::sessions::kAnotherOperationRuns));
    r.tap(82);   // a user key while the preset runs: dropped (row 59-5)
    r.until([&] { return r.reply(id) != nullptr; }, std::chrono::seconds(60));
    r.runFor(std::chrono::milliseconds(300));
    CHECK(r.reply(id) && r.reply(id)->done);
}

// ── P5: the TI steps (AC-40, AC-68, AC-24, AC-25, AC-66, EC-7) ──────────────

TEST(lib6_ac40_type_and_syncclock_refused_when_read) {
    PresetRead r;
    CHECK(!r.error(head() + "keys:\n  - type: 12\n").empty());
    CHECK(!r.error(head() + "keys:\n  - syncclock:\n").empty());
    cu59::TIPresetRules rules;
    CHECK(rules.forModel(calcu::machine::ModelId{"TI-59"}).applies("type") == calcu::presets::Applies::NotForThisModel);
    CHECK(rules.forModel(calcu::machine::ModelId{"TI-58"}).applies("card") == calcu::presets::Applies::NotForThisModel);
    CHECK(!r.error(head() + "bus-rom: x\n").empty());   // a Sharp key: unknown
}

TEST(lib6_ac40_printer_trace_sets_trace_refused_without_pc100c) {
    PresetRead r;
    CHECK(r.error(head() + "keys:\n  - printer-trace: on\n").find("printer: PC-100C") != std::string::npos);
    CHECK(!r.error(head() + "printer: PC-100C\nkeys:\n  - printer-trace: maybe\n").empty());
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "printer: PC-100C\nkeys:\n  - printer-trace: on\n"), std::string("done\n"));
    auto* controls = calcu::machine::capabilityAs<calcu::peripherals::Controls>(*c.machine(), calcu::machine::CapabilityId{"printer/controls"});
    CHECK(controls != nullptr);
    if (controls)
        for (const auto& i : controls->list())
            if (i.id.value == "TRACE") CHECK(i.on);
}

TEST(lib6_ac40_elapsed_counts_emulated_time) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "keys:\n  - elapsed-time: start\n  - wait: 2s\n  - elapsed-time: report\n"), std::string("done\n"));
    const std::string t = c("elapsed-time report");
    CHECK(t.find("0:02.") != std::string::npos);
}

TEST(lib6_ac40_screenshot_is_the_classic_segment_picture) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "keys:\n  - key: 1 2 3 4 5 6 7 8 9 . 5 +/-\n  - screenshot: shot.png\n"), std::string("done\n"));
    const fs::path shot = c.folder.path() / "shot.png";
    CHECK(fs::exists(shot));
    const std::string got = fileText(shot);
    CHECK(got.rfind("\x89PNG", 0) == 0);
    // The reference picture (D-39), made by this code in M-54 and reviewed by
    // the user at the E/F boundary: CALCU59_DISPLAY_PICTURE=1 writes it.
    const fs::path ref = "tests/reference/display/ti59-123456789.5-.png";
    if (std::getenv("CALCU59_DISPLAY_PICTURE")) {
        fs::create_directories(ref.parent_path());
        std::ofstream(ref, std::ios::binary) << got;
    }
    CHECK(fs::exists(ref));
    CHECK(got == fileText(ref));
    // Deterministic: the same output, the same bytes.
    CHECK(cu59::displayPicture(c.machine()->readDisplay()) == std::vector<std::uint8_t>(got.begin(), got.end()));
}

TEST(lib6_ac40_expect_pi_13_digits_passes_10_digits_fails) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "keys:\n  - key: π\n  - expect: 3.14159265359\n  - not-expect: 3.141592654\n"),
             std::string("done\n"));
    const std::string bad = c.preset(head() + "keys:\n  - key: π\n  - expect: 3.141592654\n", "b.ti59");
    CHECK(bad.rfind("refused: ", 0) == 0 || bad.find("3.141592654") != std::string::npos);
    CHECK_EQ(c("expect 3.14159265359"), std::string("done\n"));
    CHECK(c("expect 3.141592654").rfind("refused: ", 0) == 0);
}

TEST(lib6_ac40_expect_any_display_format_and_blinking) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    // Fix 2 shows 3.14; X is still π in 13 digits.
    CHECK_EQ(c.preset(head() + "keys:\n  - key: Fix 2 π\n  - expect: 3.14159265359\n"), std::string("done\n"));
    CHECK(c("lcd").find("3.14") != std::string::npos);
    // A number being keyed in: what the display shows.
    c("key CLR 1 2 . 5");
    c("wait");
    CHECK_EQ(c("expect 12.5"), std::string("done\n"));
    // An error blinks (1 ÷ 0): X is still compared.
    c("key Fix 9 CLR 1 ÷ 0 =");
    c("wait 300ms");
    CHECK_EQ(c("expect 9.999999999999e99"), std::string("done\n"));   // x ÷ 0 clamps; the display shows 9.9999999 99
    CHECK(c("expect 9.9999999e99").rfind("refused", 0) == 0);   // not the display's 8 digits
}

TEST(lib6_ac40_not_expect_by_the_same_comparison) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    c("key 2 x^2");
    c("wait");
    CHECK_EQ(c("not-expect 5"), std::string("done\n"));
    CHECK(c("not-expect 4").rfind("refused: ", 0) == 0);
    CHECK(c("not-expect 4.0").rfind("refused: ", 0) == 0);
}

TEST(lib6_ac40_expect_not_a_number_refused_when_read) {
    PresetRead r;
    CHECK(r.error(head() + "keys:\n  - expect: Error\n").find("A TI model compares expect: with a number.") != std::string::npos);
    CHECK(r.error(head() + "keys:\n  - not-expect: 1,5\n").find("line 4") != std::string::npos);
    CHECK_EQ(r.error(head() + "keys:\n  - expect: -1.5e-3\n"), std::string());
}

TEST(lib6_ac68_regex_refused_when_read) {
    PresetRead r;
    CHECK(r.error(head() + "keys:\n  - expect: /3\\.14/\n").find("a regular expression is not taken") != std::string::npos);
}

TEST(lib6_ac25_screenshot_into_save_folder) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    calcu::testing::TempFolder folder{"calcu59-shots"};
    testroms::RigOptions o;
    o.saveFolder = folder.path().string();
    testroms::SessionRig s(o);
    const auto reply = [&] {
        const auto id = s.post(calcu::presets::kLoadPresetCommand,
                               calcu::sessions::Value::object({{"content", head() + "keys:\n  - key: 7\n  - screenshot: seven.png\n"}, {"name", "s.ti59"}}));
        s.until([&] { return s.reply(id) != nullptr; });
        return s.reply(id) ? *s.reply(id) : calcu::sessions::ReplyMessage{};
    }();
    CHECK(reply.done);
    CHECK(fs::exists(folder.path() / "seven.png"));
    PresetRead read;
    CHECK(!read.error(head() + "keys:\n  - screenshot: a/b.png\n").empty());   // no path separator
}

TEST(lib6_ec7_expect_display_off_fails) {
    cu59::TIPresetRules rules;
    calcu::machine::DisplayOutput dark;   // nothing lit
    dark.on = true;
    calcu::machine::SegmentFrame f;
    f.positions.resize(12);
    for (auto& p : f.positions) {
        p.lit.assign(8, false);
        p.brightness = 0.0f;
    }
    dark.frame = f;
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    const auto text = std::get<calcu::presets::ExpectText>(calcu::presets::parseExpectText("0"));
    const auto reading = rules.forModel(calcu::machine::ModelId{"TI-59"}).compare(*c.machine(), dark, text);
    CHECK(reading.displayOff);
    CHECK(!reading.found);
}

// ── P6: paper at a load (AC-41) ─────────────────────────────────────────────

TEST(lib6_ac41_user_load_empties_paper_startup_preset_keeps_it) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    testroms::PartRig r;
    CHECK(r.load(head() + "printer: PC-100C\nkeys:\n  - key: 5 Prt\n").done);
    r.runFor(std::chrono::seconds(2));
    CHECK(!r.printedText().empty());
    CHECK(r.load(head() + "printer: PC-100C\nkeys:\n  - key: 7\n").done);   // a user's load: the paper is emptied (LIB-8 AC-8)
    r.runFor(std::chrono::milliseconds(200));
    CHECK(r.printedText().empty());
}

// ── P10: typing in a preset (LIB-5 AC-17, AC-26) ────────────────────────────

TEST(lib5_ac17_in_a_preset_key_finishes_at_idle) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    // A loop of seconds (99 Dsz): `- key: 5 A` finishes after its run; the next item sees its result.
    const std::string text = head() + "program:\n  keycodes: |\n    76 11 42 00 97 00 00 04 43 00 91\n"
                                      "keys:\n  - key: 9 9 A\n  - expect: 0\n";
    CHECK_EQ(c.preset(text), std::string("done\n"));
}

TEST(lib5_ac26_names_of_8_1_in_a_preset) {
    PresetRead r;
    // The key IDs and their other spellings, and every printer name of a
    // keycode (preset.md §8.1, §8.1.1), each in a `key:` step.
    std::vector<std::string> names;
    for (const auto& k : tiCatalogue().models().front().keys) {
        names.push_back(k.id.value);
        for (const auto& o : k.otherSpellings) names.push_back(o);
    }
    for (int kc = 0; kc < 100; ++kc) {
        const std::string n = cu59::mnemonic(static_cast<std::uint8_t>(kc));
        if (n == "HIR" || n.rfind('?', 0) == 0 || n.find(' ') != std::string::npos) continue;
        names.push_back(n);
    }
    int n = 0;
    for (const std::string& name : names) {
        const std::string e = r.error(head() + "keys:\n  - key: \"" + name + "\"\n");
        if (!e.empty()) std::fprintf(stderr, "  %s: %s\n", name.c_str(), e.c_str());
        CHECK(e.empty());
        ++n;
    }
    CHECK(n > 100);
    CHECK(!r.error(head() + "keys:\n  - key: HIR\n").empty());
}

TEST(lib5_ac26_2nd_before_second_function_once_in_a_preset) {
    SKIP_UNLESS(tirig::haveRoms(), kSkip);
    Calcu c;
    CHECK_EQ(c.preset(head() + "keys:\n  - key: 2nd π\n  - expect: 3.14159265359\n"), std::string("done\n"));
    CHECK_EQ(c.preset(head() + "keys:\n  - key: CLR 2nd Pgm 0 1 SBR CLR\n  - key: π\n  - expect: 3.14159265359\n", "b.ti59"),
             std::string("done\n"));
}

TEST(lib5_ac26_old_names_refused_when_read) {
    PresetRead r;
    for (const char* old : {"2ndCLR", "P>R", "S+", "mean", "StFlg", "IfFlg"}) {
        const std::string e = r.error(head() + "keys:\n  - key: " + std::string(old) + "\n");
        CHECK(e.find("line 4") != std::string::npos);
    }
    CHECK(!r.error(head() + "keys:\n  - key: 12\n").empty());   // a two-digit number is no key ID
}

