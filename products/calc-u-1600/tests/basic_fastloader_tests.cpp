// Headless C++ tests for Core/PC1500/PC1500BasicLoader.cpp -- the fast
// (poke + fix BASPRG_END) BASIC program loader. Needs a real ROM
// (roms/PC-1500_A04.ROM, relative to the repo root); skips (not fails) if
// it is missing, same convention as basictyper_tests.cpp.
//
// The keystroke typer is the oracle: type a program the slow way, capture
// the exact bytes and pointers the ROM produced, wrap them in a CE-158
// transfer file, load that the fast way on a fresh machine, and require a
// byte-identical program area + identical pointers.
//
// Build & run: see tools/run_tests.sh

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../core/PC1500/PC1500BasicLoader.hpp"
#include "support/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "TestHarness.hpp"
#include "LoaderTestSupport.hpp"

namespace {

bool bootMachine(PC1500Machine& machine) {
    if (!machine.loadROMFile("roms/PC-1500_A04.ROM")) return false;
    machine.reset();
    machine.runCycles(static_cast<uint64_t>(1300000.0 * 2));
    waitIdle(machine, static_cast<uint64_t>(1300000.0 * 5));
    return true;
}

// The loader assumes the preset already NEW0'd -- do it here, the same way
// a `keys:` section would.
void primeNew0(PC1500Machine& machine) {
    tapKey(machine, "cl");
    waitIdle(machine, static_cast<uint64_t>(1300000.0 * 2));
    std::string err;
    typeLine(machine, "NEW0", /*pressEnter=*/true, &err);
    waitIdle(machine, static_cast<uint64_t>(1300000.0 * 2));
}

uint16_t be16(PC1500Machine& m, uint16_t a) {
    return static_cast<uint16_t>((m.memory().peek(a) << 8) |
                                 m.memory().peek(static_cast<uint16_t>(a + 1)));
}

std::vector<uint8_t> readRange(PC1500Machine& m, uint16_t start, uint16_t endExclusive) {
    std::vector<uint8_t> v;
    for (uint16_t a = start; a < endExclusive; a++) v.push_back(m.memory().peek(a));
    return v;
}

std::vector<uint8_t> wrapCe158(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> f(27, 0x00);
    f[0] = 0x01;
    f[1] = 0x40;  // '@' tokenized BASIC
    f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    const char* name = "ORACLE";
    for (int i = 0; name[i] && i < 16; i++) f[5 + i] = static_cast<uint8_t>(name[i]);
    uint16_t wire = static_cast<uint16_t>(payload.size() - 1);  // capacity-1
    f[0x17] = static_cast<uint8_t>(wire >> 8);
    f[0x18] = static_cast<uint8_t>(wire & 0xFF);
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

// Types `src` through the ROM editor, returns the tokenized program image
// [BASPRG_ST .. BASPRG_END) and fills the pointers seen afterwards.
bool tokenizeViaOracle(const std::string& src, std::vector<uint8_t>* payload, uint16_t* stOut,
                       uint16_t* endOut, uint8_t* markerOut) {
    PC1500Machine m;
    if (!bootMachine(m)) return false;
    primeNew0(m);
    BasicTypeResult typed = typeBasicProgramText(m, src);
    if (!typed.ok || !typed.rejectedLines.empty()) return false;
    uint16_t st = be16(m, 0x7865);
    uint16_t end = be16(m, 0x7867);
    if (end < st) return false;
    *payload = readRange(m, st, end);
    *stOut = st;
    *endOut = end;
    *markerOut = m.memory().peek(end);
    return true;
}

void runEquivalenceCase(const char* label, const std::string& src) {
    std::vector<uint8_t> payload;
    uint16_t stA = 0, endA = 0;
    uint8_t markerA = 0;
    SKIP_UNLESS(tokenizeViaOracle(src, &payload, &stA, &endA, &markerA),
                (std::string(label) + ": ROM not available or typer rejected the program").c_str());
    CHECK(markerA == 0xFF);                       // typed program ends with a 0xFF
    CHECK(endA == stA + payload.size());

    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), (std::string(label) + ": roms/PC-1500_A04.ROM not found").c_str());
    primeNew0(m);
    BasicLoadResult r = loadertest::loadBasic(m, wrapCe158(payload));
    CHECK(r.ok);
    if (!r.ok) {
        std::fprintf(stderr, "  (%s) loader error: %s\n", label, r.error.c_str());
        return;
    }
    CHECK(r.baseAddr == stA);                     // same base as the typed load
    CHECK(r.endAddr == endA);
    CHECK(be16(m, 0x7865) == stA);
    CHECK(be16(m, 0x7867) == endA);               // BASPRG_END written
    CHECK(m.memory().peek(r.endAddr) == 0xFF);    // marker poked

