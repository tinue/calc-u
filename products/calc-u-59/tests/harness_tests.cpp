// `calcu` drives a TI-59 headless, through the library's app session with
// TIAppPart; at the end, the functions of 2.3.0's tools, `display-text` and
// the emulated time through `calcu`.
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "TIPresetRig.hpp"
#include "../app/ProductRoms.hpp"
#include "../core/TISegments.hpp"
#include "roms/Bundle.hpp"
#include <sstream>
#include <vector>
#include <filesystem>
#include <string>

using namespace calcu::harness;

TEST(harness_drives_a_ti59) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("wait 2s").rfind("done 2\nticks: ", 0) == 0);
    CHECK(reply("display-text").rfind("done 2\n          0 \nindicators:", 0) == 0);   // as ti59.py display() shows it
    CHECK_EQ(reply("key-down 7"), std::string("done\n"));
    reply("wait 100ms");
    CHECK_EQ(reply("key-up 7"), std::string("done\n"));
    reply("wait 500ms");
    CHECK_EQ(reply("display-text"), std::string("done 2\n          7 \nindicators:\n"));
    const std::string t = reply("ticks");
    CHECK(t.rfind("done 2\nticks: ", 0) == 0 && t.find("seconds: 2.6") != std::string::npos);
    CHECK_EQ(reply("audio x.wav"), std::string("refused: This machine has no sound.\n"));   // a TI has no sound
    server->finish();
}

// The harness switches the model with the library's `model`.
TEST(harness_switches_to_a_ti58) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model TI-58"), std::string("done\n"));
    CHECK(reply("display-text").rfind("done 2\n          0 \nindicators:", 0) == 0);   // ready for keys
    CHECK(reply("capabilities").find("place printer empty\n") != std::string::npos);   // the built-in default
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "TI-58");
    CHECK_EQ(reply("model ti-59"), std::string("done\n"));
    CHECK_EQ(reply("model TI-99"), std::string("refused: There is no model \"TI-99\".\n"));
    server->finish();
}

// `key` presses the names of docs/formats/preset.md §8.1 as the app types
// them and replies when
// the last key is up; a TI model has no `type`.
TEST(harness_key_on_a_ti59) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("key 2 x^2"), std::string("done\n"));
    reply("wait 300ms");
    CHECK_EQ(reply("display-text"), std::string("done 2\n          4. \nindicators:\n"));   // a result: x² of 2
    CHECK_EQ(reply("key 1 S+"), std::string("refused: Unknown key: S+.\n"));   // an old name; nothing typed
    CHECK_EQ(reply("key CLR 2nd pi"), std::string("done\n"));                  // 2nd before a second function: once
    reply("wait 300ms");
    CHECK(reply("display-text").find("3.14159265") != std::string::npos);
    CHECK_EQ(reply("type 12"), std::string("refused: Unknown command: type.\n"));   // a TI model has no `type`
    server->finish();
}

// The harness's `paper` commands on the PC-100C's paper.
TEST(harness_paper_text_of_the_pc100c) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("attach printer PC-100C"), std::string("done\n"));   // the built-in default has none
    CHECK_EQ(reply("paper text"), std::string("done\n"));   // empty paper
    reply("key 7");
    reply("key 2nd Prt");
    reply("wait 1s");
    CHECK_EQ(reply("paper text"), std::string("done 1\n          7.        \n"));
    CHECK_EQ(reply("paper text PC-100C"), std::string("done 1\n          7.        \n"));
    server->finish();
}

TEST(harness_paper_marks_and_png) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    reply("attach printer PC-100C");
    reply("key 7");
    reply("key 2nd Prt");
    reply("wait 1s");
    const std::string marks = reply("paper marks");
    CHECK(marks.rfind("done ", 0) == 0);
    CHECK(marks.find("\nreach 0 10.18\n") != std::string::npos);
    CHECK(marks.find("\ndot ") != std::string::npos);
    const std::filesystem::path png = std::filesystem::absolute(std::filesystem::path("build") / "lib8-ac23.png");
    std::filesystem::remove(png);
    CHECK_EQ(reply("paper png " + png.string()), std::string("done\n"));
    CHECK(std::filesystem::exists(png));
    CHECK_EQ(reply("cut-paper"), std::string("done\n"));
    CHECK_EQ(reply("paper text"), std::string("done\n"));
    server->finish();
}

