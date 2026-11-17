// LIB-6, the Calc-U-1600 column (design §10.6, P1 to P7, P10): the
// product's rules on the library's loader -- models and keys (AC-44), idle
// (AC-45, AC-53), what follows the build (AC-46), hardware together
// (AC-47), the picture and the text (AC-48, AC-67, AC-25), the log (AC-49),
// today's tests (AC-50), typing in a preset (LIB-5 AC-14, AC-28, AC-41).
// Read checks run SharpPresetRules on presets::readPreset; loads run the
// product's harness entry or its app session. ROM-gated cases SKIP.
#include "../app/SharpDisplayLines.hpp"
#include "../app/SharpPresetRules.hpp"
#include "../app/tests/SessionFixture.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "HarnessPreset.hpp"
#include "PresetReadRig.hpp"
#include "TestHarness.hpp"
#include "Loaders.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "peripherals/RealTimeClock.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using presettest::HarnessPreset;
using presettest::PresetReadRig;

namespace {

bool roms() { return fullRomSetPresent(); }

bool has(const std::string& text, const std::string& part) {
    const bool found = text.find(part) != std::string::npos;
    if (!found) std::fprintf(stderr, "  \"%s\" not in \"%s\"\n", part.c_str(), text.c_str());
    return found;
}

const std::string k1600 = "format-version: 2\nmodel: PC-1600\n";
const std::string k1500a = "format-version: 2\nmodel: PC-1500A\n";

bool done(const std::string& reply) {
    const bool ok = reply.rfind("done", 0) == 0;
    if (!ok) std::fprintf(stderr, "  %s", reply.c_str());
    return ok;
}

std::vector<std::uint8_t> fileBytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), {});
}

} // namespace

// ── P1: models, places, keys (AC-44) ────────────────────────────────────

TEST(lib6_ac44_extensions_models_variants_any_case) {
    PresetReadRig r;
    for (const char* model : {"PC-1500", "pc-1500:a01", "PC-1500:A03", "PC-1500:A04", "pc-1500a", "PC-1600", "PC-1600:old",
                              "pc-1600:NEW"})
        CHECK(r.ok(std::string("format-version: 2\nmodel: ") + model + "\n"));
    CHECK(has(r.error("format-version: 2\nmodel: PC-1500:A05\n"), "has no variant A05"));
    CHECK(has(r.error("format-version: 2\nmodel: PC-1350\n"), "There is no model PC-1350"));
    // The suffix does not choose the model (preset.md §1).
    CHECK(r.ok(k1600, "t.pc1500"));
}

TEST(lib6_ac44_place_keys_per_model) {
    PresetReadRig r;
    fs::create_directories(r.folder.path() / "shared");
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"})
        CHECK(r.ok(std::string("format-version: 2\nmodel: ") + model +
                   "\nprinter: CE-150\nslot-1: { name: CE-155 }\n" + (std::string(model) == "PC-1600" ? "" : "interface: CE-158\n")));
    CHECK(r.ok(k1600 + "printer: CE-1600P:old\nslot-2: { name: CE-1601M }\nhost-drive: { folder: shared }\n"));
    CHECK(r.ok(k1600 + "interface: CE-158\n"));
}

TEST(lib6_ac44_plotter_key_unknown) {
    PresetReadRig r;
    CHECK(has(r.error(k1600 + "plotter: CE-150\n"), "Unknown key plotter."));
}

TEST(lib6_ac44_floppy_and_tape_not_header_keys) {
    PresetReadRig r;
    CHECK(has(r.error(k1600 + "printer: CE-1600P\nfloppy: Formatted\n"), "Unknown key floppy."));
    CHECK(has(r.error(k1600 + "cassette: { file: t.cassette.yaml }\n"), "Unknown key cassette."));
}

TEST(lib6_ac44_ctrl_kbii_bs_refused_in_key) {
    PresetReadRig r;
    for (const char* k : {"ctrl", "kbii", "bs"}) CHECK(!r.error(k1600 + "keys:\n  - key: " + k + "\n").empty());
    CHECK(r.ok(k1600 + "keys:\n  - key: shift up up\n"));
}

// ── P2: idle (AC-45, AC-53) ─────────────────────────────────────────────

