// The cases of docs/developer/Idle.md, through the harness: each brings the
// machine into its case with timed waits only, then reads the answer of
// `idle-state` (A takes keys, W waits, the program has ended) and checks
// what the steps do with it (`wait` returns once settled, `wait-finished`
// once the program has ended, BREAK goes in at once).
#include <fstream>
#include <string>
#include <vector>

#include "HarnessRun.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"

using namespace harnesstest;

namespace {

struct Idle {
    bool takesKeys = false, waits = false, ended = false;
    bool read = false;   // the reply had the three lines
};

class Sharp {
public:
    explicit Sharp(const std::string& model) : folder_("c1600-idle"), pc1600_(model == "PC-1600") {
        if (!run_.ok()) return;
        run_.line("model " + model);
        run_.line("wait 3s");
    }
    bool ok() { return run_.ok(); }
    std::string line(const std::string& l) { return run_.line(l); }
    void pause(const char* duration) { run_.line(std::string("wait ") + duration); }
    // A key and the time the ROM takes for it (no idle wait).
    void press(const std::string& keys) {
        line("key " + keys);
        pause("300ms");
    }
    void typeLine(const std::string& text) {
        line("type " + text);
        pause("500ms");
    }

    // NEW0 in PRO mode, the program straight into memory, back to RUN mode.
    void program(const std::vector<std::string>& lines) {
        std::string text;
        for (const std::string& l : lines) text += l + "\n";
        const fs::path file = folder_.path() / "program.bas";
        std::ofstream(file, std::ios::binary) << text;
        if (pc1600_) press("mode");
        else press("cl");   // a fresh PC-1500 is in PRO mode
        typeLine("NEW0");
        if (!lines.empty()) {
            line("c1600 load-basic " + file.string());
            pause("300ms");
        }
        press("mode");
    }
    void runProgram() {
        typeLine("RUN");
        pause("500ms");
    }

    Idle idle() {
        const std::string r = line("idle-state");
        Idle i;
        auto yes = [&](const std::string& key, bool* out) {
            const auto at = r.find("\n" + key + ": ");
            if (at == std::string::npos) return false;
            *out = r.compare(at + key.size() + 3, 3, "yes") == 0;
            return true;
        };
        i.read = yes("takes keys", &i.takesKeys) && yes("waits", &i.waits) && yes("ended", &i.ended);
        return i;
    }
    static double seconds(const std::string& reply) {
        const auto at = reply.find("\nseconds: ");
        return at == std::string::npos ? -1.0 : std::stod(reply.substr(at + 10));
    }
    const fs::path& folder() const { return folder_.path(); }

private:
    calcu::testing::TempFolder folder_;
    Run run_;
    bool pc1600_;
};

void expectRow(const Idle& i, bool a, bool w, bool ended) {
    CHECK(i.read);
    CHECK_EQ(i.takesKeys, a);
    CHECK_EQ(i.waits, w);
    CHECK_EQ(i.ended, ended);
}

// A settled machine: `wait` returns after the stretch, well within a second.
void expectSettles(Sharp& s) {
    const double t = Sharp::seconds(s.line("wait"));
    CHECK(t >= 0.0 && t < 1.0);
}

// Work that ends by itself (a LOAD, a printed line): `wait` lasts until it
// has ended, then the stretch; well within the hour.
void expectSettlesAfterWork(Sharp& s) {
    const double t = Sharp::seconds(s.line("wait"));
    CHECK(t >= 0.0 && t < 60.0);
}

// Not A, not W: sampled a few times, a timed wait apart.
void expectBusy(Sharp& s, int samples = 3) {
    for (int n = 0; n < samples; ++n) {
        expectRow(s.idle(), false, false, false);
        s.pause("200ms");
    }
}

// The cases every Sharp model shares.
void sharedCases(const std::string& model) {
    {   // the prompt in RUN and PRO mode
        Sharp s(model);
        if (!s.ok()) return;
        s.program({});
        expectRow(s.idle(), true, true, true);
        expectSettles(s);
        const double t = Sharp::seconds(s.line("wait-finished"));
        CHECK(t >= 0.0 && t < 1.0);
        s.press("mode");
        expectRow(s.idle(), true, true, true);
    }
    {   // INPUT: waits for the user, the program has not ended
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 INPUT A", "20 END"});
        s.runProgram();
        expectRow(s.idle(), true, true, false);
        expectSettles(s);
    }
    {   // INKEY$ polling
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 A$=INKEY$", "20 GOTO 10"});
        s.runProgram();
        expectRow(s.idle(), true, true, false);
        expectSettles(s);
    }
    {   // a program that never reads the keyboard
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 GOTO 10"});
        s.runProgram();
        expectBusy(s);
    }
    {   // PRINT after WAIT n: a timed delay, part of the run
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 WAIT 640", "20 PRINT \"A\"", "30 GOTO 20"});
        s.runProgram();
        expectBusy(s);
    }
    {   // BREAK goes in while a program runs, at once; then the prompt
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 GOTO 10"});
        s.runProgram();
        CHECK_EQ(s.line("key break"), std::string("done\n"));
        s.pause("500ms");
        CHECK(has(s.line("display-text"), "BREAK"));
        expectRow(s.idle(), true, true, true);
    }
    {   // wait-finished lasts until the program has ended
        Sharp s(model);
        if (!s.ok()) return;
        s.program({"10 FOR I=1 TO 300", "20 NEXT I", "30 END"});   // no PRINT: the PC-1500's stops
        s.line("type RUN");
        const double t = Sharp::seconds(s.line("wait-finished"));
        CHECK(t > 0.3 && t < 60.0);
        expectRow(s.idle(), true, true, true);
    }
    {   // OFF at the prompt: nothing
        Sharp s(model);
        if (!s.ok()) return;
        s.program({});
        s.press("off");
        s.pause("500ms");
        expectRow(s.idle(), false, false, false);
    }
}

} // namespace