// A breakpoint set through the harness pauses a TI.
TEST(harness_breakpoint_pauses_the_ti) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("targets").find("TMC0501") != std::string::npos);
    CHECK(reply("pause").rfind("done", 0) == 0);
    const std::string regs = reply("registers");
    const auto at = regs.find("\npc = ");
    CHECK(at != std::string::npos);
    if (at == std::string::npos) {
        std::fprintf(stderr, "%s\n", regs.c_str());
        return;
    }
    CHECK(reply("resume").rfind("done", 0) == 0);
    reply("wait 100ms");
    const std::string pc = regs.substr(at + 6, regs.find('\n', at + 1) - at - 6);
    CHECK(reply("break TMC0501 " + pc).rfind("done", 0) == 0);
    reply("wait 100ms");
    const std::string state = reply("state run-state");
    CHECK(state.find("breakpoint") != std::string::npos);
    if (state.find("breakpoint") == std::string::npos) std::fprintf(stderr, "%s\n", state.c_str());
    CHECK(reply("history TMC0501 3").rfind("done", 0) == 0);
    server->finish();
}

// The harness starts the built-in default, the TI-59 with the Master Library module and
// no printer.
TEST(harness_starts_with_ml_and_no_printer) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    const std::string caps = c("capabilities");
    CHECK(caps.find("place slot-1 ML\n") != std::string::npos);
    CHECK(caps.find("place printer empty\n") != std::string::npos);
    CHECK_EQ(c.machine()->model().value, std::string("TI-59"));
}

// `c59 partition`, `c59 program`, `c59 data-registers`.
TEST(harness_partition_program_registers_write_and_read) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("c59 partition"), std::string("done 1\n479\n"));
    CHECK_EQ(c("c59 partition 239.29"), std::string("done\n"));
    CHECK_EQ(c("c59 partition"), std::string("done 1\n239\n"));
    CHECK_EQ(c("c59 program 76 11 42"), std::string("done\n"));          // from 000
    CHECK_EQ(c("c59 program 010 85 01"), std::string("done\n"));         // from 010
    CHECK_EQ(c("c59 program 000 count=3"), std::string("done 3\n000 76\n001 11\n002 42\n"));
    CHECK_EQ(c("c59 program 010 count=2"), std::string("done 2\n010 85\n011 01\n"));
    CHECK_EQ(c("c59 data-registers 05=1.5e-3 06=-42"), std::string("done\n"));
    CHECK_EQ(c("c59 data-registers 05 06"), std::string("done 2\n05: 1.5e-3\n06: -4.2e1\n"));
    CHECK_EQ(c("model TI-58C"), std::string("done\n"));
    CHECK_EQ(c("c59 data-registers H02=7"), std::string("done\n"));
    CHECK_EQ(c("c59 data-registers H02"), std::string("done 1\nH02: 7e0\n"));
}

TEST(harness_memory_commands_refused_where_the_section_refuses) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("c59 partition 960").rfind("refused: partition: 960 is beyond", 0) == 0);
    CHECK(c("c59 partition abc").rfind("refused: ", 0) == 0);
    CHECK(c("c59 program 76 LBL").rfind("refused: ", 0) == 0);
    CHECK(c("c59 program 1").rfind("refused: ", 0) == 0);
    CHECK(c("c59 program 959 76 11").rfind("refused: ", 0) == 0);   // past the last step
    CHECK(c("c59 data-registers H00=1").rfind("refused: ", 0) == 0);     // not a TI-58C
    CHECK(c("c59 data-registers 05=x").rfind("refused: ", 0) == 0);
    CHECK_EQ(c("model TI-58"), std::string("done\n"));
    CHECK(c("c59 data-registers 60=1").rfind("refused: ", 0) == 0);
    CHECK(c("c59 partition 480").rfind("refused: ", 0) == 0);
}

