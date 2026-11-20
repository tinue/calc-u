// The product commands under `c1600`, each on a `calcu --product c1600` run
// of its own, the expected replies written out. The machine state's
// commands are tested in tests/machine_state_tests.cpp.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "Connector/PrintableBytes.hpp"
#include "Debug/Inspect/Inspector.hpp"
#include "HarnessRun.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "harness/Commands.hpp"

using namespace harnesstest;

namespace {

std::string fileText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeFile(const fs::path& p, const std::string& bytes) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << bytes;
}

// The value of the reply line "<key>: <value>", or "".
std::string field(const std::string& reply, const std::string& key) {
    const std::string k = "\n" + key + ": ";
    const auto at = reply.find(k);
    if (at == std::string::npos) return {};
    const auto end = reply.find('\n', at + k.size());
    return reply.substr(at + k.size(), end - at - k.size());
}

// A PC-1500A with NEW0 answered and `10 PRINT 1` typed in (PRO mode).
void onePrintLine(Run& r) {
    CHECK_EQ(r.line("key cl"), std::string("done\n"));
    CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
    CHECK_EQ(r.line("type 10 PRINT 1"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
}

} // namespace

// ── The commands ─────────────────────────────────────────────────────────

TEST(commands_under_c1600_on_every_model) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    std::string usage;
    for (const std::string& u : r.server->commands().usage()) usage += u + "\n";
    for (const char* c : {"c1600 file-kind", "c1600 load-basic", "c1600 type-basic", "c1600 load-code", "c1600 list-basic",
                          "c1600 inspect", "c1600 cassette", "c1600 serial", "c1600 host-drive", "c1600 call"})
        CHECK(has(usage, c));
    CHECK(!has(usage, "c1600 dump-basic"));   // the INSPECTOR's views `pointers` and `dump-basic` show it
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        CHECK_EQ(r.line(std::string("model ") + model), std::string("done\n"));
        CHECK(starts(r.line("c1600 inspect pointers"), "done "));
        CHECK(starts(r.line("c1600 cassette status"), "refused: "));   // no cassette, but the command is there
    }
}

// ── The BASIC pointers and the program area ─────────────────────────────

// What the BASIC pointers hold and the program's bytes are INSPECTOR views:
// `pointers` and `dump-basic`.
TEST(basic_pointers_and_area_pc1500a) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    onePrintLine(r);
    const std::string pointers = r.line("c1600 inspect pointers");
    CHECK(has(pointers, "│ BASPRG_ST       │ 7865 │ 40C5  │"));
    CHECK(has(pointers, "│ BASPRG_END      │ 7867 │ 40CC  │"));
    CHECK(has(pointers, "│ BASPRG_EDT      │ 7869 │ 40C5  │"));
    CHECK(has(pointers, "│ VAR_START       │ 7899 │ 5800  │"));
    CHECK(has(pointers, "│ DATA_PTR        │ 78BE │ C0C8  │"));
    CHECK(has(pointers, "│ RAM_END         │ 7864 │ 58    │"));
    const std::string area = r.line("c1600 inspect dump-basic");
    CHECK(has(area, "\n── BASIC area 4000–57FF ──\n"));
    CHECK(has(area, "\n40C0: 00 00 00 00 00 00 0A 04  F0 97 31 0D FF 00 00 00  ..........1.....\n"));
}

