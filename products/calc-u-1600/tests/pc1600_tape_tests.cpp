// Cassette CLOAD / CSAVE on the PC-1600, end to end against the real ROMs;
// the recorder is the machine's TapeDeck. SKIPs (not fails) when the ROM
// images are absent.
// - MODE 0 through the CE-1600P's cassette interface: the bank-5 tape
//   driver bit-bangs OPC 18H b7 and times PB2 itself.
// - MODE 1 through the CE-150: the LH5803 runs the CE-150's tape code,
//   which drives the port block's serial transmitter (SDO) and reads PB2
//   against the CL1 timer -- PC-1500 tapes.
//
// The fixture in fixtures/tape/ was made with Pocket Tools 2.1.1:
//   bas2img -p 1600 pc1600_tape.bas pc1600_tape.img
//   bin2wav -p 1600 -l 3 -s 3 -nTAPEFIX pc1600_tape.img pc1600_tape.wav
// (-s 3: a 3 s leader. bin2wav's default for the PC-1600, ~2.1 s (6406
// cycles, no gap before it), is too short once the ROM's ~0.6 s motor
// start-up delay has eaten into it -- CMSYNC needs 5000 leader cycles
// after that, the ROM's own CSAVE writes 10000.)
//
// Build & run: see tools/run_tests.sh

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "../core/PC1600/PC1600LcdText.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"