TEST(lib6_ac45_idle_at_prompt_input_and_reserve) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // Each item is waited for with timeout: 3: idle at the prompt, at
    // INPUT, in the RESERVE mode.
    for (const std::string& model : {std::string("PC-1600"), std::string("PC-1500A")}) {
        const std::string head = "format-version: 2\nmodel: " + model + "\ntimeout: 3\nkeys:\n";
        const std::string name = model == "PC-1600" ? "t.pc1600" : "t.pc1500a";
        CHECK(done(h.load(head + "  - key: cl\n  - key: cl\n", name)));
        CHECK(done(h.load(head + "  - key: cl\n  - type: INPUT A\n  - key: cl\n", name)));
        CHECK(done(h.load(head + "  - key: cl\n  - key: " + (model == "PC-1600" ? "rsv" : "rcl") + "\n  - key: cl\n", name)));
    }
}

TEST(lib6_ac45_running_program_not_idle) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // PC-1500A: a BASIC program that runs is not idle.
    CHECK(has(h.load(k1500a + "timeout: 2\nkeys:\n  - key: cl\n  - type: NEW0\nprogram:\n  text: |\n    10 GOTO 10\n"
                              "keys:\n  - key: mode\n  - type: RUN\n  - key: cl\n",
                     "t.pc1500a"),
              "has not finished after 2 s"));
    // PC-1600: machine code that runs (DI; JR $) is not idle. (A BASIC
    // program reads the key matrix for BREAK, which C1600-1 AC-21 counts
    // as key readiness: M-46 finding.)
    std::string code(16, '\0');
    code[0] = char(0xFF);
    code[1] = char(0x10);
    code[4] = char(0x10);
    code[5] = 3;
    code[9] = char(0xD3);
    code[12] = char(0xD3);
    code[15] = char(0x0F);
    code += std::string("\xF3\x18\xFE", 3);
    h.write("loop.bin", code);
    CHECK(has(h.load(k1600 + "timeout: 2\nprogram:\n  file: loop.bin\nkeys:\n  - key: cl\n"), "has not finished after 2 s"));
}

TEST(lib6_ac53_sharp_idle_rule_values) {
    cu1600::SharpPresetRules rules;
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        const calcu::presets::IdleRule r = rules.forModel(calcu::machine::ModelId{model}).idle();
        const auto rate = sharp::sharpCatalogue().model(calcu::machine::ModelId{model})->tickRate;
        // Machine::isKeyReady(); on the PC-1600 also the CE-1600F's motor off (M-47).
        CHECK(static_cast<bool>(r.readyNow) == (std::string(model) == "PC-1600"));
        CHECK(r.stretch.count() > 0.333 && r.stretch.count() < 0.334);   // 20 frames
        CHECK_EQ(r.sample, rate / 60);                                    // asked every frame
    }
}

// ── P3: after the build (AC-46) ─────────────────────────────────────────

TEST(lib6_ac46_cl_at_new0_check_at_most_five_pc1500a) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // A fresh RAM with the CE-150 plugged in asks NEW0? :CHECK; the load
    // answers it before the first item.
    CHECK(h.loadIdle(k1500a + "printer: CE-150\n", "t.pc1500a"));
    CHECK(h.line("display-text").find("NEW0") == std::string::npos);
    // The bare model asks too (a cold boot), and is answered as well.
    CHECK(h.loadIdle(k1500a, "t.pc1500a"));
    CHECK(h.line("display-text").find("NEW0") == std::string::npos);
}

TEST(lib6_ac46_pc1600_presses_nothing) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(h.loadIdle(k1600));
    const std::string loaded = h.line("display-text");
    CHECK(done(h.line("cold-boot")));
    CHECK_EQ(h.line("display-text"), loaded);   // as a cold boot leaves it: no key pressed after the build
}

// ── P4: hardware together (AC-47) ───────────────────────────────────────

TEST(lib6_ac47_ce1600p_on_pc1500_refused) {
    PresetReadRig r;
    CHECK(has(r.error("format-version: 2\nmodel: PC-1500\nprinter: CE-1600P\n"), "The CE-1600P does not fit printer of the PC-1500."));
}