TEST(basic_pointers_and_area_pc1600) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));   // PRO mode
    CHECK_EQ(r.line("type NEW"), std::string("done\n"));
    CHECK_EQ(r.line("type 10 PRINT 1"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
    const std::string pointers = r.line("c1600 inspect pointers");
    CHECK(has(pointers, "│ BASPRG_ST    │ F865 │ 40C5 → Z80 C0C5 │"));
    CHECK(has(pointers, "│ BASPRG_END   │ F867 │ 40CC → Z80 C0CC │"));
    CHECK(has(pointers, "│ VARPTR       │ F899 │ 6F00 → Z80 EF00 │"));
    const std::string area = r.line("c1600 inspect dump-basic");
    CHECK(has(area, "\n── BASIC area S0 ──\n── Internal RAM, Z80 C000–EFFF ──\n"));
    CHECK(has(area, "\nC0C0: 00 00 00 00 00 00 0A 04  F0 97 31 0D FF 00 FF FF  ..........1.....\n"));
}

// ── list-basic ───────────────────────────────────────────────────────────

TEST(list_basic_pc1500a) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("c1600 list-basic"), std::string("done\n"));   // no program yet: no lines
    onePrintLine(r);
    CHECK_EQ(r.line("type 20 GOTO 10"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
    CHECK_EQ(r.line("c1600 list-basic"), std::string("done 2\n10 PRINT 1\n20 GOTO 10\n"));
}

// With a CE-1600M in slot 1, S0 runs through both of its 16 KB banks: a
// program longer than one bank is read across the bank-end mark, in order.
TEST(list_basic_pc1600_across_module_banks) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-list-basic");
    std::string listing;
    std::vector<std::string> lines;
    for (int n = 1; n <= 600; ++n) {
        lines.push_back(std::to_string(n * 10) + " REM " + std::string(36, char('A' + n % 26)));
        listing += lines.back() + "\n";
    }
    writeFile(dir.path() / "long.bas", listing);
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("attach slot-1 CE-1600M"), std::string("done\n"));
    CHECK_EQ(r.line("cold-boot"), std::string("done\n"));   // the ROM finds the module at the start
    CHECK(starts(r.line("wait"), "done "));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
    CHECK(starts(r.line("c1600 load-basic " + (dir.path() / "long.bas").string()), "done"));
    std::string want = "done 600\n";
    for (const std::string& l : lines) want += l + "\n";
    CHECK_EQ(r.line("c1600 list-basic"), want);
}

TEST(list_basic_refused_in_mode1) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("type MODE1"), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));
    CHECK_EQ(r.line("c1600 list-basic"), std::string("refused: list-basic reads a program in MODE 0 only.\n"));
}

// ── inspect ──────────────────────────────────────────────────────────────

TEST(every_view_per_model) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    const std::vector<std::string> both = {"pointers", "inventory", "lh", "basic", "physical", "dump-basic"};
    const std::vector<std::string> only1600 = {"z80", "programs", "disks", "dump-s1", "dump-s2", "dump-disk-s1", "dump-disk-s2"};
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        CHECK_EQ(r.line(std::string("model ") + model), std::string("done\n"));
        const bool pc1600 = std::string(model) == "PC-1600";
        std::vector<std::string> all = both;
        all.push_back("dump-ml");
        all.insert(all.end(), only1600.begin(), only1600.end());
        for (const std::string& name : all) {
            inspect::View v;
            CHECK(inspect::viewFromName(name, &v));
            // The reply is the view as the INSPECTOR shows it, on the same machine.
            const auto* k = calcu::machine::first<sharp::InspectorViews>(*r.machine());
            CHECK(k != nullptr);
            if (!k) return;
            const std::vector<std::string> lines = k->view(v);
            std::string want = "done " + std::to_string(lines.size()) + "\n";
            for (const std::string& l : lines) want += l + "\n";
            const std::string got = r.line("c1600 inspect " + name);
            CHECK_EQ(got, want);
            const bool onModel = name == "dump-ml" ? !pc1600
                                 : std::find(only1600.begin(), only1600.end(), name) != only1600.end() ? pc1600
                                                                                                       : true;
            if (!onModel) {   // the one line of a view the model does not have
                CHECK(starts(got, "done 1\n"));
                CHECK(has(got, pc1600 ? ": not on the PC-1600\n" : ": not on the PC-1500\n"));
            } else {
                CHECK(!has(got, ": not on the PC-1"));
            }
        }
    }
}

TEST(unknown_view_refused) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("c1600 inspect registers"),
             std::string("refused: No view registers. The views: pointers, inventory, z80, lh, basic, programs, disks, "
                         "physical, dump-basic, dump-s1, dump-s2, dump-disk-s1, dump-disk-s2, dump-ml.\n"));
    CHECK(starts(r.line("c1600 inspect"), "refused: usage: "));
}