// ── PC-1500, PC-1500A ────────────────────────────────────────────────────

TEST(idle_pc1500_shared_cases) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    sharedCases("PC-1500");
}

TEST(idle_pc1500a_shared_cases) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    sharedCases("PC-1500A");
}

TEST(idle_pc1500_new0_check_takes_keys) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");   // a fresh PC-1500A asks NEW0? :CHECK
    if (!s.ok()) return;
    CHECK(has(s.line("display-text"), "NEW0?"));
    expectRow(s.idle(), true, true, true);
}

TEST(idle_pc1500_print_stop_without_wait_statement) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");   // WAIT_CFG 0 after NEW0: PRINT stops until ENTER
    if (!s.ok()) return;
    s.program({"10 PRINT \"A\"", "20 GOTO 10"});
    s.runProgram();
    expectRow(s.idle(), true, true, false);
    expectSettles(s);
}

TEST(idle_pc1500_call_to_the_key_wait) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");
    if (!s.ok()) return;
    s.program({"10 CALL &E243", "20 END"});
    s.runProgram();
    expectRow(s.idle(), true, true, false);
    expectSettles(s);
}

TEST(idle_pc1500_beep_and_ce150_are_busy) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    {
        Sharp s("PC-1500A");
        if (!s.ok()) return;
        s.program({});
        s.line("type BEEP 20");
        s.pause("300ms");
        expectBusy(s);
    }
    {
        Sharp s("PC-1500A");
        if (!s.ok()) return;
        s.line("attach printer CE-150");
        s.pause("3s");
        s.press("cl");   // the memory changed: NEW0? :CHECK
        s.program({});
        s.line("type LPRINT \"HELLO CE-150\"");
        s.pause("200ms");
        expectBusy(s, 2);
        expectSettlesAfterWork(s);   // once the line is out
    }
}

// ── PC-1600 ──────────────────────────────────────────────────────────────

TEST(idle_pc1600_shared_cases) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    sharedCases("PC-1600");
}

TEST(idle_pc1600_print_stops_only_after_a_plain_wait) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    {   // MODE 0: RUN sets WAIT 0, PRINT goes on
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.program({"10 PRINT \"A\"", "20 GOTO 10"});
        s.runProgram();
        expectBusy(s);
    }
    {   // after WAIT alone, PRINT stops until ENTER
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.program({"10 WAIT", "20 PRINT \"A\"", "30 GOTO 20"});
        s.runProgram();
        expectRow(s.idle(), true, true, false);
        expectSettles(s);
    }
}

