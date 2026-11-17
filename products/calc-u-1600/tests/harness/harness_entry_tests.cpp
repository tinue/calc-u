// LIB-11 Part D, D.6 (H1 to H3; M-51): the harness's Calc-U-1600 entry
// built through app/SharpAppPart (AC-20), `lcd` with the cursor cell
// (AC-19), and every function of the tools of the pin through `calcu`
// (AC-18, the exit check of M-51).
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "../../app/SharpDisplayLines.hpp"
#include "HarnessRun.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"

using namespace harnesstest;

namespace {

std::string fileText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The first row of an `lcd` reply ("done <n>\n<row>\n...").
std::string firstRow(const std::string& lcd) {
    const auto a = lcd.find('\n');
    if (a == std::string::npos) return {};
    return lcd.substr(a + 1, lcd.find('\n', a + 1) - a - 1);
}

} // namespace

// ── H1: AC-20 ────────────────────────────────────────────────────────────

// The run is the app session with the product's app layer, as the
// frontends host it: its topics and commands are there.
TEST(lib11_ac20_entry_runs_sharp_app_part) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    // SharpAppPart's own topics (c1600.lamps, c1600.serial-links) ...
    CHECK_EQ(r.line("state c1600.serial-links"), std::string("done 1\n{\"rs232c\":\"\",\"ce158\":\"\",\"moves\":0}\n"));
    CHECK(starts(r.line("state c1600.lamps"), "done 1\n{\"floppyMotor\":false"));
    // ... LIB-4's control state with the product's rules (the paper panel of
    // the plotters among the places' peripherals) and LIB-7's media.
    const std::string control = r.line("state control-state");
    CHECK(starts(control, "done 1\n{\"models\":[{\"id\":\"PC-1500\""));
    CHECK(has(control, "\"model\":\"PC-1500A\""));
    CHECK(starts(r.line("state media"), "done 1\n{\"kinds\":[{\"kind\":\"battery-backed-module\""));
    // SharpAppPart's commands: the host drive with its folder (C1600-5 AC-11).
    calcu::testing::TempFolder folder("lib11-ac20");
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("pc1600 host-drive " + folder.str()), std::string("done\n"));
    CHECK(has(r.line("capabilities"), "place host-drive host-drive\n"));
}

// AC-20: the ROMs of roms/ through sharp::lookUpRoms and the module
// definitions where the Qt6 frontend has them (frontends/qt6/resources/
// cards/), with the ROM set's CE-502B.
TEST(lib11_ac20_roms_and_module_definitions_as_the_frontends) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    const std::string media = r.line("media");
    for (const char* bundled : {"\"CE-1600M\"", "\"CE-1601M\"", "\"CE-161\"", "\"CE-1638\"", "\"CE-163F\"", "\"superRAM\"",
                                "\"Formatted\"", "\"Blank\"", "\"Empty\""})
        CHECK(has(media, bundled));
    CHECK(has(media, "\"module:CE-502B\""));   // the ROM set's module (LIB-12 AC-14)
    CHECK(has(media, "\"module:CE-151\"") && has(media, "\"module:CE-155\""));
    CHECK_EQ(r.line("attach slot-1 CE-502B"), std::string("done\n"));
    CHECK(has(r.line("capabilities"), "place slot-1 CE-502B\n"));
}

// ── H2: AC-19 ────────────────────────────────────────────────────────────

TEST(lib11_ac19_lcd_rows_indicators_not_text_off) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("lcd"), std::string("done 3\nNEW0? :CHECK\nindicators: DE G PRO I\nnot text: 0\n"));
    CHECK_EQ(r.line("key off"), std::string("done\n"));
    r.line("wait 500ms");
    CHECK_EQ(r.line("lcd"), std::string("done 1\ndisplay off\n"));
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    const std::string lcd = r.line("lcd");
    CHECK(has(lcd, "\n>\n"));
    CHECK(has(lcd, "\nindicators: DE G RUN I\nnot text: 0\n"));
}

// The cell under the cursor reads as the character the ROM saved there, as
// --lcd-text did: `ABC` typed, the cursor moved onto the B (the PC-1500A's
// cursor is the full block, its glyph 7FH; the PC-1600's an underline).
// While the ROM reports the cursor, `lcd` gives ABC whatever phase of the
// blink the display shows; when it does not, `lcd` reads the display as it
// is. Sampled every 20 ms for a second.
TEST(lib11_ac19_cursor_cell_read_as_text) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        Run r;
        CHECK(r.ok());
        if (!r.ok()) return;
        CHECK_EQ(r.line(std::string("model ") + model), std::string("done\n"));
        if (std::string(model) == "PC-1500A") {
            r.line("key cl");
            r.line("type NEW0");
        }
        CHECK_EQ(r.line("key a b c left left"), std::string("done\n"));
        int covered = 0, read = 0;
        for (int i = 0; i < 50; ++i) {
            r.line("wait 20ms");
            const std::string row = firstRow(r.line("lcd"));
            // The display as it is, read with the font alone.
            calcu::machine::Machine* m = r.machine();
            const calcu::harness::DisplayText raw = cu1600::displayLines(*m, m->readDisplay());
            const std::string rawRow = raw.rows.empty() ? std::string() : raw.rows[0];
            const auto start = rawRow.find_first_not_of(' ');
            const std::string rawText = start == std::string::npos ? std::string() : rawRow.substr(start, 3);
            if (row.find("ABC") != std::string::npos) {
                ++read;
                if (rawText != "ABC") ++covered;   // the cursor over the B, read as B
            } else {
                CHECK_EQ(row, rawRow);   // no cursor reported: the display as it is
            }
        }
        CHECK(covered > 0);
        CHECK(read > 25);
    }
}

// ── H3: AC-18, the exit check of M-51 ───────────────────────────────────