TEST(harness_program_count_reads_keycodes) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("c59 program 010 05"), std::string("done\n"));   // a bare token is a keycode
    CHECK_EQ(c("c59 program 010 count=1"), std::string("done 1\n010 05\n"));
    CHECK(c("c59 program 010 count=0").rfind("refused: ", 0) == 0);
    CHECK(c("c59 program 010 count=1 05").rfind("refused: ", 0) == 0);
}

// A preset, the bare wait, expect and screenshot through `calcu --product c59`.
TEST(harness_preset_wait_expect_screenshot_on_ti59) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("preset examples:sum.ti59"), std::string("done\n"));
    CHECK_EQ(c("key 5 A"), std::string("done\n"));
    CHECK(c("wait").rfind("done", 0) == 0);
    CHECK_EQ(c("expect 15"), std::string("done\n"));
    CHECK_EQ(c("screenshot sum.png"), std::string("done\n"));
    CHECK(std::filesystem::exists(c.folder.path() / "sum.png"));
    CHECK(c("preset examples:nothere.ti59").rfind("refused: ", 0) == 0);
    CHECK(c("check-preset examples/diag.ti59").rfind("done", 0) == 0);
    CHECK(c("preset-list").find("Sum of N numbers") != std::string::npos);
}

TEST(harness_printer_trace_on_off) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("printer-trace on").rfind("refused: ", 0) == 0);   // no PC-100C
    CHECK_EQ(c("attach printer PC-100C"), std::string("done\n"));
    CHECK_EQ(c("printer-trace on"), std::string("done\n"));
    CHECK_EQ(c("printer-trace off"), std::string("done\n"));
}

// ── The debug panel's commands and `open` on Calc-U-59 ───────────────────────

TEST(harness_asm_load_run_clear) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("c59 asm run"), std::string("refused: No ASM overlay loaded.\n"));
    CHECK_EQ(c("c59 asm load examples/assembly/simple_count.asm"), std::string("done 1\n6 instructions\n"));
    const std::string run = c("c59 asm run");
    CHECK(run.rfind("done 1\nASM entered at 0x1800 (", 0) == 0);
    CHECK(c("state c59.asm-overlay").find("simple_count.asm") != std::string::npos);
    CHECK_EQ(c("c59 asm clear"), std::string("done\n"));
    CHECK(c("c59 asm load examples/assembly/README.md").rfind("refused: README.md cannot be read: an ASM overlay starts with", 0) == 0);
    CHECK(c("c59 asm load /no/such.asm").rfind("refused: such.asm could not be read.", 0) == 0);
    CHECK(c("c59 asm").rfind("refused: usage:", 0) == 0);
}

TEST(harness_dump_replies_lines) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("c59 dump scom").rfind("done 17\n── SCOM ──\nS00 ", 0) == 0);
    CHECK(c("c59 dump prog").rfind("done ", 0) == 0);
    CHECK(c("c59 dump vars").rfind("done ", 0) == 0);
    CHECK(c("c59 dump memory").rfind("done ", 0) == 0);
    CHECK(c("c59 dump all").rfind("refused: ", 0) == 0);
}

TEST(harness_log_level) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("c59 log-level debug"), std::string("done\n"));
    CHECK(c("state c59.debug-view").find("debug") != std::string::npos);
    CHECK_EQ(c("c59 log-level off"), std::string("done\n"));
    CHECK(c("c59 log-level loud").rfind("refused: ", 0) == 0);
}

TEST(harness_heatmap_reset) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("c59 heatmap reset"), std::string("done\n"));
    CHECK(c("c59 heatmap").rfind("refused: ", 0) == 0);
    // The CALCULATOR and CPU tabs are read with `state`.
    c("wait 200ms");
    CHECK(c("state c59.calculator-debug").find("partition") != std::string::npos);
    CHECK(c("state c59.cpu-debug").rfind("done", 0) == 0);
}