// ── cassette ─────────────────────────────────────────────────────────────

namespace {
// A PC-1500A with a CE-150, a one-line program saved to tape with CSAVE.
void csave(Run& r) {
    onePrintLine(r);
    CHECK_EQ(r.line("type CSAVE \"A\""), std::string("done\n"));
    CHECK(starts(r.line("wait"), "done "));   // until idle: the motor has stopped
}
} // namespace

TEST(record_writes_wav_at_motor_stop_and_eject) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-cassette-record");
    const fs::path wav = dir.path() / "out.wav";
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette record " + wav.string()), std::string("done\n"));
    CHECK(!fs::exists(wav));   // nothing recorded yet
    csave(r);
    CHECK(fs::exists(wav));   // written when the motor stopped
    const std::string atStop = fileText(wav);
    CHECK(atStop.size() > 44 && atStop.compare(0, 4, "RIFF") == 0);
    // The recording is a tape a loader reads: the file CSAVE wrote.
    CHECK_EQ(r.line("c1600 file-kind " + wav.string()),
             std::string("done 4\nkind: basic-pc1500\nname: A\nlength: 7\nfile 1: A\n"));
    CHECK_EQ(r.line("eject cassette"), std::string("done\n"));
    CHECK(fileText(wav) == atStop);   // nothing new to write
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: empty\nmotor: off\n"));
    // Taken out while the motor runs.
    const fs::path second = dir.path() / "second.wav";
    CHECK_EQ(r.line("c1600 cassette record " + second.string()), std::string("done\n"));
    CHECK_EQ(r.line("type CSAVE \"B\""), std::string("done\n"));
    r.line("wait 2s");
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: recording 2.049703077\nmotor: on\n"));
    const std::size_t before = fs::exists(second) ? fileText(second).size() : 0;   // to the last stop of the motor
    CHECK_EQ(r.line("eject cassette"), std::string("done\n"));
    // Written: what the deck holds, to its last motor stop.
    CHECK(fs::exists(second) && fileText(second).size() >= before && fileText(second).compare(0, 4, "RIFF") == 0);
}

TEST(record_ejects_the_tape_in_first) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-cassette-record-ejects");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette record " + (dir.path() / "a.wav").string()), std::string("done\n"));
    csave(r);
    const std::string a = fileText(dir.path() / "a.wav");
    // A second record: the first tape is written and taken out, a new one goes in.
    CHECK_EQ(r.line("c1600 cassette record " + (dir.path() / "b.wav").string()), std::string("done\n"));
    CHECK(fileText(dir.path() / "a.wav") == a && a.size() > 44);
    CHECK(!fs::exists(dir.path() / "b.wav"));
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: recording 0.000000000\nmotor: off\n"));
}

// The CE-150 on the PC-1600 (MODE 1) drives its own cassette recorder: CLOAD
// starts the motor and the tape moves.
TEST(ce150_on_pc1600_plays_its_cassette) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-ce150-pc1600-tape");
    fs::copy_file("tests/fixtures/tape/pc1500_tape.wav", dir.path() / "t.wav");
    writeFile(dir.path() / "t.cassette.yaml",
              "format: cassette\nformat-version: 1\ncassette-name: \"T\"\naudio: { file: t.wav }\n");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    r.line("wait 15s");
    for (const char* l : {"key mode", "type NEW0", "key mode", "type MODE1"}) {
        r.line(l);
        r.line("wait 500ms");
    }
    CHECK_EQ(r.line("cassette { file: " + (dir.path() / "t.cassette.yaml").string() + " }"), std::string("done\n"));
    r.line("type CLOAD");
    r.line("wait 5s");
    const std::string status = r.line("c1600 cassette status");
    CHECK(has(status, "\nmotor: on\n"));
    CHECK(!has(status, "playing 0.000000000"));
}