    // Byte-identical program area, marker included.
    std::vector<uint8_t> fast = readRange(m, r.baseAddr, static_cast<uint16_t>(r.endAddr + 1));
    std::vector<uint8_t> want = payload;
    want.push_back(0xFF);
    CHECK(fast == want);
}

TEST(c1600_2_ac4_same_memory_as_typed_pc1500) {
    runEquivalenceCase("small", "10 PRINT \"AB\"\n20 GOTO 10\n");
}

TEST(test_equivalence_rem_and_data) {
    runEquivalenceCase("rem+data",
                       "10 REM demo header\n"
                       "20 DATA 1,2,3,4,5\n"
                       "30 FOR I=1 TO 5\n"
                       "40 READ X\n"
                       "50 PRINT X\n"
                       "60 NEXT I\n"
                       "70 END\n");
}

TEST(c1600_2_ac5_pc1500_refuses_pc1600_file) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    // Minimal PC-1600 header (FF 10 00 00, type 0x21) + a one-line payload.
    std::vector<uint8_t> payload = {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D};
    std::vector<uint8_t> f(16, 0x00);
    f[0] = 0xFF; f[1] = 0x10; f[4] = 0x21;
    f[5] = static_cast<uint8_t>(payload.size());
    f[0x0F] = 0x0F;
    f.insert(f.end(), payload.begin(), payload.end());
    BasicLoadResult r = loadertest::loadBasic(m, f);
    CHECK(!r.ok);
    CHECK(r.error.find("PC-1600") != std::string::npos);
}

// A bare-booted machine (never NEW0'd, e.g. one where the user forgot to
// run NEW) leaves BASPRG_ST uninitialised ($FFFF after a bare boot, or a
// $00 low byte). The loader must reject that with an actionable message
// rather than poking into system RAM, while still accepting the perfectly
// valid post-NEW0 base $00C5 (16K RAM card in the low window), or any other
// live BASPRG_ST value a `NEW &nnnn` might leave behind -- the loader has no
// notion of "the" expected NEW0 target, only of "is the current pointer
// plausible."
TEST(c1600_2_ec3_new0_check_hint_to_run_new) {
    std::vector<uint8_t> payload;
    uint16_t st = 0, end = 0;
    uint8_t marker = 0;
    SKIP_UNLESS(tokenizeViaOracle("10 PRINT \"AB\"\n", &payload, &st, &end, &marker), "ROM not available");
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    // Deliberately do NOT primeNew0(m).
    BasicLoadResult r = loadertest::loadBasic(m, wrapCe158(payload));
    CHECK(!r.ok);
    CHECK(r.error.find("BASPRG_ST") != std::string::npos);
}

// A live BASPRG_END that doesn't sit at/after BASPRG_ST (corrupted or
// otherwise implausible) must be rejected too -- it's the pointer the
// loader relies on to know how much of the resident program to erase.
TEST(c1600_2_ac6_refused_names_pointer_pc1500) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    uint16_t st = be16(m, 0x7865);
    // Corrupt BASPRG_END to point before BASPRG_ST.
    uint16_t badEnd = static_cast<uint16_t>(st - 1);
    m.memory().poke(0x7867, static_cast<uint8_t>(badEnd >> 8));
    m.memory().poke(0x7868, static_cast<uint8_t>(badEnd & 0xFF));

    std::vector<uint8_t> payload = {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D};  // 10 END
    BasicLoadResult r = loadertest::loadBasic(m, wrapCe158(payload));
    CHECK(!r.ok);
    CHECK(r.error.find("BASPRG_END") != std::string::npos);
}

// Reloading over a resident program must not leave any of its tail dangling
// past the new (shorter) BASPRG_END: LOADing "10 PRINT X" then a shorter
// "10 END" must fully erase the first program's leftover bytes, not just
// move the end pointer back.
TEST(test_reload_over_shorter_program_clears_tail) {
    std::vector<uint8_t> longPayload, shortPayload;
    uint16_t stLong = 0, endLong = 0, stShort = 0, endShort = 0;
    uint8_t markerLong = 0, markerShort = 0;
    SKIP_UNLESS(tokenizeViaOracle("10 PRINT \"HELLO WORLD\"\n20 GOTO 10\n", &longPayload, &stLong, &endLong, &markerLong) && tokenizeViaOracle("10 END\n", &shortPayload, &stShort, &endShort, &markerShort), "ROM not available");
    SKIP_UNLESS(longPayload.size() > shortPayload.size(), "fixture programs not ordered");
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    BasicLoadResult first = loadertest::loadBasic(m, wrapCe158(longPayload));
    CHECK(first.ok);
    if (!first.ok) return;
    uint16_t oldEnd = first.endAddr;

    BasicLoadResult second = loadertest::loadBasic(m, wrapCe158(shortPayload));
    CHECK(second.ok);
    if (!second.ok) return;

    CHECK(second.baseAddr == first.baseAddr);              // same BASPRG_ST, no NEW involved
    CHECK(second.endAddr < oldEnd);                        // shorter program, tighter end
    CHECK(be16(m, 0x7867) == second.endAddr);              // BASPRG_END reflects only the new program

    // Everything between the new marker and the old program's old end must
    // now read 0x00 -- no leftover tokens from the first program survive.
    for (uint16_t a = static_cast<uint16_t>(second.endAddr + 1); a <= oldEnd; a++) {
        CHECK(m.memory().peek(a) == 0x00);
    }
}