// They are commands only: a preset has no steps for them.
TEST(harness_debug_commands_have_no_preset_steps) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    calcu::testing::TempFolder dir("calcu59-steps");
    for (const char* step : {"asm: run", "dump: scom", "log-level: debug", "heatmap: reset"}) {
        dir.write("p.ti59", std::string("format-version: 2\nmodel: TI-59\nkeys:\n  - ") + step + "\n");
        const std::string r = c("preset " + (dir.path() / "p.ti59").string());
        CHECK(r.rfind("refused: ", 0) == 0);
    }
}

// `open <path>` takes every Calc-U-59 file.
TEST(harness_open_takes_every_calc_u_59_file) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    calcu::testing::TempFolder dir("calcu59-open");
    dir.write("p.ti59", "format-version: 2\nmodel: TI-59\n");
    dir.write("two.keystrokes.yaml",
              "format: ti-keystroke-program\nformat-version: 1\nprogram-name: Two\nkeycodes: |\n  000 76 LBL\n  001 11 A\n");
    dir.write("ML.rom.yaml", "x\n");
    CHECK(c("open " + (dir.path() / "p.ti59").string()).find("preset") != std::string::npos);
    CHECK(c("open examples/assembly/DPT.asm").find("asm-overlay") != std::string::npos);
    CHECK(c("open " + (dir.path() / "two.keystrokes.yaml").string()).find("keystroke-program") != std::string::npos);
    CHECK_EQ(c("c59 program 000 count=2"), std::string("done 2\n000 76\n001 11\n"));
    CHECK(c("open " + (dir.path() / "ML.rom.yaml").string()).rfind("refused: ML.rom.yaml was not opened: ", 0) == 0);
}

// ── The harness entry of Calc-U-59 ──

namespace {

std::vector<std::string> replyLines(const std::string& reply) {
    std::vector<std::string> out;
    std::istringstream in(reply);
    std::string l;
    std::getline(in, l);
    while (std::getline(in, l)) out.push_back(l);
    return out;
}

bool done(const std::string& reply) { return reply.rfind("done", 0) == 0; }

// The instructions the profile counted (every address's count).
std::uint64_t profiled(tirig::Calcu& c) {
    std::uint64_t n = 0;
    for (const auto& l : replyLines(c("profile report 100000"))) {
        std::istringstream in(l);
        std::string target, address;
        std::uint64_t count = 0;
        if (in >> target >> address >> count && target == "TMC0501") n += count;
    }
    return n;
}

std::uint64_t ticksOf(tirig::Calcu& c) {
    const auto l = replyLines(c("ticks"));
    return l.empty() ? 0 : std::stoull(l[0].substr(7));
}

} // namespace

// The entry runs the app layer, TIAppPart, as the frontends host it.
TEST(harness_entry_runs_ti_app_part) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c);
    if (!c) return;
    // TIAppPart's topic, the library's media and control state, a c59 command.
    CHECK(done(c("state c59.cue-card")));
    CHECK(c("state media").find("\"place\":\"card-reader\"") != std::string::npos);
    CHECK(c("state control-state").find("\"model\":\"TI-59\"") != std::string::npos);
    CHECK_EQ(c("c59 partition"), std::string("done 1\n479\n"));
    // The model switch is the library's on TIAppPart's machines; the part follows.
    CHECK_EQ(c("model TI-58C"), std::string("done\n"));
    CHECK(c("state control-state").find("\"model\":\"TI-58C\"") != std::string::npos);
    CHECK(c("state c59.calc-indicator").find("refused") == std::string::npos);
    // No c59.led (now c59.calc-indicator), no c59.printer (the control state's `controls`).
    CHECK(c("state c59.led").find("refused") != std::string::npos);
    CHECK(c("state c59.printer").find("refused") != std::string::npos);
    CHECK_EQ(c("attach printer PC-100C"), std::string("done\n"));
    CHECK(c("state control-state").find("\"controls\":[{\"place\":\"printer\",\"peripheral\":\"PC-100C\"") !=
          std::string::npos);
    CHECK_EQ(c("detach printer"), std::string("done\n"));
    // No part is in the session twice: one reply per command.
    CHECK_EQ(c("attach printer PC-100C"), std::string("done\n"));
    CHECK_EQ(c("detach printer"), std::string("done\n"));
}