TEST(status_playing_recording_empty) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-cassette-status");
    const fs::path wav = dir.path() / "rec.wav";
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: empty\nmotor: off\n"));
    CHECK_EQ(r.line("c1600 cassette record " + wav.string()), std::string("done\n"));
    csave(r);
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: recording 12.399006923\nmotor: off\n"));
    CHECK_EQ(r.line("eject cassette"), std::string("done\n"));
    // The recording played back: a cassette definition beside it.
    writeFile(dir.path() / "rec.cassette.yaml",
              "format: cassette\nformat-version: 1\ncassette-name: \"Rec\"\naudio: { file: \"rec.wav\" }\n");
    CHECK_EQ(r.line("cassette { file: " + (dir.path() / "rec.cassette.yaml").string() + " }"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: playing 0.000000000 of 12.399000000\nmotor: off\n"));
    calcu::harness::RunOptions o;
    o.json = true;
    Run j(o);
    CHECK(j.ok());
    if (!j.ok()) return;
    j.line("attach printer CE-150");
    CHECK_EQ(j.line("c1600 cassette status"),
             std::string("{\"done\":true,\"result\":{\"cassette\":\"empty\",\"position\":\"0.000000000\",\"length\":"
                         "\"0.000000000\",\"motor\":false},\"reason\":null}\n"));
}

TEST(tape_without_cassette_refused) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-5-ec2");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    const std::string no = "refused: The machine has no cassette: neither a CE-150 nor a CE-1600P is plugged in.\n";
    CHECK_EQ(r.line("c1600 cassette record " + (dir.path() / "x.wav").string()), no);
    CHECK_EQ(r.line("c1600 cassette status"), no);
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette status"), no);
    CHECK_EQ(r.line("attach printer CE-1600P"), std::string("done\n"));   // its cassette
    CHECK_EQ(r.line("c1600 cassette status"), std::string("done 2\ncassette: empty\nmotor: off\n"));
}

TEST(unwritable_path_notice_at_motor_stop) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-5-ec6");
    writeFile(dir.path() / "file", "not a folder");
    const fs::path wav = dir.path() / "file" / "x.wav";   // its folder is a file
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 cassette record " + wav.string()), std::string("done\n"));   // the command itself is done
    r.line("state notices");
    csave(r);
    const std::string n = r.line("state notices");
    CHECK(starts(n, "done 1\n"));
    CHECK(has(n, "The cassette recording \xE2\x80\x9Cx.wav\xE2\x80\x9D could not be saved: "));
}

// ── serial ───────────────────────────────────────────────────────────────

namespace {
// A PC-1500A with a CE-158, the CE-158 echo program loaded, in RUN mode.
void echoProgram(Run& r) {
    CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
    r.line("key cl");
    r.line("type NEW0");
    CHECK_EQ(r.line("c1600 load-basic tests/fixtures/basic/ce158_echo.bas"), std::string("done\n"));
    CHECK_EQ(r.line("key cl mode"), std::string("done\n"));
}
} // namespace

// Feed: one byte in each character time (300 baud, ten bits: 43 333
// ticks of the PC-1500A's 1.3 MHz), the first `hold` of them silent.
TEST(feed_one_byte_per_character_time_and_hold) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-serial-feed");
    writeFile(dir.path() / "in.bin", std::string(200, 'x'));
    for (int hold : {0, 10}) {
        Run r;
        CHECK(r.ok());
        if (!r.ok()) return;
        CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
        const std::string line = "c1600 serial ce158 feed " + (dir.path() / "in.bin").string() + (hold ? " hold=10" : "");
        CHECK_EQ(r.line(line), std::string("done\n"));
        CHECK(starts(r.line("wait 1s"), "done 2\n"));   // 1 300 000 ticks
        const std::string rep = r.line("c1600 serial ce158 report");
        CHECK_EQ(field(rep, "speed"), std::string("300 baud"));
        // 1 300 000 ticks: 30 character times, ten of them silent with hold=10.
        CHECK_EQ(field(rep, "received"), (hold ? std::string("20") : std::string("30")) + " of 200 bytes fed");
    }
}

