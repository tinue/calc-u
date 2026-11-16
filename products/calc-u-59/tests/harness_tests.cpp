// The exit check of M-28 (LIB-11 design §6, T14): `calcu` drives a TI-59
// headless, through the library's app session (the product ACs of LIB-11,
// AC-14 to AC-17, are tested in M-59).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "TIPresetRig.hpp"
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
    CHECK(reply("lcd").rfind("done 2\n          0 \nindicators:", 0) == 0);   // as ti59.py display() shows it
    CHECK_EQ(reply("key-down 7"), std::string("done\n"));
    reply("wait 100ms");
    CHECK_EQ(reply("key-up 7"), std::string("done\n"));
    reply("wait 500ms");
    CHECK_EQ(reply("lcd"), std::string("done 2\n          7 \nindicators:\n"));
    const std::string t = reply("ticks");
    CHECK(t.rfind("done 2\nticks: ", 0) == 0 && t.find("seconds: 2.6") != std::string::npos);
    CHECK_EQ(reply("audio x.wav"), std::string("refused: This machine has no sound.\n"));   // EC-6 on a TI
    server->finish();
}

// LIB-18 design P59 (LIB-11 AC-2 through a product): the harness switches
// the model with LIB-18's `model`.
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
    CHECK(reply("lcd").rfind("done 2\n          0 \nindicators:", 0) == 0);   // ready for keys
    CHECK(reply("capabilities").find("place printer empty\n") != std::string::npos);   // the built-in default (LIB-18 AC-18)
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "TI-58");
    CHECK_EQ(reply("model ti-59"), std::string("done\n"));
    CHECK_EQ(reply("model TI-99"), std::string("refused: There is no model \"TI-99\".\n"));
    server->finish();
}

// LIB-5 (design §6, L12 on a TI; AC-15, AC-16, AC-17, AC-26; D-8): `key`
// presses the names of preset.md §8.1 as the app types them and replies when
// the last key is up; a TI model has no `type`.
TEST(lib5_harness_key_on_a_ti59) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("key 2 x^2"), std::string("done\n"));
    reply("wait 300ms");
    CHECK_EQ(reply("lcd"), std::string("done 2\n          4. \nindicators:\n"));   // a result: x² of 2
    CHECK_EQ(reply("key 1 S+"), std::string("refused: Unknown key: S+.\n"));   // AC-26: an old name; nothing typed
    CHECK_EQ(reply("key CLR 2nd pi"), std::string("done\n"));                  // 2nd before a second function: once
    reply("wait 300ms");
    CHECK(reply("lcd").find("3.14159265") != std::string::npos);
    CHECK_EQ(reply("type 12"), std::string("refused: Unknown command: type.\n"));   // D-8
    server->finish();
}

// LIB-8 AC-23 (design §11.6, E8-7): the harness's `paper` commands on the
// PC-100C's paper since M-53.
TEST(lib8_ac23_harness_paper_text_of_the_pc100c) {
    SKIP_UNLESS(std::filesystem::exists("roms/TMC0571B.txt"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("attach printer PC-100C"), std::string("done\n"));   // the built-in default has none (LIB-18 AC-18)
    CHECK_EQ(reply("paper text"), std::string("done\n"));   // empty paper (EC-2)
    reply("key 7");
    reply("key 2nd Prt");
    reply("wait 1s");
    CHECK_EQ(reply("paper text"), std::string("done 1\n          7.        \n"));
    CHECK_EQ(reply("paper text PC-100C"), std::string("done 1\n          7.        \n"));
    server->finish();
}

TEST(lib8_ac23_harness_paper_marks_and_png) {
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

// LIB-10 AC-31 (E10-4): a breakpoint set through the harness pauses a TI.
TEST(lib10_ac31_harness_breakpoint_pauses_the_ti) {
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

// LIB-18 design P59-6 (AC-18; D-17 of the harness is gone): the harness
// starts the built-in default, the TI-59 with the Master Library module and
// no printer.
TEST(harness_starts_with_ml_and_no_printer) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    const std::string caps = c("capabilities");
    CHECK(caps.find("place slot-1 ML\n") != std::string::npos);
    CHECK(caps.find("place printer empty\n") != std::string::npos);
    CHECK_EQ(c.machine()->model().value, std::string("TI-59"));
}

// C59-2 AC-9 (design §3.4, T8): `ti59 partition`, `ti59 program`, `ti59 registers`.
TEST(c59_2_ac9_partition_program_registers_write_and_read) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("ti59 partition"), std::string("done 1\n479\n"));
    CHECK_EQ(c("ti59 partition 239.29"), std::string("done\n"));
    CHECK_EQ(c("ti59 partition"), std::string("done 1\n239\n"));
    CHECK_EQ(c("ti59 program 76 11 42"), std::string("done\n"));          // from 000
    CHECK_EQ(c("ti59 program 010 85 01"), std::string("done\n"));         // from 010
    CHECK_EQ(c("ti59 program 000 count=3"), std::string("done 3\n000 76\n001 11\n002 42\n"));
    CHECK_EQ(c("ti59 program 010 count=2"), std::string("done 2\n010 85\n011 01\n"));
    CHECK_EQ(c("ti59 registers 05=1.5e-3 06=-42"), std::string("done\n"));
    CHECK_EQ(c("ti59 registers 05 06"), std::string("done 2\n05: 1.5e-3\n06: -4.2e1\n"));
    CHECK_EQ(c("model TI-58C"), std::string("done\n"));
    CHECK_EQ(c("ti59 registers H02=7"), std::string("done\n"));
    CHECK_EQ(c("ti59 registers H02"), std::string("done 1\nH02: 7e0\n"));
}