// `display-text` reads each position by the segment table.
TEST(display_text_every_segment_pattern_gives_its_character) {
    auto position = [](std::uint8_t bits, bool point, float brightness) {
        calcu::machine::SegmentPosition p;
        p.lit.assign(8, false);
        for (int s = 0; s < 7; ++s) p.lit[static_cast<std::size_t>(s)] = (bits >> s) & 1;
        p.lit[kTIDecimalPoint] = point;
        p.brightness = brightness;
        return p;
    };
    for (bool point : {false, true}) {
        const std::string dot = point ? "." : "";
        for (int d = 0; d < 10; ++d)
            CHECK_EQ(cu59::tiPositionText(position(kDigitSegments[d], point, 1.0f)), std::string(1, char('0' + d)) + dot);
        CHECK_EQ(cu59::tiPositionText(position(kSegmentsMinus, point, 1.0f)), "-" + dot);
        CHECK_EQ(cu59::tiPositionText(position(kSegmentsDegree, point, 1.0f)), "\xC2\xB0" + dot);
        CHECK_EQ(cu59::tiPositionText(position(kSegmentsQuote, point, 1.0f)), "\"" + dot);
        CHECK_EQ(cu59::tiPositionText(position(kSegmentsApostrophe, point, 1.0f)), "'" + dot);
        CHECK_EQ(cu59::tiPositionText(position(0, point, 0.0f)), " " + dot);          // not driven: a blank
        CHECK_EQ(cu59::tiPositionText(position(0b0110110, point, 1.0f)), " " + dot);  // no glyph of the table: a blank
    }
}

TEST(display_text_lcd_twelve_positions_with_points) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    if (!c) return;
    c("key 1 . 5 +/-");
    c("wait 300ms");
    CHECK_EQ(c("display-text"), std::string("done 2\n        -1.5 \nindicators:\n"));
    c("key CLR 1 2 3 4 5 6 7 8 9 0");
    c("wait 300ms");
    CHECK_EQ(c("display-text"), std::string("done 2\n 1234567890 \nindicators:\n"));
}

TEST(display_text_indicator_c_line_and_brightness_in_json) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (bool json : {false, true}) {
        calcu::harness::RunOptions o;
        o.json = json;
        tirig::Calcu c(o);
        if (!c) return;
        c("c59 program 000 76 11 42 00 76 12 97 00 12 91");
        c("key 3 A");
        c("wait 2s");
        if (json)
            CHECK_EQ(c("display-text"), std::string("{\"done\":true,\"result\":{\"on\":true,\"rows\":[\"          3. \"],"
                                           "\"indicators\":[{\"name\":\"C\",\"brightness\":0.021}],\"notText\":null},\"reason\":null}\n"));
        else
            CHECK_EQ(c("display-text"), std::string("done 2\n          3. \nindicators: C\n"));
    }
}

// An instruction in the ROM's idle state weighs four on the
// TI-59 and TI-58, one on the TI-58C; ticks ÷ 32 are the instructions as 2.3.0's tools counted them.
TEST(idle_instruction_weighs_four_on_ti59_and_ti58) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (const char* model : {"TI-59", "TI-58"}) {
        tirig::Calcu c;
        if (!c) return;
        c(std::string("model ") + model);
        c("wait 100ms");
        c("profile on");
        const std::uint64_t t0 = ticksOf(c);
        c("wait 1s");
        const std::uint64_t ticks = ticksOf(c) - t0;
        const std::uint64_t n = profiled(c);
        CHECK(n > 0);
        // At key readiness every instruction is in the idle state: 4 × 32 ticks.
        CHECK(n * 128 <= ticks + 128 && ticks <= n * 128 + 128);
    }
}