namespace {

constexpr uint64_t kSecond = PC1600Machine::kTStateHz;
constexpr uint16_t kProgramStart = 0xC0C5; // S0 BASIC program, Z-80 view
const char* kFixtureWav = "tests/fixtures/tape/pc1600_tape.wav";
const char* kFixtureImg = "tests/fixtures/tape/pc1600_tape.img";
const char* kPc1500Wav = "tests/fixtures/tape/pc1500_tape.wav";
const char* kPc1500Img = "tests/fixtures/tape/pc1500_tape.img";

std::vector<uint8_t> readFile(const char* path) {
    std::vector<uint8_t> bytes;
    CHECK(readRomImage(path, &bytes));
    return bytes;
}

bool bootWithCE1600P(PC1600Machine& m) {
    std::string error;
    if (!loadPC1600Roms(m) || !presetroms::attachCE1600P(m, testPresetRoms(), "new", &error)) return false;
    m.coldBoot();
    sharptest::runUntilKeyReady(m);
    sharptest::tap(m, "mode"); // RUN -> PRO
    sharptest::runUntilSettled(m, double(kSecond) / m.descriptor().tickRate);
    return true;
}

// Types `line` + ENTER and runs until the BASIC prompt is back (a tape
// command takes tens of emulated seconds).
void type(PC1600Machine& m, const std::string& line) {
    std::string err;
    CHECK(sharptest::typeLine(m, line));
    sharptest::runUntilFinished(m, double(300 * kSecond) / m.descriptor().tickRate);
    sharptest::runUntilSettled(m, double(kSecond) / m.descriptor().tickRate);
}

std::string screen(PC1600Machine& m) { return pc1600LcdText(m).plainText(); }

bool programIs(PC1600Machine& m, const std::vector<uint8_t>& image) {
    for (size_t i = 0; i < image.size(); ++i)
        if (m.debugPeek(static_cast<uint16_t>(kProgramStart + i)) != image[i]) return false;
    return true;
}

std::vector<float> asPlayback(const std::vector<int16_t>& recording) {
    std::vector<float> out;
    out.reserve(recording.size());
    for (int16_t v : recording) out.push_back(v / 32768.0f);
    return out;
}

// A bin2wav tape CLOADs into exactly bas2img's program image.
TEST(test_pc1600_cload_bin2wav_tape) {
    const std::vector<uint8_t> image = readFile(kFixtureImg);
    PC1600Machine m;
    SKIP_UNLESS(!image.empty() && bootWithCE1600P(m), "PC-1600 ROM images or fixture not found");
    type(m, "NEW0");
    std::string error;
    CHECK(m.tapePlay(kFixtureWav, error));
    CHECK(!m.tapeStatus().motor); // nothing runs the tape before CLOAD
    const uint64_t buzzerEdges = m.memory().piezo().edgeCount();
    type(m, "CLOAD");
    // The tape sounds on the buzzer through its gate (PB2), as on a real
    // unit.
    CHECK(m.memory().piezo().edgeCount() - buzzerEdges > 10000);
    const std::string lcd = screen(m);
    CHECK(lcd.find("TAPEFIX") != std::string::npos);
    CHECK(lcd.find("ERROR") == std::string::npos);
    CHECK(programIs(m, image));
    CHECK(!m.tapeStatus().motor); // the ROM stops the motor again
    CHECK(m.tapeStatus().position > 5.0);
    if (lcd.find("TAPEFIX") == std::string::npos) std::fprintf(stderr, "  screen:\n%s", lcd.c_str());
}

// CSAVE onto a blank tape, then CLOAD that recording into a fresh machine.
TEST(test_pc1600_csave_cload_round_trip) {
    const std::vector<uint8_t> image = readFile(kFixtureImg);
    PC1600Machine a;
    SKIP_UNLESS(!image.empty() && bootWithCE1600P(a), "PC-1600 ROM images or fixture not found");
    type(a, "NEW0");
    std::string error;
    CHECK(a.tapePlay(kFixtureWav, error));
    type(a, "CLOAD");
    a.tapeRecord("");
    type(a, "CSAVE \"ROUNDTRIP\"");
    CHECK(screen(a).find("ERROR") == std::string::npos);
    const std::vector<int16_t> recording = a.tapeDeck().recording();
    CHECK(recording.size() > 10 * TapeDeck::kRecordSampleRate);

    PC1600Machine b;
    CHECK(bootWithCE1600P(b));
    type(b, "NEW0");
    b.tapeDeck().loadForPlay(asPlayback(recording), TapeDeck::kRecordSampleRate);
    type(b, "CLOAD \"ROUNDTRIP\"");
    const std::string lcd = screen(b);
    CHECK(lcd.find("ROUNDTRIP") != std::string::npos);
    CHECK(lcd.find("ERROR") == std::string::npos);
    CHECK(programIs(b, image));

    // CLOAD? compares the tape with memory.
    b.tapeDeck().loadForPlay(asPlayback(recording), TapeDeck::kRecordSampleRate);
    type(b, "CLOAD? \"ROUNDTRIP\"");
    CHECK(screen(b).find("ERROR") == std::string::npos);

    // A tape with a dropout in its data doesn't load quietly. (Inverting
    // the signal wouldn't do: the ROM classifies either half-cycle the
    // same way.) The data block ends where the signal does; 0.2 s of
    // silence a second before that is well inside it.
    std::vector<int16_t> damaged = recording;
    size_t end = damaged.size();
    while (end > 0 && std::abs(damaged[end - 1]) < 1000) --end;
    const size_t rate = TapeDeck::kRecordSampleRate;
    CHECK(end > 2 * rate);
    for (size_t i = end - rate; i < end - rate + rate / 5; ++i) damaged[i] = 0;
    b.tapeDeck().loadForPlay(asPlayback(damaged), TapeDeck::kRecordSampleRate);
    type(b, "CLOAD? \"ROUNDTRIP\"");
    CHECK(screen(b).find("ERROR") != std::string::npos);
}

// PC-1600 + CE-150, MODE 1 (the LH5803 runs PC-1500 BASIC), NEW0.
bool bootMode1WithCE150(PC1600Machine& m) {
    std::vector<uint8_t> ce150;
    if (!loadPC1600Roms(m) || !readRomImage("roms/CE-150.ROM", &ce150)) return false;
    if (!m.attachCE150(ce150.data(), ce150.size())) return false;
    m.coldBoot();
    sharptest::runUntilKeyReady(m);
    sharptest::tap(m, "mode"); // RUN -> PRO
    sharptest::runUntilSettled(m, double(kSecond) / m.descriptor().tickRate);
    type(m, "MODE 1");
    type(m, "NEW0");
    return true;
}

// The MODE 1 program area, from the pointers the LH5803 shares with the
// SC7852 (F865H, big-endian, an LH5803 address; the SC7852 sees it +8000H).
bool mode1ProgramIs(PC1600Machine& m, const std::vector<uint8_t>& image) {
    const uint16_t start = static_cast<uint16_t>(m.debugPeek(0xF865) << 8 | m.debugPeek(0xF866));
    for (size_t i = 0; i < image.size(); ++i)
        if (m.debugPeek(static_cast<uint16_t>(start + 0x8000 + i)) != image[i]) return false;
    return true;
}

// A PC-1500 tape (bin2wav -p 1500) CLOADs in MODE 1 through the CE-150.
TEST(test_pc1600_mode1_ce150_cloads_pc1500_tape) {
    const std::vector<uint8_t> image = readFile(kPc1500Img);
    PC1600Machine m;
    SKIP_UNLESS(!image.empty() && bootMode1WithCE150(m), "PC-1600 or CE-150 ROM images or fixture not found");
    std::string error;
    CHECK(m.tapePlay(kPc1500Wav, error));
    type(m, "CLOAD");
    const std::string lcd = screen(m);
    CHECK(lcd.find("ERROR") == std::string::npos);
    CHECK(mode1ProgramIs(m, image));
    CHECK(!m.tapeStatus().motor);
    if (!mode1ProgramIs(m, image)) std::fprintf(stderr, "  screen:\n%s", lcd.c_str());
}

// CSAVE in MODE 1 through the CE-150 writes the PC-1500's tones (2539 /
// 1270 Hz, as a real unit records them) and a tape that CLOADs back.
TEST(test_pc1600_mode1_ce150_csave_cload_round_trip) {
    const std::vector<uint8_t> image = readFile(kPc1500Img);
    PC1600Machine a;
    SKIP_UNLESS(!image.empty() && bootMode1WithCE150(a), "PC-1600 or CE-150 ROM images or fixture not found");
    std::string error;
    CHECK(a.tapePlay(kPc1500Wav, error));
    type(a, "CLOAD");
    a.tapeRecord("");
    type(a, "CSAVE \"ROUNDTRIP\"");
    CHECK(screen(a).find("ERROR") == std::string::npos);
    const std::vector<int16_t> recording = a.tapeDeck().recording();
    CHECK(recording.size() > 8 * TapeDeck::kRecordSampleRate);
    // The leader's tone: zero crossings over one second of it, 2 s in.
    const size_t rate = TapeDeck::kRecordSampleRate;
    size_t first = 0;
    while (first < recording.size() && std::abs(recording[first]) < 1000) ++first;
    size_t crossings = 0;
    for (size_t i = first + 2 * rate; i + 1 < first + 3 * rate && i + 1 < recording.size(); ++i)
        if ((recording[i] < 0) != (recording[i + 1] < 0)) ++crossings;
    CHECK(crossings > 2 * 2500 && crossings < 2 * 2580);
    if (!(crossings > 2 * 2500 && crossings < 2 * 2580)) std::fprintf(stderr, "  leader: %zu crossings/s\n", crossings);

    PC1600Machine b;
    CHECK(bootMode1WithCE150(b));
    b.tapeDeck().loadForPlay(asPlayback(recording), TapeDeck::kRecordSampleRate);
    type(b, "CLOAD \"ROUNDTRIP\"");
    CHECK(screen(b).find("ERROR") == std::string::npos);
    CHECK(mode1ProgramIs(b, image));
}

// The recording is a function of emulated time only: running the same
// CSAVE in small or large slices (as GUI pacing, full speed and the harness do)
// writes the identical tape.
std::vector<int16_t> csaveInSlices(uint64_t slice) {
    PC1600Machine m;
    if (!bootWithCE1600P(m)) return {};
    std::string err;
    sharptest::typeLine(m, "10 PRINT \"SPEED\"");
    sharptest::runUntilFinished(m, double(10 * kSecond) / m.descriptor().tickRate);
    m.tapeRecord("");
    sharptest::typeLine(m, "CSAVE \"S\"");
    for (uint64_t run = 0; run < 30 * kSecond; run += slice) m.runCycles(slice);
    return m.tapeDeck().recording();
}

TEST(test_recording_independent_of_pacing) {
    const std::vector<int16_t> small = csaveInSlices(997);
    SKIP_UNLESS(!small.empty(), "PC-1600 ROM images not found");
    const std::vector<int16_t> large = csaveInSlices(kSecond / 3);
    CHECK(small.size() > 5 * TapeDeck::kRecordSampleRate);
    const size_t n = std::min(small.size(), large.size());
    CHECK(std::equal(small.begin(), small.begin() + n, large.begin()));
}

} // namespace