TEST(lib6_ac47_ce158_with_ce1600p_names_both) {
    PresetReadRig r;
    const std::string e = r.error(k1600 + "printer: CE-1600P\n\ninterface: CE-158\n");
    CHECK(has(e, ", line 5: The CE-1600P and the CE-158 cannot be used together."));
}

// The `floppy:` clause (M-47, LIB-7 1600): [1600: PC-1600 with CE-1600P];
// `cassette:` [with CE-150 or CE-1600P].
TEST(lib6_ac47_floppy_without_ce1600p_refused) {
    HarnessPreset h;
    CHECK(h.ok());
    std::string r = h.load(k1600 + "keys:\n  - floppy: { name: Formatted }\n");
    CHECK(has(r, "floppy: the PC-1600 has no place for a floppy disk without the CE-1600P."));
    r = h.load("format-version: 2\nmodel: PC-1500\nkeys:\n  - floppy: { name: Formatted }\n", "t.pc1500");
    CHECK(has(r, "floppy is not available on the PC-1500."));
    r = h.load(k1600 + "keys:\n  - cassette: { name: Empty }\n");
    CHECK(has(r, "cassette: the PC-1600 has no place for a cassette without the CE-150 or CE-1600P."));
    CHECK(has(h.line("check-preset " + h.write("ok.pc1600", k1600 + "printer: CE-1600P\nkeys:\n  - floppy: { name: Formatted }\n  - cassette: { name: Empty }\n").generic_string()),
              "ok "));
}

TEST(lib6_ac47_slot2_on_pc1500_not_available) {
    PresetReadRig r;
    CHECK(has(r.error("format-version: 2\nmodel: PC-1500\nslot-2: { name: CE-155 }\n"), "slot-2 is not available on the PC-1500."));
}

// ── P5: picture and text (AC-48, AC-67, AC-25) ──────────────────────────

TEST(lib6_ac48_screenshot_is_copy_screen_picture) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    for (const bool pc1600 : {false, true}) {
        HarnessPreset h;
        const std::string head = pc1600 ? k1600 : k1500a + "keys:\n  - key: cl\n";
        CHECK(h.loadIdle(head + (pc1600 ? "keys:\n" : "") + "  - type: PRINT 12345\n  - screenshot: shot.png\n",
                         pc1600 ? "t.pc1600" : "t.pc1500a"));
        calcu::machine::Machine* m = h.machine();
        CHECK(m != nullptr);
        if (!m) continue;
        // Copy Screen's picture: the dot matrix at the real display's size,
        // the indicators left out (SharpAppPart::screenImage, the same render).
        const std::vector<std::uint8_t> want = cu1600::screenPng(m->readDisplay(), pc1600);
        CHECK(fileBytes(h.folder.path() / "shot.png") == want);
    }
}

TEST(lib6_ac48_expect_one_line_case_and_spaces) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    const std::string head = k1600 + "keys:\n  - type: PRINT \"Ab  C\"\n";
    CHECK(done(h.load(head + "  - expect: Ab  C\n")));
    CHECK(has(h.load(head + "  - expect: ab  c\n"), "refused: "));
    CHECK(has(h.load(head + "  - expect: Ab C\n"), "refused: "));
    CHECK(done(h.load(head + "  - not-expect: Ab C\n")));
}

TEST(lib6_ac48_backslash_and_xhh) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(done(h.load(k1600 + "keys:\n  - type: PRINT CHR$ 92\n  - expect: \\\\\n")));
    CHECK(done(h.load(k1600 + "keys:\n  - type: PRINT \"N\";CHR$ 132\n  - expect: N\\x84\n")));
}

TEST(lib6_ac67_regex_error_1_not_error_10) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(done(h.load(k1600 + "keys:\n  - type: PRINT \"ERROR 10\"\n  - not-expect: /ERROR 1\\b/\n")));
    CHECK(has(h.load(k1600 + "keys:\n  - type: PRINT \"ERROR 1\"\n  - not-expect: /ERROR 1\\b/\n"), "refused: "));
}

TEST(lib6_ac67_bad_regex_refused_at_read) {
    PresetReadRig r;
    CHECK(!r.error(k1600 + "keys:\n  - expect: /[A-/\n").empty());
}

