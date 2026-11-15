// Edit > Paste Text and typed text on the Sharp models (LIB-5 AC-30, AC-34,
// AC-35, AC-37; ported in M-36 from 0.8.1's KeyPaste feeder tests): the keys
// a character gives by the model's typing profile (app/SharpTyping), the
// KBII sequences and the latches, and -- ROM-gated -- a paste through the
// library's typing engine reaching each model's ROM without a lost
// character, and no ENTER. ROM-gated tests SKIP when the images are absent.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../app/tests/SessionFixture.hpp"
#include "../core/KeyPaste.hpp"
#include "../core/PC1600/PC1600TypedInput.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "TestHarness.hpp"
#include "app/SharpTyping.hpp"
#include "typing/TypingEngine.hpp"
#include "typing/TypingSupport.hpp"

namespace {

using calcu::machine::ModelId;
using calcu::typing::Press;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;

const ModelId kPC1500{sharp::kPC1500A};
const ModelId kPC1600{sharp::kPC1600};

// What a text types, as 0.8.1's tests wrote steps: "^" for a shift press,
// "~" before each press of an accent's KBII sequence, "_" for space; a
// character without a key is left out (as a paste does, AC-12).
std::string typed(const ModelId& m, const std::string& text, const calcu::typing::TypingSupport* s = nullptr) {
    const calcu::typing::TypingProfile* p = shared::typingProfile(m);
    std::string out;
    for (char32_t c : calcu::typing::decodeUtf8(text)) {
        if (c < 0x20 || c == 0x7F) continue;
        const auto keys = p->charKeys(c, s);
        if (!keys) continue;
        for (const Press& k : *keys) out += (k.key.value == "space" ? std::string("_") : k.key.value) + (k.mustFinish ? "!" : "") + " ";
        out += "| ";
    }
    return out;
}

struct Latches final : calcu::typing::TypingSupport {
    bool shift = false, kbii = false;
    std::vector<std::pair<std::string, bool>> latches() const override { return {{"shift", shift}, {"kbii", kbii}}; }
};

TEST(lib5_ac30_shift_and_skip) {
    // Lowercase + '"' + ':' + '#' need SHIFT; tab and 'é' (on the PC-1500) are left out.
    CHECK_EQ(typed(kPC1500, "aB\"\t:#\xC3\xA9 1"), std::string("shift a | b | shift f2 | shift * | shift f3 | _ | 1 | "));
    CHECK_EQ(typed(kPC1600, "aB\"\t:# 1"), std::string("shift a | b | shift f2 | shift * | shift f3 | _ | 1 | "));
}

TEST(lib5_ac30_kbii_case_exact) {
    // é/É: SHIFT+KBII S / KBII S. Ë has no uppercase in the ROM -> ë (W).
    // û only exists as SHIFT+KBII. ß has no key and is left out.
    const std::string text = "\xC3\xA9\xC3\x89\xC3\x8B\xC3\xBB\xC3\x9F";
    CHECK_EQ(typed(kPC1600, text), std::string("kbii shift s kbii! | kbii s kbii! | kbii w kbii! | kbii shift b kbii! | "));
    CHECK_EQ(typed(kPC1500, text), std::string(""));   // the PC-1500 has no KBII
}

TEST(test_kbii_resolver) {
    // One rule everywhere: the character as typed; an uppercase the ROM
    // lacks falls back to its lowercase.
    std::string key;
    bool shift = true;
    CHECK(pc1600ResolveKbiiChar(U'ö', &key, &shift) && key == "R" && shift);   // ö
    CHECK(pc1600ResolveKbiiChar(U'Ö', &key, &shift) && key == "R" && !shift);  // Ö
    CHECK(pc1600ResolveKbiiChar(U'ë', &key, &shift) && key == "W" && !shift);  // ë
    CHECK(pc1600ResolveKbiiChar(U'Ë', &key, &shift) && key == "W" && !shift);  // Ë
    CHECK(pc1600ResolveKbiiChar(U'Û', &key, &shift) && key == "B" && shift);   // Û -> û
    CHECK(pc1600ResolveKbiiChar(U'«', &key, &shift) && key == "(" && shift);   // «
    CHECK(!pc1600ResolveKbiiChar(U'ß', &key, &shift));                        // ß
}

TEST(test_pc1500_lacks_digit_row_legends) {
    // '[' only exists on the PC-1600 keyboard (SHIFT + 2).
    CHECK_EQ(typed(kPC1500, "["), std::string(""));
    CHECK_EQ(typed(kPC1600, "["), std::string("shift 2 | "));
}

// AC-34: what the user has latched shapes (or drops) an accented
// character's sequence; with no typing support it is as with none latched,
// and "has a key" never depends on the latches (D-14).
TEST(lib5_ac34_kbii_respects_latches) {
    Latches l;
    const std::string oe = "\xC3\x96";   // Ö
    CHECK_EQ(typed(kPC1600, oe, &l), std::string("kbii r kbii! | "));
    l.shift = true;
    CHECK_EQ(typed(kPC1600, oe, &l), std::string("shift kbii r kbii! | "));   // SHIFT un-latched first
    l.shift = false;
    l.kbii = true;
    CHECK_EQ(typed(kPC1600, oe, &l), std::string("| "));   // KBII on: left out, but it has a key
    CHECK(shared::typingProfile(kPC1600)->charKeys(U'Ö', &l).has_value());
    CHECK_EQ(typed(kPC1600, oe, nullptr), std::string("kbii r kbii! | "));
}

// The line buffer of the PC-1600's console input (FBB0H), through the
// loader memory (the Z-80 view).
std::string pc1600LineBuffer(SessionFixture& f) {
    std::string buf;
    auto* mem = sharp::firstOf<sharp::LoaderMemory>(*f.part().machine());
    for (std::uint16_t a = 0xFBB0; mem && a <= 0xFBFF; ++a) buf.push_back(static_cast<char>(mem->peek(a)));
    return buf;
}

std::uint16_t peekBE16(SessionFixture& f, std::uint16_t addr) {
    auto* mem = sharp::firstOf<sharp::LoaderMemory>(*f.part().machine());
    return mem ? static_cast<std::uint16_t>((mem->peek(addr) << 8) | mem->peek(static_cast<std::uint16_t>(addr + 1))) : 0;
}

// The motivating example: a listing line pasted verbatim, no ENTER sent;
// it reaches the console input buffer on commit, so the test taps ENTER
// itself afterwards and checks the characters made it.
TEST(lib5_ac34_pc1600_paste_line_no_enter) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.paste("OPEN \"X:ATLANTIS.PUN\"FOR INPUT AS #1");
    CHECK(f.runUntilTyped(60));
    f.tap("enter");
    f.runFor(1.0);
    const std::string buf = pc1600LineBuffer(f);
    // The ROM keyword-tokenises on commit, so check the literal parts.
    CHECK(buf.find("\"X:ATLANTIS.PUN\"") != std::string::npos);
    CHECK(buf.find("#1") != std::string::npos);
}