TEST(second_feed_replaces_rest) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-serial-second");
    writeFile(dir.path() / "a.bin", std::string(200, 'a'));
    writeFile(dir.path() / "b.bin", "HELLO\r");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    r.line("attach interface CE-158");
    r.line("c1600 serial ce158 feed " + (dir.path() / "a.bin").string());
    r.line("wait 100ms");
    CHECK_EQ(r.line("c1600 serial ce158 feed " + (dir.path() / "b.bin").string()), std::string("done\n"));
    r.line("wait 1s");
    CHECK_EQ(field(r.line("c1600 serial ce158 report"), "received"), std::string("6 of 6 bytes fed"));
}

// Capture and report, and the report's text: the CE-158 echo program sends its
// prompt, prints on the Centronics port, takes the line fed and prints it.
TEST(capture_complete_at_quit_and_switch) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-serial-capture");
    writeFile(dir.path() / "in.txt", "HELLO\r");
    {
        Run r;
        CHECK(r.ok());
        if (!r.ok()) return;
        echoProgram(r);
        CHECK_EQ(r.line("c1600 serial ce158 capture " + (dir.path() / "a.bin").string()), std::string("done\n"));
        CHECK(fs::exists(dir.path() / "a.bin") && fileText(dir.path() / "a.bin").empty());
        CHECK_EQ(r.line("type RUN"), std::string("done\n"));
        r.line("wait 2s");
        // Another capture file: the first is complete.
        CHECK_EQ(r.line("c1600 serial ce158 capture " + (dir.path() / "b.bin").string()), std::string("done\n"));
        CHECK_EQ(fileText(dir.path() / "a.bin"), std::string("TYPE A LINE AND PRESS RETURN\r"));
        CHECK_EQ(r.line("c1600 serial ce158 feed " + (dir.path() / "in.txt").string()), std::string("done\n"));
        r.line("wait 2s");
        CHECK_EQ(r.line("c1600 serial ce158 report"),
                 std::string("done 10\n"
                             "speed: 300 baud\n"
                             "bits: not set\n"
                             "used: yes\n"
                             "host link: no\n"
                             "received: 6 of 6 bytes fed\n"
                             "sent: 29 bytes\n"
                             "| TYPE A LINE AND PRESS RETURN\n"
                             "printer: 38 bytes\n"
                             "| HELLO FROM THE CE-158\n"
                             "| RECEIVED: HELLO\n"));
        CHECK(has(r.line("display-text"), "\n>\n"));   // the program ended
        // What the port sends now goes to b.bin, complete at the end of the run.
        r.line("key cl");
        r.line("type RUN");
        r.line("wait 2s");
    }
    CHECK_EQ(fileText(dir.path() / "b.bin"), std::string("TYPE A LINE AND PRESS RETURN\r"));
}

TEST(link_gives_the_path_and_refuses_feed) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
#ifdef _WIN32
    SKIP_UNLESS(false, "no pseudo-terminals on Windows");
#endif
    calcu::testing::TempFolder dir("c1600-serial-link");
    writeFile(dir.path() / "in.bin", "x");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    r.line("attach interface CE-158");
    CHECK_EQ(r.line("c1600 serial ce158 link"), std::string("done 1\npath: <save folder>/calcu1600-ce158.serial\n"));
    CHECK(fs::exists(fs::path(r.saveFolder()) / "calcu1600-ce158.serial"));
    const std::string refused = "refused: The port has a host link; feed and capture are refused for it.\n";
    CHECK_EQ(r.line("c1600 serial ce158 feed " + (dir.path() / "in.bin").string()), refused);
    CHECK_EQ(r.line("c1600 serial ce158 capture " + (dir.path() / "out.bin").string()), refused);
    CHECK_EQ(field(r.line("c1600 serial ce158 report"), "host link"), std::string("yes"));
}

