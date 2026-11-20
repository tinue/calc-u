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
        s.program({"10 FOR I=1 TO 300", "20 NEXT I", "30 PRINT \"DO\"+\"NE\""});
        s.line("type RUN");
        const double t = Sharp::seconds(s.line("wait-finished"));
        CHECK(t > 0.3);
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