TEST(test_rejects_garbage) {
    PC1500Machine m;
    if (!bootMachine(m)) return;
    std::vector<uint8_t> junk(40, 0xAB);
    BasicLoadResult r = loadertest::loadBasic(m, junk);
    CHECK(!r.ok);
}

// A tokenized CE-158 file (.bbin) lands byte for byte where its listing does.
TEST(test_bbin_file_equals_listing) {
    std::vector<uint8_t> fromListing, fromBbin;
    for (const char* path : {"tests/fixtures/basic/lissajou-1500.bas",
                             "tests/fixtures/basic/lissajou-1500.bbin"}) {
        PC1500Machine m;
        if (!bootMachine(m)) return;
        primeNew0(m);
        BasicLoadResult r = loadertest::loadBasicFile(m, path);
        CHECK(r.ok);
        if (!r.ok) {
            std::fprintf(stderr, "  %s: loader error: %s\n", path, r.error.c_str());
            return;
        }
        (fromListing.empty() ? fromListing : fromBbin) = readRange(m, r.baseAddr, static_cast<uint16_t>(r.endAddr + 1));
    }
    CHECK(!fromListing.empty());
    CHECK(fromBbin == fromListing);
}

// AC-6: the program goes where BASIC's program starts (BASPRG_ST), and a
// program that does not fit is refused with the room, writing nothing.
TEST(c1600_2_ac6_pc1500_program_start) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    const uint16_t st = be16(m, 0x7865);
    const std::string listing = "10 PRINT 1\n";
    BasicLoadResult r = loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end()));
    CHECK(r.ok && r.baseAddr == st);
}

TEST(c1600_2_ac6_refused_room_nothing_written_pc1500) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    const uint16_t st = be16(m, 0x7865);
    const std::vector<uint8_t> before = readRange(m, st, static_cast<uint16_t>(st + 64));
    std::vector<uint8_t> big;   // 36 000 bytes of `10 END` records: more than any user RAM
    for (int i = 0; i < 6000; ++i) big.insert(big.end(), {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D});
    BasicLoadResult r = loadertest::loadBasic(m, wrapCe158(big));
    CHECK(!r.ok && r.error.find("too large") != std::string::npos);
    CHECK(readRange(m, st, static_cast<uint16_t>(st + 64)) == before);
}

// AC-4: no reset, no NEW: the machine goes on where it was.
TEST(c1600_2_ac4_nothing_else_changes_pc1500) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    const uint64_t cycles = m.elapsedCycles();
    const uint8_t ramSt = m.memory().peek(0x7863), ramEnd = m.memory().peek(0x7864);
    const std::string listing = "10 PRINT 1\n";
    CHECK(loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end())).ok);
    CHECK(m.elapsedCycles() == cycles);   // nothing ran
    CHECK(m.memory().peek(0x7863) == ramSt && m.memory().peek(0x7864) == ramEnd);
}

// EC-5: machine code given to the BASIC loader: the hint to load it as machine code.
TEST(c1600_2_ec5_code_to_basic_loader_hint) {
    PC1500Machine m;
    SKIP_UNLESS(bootMachine(m), "roms/PC-1500_A04.ROM not found");
    primeNew0(m);
    std::vector<uint8_t> f(27, 0x00);
    f[0] = 0x01; f[1] = 0x42; f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    f[0x15] = 0x7C; f[0x16] = 0x01;   // load &7C01
    f[0x18] = 0x01;                   // 2 bytes
    f.insert(f.end(), {0x9A, 0x9A});
    BasicLoadResult r = loadertest::loadBasic(m, f);
    CHECK(!r.ok && r.error.find("machine code") != std::string::npos && r.error.find("Load Machine Code") != std::string::npos);
}

}  // namespace