TEST(report_ce158_with_printer_port) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    // The bundled example: it prints, sends its prompt and waits for a line.
    CHECK_EQ(r.line("preset examples:interfaces/ce158_demo.pc1500a"), std::string("done\n"));
    r.line("wait 2s");
    CHECK_EQ(r.line("c1600 serial ce158 report"),
             std::string("done 9\n"
                         "speed: 300 baud\n"
                         "bits: not set\n"
                         "used: yes\n"
                         "host link: no\n"
                         "received: 0 of 0 bytes fed\n"
                         "sent: 29 bytes\n"
                         "| TYPE A LINE AND PRESS RETURN\n"
                         "printer: 22 bytes\n"
                         "| HELLO FROM THE CE-158\n"));
}

TEST(rs232c_on_pc1600) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-serial-rs232c");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    // The bundled example sets COM1: up; RUN "S" saves the program over it.
    CHECK_EQ(r.line("preset examples:interfaces/setcom1600.pc1600"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 serial rs232c capture " + (dir.path() / "com1.txt").string()), std::string("done\n"));
    CHECK_EQ(r.line("type RUN \"S\""), std::string("done\n"));
    r.line("wait 5s");
    const std::string rep = r.line("c1600 serial rs232c report");
    CHECK_EQ(field(rep, "speed"), std::string("9600 baud"));
    CHECK_EQ(field(rep, "sent"), std::string("308 bytes"));
    CHECK(has(rep, "\n| 61010 SETCOM \"COM1:\",9600,8,N,1,N,N\n"));
    CHECK(has(rep, "\n| <1A>\n"));   // a byte that is no printable ASCII
    CHECK(!has(rep, "\nprinter: "));   // the CE-158's alone
    const std::string sent = fileText(dir.path() / "com1.txt");
    CHECK(sent.size() == 308 && sent.find("61110 SAVE \"COM1:\",A") != std::string::npos);
}

TEST(serial_text_rule_for_every_byte) {
    std::vector<std::uint8_t> all;
    for (int b = 0; b < 256; ++b) all.push_back(static_cast<std::uint8_t>(b));
    std::string want;
    for (int b = 0; b < 256; ++b) {
        if (b == '\r') continue;   // left out
        if (b == '\n' || (b >= 0x20 && b < 0x7F)) {
            want += static_cast<char>(b);
            continue;
        }
        char hex[8];
        std::snprintf(hex, sizeof hex, "<%02X>", b);
        want += hex;
    }
    CHECK_EQ(printableBytes(all.data(), all.size()), want);
    const std::uint8_t sample[] = {'A', '\r', '\n', 0x00, 0x7F, 0xFF};
    CHECK_EQ(printableBytes(sample, sizeof sample), std::string("A\n<00><7F><FF>"));
}

// No serial port has a host link unless `link` asked: no serial port
// file in the run's folders, the bytes the ports send stay with the run.
TEST(no_host_link_unless_asked) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));   // the RS-232C port, a file at once in a frontend
    CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
    r.line("wait 1s");
    CHECK_EQ(r.line("state c1600.serial-links"), std::string("done 1\n{\"rs232c\":\"\",\"ce158\":\"\",\"moves\":0}\n"));
    for (const auto& e : fs::recursive_directory_iterator(r.server->setup().runFolder()))
        CHECK(e.path().extension() != ".serial");
    CHECK_EQ(field(r.line("c1600 serial rs232c report"), "host link"), std::string("no"));
}

TEST(port_missing_refused_not_held) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder dir("c1600-5-ec3");
    writeFile(dir.path() / "in.bin", "x");
    const std::string feed = "c1600 serial ce158 feed " + (dir.path() / "in.bin").string();
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line(feed), std::string("refused: The machine has no CE-158.\n"));
    CHECK_EQ(r.line("c1600 serial rs232c report"),
             std::string("refused: The machine has no RS-232C port: only the PC-1600 has one.\n"));
    CHECK_EQ(r.line("model PC-1500"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 serial rs232c link"),
             std::string("refused: The machine has no RS-232C port: only the PC-1600 has one.\n"));
    // Not held for a machine built later: the CE-158 plugged in now has no feed.
    CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
    r.line("wait 500ms");
    CHECK_EQ(field(r.line("c1600 serial ce158 report"), "received"), std::string("0 of 0 bytes fed"));
    CHECK(starts(r.line("c1600 serial ce158 jump"), "refused: usage: "));
}