TEST(lib6_ac67_leading_slash_escaped_literal) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    CHECK(done(h.load(k1600 + "keys:\n  - type: PRINT \"/AB/\"\n  - expect: \\/AB/\n")));
}

TEST(lib6_ac25_screenshot_into_save_folder) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    sessionfixture::SessionFixture f;
    CHECK(f.start("PC-1600"));
    const fs::path preset = f.root / "shot.pc1600";
    std::ofstream(preset) << k1600 << "keys:\n  - type: PRINT 1\n  - screenshot: ac25.png\n";
    const calcu::sessions::ReplyMessage r = f.loadPreset(preset.generic_string());
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
    const auto folder = f.paths->places().screenshotFolder();
    CHECK(folder && fs::exists(fs::path(*folder) / "ac25.png"));
}

// ── P6: the log (AC-49) ─────────────────────────────────────────────────

TEST(lib6_ac49_log_lines_on_log_topic) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    sessionfixture::SessionFixture f;
    CHECK(f.start("PC-1500A"));
    (void)f.listener.mirror().takeAppended(calcu::sessions::kLogTopic);
    CHECK(f.loadPreset("examples:startup/default-pc1500a.pc1500a").done);
    std::string all;
    for (const auto& p : f.listener.mirror().takeAppended(calcu::sessions::kLogTopic))
        if (const auto* l = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p)) all += l->text + "\n";
    CHECK(has(all, "Preset examples:startup/default-pc1500a.pc1500a: PC-1500A"));
    CHECK(has(all, "carried out to its end"));
}

// ── P7: today's tests (AC-50) ───────────────────────────────────────────

TEST(lib6_ac50_test_inline_comment_and_raw_values) {
    PresetReadRig r;
    // A comment after a key step goes; a type: step keeps ` # ` as typed;
    // a `#` without a space before it is kept; quoted values.
    const auto plan = r.read(k1600 + "keys:   # the keys\n  - key: cl   # clear\n  - type: PRINT \"A # B\"\n"
                                     "  - type: A=5#3\n  - expect: \"quoted\"\n");
    const auto* p = std::get_if<calcu::presets::LoadPlan>(&plan);
    CHECK(p && p->items.size() == 4);
    if (p && p->items.size() == 4) {
        CHECK_EQ(p->items[0].item->logText(), std::string("key: cl"));
        CHECK(has(p->items[1].item->logText(), "PRINT \"A # B\""));
        CHECK(has(p->items[2].item->logText(), "A=5#3"));
    }
}

TEST(lib6_ac50_test_keys_given_twice_and_names_any_case) {
    PresetReadRig r;
    CHECK(has(r.error(k1600 + "model: PC-1500\n"), "given twice"));
    CHECK(has(r.error(k1600 + "printer: CE-150\nprinter: CE-150\n"), "printer: is given twice"));
    CHECK(r.ok(k1600 + "printer: ce-150\n"));
    CHECK(!r.error(k1600 + "printer: CE150\n").empty());   // the hyphen is required
}

TEST(lib6_ac50_test_wait_step_value) {
    PresetReadRig r;
    CHECK(r.ok(k1600 + "keys:\n  - wait: 2s\n  - wait: 0.5s\n  - wait: 500ms\n"));
    CHECK(!r.error(k1600 + "keys:\n  - wait:\n").empty());     // AC-21: the form without a value
    CHECK(!r.error(k1600 + "keys:\n  - wait: 2\n").empty());   // a unit is required
    CHECK(!r.error(k1600 + "keys:\n  - wait: -1s\n").empty());
    CHECK(!r.error(k1600 + "keys:\n  - wait: 2s later\n").empty());
}

TEST(lib6_ac50_test_syncclock_step) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;   // the harness's host clock: 2000-01-01 00:00:00 (LIB-11 AC-21)
    CHECK(h.loadIdle(k1500a + "keys:\n  - wait: 30s\n  - syncclock:\n", "t.pc1500a"));
    calcu::machine::Machine* m = h.machine();
    auto* clock = m ? calcu::machine::first<calcu::peripherals::RealTimeClock>(*m) : nullptr;
    CHECK(clock != nullptr);
    if (clock) {
        const auto t = clock->read();
        CHECK(t.month == 1 && t.day == 1 && t.hour == 0 && t.minute == 0 && t.second < 5);
    }
}

