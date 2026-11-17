// C1600-5 design §6, T8 (AC-13; LIB-11 AC-24): the machine state of the
// Sharp models through `state-save` / `state-restore`, each test on
// `calcu --product c1600` runs of its own (tests/harness/HarnessRun.hpp).
// A state saved while a program runs, then 2 s of emulated time; restored,
// in the same run and in another, the same 2 s give the same display, the
// same ticks and the same sound. A program of its own (CTest
// CalcU1600MachineStateTests, Longer), so that CoreTests' budget stays.
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "harness/HarnessRun.hpp"

using namespace harnesstest;
using calcu::testing::TempFolder;

namespace {

std::string fileBytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// What a run gives in `seconds` after a save or a restore: the replies of
// the display, the time and `more`, the sound as a WAV, and the whole state
// at the end (every part of the core, the media with them).
struct After {
    std::vector<std::string> replies;
    std::string sound;
    std::string state;
    bool operator==(const After&) const = default;
};

After after(Run& r, const fs::path& folder, const std::string& name, const std::string& seconds,
            const std::vector<std::string>& more) {
    After a;
    const fs::path wav = folder / (name + ".wav");
    const fs::path state = folder / (name + ".bin");
    CHECK_EQ(r.line("audio " + wav.string()), std::string("done\n"));
    a.replies.push_back(r.line("wait " + seconds));
    for (const char* l : {"display-text", "display", "ticks"}) a.replies.push_back(r.line(l));
    for (const std::string& l : more) a.replies.push_back(r.line(l));
    CHECK_EQ(r.line("audio " + (folder / "closed.wav").string()), std::string("done\n"));   // the WAV above complete
    a.sound = fileBytes(wav);
    CHECK(a.sound.size() > 44);   // more than a header
    CHECK_EQ(r.line("state-save " + state.string()), std::string("done\n"));
    a.state = fileBytes(state);
    return a;
}

void checkSame(const After& got, const After& want) {
    CHECK_EQ(got.replies.size(), want.replies.size());
    for (std::size_t i = 0; i < want.replies.size() && i < got.replies.size(); ++i) CHECK_EQ(got.replies[i], want.replies[i]);
    CHECK(got.sound == want.sound);
    CHECK(got.state == want.state);
}

// The BASIC loop of the tests: a count on the display and a beep each time.
void runCountingLoop(Run& r) {
    CHECK_EQ(r.line("key cl"), std::string("done\n"));
    CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
    CHECK_EQ(r.line("type 10 WAIT 0:A=A+1:PRINT A:BEEP 1:GOTO 10"), std::string("done\n"));
    CHECK_EQ(r.line("key mode"), std::string("done\n"));
    CHECK_EQ(r.line("type RUN"), std::string("done\n"));
    r.line("wait 700ms");
}

// Save; the seconds after the save; the same after a restore in the same
// run and after a restore in another run set up by `setUp`. Saving right
// after a restore gives the bytes restored (a save only reads).
template <class SetUp>
void restoreContinuesIdentically(SetUp setUp, const std::vector<std::string>& more = {}) {
    TempFolder f;
    const std::string state = (f.path() / "state.bin").string();
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    setUp(r, f.path());
    CHECK_EQ(r.line("state-save " + state), std::string("done\n"));
    const After first = after(r, f.path(), "first", "2s", more);
    CHECK(starts(first.replies[0], "done "));
    CHECK_EQ(r.line("state-restore " + state), std::string("done\n"));
    checkSame(after(r, f.path(), "again", "2s", more), first);

    // In another run, set up the same way, and later.
    Run other;
    CHECK(other.ok());
    if (!other.ok()) return;
    setUp(other, f.path());
    other.line("wait 1337ms");   // another history than the first run's
    CHECK_EQ(other.line("state-restore " + state), std::string("done\n"));
    checkSame(after(other, f.path(), "elsewhere", "2s", more), first);

    const std::string again = (f.path() / "again.bin").string();
    CHECK_EQ(other.line("state-restore " + state), std::string("done\n"));
    CHECK_EQ(other.line("state-save " + again), std::string("done\n"));
    CHECK(fileBytes(again) == fileBytes(state));
}

void countingOn(const std::string& model) {
    restoreContinuesIdentically([&](Run& r, const fs::path&) {
        CHECK_EQ(r.line("model " + model), std::string("done\n"));
        runCountingLoop(r);
    });
}

} // namespace

TEST(c1600_5_ac13_restore_continues_identically_pc1500) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    countingOn("PC-1500");
}

TEST(c1600_5_ac13_restore_continues_identically_pc1500a) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    countingOn("PC-1500A");
    // With what plugs into a PC-1500A: the CE-150 recording a CSAVE on its
    // cassette, the CE-158, a module.
    restoreContinuesIdentically(
        [](Run& r, const fs::path& folder) {
            CHECK_EQ(r.line("model PC-1500A"), std::string("done\n"));
            CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
            CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
            CHECK_EQ(r.line("attach slot-1 CE-161"), std::string("done\n"));
            CHECK_EQ(r.line("key cl"), std::string("done\n"));
            CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
            CHECK_EQ(r.line("type 10 A=A+1:PRINT A:BEEP 1:GOTO 10"), std::string("done\n"));
            CHECK_EQ(r.line("key mode"), std::string("done\n"));
            CHECK(starts(r.line("c1600 cassette record " + (folder / "tape.wav").string()), "done"));
            CHECK_EQ(r.line("type CSAVE \"A\""), std::string("done\n"));
            r.line("wait 500ms");
            CHECK(has(r.line("c1600 cassette status"), "motor: on"));   // saved while the CSAVE records
        },
        {"c1600 cassette status", "c1600 serial ce158 report", "c1600 inspect inventory", "c1600 dump-basic"});
}