// ── host-drive ───────────────────────────────────────────────────────────

TEST(host_drive_attach_folder_and_off) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder one("c1600-host-drive-one"), two("c1600-host-drive-two");
    writeFile(one.path() / "HELLO.TXT", "hi\n");
    writeFile(two.path() / "OTHER.TXT", "hi\n");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 host-drive " + one.str()), std::string("done\n"));
    CHECK(has(r.line("capabilities"), "place host-drive host-drive\n"));
    // A listing of S3: as help/09-host-drive.md describes it.
    CHECK_EQ(r.line("type FILES \"S3:\""), std::string("done\n"));
    r.line("wait");
    CHECK(has(r.line("display-text"), "\nHELLO   .TXT,"));
    // Attached already, it shows the other folder from then on.
    CHECK_EQ(r.line("c1600 host-drive " + two.str()), std::string("done\n"));
    r.line("type FILES \"S3:\"");
    r.line("wait");
    const std::string lcd = r.line("display-text");
    CHECK(has(lcd, "\nOTHER   .TXT,") && !has(lcd, "HELLO"));
    CHECK_EQ(r.line("c1600 host-drive off"), std::string("done\n"));
    CHECK(has(r.line("capabilities"), "place host-drive empty\n"));
}

// A command typed in direct mode reads the keyboard for BREAK while it works,
// as the prompt does; `wait` still lasts until it has ended: the file is
// whole after SAVE, the program whole after LOAD.
TEST(wait_lasts_until_a_direct_save_and_load_end) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder drive("c1600-host-drive-wait");
    std::string program;
    for (int i = 1; i <= 200; ++i)
        program += std::to_string(10 * i) + " PRINT \"LINE " + std::to_string(i) + " ABCDEFGHIJKLMNOPQRSTUVWXYZ\"\n";
    writeFile(drive.path() / "p.bas", program);
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("c1600 host-drive " + drive.str()), std::string("done\n"));
    r.line("wait");
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
    CHECK(starts(r.line("c1600 load-basic " + (drive.path() / "p.bas").string()), "done"));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    r.line("wait");
    CHECK_EQ(r.line("type SAVE\"S3:P.BAS\""), std::string("done\n"));
    r.line("wait");
    const auto saved = fs::file_size(drive.path() / "P.BAS");
    r.line("wait 5s");
    CHECK_EQ(fs::file_size(drive.path() / "P.BAS"), saved);
    CHECK(has(r.line("display-text"), "\n>\n"));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    CHECK_EQ(r.line("type LOAD\"S3:P.BAS\""), std::string("done\n"));
    r.line("wait");
    CHECK(has(r.line("display-text"), "\n>\n"));
    CHECK(has(r.line("c1600 list-basic"), "\n2000 PRINT \"LINE 200 "));
}

TEST(refused_on_pc1500_and_for_no_folder) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder one("c1600-5-ec5");
    writeFile(one.path() / "file", "x");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    for (const char* model : {"PC-1500", "PC-1500A"}) {
        CHECK_EQ(r.line(std::string("model ") + model), std::string("done\n"));
        CHECK_EQ(r.line("c1600 host-drive " + one.str()), std::string("refused: The host drive is a PC-1600 peripheral.\n"));
    }
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    const std::string notFolder = (one.path() / "file").string();
    CHECK_EQ(r.line("c1600 host-drive " + notFolder), "refused: The path " + notFolder + " is no folder.\n");
    CHECK(has(r.line("capabilities"), "place host-drive empty\n"));
}

// ── call ─────────────────────────────────────────────────────────────────

namespace {
// The registers of the reply's lines "<name> = <hex>".
std::string reg(const std::string& reply, const std::string& name) {
    const std::string k = "\n" + name + " = ";
    const auto at = reply.find(k);
    if (at == std::string::npos) return {};
    return reply.substr(at + k.size(), reply.find('\n', at + k.size()) - at - k.size());
}
} // namespace