// One block per row of the table of AC-18 (LIB-11 design D.5): each function
// of `pc1500_cli` and `pc1600_cli` at the pin through `calcu`, on the model
// the row needs, with its reply.
TEST(lib11_ac18_every_function_of_the_table) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder out("lib11-ac18-out"), save("lib11-ac18-save");
    calcu::harness::RunOptions o;
    o.outputFolder = out.str();
    o.saveFolder = save.str();
    Run r(o);
    CHECK(r.ok());
    if (!r.ok()) return;

    // --preset <file>: build a machine from a preset and run it (LIB-6).
    CHECK_EQ(r.line("preset examples:basic/hanoi.pc1600"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
    CHECK_EQ(r.line("expect Hanoy Towers"), std::string("done\n"));
    // --check-preset <file>…, tools/check_presets.sh: presets read and checked (LIB-6).
    CHECK(starts(r.line("check-preset examples/basic/hanoi.pc1600 examples/interfaces/ce158_demo.pc1500a"), "done"));
    // --rom, --ce1600p-rom: the variants (LIB-18).
    CHECK_EQ(r.line("variant Old"), std::string("done\n"));
    CHECK_EQ(r.line("attach printer CE-1600P:old"), std::string("done\n"));
    CHECK(has(r.line("capabilities"), "place printer CE-1600P:Old\n"));
    // --run-after <ticks>, the run for a number of ticks: emulated time (LIB-11).
    CHECK(starts(r.line("wait 1s"), "done 2\nticks: "));
    // --lcd-text: the display as text (AC-10, AC-19).
    CHECK(has(r.line("lcd"), "\nindicators: "));
    // --lcd-png: the display as an image, Copy Screen's (LIB-6).
    CHECK_EQ(r.line("screenshot shot.png"), std::string("done\n"));
    CHECK(fs::exists(out.path() / "shot.png"));
    // --wav <file>: the buzzer of the run as a WAV (LIB-11 `audio`).
    CHECK_EQ(r.line("audio " + (out.path() / "buzz.wav").string()), std::string("done\n"));
    r.line("key cl");
    r.line("type BEEP 1");
    r.line("wait 1s");
    // --dump-mem <address>,<length>: memory as a hex dump (LIB-10).
    CHECK(starts(r.line("peek SC7852 memory 0xF865 2"), "done 2\n0xf865: 0x"));
    // --inspect <view>: an INSPECTOR view (C1600-5).
    CHECK(starts(r.line("pc1600 inspect pointers"), "done "));
    // --dump-basic: the BASIC pointers and the program area (C1600-5).
    CHECK(has(r.line("pc1600 dump-basic"), "\nBASPRG_ST  $F865 = $"));
    // The summary after a run: registers, the last instructions, where the
    // program counter spent its time (LIB-10).
    r.line("pause");
    CHECK(has(r.line("registers SC7852"), "\npc = 0x"));
    CHECK(starts(r.line("history SC7852 4"), "done "));
    CHECK_EQ(r.line("profile on"), std::string("done\n"));
    r.line("resume");
    r.line("wait 100ms");
    CHECK(starts(r.line("profile report 3"), "done "));
    CHECK_EQ(r.line("profile off"), std::string("done\n"));
    // --tape-in, --tape-out: a cassette played and recorded (LIB-7; C1600-5).
    CHECK_EQ(r.line("pc1600 cassette record " + (out.path() / "tape.wav").string()), std::string("done\n"));
    CHECK_EQ(r.line("pc1600 cassette status"), std::string("done 2\ncassette: recording 0.000000000\nmotor: off\n"));
    CHECK_EQ(r.line("eject cassette"), std::string("done\n"));
    CHECK_EQ(r.line("cassette { name: Empty }"), std::string("done\n"));   // the bundled blank tape
    CHECK(starts(r.line("pc1600 cassette status"), "done 2\ncassette: recording "));
    // --save-dir, --modules-dir: the save folder; the bundled module
    // definitions (LIB-11 AC-8, LIB-7).
    CHECK_EQ(r.line("saveas { place: cassette, as: live, name: Mine }"), std::string("done\n"));
    CHECK(fs::exists(save.path() / "Cassettes" / "Mine.cassette.yaml"));
    CHECK(has(r.line("media"), "\"CE-1600M\""));

    // --ce158-pty, --ce158-rx, --ce158-rx-hold, --ce158-tx: the CE-158's
    // serial port (C1600-5).
    calcu::testing::TempFolder dir("lib11-ac18-ce158");
    { std::ofstream(dir.path() / "in.txt", std::ios::binary) << "HELLO\r"; }
    CHECK_EQ(r.line("model PC-1500A"), std::string("done\n"));
    CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
    CHECK_EQ(r.line("pc1600 serial ce158 feed " + (dir.path() / "in.txt").string() + " hold=2"), std::string("done\n"));
    CHECK_EQ(r.line("pc1600 serial ce158 capture " + (dir.path() / "tx.bin").string()), std::string("done\n"));
    r.line("wait 1s");
    CHECK(has(r.line("pc1600 serial ce158 report"), "\nreceived: 6 of 6 bytes fed\n"));
#ifndef _WIN32
    CHECK_EQ(r.line("pc1600 serial ce158 link"), std::string("done 1\npath: <save folder>/calcu1600-ce158.serial\n"));
#endif
    r.server->finish();
    CHECK(fileText(out.path() / "buzz.wav").compare(0, 4, "RIFF") == 0);   // complete at the end of the run
    // The probes (not taken over), read_trace.py, dap_smoke.py and --shots
    // are the CTests CalcU1600NoOldTools, CalcU1600ReadTrace,
    // CalcU1600DapSmoke and CalcU1600Shots.* (docs/test-groups.md).
}