TEST(idle_pc1600_mode1_prompt_and_print_stop) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    s.program({});
    s.typeLine("MODE1");
    expectRow(s.idle(), true, true, true);
    s.press("mode");   // PRO mode, MODE 1's keywords
    s.typeLine("10 PRINT \"A\"");
    s.typeLine("20 GOTO 10");
    s.press("mode");
    s.runProgram();   // RUN sets WAIT 0 in MODE 1: PRINT stops
    expectRow(s.idle(), true, true, false);
}

TEST(idle_pc1600_calls_to_keyget) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    {   // from a program: CURLINE holds the line, BUSY on
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.program({"10 CALL &166", "20 END"});
        s.runProgram();
        expectRow(s.idle(), true, true, false);
        expectSettles(s);
    }
    {   // direct: CURLINE 0, BUSY on
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.program({});
        s.typeLine("CALL &166");
        expectRow(s.idle(), true, true, false);
        expectSettles(s);
    }
}

TEST(idle_pc1600_beep_is_busy) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    s.program({});
    s.line("type BEEP 20,100,2000");
    s.pause("300ms");
    expectBusy(s);
}

TEST(idle_pc1600_direct_load_is_busy_until_it_ends) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    std::vector<std::string> lines;
    for (int i = 1; i <= 200; ++i)
        lines.push_back(std::to_string(10 * i) + " PRINT \"LINE " + std::to_string(i) + " ABCDEFGHIJKLMNOPQRSTUVWXYZ\"");
    s.line("c1600 host-drive " + s.folder().string());
    s.pause("1s");
    s.program(lines);
    s.typeLine("SAVE\"S3:P.BAS\"");
    s.line("wait");
    s.program({});
    s.line("type LOAD\"S3:P.BAS\"");
    s.pause("100ms");
    expectBusy(s, 1);
    expectSettlesAfterWork(s);   // once the LOAD has ended
    CHECK(has(s.line("c1600 list-basic"), "\n2000 PRINT \"LINE 200 "));
}

TEST(idle_pc1600_ce1600p_is_busy_while_it_prints) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    s.line("attach printer CE-1600P");
    s.pause("15s");   // the CE-1600P's power-on: the pen's home run
    s.program({});
    s.line("type LPRINT \"HELLO CE-1600P\"");
    s.pause("200ms");
    expectBusy(s, 2);
}

// ── A medium awaited (docs/developer/Idle.md, "A medium awaited") ─────────

namespace {

// A cassette definition beside a copy of the fixture's sound file.
std::string cassetteOf(const fs::path& folder, const char* wav) {
    fs::copy_file(fs::path("tests/fixtures/tape") / wav, folder / wav, fs::copy_options::overwrite_existing);
    const fs::path yaml = folder / (std::string(wav) + ".cassette.yaml");
    std::ofstream(yaml, std::ios::binary) << "format: cassette\nformat-version: 1\ncassette-name: \"T\"\naudio: { file: "
                                          << wav << " }\n";
    return yaml.string();
}

// A PC-1500A with a CE-150 at the prompt in RUN mode, NEW0 done.
void pc1500WithCe150(Sharp& s) {
    s.line("attach printer CE-150");
    s.pause("5s");
    s.program({});   // CL answers the memory check, NEW0, RUN mode
}

// Waits for the medium, the program has not ended.
void expectAwaitsMedium(Sharp& s) {
    expectRow(s.idle(), false, true, false);
    expectSettles(s);
}

} // namespace

TEST(idle_pc1500_cload_without_a_cassette_awaits_the_tape) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");
    if (!s.ok()) return;
    pc1500WithCe150(s);
    s.line("type CLOAD");
    s.pause("2s");   // past the motor start (1 s), in LOAD_NIBBLE's edge wait
    expectAwaitsMedium(s);
}