// Only the first line is pasted, and never entered: in PRO mode nothing is
// stored (BASPRG_END, F867H big-endian, stays put) until ENTER is tapped by
// hand, and then exactly that one line.
TEST(lib5_ac12_pc1600_paste_multiline_types_first_line) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.tap("mode");   // RUN -> PRO
    f.runFor(0.5);
    const std::uint16_t before = peekBE16(f, 0xF867);
    f.paste("10 PRINT \"Hi\"\n20 A=1\n30 END");
    CHECK(f.runUntilTyped(60));
    f.runFor(0.5);
    CHECK(peekBE16(f, 0xF867) == before);
    f.tap("enter");
    f.runFor(0.5);
    const std::uint16_t afterOne = peekBE16(f, 0xF867);
    CHECK(afterOne > before);
    f.tap("enter");   // an empty line stores nothing more
    f.runFor(0.5);
    CHECK(peekBE16(f, 0xF867) == afterOne);
}

bool memoryContains(SessionFixture& f, const std::string& needle) {
    auto* mem = sharp::firstOf<sharp::LoaderMemory>(*f.part().machine());
    for (std::uint32_t a = 0x4000; mem && a + needle.size() <= 0x8000; ++a) {
        bool match = true;
        for (std::size_t i = 0; i < needle.size() && match; ++i)
            match = mem->peek(static_cast<std::uint16_t>(a + i)) == static_cast<std::uint8_t>(needle[i]);
        if (match) return true;
    }
    return false;
}

TEST(lib5_ac12_pc1500_paste_multiline_types_first_line) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    // A cold-booted PC-1500 needs CL + NEW0 before it stores program lines.
    f.tap("cl");
    f.runFor(0.5);
    f.typeLine("NEW0");
    CHECK(f.runUntilTyped(30));
    f.runFor(1.0);
    f.paste("10 PRINT \"Paste: ok\"\n20 REM Tail#x");
    CHECK(f.runUntilTyped(60));
    CHECK(memoryContains(f, "Paste: ok"));   // case kept via the SHIFT presses
    CHECK(!memoryContains(f, "Tail#x"));     // the second line is never typed
    // No ENTER: BASPRG_END ($7867, big-endian) only moves once the line is
    // committed by hand.
    const int endBeforeEnter = peekBE16(f, 0x7867);
    f.runFor(0.5);
    CHECK(peekBE16(f, 0x7867) == endBeforeEnter);
    f.tap("enter");
    f.runFor(2.0);
    CHECK(peekBE16(f, 0x7867) > endBeforeEnter);
}

} // namespace