TEST(c59_2_ac9_refused_where_the_section_refuses) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("ti59 partition 960").rfind("refused: partition: 960 is beyond", 0) == 0);
    CHECK(c("ti59 partition abc").rfind("refused: ", 0) == 0);
    CHECK(c("ti59 program 76 LBL").rfind("refused: ", 0) == 0);
    CHECK(c("ti59 program 1").rfind("refused: ", 0) == 0);
    CHECK(c("ti59 program 959 76 11").rfind("refused: ", 0) == 0);   // past the last step
    CHECK(c("ti59 registers H00=1").rfind("refused: ", 0) == 0);     // not a TI-58C
    CHECK(c("ti59 registers 05=x").rfind("refused: ", 0) == 0);
    CHECK_EQ(c("model TI-58"), std::string("done\n"));
    CHECK(c("ti59 registers 60=1").rfind("refused: ", 0) == 0);
    CHECK(c("ti59 partition 480").rfind("refused: ", 0) == 0);
}

TEST(c59_2_ac9_count_reads_keycodes) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("ti59 program 010 05"), std::string("done\n"));   // a bare token is a keycode (Q-2)
    CHECK_EQ(c("ti59 program 010 count=1"), std::string("done 1\n010 05\n"));
    CHECK(c("ti59 program 010 count=0").rfind("refused: ", 0) == 0);
    CHECK(c("ti59 program 010 count=1 05").rfind("refused: ", 0) == 0);
}

// LIB-6 design §11.6 P12 (AC-34): a preset, the bare wait, expect and
// screenshot through `calcu --product 59`.
TEST(lib6_ac34_preset_wait_expect_screenshot_on_ti59) {
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

TEST(lib6_ac34_printer_trace_on_off) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("printer-trace on").rfind("refused: ", 0) == 0);   // no PC-100C
    CHECK_EQ(c("attach printer PC-100C"), std::string("done\n"));
    CHECK_EQ(c("printer-trace on"), std::string("done\n"));
    CHECK_EQ(c("printer-trace off"), std::string("done\n"));
}

// ── C59-4 AC-22 (design §3.10, T16) and LIB-13 AC-9 on Calc-U-59 ─────────────

TEST(c59_4_ac22_asm_load_run_clear) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("ti59 asm run"), std::string("refused: No ASM overlay loaded.\n"));
    CHECK_EQ(c("ti59 asm load examples/assembly/simple_count.asm"), std::string("done 1\n6 instructions\n"));
    const std::string run = c("ti59 asm run");
    CHECK(run.rfind("done 1\nASM entered at 0x1800 (", 0) == 0);
    CHECK(c("state asm-overlay").find("simple_count.asm") != std::string::npos);
    CHECK_EQ(c("ti59 asm clear"), std::string("done\n"));
    CHECK(c("ti59 asm load examples/assembly/README.md").rfind("refused: README.md is an ASM overlay in an older format.", 0) == 0);
    CHECK(c("ti59 asm load /no/such.asm").rfind("refused: such.asm could not be read.", 0) == 0);
    CHECK(c("ti59 asm").rfind("refused: usage:", 0) == 0);
}

TEST(c59_4_ac22_dump_replies_lines) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK(c("ti59 dump scom").rfind("done 17\n── SCOM ──\nS00 ", 0) == 0);
    CHECK(c("ti59 dump prog").rfind("done ", 0) == 0);
    CHECK(c("ti59 dump vars").rfind("done ", 0) == 0);
    CHECK(c("ti59 dump memory").rfind("done ", 0) == 0);
    CHECK(c("ti59 dump all").rfind("refused: ", 0) == 0);
}

TEST(c59_4_ac22_log_level) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("ti59 log-level debug"), std::string("done\n"));
    CHECK(c("state debug-view").find("debug") != std::string::npos);
    CHECK_EQ(c("ti59 log-level off"), std::string("done\n"));
    CHECK(c("ti59 log-level loud").rfind("refused: ", 0) == 0);
}

TEST(c59_4_ac22_heatmap_reset) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    CHECK_EQ(c("ti59 heatmap reset"), std::string("done\n"));
    CHECK(c("ti59 heatmap").rfind("refused: ", 0) == 0);
    // The CALCULATOR and CPU tabs are read with `state` (AC-25, design §3.1).
    c("wait 200ms");
    CHECK(c("state calculator-debug").find("partition") != std::string::npos);
    CHECK(c("state cpu-debug").rfind("done", 0) == 0);
}

// They are commands only: a preset has no steps for them.
TEST(c59_4_ac22_no_preset_steps_for_them) {
    SKIP_UNLESS(tirig::haveRoms(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    tirig::Calcu c;
    calcu::testing::TempFolder dir("calcu59-steps");
    for (const char* step : {"asm: run", "dump: scom", "log-level: debug", "heatmap: reset"}) {
        dir.write("p.ti59", std::string("format-version: 2\nmodel: TI-59\nkeys:\n  - ") + step + "\n");
        const std::string r = c("preset " + (dir.path() / "p.ti59").string());
        CHECK(r.rfind("refused: ", 0) == 0);
    }
}

// LIB-13 AC-9 (§10.3): `open <path>` takes every Calc-U-59 file.
TEST(lib13_ac9_open_takes_every_calc_u_59_file) {
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
    CHECK_EQ(c("ti59 program 000 count=2"), std::string("done 2\n000 76\n001 11\n"));
    CHECK(c("open " + (dir.path() / "ML.rom.yaml").string()).rfind("refused: ML.rom.yaml was not opened: ", 0) == 0);
}
