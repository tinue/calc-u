// Headless C++ tests for tests/support/PC1600BasicTyper.cpp -- the PC-1600
// analog of basictyper_tests.cpp.
//
// The PC-1600's native BASIC does not tokenize (a line is stored as plain
// ASCII), so typeBasicProgramText() is a thin loop over typeLine() with an
// up-front line-length guard -- there is no program-end pointer to watch.
// The length-guard logic is covered here without a ROM; the ROM-gated
// tests confirm typed input (case preserved) reaches the ROM's line
// editor. Full "program stored correctly in PRO mode" validation is a
// manual/GUI check -- locating the PC-1600 native-BASIC program store
// needs ROM disassembly and is out of scope here.
//
// ROM-gated tests need the confirmed PC-1600 ROM set at its repo-root path
// (roms/PC1600-*.bin); they SKIP (not fail) when the images are absent.
//
// Build & run: see tools/run_tests.sh

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "support/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"

namespace {

// No ROM needed: an untypeable character fails the line cleanly with an
// error rather than half-typing it.
TEST(test_typeline_rejects_untypeable_char) {
    PC1600Machine m;
    std::string err;
    CHECK(!typeLine(m, std::string("a\x01""b"), /*pressEnter=*/true, &err));
    CHECK(!err.empty());
}

// No ROM needed: the up-front length guard flags an over-limit line
// without typing it. (Without a ROM the program-end pointer never moves,
// so the fitting lines get flagged too -- that path is covered by the
// ROM-gated tests below; here we only assert the over-limit line is
// caught and never typed.)
TEST(test_typebasicprogram_length_guard) {
    PC1600Machine m; // no ROM: keystrokes just spin the CPU

    std::string tooLong = "10 REM ";
    while (static_cast<int>(tooLong.size()) <= kMaxBasicLineLength) tooLong += 'X';

    BasicTypeResult r = typeBasicProgramText(m, tooLong + "\n");
    CHECK(!r.ok);
    CHECK(r.rejectedLines.size() == 1);
    CHECK(!r.rejectedLines.empty() && r.rejectedLines[0] == tooLong);
    CHECK(!r.error.empty());
}

// ROM-gated: in PRO mode a fitting program loads (every line advances
// BASPRG_END), and an over-length line is the only one rejected.
TEST(test_typebasicprogram_pro_mode_load) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    tapKey(m, "mode"); // RUN -> PRO
    m.runCycles(static_cast<uint64_t>(PC1600Machine::kTStateHz) / 2);

    std::string tooLong = "30 REM ";
    while (static_cast<int>(tooLong.size()) <= kMaxBasicLineLength) tooLong += 'X';

    BasicTypeResult r = typeBasicProgramText(
        m, "10 PRINT 12345\n20 GOTO 10\n" + tooLong + "\n40 END\n");
    CHECK(!r.ok);                                // the one over-length line
    CHECK(r.rejectedLines.size() == 1);
    CHECK(!r.rejectedLines.empty() && r.rejectedLines[0] == tooLong);
}

// ROM-gated: re-typing a line number with a same-length body replaces the
// line without moving BASPRG_END -- that is still a stored line.
TEST(test_pc1600_typebasicprogram_same_length_replacement_is_stored) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    tapKey(m, "mode"); // RUN -> PRO
    m.runCycles(static_cast<uint64_t>(PC1600Machine::kTStateHz) / 2);

    BasicTypeResult r = typeBasicProgramText(m, "10 A=1\n10 A=2\n");
    CHECK(r.ok);
    CHECK(r.rejectedLines.empty());
}

// ROM-gated: called in RUN mode (no `key: mode`), the editor stores
// nothing -- every line is reported rejected and the error names PRO mode.
TEST(test_typebasicprogram_detects_run_mode) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    BasicTypeResult r = typeBasicProgramText(m, "10 PRINT 1\n20 PRINT 2\n");
    CHECK(!r.ok);
    CHECK(r.rejectedLines.size() == 2);
    CHECK(r.error.find("PRO mode") != std::string::npos);
}

// typeLine() drives the ROM's line editor: a typed run reaches the console
// input buffer (FBB0H-FBFFH -- the same buffer pc1600_preset_tests.cpp's
// shifted-punctuation test reads), with lowercase preserved via the
// SHIFT-tap path rather than silently uppercased (the pre-typer raw-
// keystroke behaviour). The ROM materialises this buffer on the line
// commit (ENTER), so type WITH enter and read it back after.
TEST(test_typeline_lowercase_reaches_input_buffer) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    std::string err;
    CHECK(typeLine(m, "abcXYZ", /*pressEnter=*/true, &err));

    std::string buf;
    for (uint16_t a = 0xFBB0; a <= 0xFBFF; ++a) buf.push_back(static_cast<char>(m.memory().read(a)));
    CHECK(buf.find("abcXYZ") != std::string::npos);
    // ...not the fully-uppercased form.
    CHECK(buf.find("ABCXYZ") == std::string::npos);
}