TEST(idle_pc1500_cload_from_a_playing_tape_is_work_until_loaded) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");
    if (!s.ok()) return;
    pc1500WithCe150(s);
    CHECK_EQ(s.line("cassette { file: " + cassetteOf(s.folder(), "pc1500_tape.wav") + " }"), std::string("done\n"));
    s.line("type CLOAD");
    s.pause("1500ms");   // the leader
    expectBusy(s, 2);
    expectSettlesAfterWork(s);   // until the program is in
    expectRow(s.idle(), true, true, true);
    s.typeLine("RUN");
    s.pause("2s");
    CHECK(has(s.line("display-text"), "SUM= 385"));
    // A second CLOAD finds the tape at its end: it waits for a medium again.
    s.line("type CLOAD");
    s.pause("2s");
    expectAwaitsMedium(s);
}

TEST(idle_pc1500_ce158_input_awaits_the_port) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1500A");
    if (!s.ok()) return;
    s.line("attach interface CE-158");
    s.pause("3s");
    s.program({"10 SETCOM 1200,8,N,1:OUTSTAT 0", "20 SETDEV KI", "30 INPUT N$", "40 SETDEV", "50 PRINT N$"});
    s.runProgram();
    expectAwaitsMedium(s);   // nothing fed
    // A feed still to come (held): bytes arrive, work.
    const fs::path name = s.folder() / "name.txt";
    std::ofstream(name, std::ios::binary) << "WORLD\r";
    s.line("c1600 serial ce158 feed " + name.string() + " hold=240");   // 2 s at 1200 baud
    s.pause("500ms");
    expectBusy(s, 2);
    expectSettlesAfterWork(s);
    CHECK(has(s.line("display-text"), "WORLD"));
}

TEST(idle_pc1600_com1_input_awaits_the_port) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    s.program({"10 SETCOM \"COM1:\",9600,8,N,1,N,N", "20 SETDEV \"COM1:\",KI", "30 INPUT N$", "40 SETDEV \"COM2:\"",
               "50 PRINT N$"});
    s.runProgram();
    expectAwaitsMedium(s);   // nothing fed: CRCVA waits on an empty ring
    const fs::path name = s.folder() / "name.txt";
    std::ofstream(name, std::ios::binary) << "WORLD\r";
    s.line("c1600 serial rs232c feed " + name.string() + " hold=1920");   // 2 s at 9600 baud
    s.pause("500ms");
    expectBusy(s, 2);
    expectSettlesAfterWork(s);
    CHECK(has(s.line("display-text"), "WORLD"));
}

TEST(idle_pc1600_direct_load_com1_awaits_the_port_then_loads) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Sharp s("PC-1600");
    if (!s.ok()) return;
    s.program({});
    s.typeLine("SETCOM \"COM1:\",9600,8,N,1,N,N");
    s.line("type LOAD\"COM1:\"");
    s.pause("500ms");
    expectAwaitsMedium(s);   // nothing fed yet
    const fs::path file = s.folder() / "p.txt";
    std::ofstream(file, std::ios::binary) << "10 A=6*7\r\n20 PRINT A\r\n\x1a";
    s.line("c1600 serial rs232c feed " + file.string());
    expectSettlesAfterWork(s);   // the LOAD has ended
    expectRow(s.idle(), true, true, true);
    CHECK(has(s.line("c1600 list-basic"), "\n20 PRINT A\n"));
}

TEST(idle_pc1600_cload_awaits_the_tape_and_a_playing_tape_is_work) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    {   // no cassette: CMBITIN's first edge wait, without a timeout
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.line("attach printer CE-1600P");
        s.pause("15s");   // the pen's home run
        s.program({});
        s.line("type CLOAD");
        s.pause("2s");
        expectAwaitsMedium(s);
    }
    {   // a playing tape, its silence between header and data too
        Sharp s("PC-1600");
        if (!s.ok()) return;
        s.line("attach printer CE-1600P");
        s.pause("15s");
        s.program({});
        CHECK_EQ(s.line("cassette { file: " + cassetteOf(s.folder(), "pc1600_tape.wav") + " }"), std::string("done\n"));
        s.line("type CLOAD");
        s.pause("4500ms");   // the tape at 4.2-5.2 s: silence, the loader in its edge wait, the motor on
        expectBusy(s, 2);
        expectSettlesAfterWork(s);
        expectRow(s.idle(), true, true, true);
        CHECK(has(s.line("c1600 list-basic"), "\n180 END\n500 PRINT \"SUBROUTINE\":RETURN\n"));   // the whole program
    }
}