// LH5801 `LDI A,42H` `RTN` at 4800H; Z-80 `LD A,42H` `RET` at D000H. The
// routine returns to the caller: the program counter and the stack pointer
// as before, A = 42H; a second run gives the same reply.
TEST(call_returns_registers_and_time) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::string first;
    for (int round = 0; round < 2; ++round) {
        Run r;
        CHECK(r.ok());
        if (!r.ok()) return;
        CHECK(starts(r.line("pause"), "done 1\npaused"));
        const std::string before = r.line("registers LH5801");
        CHECK_EQ(r.line("poke LH5801 me0 0x4800 0xB5 0x42 0x9A"), std::string("done\n"));
        const std::string call = r.line("c1600 call LH5801 0x4800");
        CHECK(starts(call, "done 13\nreturned: yes\n"));
        CHECK_EQ(reg(call, "a"), std::string("0x42"));
        CHECK_EQ(reg(call, "p"), reg(before, "p"));
        CHECK_EQ(reg(call, "s"), reg(before, "s"));
        CHECK(field(call, "ticks") != "" && field(call, "ticks") != "0");
        CHECK(starts(r.line("state run-state"), "done 1\n{\"paused\":true"));
        if (round == 0) first = call;
        else CHECK_EQ(call, first);   // the same reply in another run
    }
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    r.line("pause");
    const std::string before = r.line("registers SC7852");
    CHECK_EQ(r.line("poke SC7852 memory 0xD000 0x3E 0x42 0xC9"), std::string("done\n"));
    const std::string call = r.line("c1600 call sc7852 0xD000");   // the target in any case
    CHECK(starts(call, "done 20\nreturned: yes\n"));
    CHECK_EQ(reg(call, "af").substr(0, 4), std::string("0x42"));
    CHECK_EQ(reg(call, "pc"), reg(before, "pc"));
    CHECK_EQ(reg(call, "sp"), reg(before, "sp"));
}

// A routine that never returns (`JMP 4810H` on itself): past the timeout
// the machine is paused and the reply says so. The PC-1600's LH5803 does
// not run while the Z-80 runs the machine, so a call on it ends the same way.
TEST(timeout_pauses_and_says_so) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    r.line("pause");
    CHECK_EQ(r.line("poke LH5801 me0 0x4810 0xBA 0x48 0x10"), std::string("done\n"));
    const std::string call = r.line("c1600 call LH5801 0x4810 timeout=100ms");
    CHECK(starts(call, "done 13\nreturned: no, timeout\n"));
    CHECK_EQ(reg(call, "p"), std::string("0x4810"));
    const long long ticks = std::stoll(field(call, "ticks"));   // 100 ms of 1.3 MHz, to the end of an instruction
    CHECK(ticks >= 130000 && ticks < 130100);
    CHECK(starts(r.line("state run-state"), "done 1\n{\"paused\":true"));
    CHECK(starts(r.line("c1600 call LH5801 0x4810 timeout=1"), "refused: timeout= takes a duration"));
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    r.line("pause");
    r.line("poke LH5803 me0 0x5100 0xB5 0x42 0x9A");
    const std::string sub = r.line("c1600 call LH5803 0x5100 timeout=100ms");
    CHECK(starts(sub, "done 13\nreturned: no, timeout\n"));
    CHECK_EQ(reg(sub, "p"), std::string("0x5100"));
}

TEST(refused_unless_paused) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("c1600 call LH5801 0x4800"), std::string("refused: The machine is not paused (pause first).\n"));
    r.line("pause");
    CHECK_EQ(r.line("c1600 call Z80 0x4800"), std::string("refused: The machine has no debug target Z80.\n"));
    CHECK_EQ(r.line("c1600 call LH5801 0x12345"), std::string("refused: The address 0x12345 is no address of 16 bits.\n"));
    CHECK(starts(r.line("c1600 call LH5801"), "refused: usage: "));
}