TEST(c1600_5_ac13_restore_continues_identically_pc1600) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    countingOn("PC-1600");
}

// Everything that can be plugged into a PC-1600 at once, with its media:
// the CE-1600P with its floppy disk (side 1) and a tape recording a CSAVE,
// a module in each slot, the host drive. The CE-158 cannot be used with the
// CE-1600P (the model's rule), so a second machine has it with the CE-150
// and its cassette.
TEST(c1600_5_ac13_full_pc1600_with_ce1600p_disk_tape_ce158_modules) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    restoreContinuesIdentically(
        [](Run& r, const fs::path& folder) {
            CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
            CHECK_EQ(r.line("attach printer CE-1600P"), std::string("done\n"));
            CHECK_EQ(r.line("attach slot-1 CE-1600M"), std::string("done\n"));
            CHECK_EQ(r.line("attach slot-2 CE-161"), std::string("done\n"));
            CHECK_EQ(r.line("floppy { name: Formatted, side: 1 }"), std::string("done\n"));
            fs::create_directories(folder / "drive");
            CHECK(starts(r.line("c1600 host-drive " + (folder / "drive").string()), "done"));
            CHECK_EQ(r.line("key cl"), std::string("done\n"));
            CHECK_EQ(r.line("type NEW0"), std::string("done\n"));
            CHECK_EQ(r.line("type 10 A=A+1:PRINT A:BEEP 1:GOTO 10"), std::string("done\n"));
            CHECK_EQ(r.line("key mode"), std::string("done\n"));
            CHECK(starts(r.line("c1600 cassette record " + (folder / "tape.wav").string()), "done"));
            CHECK_EQ(r.line("type CSAVE \"CAS:P\""), std::string("done\n"));
            r.line("wait 500ms");
            CHECK(has(r.line("c1600 cassette status"), "motor: on"));   // saved while the CSAVE records
        },
        {"c1600 cassette status", "c1600 inspect disks", "c1600 inspect inventory", "c1600 dump-basic"});

    restoreContinuesIdentically(
        [](Run& r, const fs::path&) {
            CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
            CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
            CHECK_EQ(r.line("attach interface CE-158"), std::string("done\n"));
            CHECK_EQ(r.line("attach slot-1 CE-1600M"), std::string("done\n"));
            CHECK_EQ(r.line("attach slot-2 CE-1601M"), std::string("done\n"));
            runCountingLoop(r);
        },
        {"c1600 cassette status", "c1600 serial ce158 report", "c1600 inspect inventory"});
}

// LIB-11 AC-24: a state of another set of plugged peripherals, or of
// another model, is refused and changes nothing.
TEST(c1600_5_ac13_refused_for_other_peripherals) {
    SKIP_UNLESS(romsPresent(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    TempFolder f;
    const std::string state = (f.path() / "state.bin").string();
    {
        Run r;
        CHECK(r.ok());
        if (!r.ok()) return;
        CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
        CHECK_EQ(r.line("attach printer CE-1600P"), std::string("done\n"));
        CHECK_EQ(r.line("state-save " + state), std::string("done\n"));
    }
    const std::string refusal =
        "refused: The file is of another product version, model, variant or set of plugged peripherals.\n";
    Run r;
    CHECK(r.ok());
    if (!r.ok()) return;
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    r.line("wait 100ms");
    const std::string ticks = r.line("ticks");
    const std::string lcd = r.line("display-text");
    CHECK_EQ(r.line("state-restore " + state), refusal);   // no printer
    CHECK_EQ(r.line("ticks"), ticks);
    CHECK_EQ(r.line("display-text"), lcd);
    CHECK_EQ(r.line("attach printer CE-150"), std::string("done\n"));
    CHECK_EQ(r.line("state-restore " + state), refusal);   // another printer
    CHECK_EQ(r.line("detach printer"), std::string("done\n"));
    CHECK_EQ(r.line("attach printer CE-1600P:old"), std::string("done\n"));
    CHECK_EQ(r.line("state-restore " + state), refusal);   // its other variant
    CHECK_EQ(r.line("model PC-1500A"), std::string("done\n"));
    CHECK_EQ(r.line("state-restore " + state), refusal);   // another model
    CHECK_EQ(r.line("model PC-1600"), std::string("done\n"));
    CHECK_EQ(r.line("attach printer CE-1600P"), std::string("done\n"));
    CHECK_EQ(r.line("state-restore " + state), std::string("done\n"));   // the same set again

    // A file that is no state, and a state cut short behind the header.
    const std::string junk = (f.path() / "junk.bin").string();
    { std::ofstream(junk, std::ios::binary) << "not a state"; }
    CHECK_EQ(r.line("state-restore " + junk), std::string("refused: The file holds no machine state.\n"));
    const std::string bytes = fileBytes(state);
    const std::string cut = (f.path() / "cut.bin").string();
    { std::ofstream(cut, std::ios::binary) << bytes.substr(0, bytes.size() / 2); }
    const std::string before = r.line("ticks");
    CHECK(starts(r.line("state-restore " + cut), "refused: The state does not fit this machine: "));
    CHECK_EQ(r.line("ticks"), before);
}