TEST(lib6_ac50_test_preset_screenshot_parse) {
    PresetReadRig r;
    CHECK(r.ok(k1600 + "keys:\n  - screenshot: shot.png\n"));
    CHECK(has(r.error(k1600 + "keys:\n  - screenshot: sub/shot.png\n"), "without a folder"));
}

TEST(lib6_ac50_in_parts_against_in_one_go) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    // The same preset carried out in one go (the harness) and in parts, one
    // host call at a time on a scripted clock (the app session): the same
    // display and the same memory.
    const std::string preset = "tests/fixtures/presets/maxed-out-mem.pc1600";
    HarnessPreset h;
    CHECK(done(h.line("preset " + preset)));
    CHECK(done(h.line("wait")));
    sessionfixture::SessionFixture f;
    f.readCost = std::chrono::milliseconds(1);
    // The harness's clock: 2000-01-01 00:00:00 (FILES shows the time).
    std::tm t{};
    t.tm_year = 100;
    t.tm_mday = 1;
    t.tm_isdst = -1;
    f.fixedHostTime = true;
    f.fixedTime = std::chrono::system_clock::from_time_t(std::mktime(&t));
    CHECK(f.start("PC-1600"));
    const calcu::sessions::ReplyMessage r = f.loadPreset(preset);
    CHECK(r.done);
    CHECK(f.ok(calcu::presets::kWaitIdleCommand));
    calcu::machine::Machine* a = h.machine();
    calcu::machine::Machine* b = f.part().machine();
    CHECK(a && b);
    if (!a || !b) return;
    std::string sa, sb;
    for (const auto& row : cu1600::displayLines(*a, a->readDisplay()).rows) sa += row + "\n";
    for (const auto& row : cu1600::displayLines(*b, b->readDisplay()).rows) sb += row + "\n";
    CHECK_EQ(sa, sb);
}

// ── P10: typing in a preset (LIB-5 AC-14, AC-28, AC-41) ─────────────────

TEST(lib5_ac14_in_a_preset_type_at_readiness_pc1600_at_once_pc1500) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // The first press of a step: at key readiness on the PC-1600, at once
    // on the PC-1500 (whose presses wait for readiness each); after
    // skip-wait: the first press goes down while the machine still works.
    CHECK(h.loadIdle(k1600 + "keys:\n  - type: PRINT 7*6\n"));
    CHECK(has(h.line("display-text"), "42"));
    CHECK(h.loadIdle(k1500a + "keys:\n  - key: cl\n  - type: PRINT 7*6\n", "t.pc1500a"));
    CHECK(has(h.line("display-text"), "42"));
}

TEST(lib5_ac28_in_a_preset_cadence) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    // At full speed every character arrives, by the profile's cadence.
    CHECK(h.loadIdle(k1600 + "keys:\n  - type: PRINT \"ABCDEFGHIJKLMNOPQRSTUVW\"\n"
                             "  - expect: ABCDEFGHIJKLMNOPQRSTUVW\n"));
    CHECK(h.loadIdle(k1500a + "keys:\n  - key: cl\n  - type: PRINT \"ABCDEFGHIJKLMNOPQRSTUVW\"\n"
                              "  - expect: ABCDEFGHIJKLMNOPQRSTUVW\n",
                     "t.pc1500a"));
}

// LIB-10 AC-40 (the format 2 clause, M-46): the debugger steps are not
// defined on the Sharp models (preset.md §7) and refuse a preset when it is
// read; `debug-panel:` is Calc-U-59's.
TEST(lib10_ac40_debugger_steps_refused_at_read) {
    SKIP_UNLESS(roms(), "no ROMs in roms/");
    HarnessPreset h;
    for (const char* step : {"pause: program", "pause-on-start: cpu", "step: { level: cpu, count: 1 }", "resume:", "debug-panel: cpu"})
        CHECK(has(h.load(k1600 + "keys:\n  - " + step + "\n"), "refused: "));
}

TEST(lib5_ac41_ent_refused_in_a_preset) {
    PresetReadRig r;
    CHECK(!r.error(k1600 + "keys:\n  - key: ent\n").empty());
    CHECK(r.ok(k1600 + "keys:\n  - key: enter\n"));
}