TEST(ti58c_instruction_weighs_one) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    if (!c) return;
    c("model TI-58C");
    c("wait 100ms");
    c("profile on");
    const std::uint64_t t0 = ticksOf(c);
    c("wait 1s");
    const std::uint64_t ticks = ticksOf(c) - t0;
    const std::uint64_t n = profiled(c);
    CHECK(n > 0);
    CHECK(n * 32 <= ticks + 32 && ticks <= n * 32 + 32);
}

// Every function of Calc-U-59 2.3.0's tools (one block per function)
// through `calcu`.
TEST(harness_every_function_of_the_old_tools) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("calcu59-ac14");
    calcu::harness::RunOptions o;
    o.saveFolder = (dir.path() / "save").string();
    std::filesystem::create_directories(dir.path() / "save");
    tirig::Calcu c(o);
    CHECK(c);
    if (!c) return;
    // headless/: run without a window, the ROMs through the lookup (one-shot and serve).
    CHECK(c("wait 1s").rfind("done 2\nticks: ", 0) == 0);
    {
        tirig::Calcu pipe;   // serve ends its run at the end of the input
        std::vector<std::string> in{"ticks", "display-text"}, out;
        std::size_t i = 0;
        pipe.server->serve([&]() -> std::optional<std::string> { return i < in.size() ? std::optional<std::string>(in[i++]) : std::nullopt; },
                        [&](const std::string& s) { out.push_back(s); });
        CHECK(out.size() == 2 && out[1].find("indicators:") != std::string::npos);
    }
    // the ROMs missing, named (the start itself: CalcU59RomMissingBuild)
    {
        const auto r = cu59::lookUpRoms(calcu::roms::DirectoryBundle((dir.path() / "none").string()));
        CHECK(!r.set && calcu::roms::describe(r.missing).find("TMC0571B.txt") != std::string::npos);
    }
    // the model; a module; with or without printer; a power cycle, reset, cold boot
    CHECK_EQ(c("model TI-58C"), std::string("done\n"));
    CHECK_EQ(c("model TI-59"), std::string("done\n"));
    CHECK_EQ(c("detach slot-1"), std::string("done\n"));
    CHECK_EQ(c("attach slot-1 ML"), std::string("done\n"));
    CHECK_EQ(c("attach printer PC-100C"), std::string("done\n"));
    CHECK_EQ(c("power-cycle"), std::string("done\n"));
    CHECK_EQ(c("reset"), std::string("done\n"));
    CHECK_EQ(c("cold-boot"), std::string("done\n"));
    // press and release a key by name; type keys by name, second functions included
    CHECK_EQ(c("key-down 7"), std::string("done\n"));
    c("wait 100ms");
    CHECK_EQ(c("key-up 7"), std::string("done\n"));
    CHECK_EQ(c("key CLR 2nd pi"), std::string("done\n"));
    c("wait 300ms");
    CHECK(c("display-text").find("3.14159265") != std::string::npos);
    // the PC-100C's TRACE, PRINT and ADV
    CHECK_EQ(c("set-control PC-100C TRACE on"), std::string("done\n"));
    CHECK_EQ(c("set-control PC-100C TRACE off"), std::string("done\n"));
    CHECK_EQ(c("set-control PC-100C PRINT on"), std::string("done\n"));
    CHECK_EQ(c("set-control PC-100C PRINT off"), std::string("done\n"));
    CHECK_EQ(c("set-control PC-100C ADV on"), std::string("done\n"));
    CHECK_EQ(c("set-control PC-100C ADV off"), std::string("done\n"));
    // let instructions pass; until idle; an exact count on a paused machine
    CHECK(done(c("wait 500ms")));
    CHECK(c("wait").find("hour reached: no") != std::string::npos);
    CHECK(done(c("pause")));
    CHECK(done(c("step { level: cpu, count: 10 }")));
    CHECK(done(c("resume")));
    // the emulated time (ticks ÷ 32 are the instructions as 2.3.0's tools counted them)
    CHECK(c("ticks").find("seconds: ") != std::string::npos);
    // a record per instruction; the power-on trace (read with tools/read_trace.py, CalcU59TraceReader)
    CHECK(done(c("trace k.trace")));
    c("key 5");
    CHECK(done(c("trace off")));
    CHECK(done(c("trace boot.trace")));
    CHECK(done(c("cold-boot")));
    CHECK(done(c("trace off")));
    CHECK(std::filesystem::exists(c.folder.path() / "k.trace") && std::filesystem::exists(c.folder.path() / "boot.trace"));
    // the addresses a key runs, with their counts
    CHECK(done(c("profile on")));
    c("key 5");
    CHECK(c("profile report 3").find("different addresses") != std::string::npos);
    // ROM coverage of a run (coverage.py's command; the script: CalcU59Scripts)
    CHECK(c("c59 coverage").find(" of 6144") != std::string::npos);
    CHECK(done(c("profile off")));
    // the processor's registers and SCOM; disassembly; RAM read and write
    CHECK(c("registers TMC0501").find("pc = ") != std::string::npos);
    CHECK(done(c("peek TMC0501 scom 0 1")));
    CHECK(done(c("disassemble TMC0501 0x0000 2")));
    CHECK(done(c("poke TMC0501 ram 0 0")));
    CHECK(done(c("peek TMC0501 ram 0 1")));
    // write a program; the display as text and as segments
    CHECK_EQ(c("c59 program 000 76 11 01 99 02 99 98 03 99 91"), std::string("done\n"));
    CHECK(c("display-text").find("indicators:") != std::string::npos);
    CHECK(c("display").find("segments:") != std::string::npos);
    // the printer's text and dot lines
    c("key A");
    CHECK(c("c59 line-times").find("printer floor: ") != std::string::npos);
    CHECK(c("paper text").find("3.") != std::string::npos);
    CHECK(done(c("paper marks")));
    // a card: fed, the machine waiting for one, what a pass wrote
    CHECK_EQ(c("c59 program 000 76 11 01 02 03 91"), std::string("done\n"));
    CHECK_EQ(c("key 1 Write"), std::string("done\n"));
    c("wait 100ms");
    CHECK(c("state media").find("\"waits\":true") != std::string::npos);
    CHECK_EQ(c("card { mem: X, side: 0 }"), std::string("done\n"));
    CHECK_EQ(c("saveas { card: X, as: live, name: Saved }"), std::string("done\n"));
    CHECK(std::filesystem::exists(dir.path() / "save" / "Saved.magnetcard.yaml"));
    CHECK(done(c("arm { name: Saved }")));
    // agentic/: a preset, its keys' run time, its printout; line times; the hottest steps
    CHECK_EQ(c("preset examples:sum.ti59"), std::string("done\n"));
    CHECK(c("c59 run-time 5 A").find("total ") != std::string::npos);
    c("c59 program 000 76 11 42 00 76 12 97 00 12 91");
    c("key 2 0 A");
    CHECK(c("c59 hot-steps limit=1s").find("notes: ") != std::string::npos);
    CHECK(done(c("c59 step-trace 100ms 200ms")));
    CHECK_EQ(c("c59 step-cost 43 22"), std::string("refused: A program runs: let it end first (`wait`).\n"));
    c("wait");
    CHECK(c("c59 step-cost 43 22").find("fast ") != std::string::npos);
    // kdis.py: the program instruction by instruction, the registers, the partition
    CHECK(c("c59 keycodes disassemble").find("Lbl A") != std::string::npos || done(c("c59 keycodes disassemble")));
    CHECK(done(c("c59 data-registers 00 01")));
    CHECK(done(c("c59 partition")));
    // a listing to paste into a preset; asm59.py's plain assembly
    CHECK(done(c("c59 keycodes list")));
    dir.write("a.asm59", "000  Lbl A\n002  RTN\n");
    CHECK_EQ(c("c59 keycodes assemble " + (dir.path() / "a.asm59").string()), std::string("done 1\nsteps: 000-002\n"));
    // fastmode_matrix.py, the calendar scripts and read_trace.py stay scripts:
    // CalcU59Scripts, CalcU59CalendarScripts, CalcU59TraceReader.
}