// ROM-gated: a typed '^' reaches the console input buffer as a real caret
// byte (0x5E), same check shape as the lowercase test above.
TEST(test_typeline_caret_reaches_input_buffer) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    std::string err;
    CHECK(typeLine(m, "X^Y", /*pressEnter=*/true, &err));

    std::string buf;
    for (uint16_t a = 0xFBB0; a <= 0xFBFF; ++a) buf.push_back(static_cast<char>(m.memory().read(a)));
    CHECK(buf.find("X^Y") != std::string::npos);
}

std::string inputBuffer(PC1600Machine& m) {
    std::string buf;
    for (uint16_t a = 0xFBB0; a <= 0xFBFF; ++a) buf.push_back(static_cast<char>(m.memory().read(a)));
    return buf;
}

// ROM-gated: accented characters (UTF-8) are typed through the ROM's KBII
// tables, case-exact: `é` is KBII SHIFT S (82H), `É` KBII S (90H), `Ë`
// (no uppercase in the ROM) falls back to ë (89H), and `û` / `«` exist only
// as SHIFT+KBII (96H / AEH). Plain letters keep their case around them, and
// KBII (RAM F3C6H bit 7) is off again afterwards.
TEST(test_typeline_kbii_chars_reach_input_buffer) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    std::string err;
    CHECK(typeLine(m, "m\xC3\xA9rci\xC3\x84\xC3\xA4\xC3\x8B\xC3\xBB\xC3\x9C\xC2\xAB\xC3\x89", /*pressEnter=*/true, &err));
    CHECK(err.empty());
    CHECK(inputBuffer(m).find("m\x82rci\x8E\x84\x89\x96\x9A\xAE\x90") != std::string::npos);
    CHECK((m.memory().peek(0xF3C6) & 0x80) == 0);
}

// ROM-gated: SML doesn't change a KBII character (KEYCONV tests KBII
// before the SML/caps bit), so a locked SML still gives the same codes.
TEST(test_typeline_kbii_chars_ignore_sml) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    tapKey(m, "sml");
    m.runCycles(PC1600Machine::kTStateHz / 4);
    std::string err;
    CHECK(typeLine(m, "\xC3\x84\xC3\xA4\xC3\xBB", /*pressEnter=*/true, &err));
    CHECK(inputBuffer(m).find("\x8E\x84\x96") != std::string::npos);
}

// ROM-gated: a SHIFT the user latched is un-latched first (with SHIFT on,
// the KBII key would toggle the key click instead): `ö` still gives ö
// (94H), and SHIFT (SYMB0 F64EH bit 1), KBII and the key click (KEYWK1
// F079H bit 1) end up as before.
TEST(test_typeline_kbii_char_after_latched_shift) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    const uint8_t click = m.memory().peek(0xF079) & 0x02;
    tapKey(m, "shift");
    m.runCycles(PC1600Machine::kTStateHz / 4);
    CHECK((m.memory().peek(0xF64E) & 0x02) != 0);
    std::string err;
    CHECK(typeLine(m, "\xC3\xB6", /*pressEnter=*/true, &err));
    CHECK(inputBuffer(m).find("\x94") != std::string::npos);
    CHECK((m.memory().peek(0xF64E) & 0x02) == 0);
    CHECK((m.memory().peek(0xF3C6) & 0x80) == 0);
    CHECK((m.memory().peek(0xF079) & 0x02) == click);
}

// ROM-gated: with KBII latched by the user an accented character is
// dropped, and KBII stays on -- the next plain A types KBII A = á (A0H).
TEST(test_typeline_kbii_char_ignored_while_kbii_latched) {
    PC1600Machine m;
    SKIP_UNLESS(bootPC1600(m), "PC-1600 ROM images not found");
    tapKey(m, "kbii");
    m.runCycles(PC1600Machine::kTStateHz / 4);
    CHECK((m.memory().peek(0xF3C6) & 0x80) != 0);
    std::string err;
    CHECK(typeLine(m, "\xC3\xB6" "A", /*pressEnter=*/true, &err));
    const std::string buf = inputBuffer(m);
    CHECK(buf.find("\xA0\r") != std::string::npos);
    CHECK(buf.find("\x94") == std::string::npos);
    CHECK(buf.find("\x99") == std::string::npos);
}

// No ROM needed: a character without a key (not in the KBII tables) fails
// the line and names the character.
TEST(test_typeline_rejects_unknown_non_ascii) {
    PC1600Machine m;
    std::string err;
    CHECK(!typeLine(m, "Gr\xC3\x9F" "e", /*pressEnter=*/true, &err));
    CHECK(err.find("\xC3\x9F") != std::string::npos);
}

} // namespace
