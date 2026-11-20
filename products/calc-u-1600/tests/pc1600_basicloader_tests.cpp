// Headless C++ tests for core/PC1600/PC1600BasicLoader.cpp -- the fast
// (poke + fix BASPRG_END) PC-1600 BASIC loader, checked byte-for-byte
// against the keystroke typer. ROM-gated: SKIPs (not fails) when the
// confirmed PC-1600 ROM set is absent, like pc1600_basictyper_tests.cpp.
//
// Build & run: see tools/run_tests.sh

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../core/PC1600/PC1600BasicLoader.hpp"
#include "support/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "TestCards.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"
#include "LoaderTestSupport.hpp"

namespace {

// Boots, switches to PRO, and runs NEW0 -- the loadable state the loader
// assumes the preset left the machine in.
bool bootIntoProNew0(PC1600Machine& m) {
    if (!bootPC1600(m)) return false;
    tapKey(m, "mode");  // RUN -> PRO
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    std::string err;
    typeLine(m, "NEW0", /*pressEnter=*/true, &err);
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    return true;
}

// Boot with an N-byte plain RAM module in Slot 1 (32 KB behaves exactly as
// a CE-1600M for placement purposes: unbanked, PVOUT half-split). Attached
// before the boot so the boot ROM folds it into the S0 user area -- NEW0
// then relocates the BASIC program into the module window ($F865 -> $00C5).
bool bootIntoProNew0Slot1Ram(PC1600Machine& m, size_t sizeBytes) {
    auto ram = plainRamCard(sizeBytes);
    if (!ram) return false;
    m.memory().attachSlot1Card(std::move(ram));
    if (!bootPC1600(m)) return false;
    tapKey(m, "mode");  // RUN -> PRO
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    std::string err;
    typeLine(m, "NEW0", /*pressEnter=*/true, &err);
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    return true;
}

uint16_t be16(PC1600Machine& m, uint16_t a) {
    return static_cast<uint16_t>((m.memory().peek(a) << 8) |
                                 m.memory().peek(static_cast<uint16_t>(a + 1)));
}
uint16_t toZ80(uint16_t lh) {
    return (lh >= 0x4000 && lh < 0x8000) ? static_cast<uint16_t>(lh + 0x8000) : lh;
}
std::vector<uint8_t> readRange(PC1600Machine& m, uint16_t s, uint16_t e) {
    std::vector<uint8_t> v;
    for (uint16_t a = s; a < e; a++) v.push_back(m.memory().peek(a));
    return v;
}

// PRGADR's start/end triples ($FE3C-$FE41: address lo, hi, bank each) --
// what LIST reads. The ROM sets them when it stores a typed line; the fast
// loader has to leave the same bytes, or the program lists as empty.
std::vector<uint8_t> prgAdr(PC1600Machine& m) { return readRange(m, 0xFE3C, 0xFE42); }

// Run `RUN`, let it settle, return the VARIABLE POINTER ($F899, BE). A
// program that assigns a variable moves it; a program that errors out at
// RUN time doesn't -- so it doubles as "did the interpreter actually
// execute the program".
uint16_t runAndReadVarPtr(PC1600Machine& m) {
    tapKey(m, "mode");  // PRO -> RUN
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    std::string err;
    typeLine(m, "RUN", /*pressEnter=*/true, &err);
    waitUntilBasicIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz) * 6);  // back at the prompt
    return be16(m, 0xF899);
}

// The generalized-placement case: a Slot-1 RAM module has pushed the BASIC
// program area into the module window. The fast loader must scatter the
// tokenised image into the module's backing store byte-for-byte the way
// the keystroke typer's stored program looks, and the result must RUN.
TEST(s0_parts_in_table_order_line_not_split) {
    const char* src = "10 A=42\n20 B=A+1\n30 END\n";

    // Oracle: type the program with the module fitted; read the stored
    // tokens straight out of the module backing store.
    std::vector<uint8_t> typedPayload;
    std::vector<uint8_t> typedPrgAdr;
    uint16_t typedStLh = 0, typedEndLh = 0, typedVarPtrAfterRun = 0;
    {
        PC1600Machine m;
        SKIP_UNLESS(bootIntoProNew0Slot1Ram(m, 0x8000), "PC-1600 ROM set not found");
        BasicTypeResult typed = typeBasicProgramText(m, src);
        CHECK(typed.ok);
        CHECK(typed.rejectedLines.empty());
        typedStLh = be16(m, 0xF865);
        typedEndLh = be16(m, 0xF867);
        CHECK(typedStLh == 0x00C5);           // relocated into the module window
        CHECK(typedEndLh > typedStLh);
        std::vector<uint8_t> img = m.debugSlotImage(1);
        CHECK(img.size() == 0x8000);
        if (img.size() == 0x8000 && typedEndLh > typedStLh && typedEndLh < 0x8000) {
            typedPayload.assign(img.begin() + typedStLh, img.begin() + typedEndLh);
            CHECK(img[typedEndLh] == 0xFF);
        }
        typedPrgAdr = prgAdr(m);
        typedVarPtrAfterRun = runAndReadVarPtr(m);
    }
    CHECK(!typedPayload.empty());
    if (typedPayload.empty()) return;

    // Fast path: load that payload into a fresh module machine.
    PC1600Machine m;
    if (!bootIntoProNew0Slot1Ram(m, 0x8000)) return;
    BasicLoadResult r = loadertest::loadPayload(m, typedPayload);
    CHECK(r.ok);
    if (!r.ok) {
        std::fprintf(stderr, "  loader error: %s\n", r.error.c_str());
        return;
    }
    CHECK(r.baseAddr == static_cast<uint16_t>(typedStLh + 0x8000));   // Z-80 $80C5
    CHECK(r.endAddr == static_cast<uint16_t>(typedEndLh + 0x8000));
    CHECK(be16(m, 0xF867) == static_cast<uint16_t>(typedStLh + typedPayload.size()));

    std::vector<uint8_t> img = m.debugSlotImage(1);
    std::vector<uint8_t> want = typedPayload;
    want.push_back(0xFF);
    std::vector<uint8_t> got(img.begin() + typedStLh,
                             img.begin() + typedStLh + typedPayload.size() + 1);
    CHECK(got == want);   // byte-for-byte == the keystroke typer's stored program
    CHECK(prgAdr(m) == typedPrgAdr);   // module bank byte and end address as the ROM sets them

    // ... and it RUNs: the interpreter reads the scattered program and
    // lands in exactly the state the typed program's RUN produced (same
    // BASPRG_END, same VARIABLE POINTER) -- i.e. the fast-loaded program
    // behaves identically to a correctly-stored one.
    uint16_t varPtrAfter = runAndReadVarPtr(m);
    CHECK(varPtrAfter == typedVarPtrAfterRun);
    CHECK(be16(m, 0xF867) == static_cast<uint16_t>(typedStLh + typedPayload.size()));

    // The program survived RUN in the module backing store.
    std::vector<uint8_t> post = m.debugSlotImage(1);
    std::vector<uint8_t> afterRun(post.begin() + typedStLh,
                                  post.begin() + typedStLh + typedPayload.size() + 1);
    CHECK(afterRun == want);
}

// Reloading over a resident program must fully erase its tail rather than
// just moving BASPRG_END back -- otherwise a shorter reload leaves stale
// tokens dangling in RAM (harmless to RUN, but a stray dump of that area
// would show leftover garbage from the previous program).
TEST(test_reload_over_shorter_program_clears_tail_stock) {
    PC1600Machine m;
    SKIP_UNLESS(bootIntoProNew0(m), "PC-1600 ROM set not found");
    std::vector<uint8_t> longPayload = {0x00, 0x0A, 0x09, 0xF1, 0x22, 0x48, 0x45, 0x4C, 0x4C, 0x4F,
                                       0x22, 0x0D};
    std::vector<uint8_t> shortPayload = {0x00, 0x0A, 0x03, 0xF1, 0x30, 0x0D};
    CHECK(longPayload.size() > shortPayload.size());

    BasicLoadResult first = loadertest::loadPayload(m, longPayload);
    CHECK(first.ok);
    if (!first.ok) {
        std::fprintf(stderr, "  loader error: %s\n", first.error.c_str());
        return;
    }
    uint16_t oldEnd = first.endAddr;

    BasicLoadResult second = loadertest::loadPayload(m, shortPayload);
    CHECK(second.ok);
    if (!second.ok) {
        std::fprintf(stderr, "  loader error: %s\n", second.error.c_str());
        return;
    }
    CHECK(second.baseAddr == first.baseAddr);   // same BASPRG_ST, no NEW involved
    CHECK(second.endAddr < oldEnd);
    CHECK(toZ80(be16(m, 0xF867)) == second.endAddr);

    for (uint16_t a = static_cast<uint16_t>(second.endAddr + 1); a <= oldEnd; a++) {
        CHECK(m.memory().peek(a) == 0x00);
    }
}

// A live BASPRG_END that doesn't sit at/after BASPRG_ST must be rejected --
// the loader relies on it to know how much of the resident program to erase.
TEST(refused_names_pointer_or_room_nothing_written) {
    PC1600Machine m;
    SKIP_UNLESS(bootIntoProNew0(m), "PC-1600 ROM set not found");
    uint16_t st = be16(m, 0xF865);
    uint16_t badEnd = static_cast<uint16_t>(st - 1);
    m.memory().poke(0xF867, static_cast<uint8_t>(badEnd >> 8));
    m.memory().poke(0xF868, static_cast<uint8_t>(badEnd & 0xFF));

    std::vector<uint8_t> payload = {0x00, 0x0A, 0x03, 0xF1, 0x30, 0x0D};
    BasicLoadResult r = loadertest::loadPayload(m, payload);
    CHECK(!r.ok);
    CHECK(r.error.find("BASPRG_END") != std::string::npos);
}

// Same as test_reload_over_shorter_program_clears_tail_stock, but with a
// Slot-1 RAM module fitted, so the vacated tail lives in the module's
// backing store rather than internal RAM -- the erase plan must still find
// and clear it via the same scattered-placement logic used for writes.
TEST(test_reload_over_shorter_program_clears_tail_module) {
    PC1600Machine m;
    SKIP_UNLESS(bootIntoProNew0Slot1Ram(m, 0x8000), "PC-1600 ROM set not found");
    std::vector<uint8_t> longPayload = {0x00, 0x0A, 0x09, 0xF1, 0x22, 0x48, 0x45, 0x4C, 0x4C, 0x4F,
                                       0x22, 0x0D};
    std::vector<uint8_t> shortPayload = {0x00, 0x0A, 0x03, 0xF1, 0x30, 0x0D};

    BasicLoadResult first = loadertest::loadPayload(m, longPayload);
    CHECK(first.ok);
    if (!first.ok) {
        std::fprintf(stderr, "  loader error: %s\n", first.error.c_str());
        return;
    }
    uint16_t oldEndLh = be16(m, 0xF867);

    BasicLoadResult second = loadertest::loadPayload(m, shortPayload);
    CHECK(second.ok);
    if (!second.ok) {
        std::fprintf(stderr, "  loader error: %s\n", second.error.c_str());
        return;
    }
    uint16_t newEndLh = be16(m, 0xF867);
    CHECK(newEndLh < oldEndLh);

    // Both ends land inside the module window here (0x00C5-based, well below
    // the module's 32K size), so the vacated tail is entirely in the
    // module's own backing store.
    std::vector<uint8_t> img = m.debugSlotImage(1);
    CHECK(!img.empty());
    if (img.empty() || oldEndLh >= img.size()) return;
    for (uint16_t a = static_cast<uint16_t>(newEndLh + 1); a <= oldEndLh; a++) {
        CHECK(img[a] == 0x00);
    }
}

TEST(test_rejects_pc1500_transfer_file) {
    PC1600Machine m;
    SKIP_UNLESS(bootIntoProNew0(m), "PC-1600 ROM set not found");
    // CE-158 header (0x01 .. "COM", type 0x40) + a one-line payload.
    std::vector<uint8_t> payload = {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D};
    std::vector<uint8_t> f(27, 0x00);
    f[0] = 0x01; f[1] = 0x40; f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    uint16_t wire = static_cast<uint16_t>(payload.size() - 1);
    f[0x17] = static_cast<uint8_t>(wire >> 8);
    f[0x18] = static_cast<uint8_t>(wire & 0xFF);
    f.insert(f.end(), payload.begin(), payload.end());
    BasicLoadResult r = loadertest::loadBasic(m, f);
    CHECK(!r.ok);
    CHECK(r.error.find("PC-1500") != std::string::npos);
}


}  // namespace

// ── Typed vs fast-loaded: the work area must match ──────────────────
//
// The ROM's own line editor stores the typed program; the fast loader must
// leave the same pointers (LOADEND, rom3b 70E1H). Compared: the slot
// descriptors and S0 banks (F015-F02C), the logical banks + TITLE + ADTBL
// (F1C1-F1DA), the BASIC pointers (F864-F8BD), PRGADR (FE3C-FE41) and the
// first 8 bytes of each module (the program-module header).
//
// Not compared: what the line editor leaves behind but LOAD doesn't touch
// (the SEARCH START / FOUND banks F1C2/F1C3, RESTORE and INTERPRET banks
// F1CD/F1CE, the search cache F8A6-F8AB, display flags F880), and the BASIC
// interrupt state F1CF-F1D4, which the 0.5 s interrupt sets at any time.
bool editorOrRuntimeOnly(uint16_t a) {
    return a == 0xF1C2 || a == 0xF1C3 || (a >= 0xF1CD && a <= 0xF1D4) || a == 0xF880 ||
           (a >= 0xF8A6 && a <= 0xF8AB);
}

const char* const kLongProgram =
    "10 REM AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"
    "20 REM BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n"
    "30 REM CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n"
    "40 REM DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\n"
    "50 REM EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n"
    "60 POKE &FF80,77\n"
    "70 END\n";

// Every other placement only needs a program, not a long one: typing costs
// ~0.13 s emulated per character.
const char* const kShortProgram =
    "10 REM A\n"
    "60 POKE &FF80,77\n"
    "70 END\n";

// The tokenized payload of `program` (MODE 0 tokens, as the keyboard
// enters them in both MODEs): loaded on a stock machine, where it lies
// contiguously in internal RAM. Typing it would cost ~0.13 s emulated per
// character; the typed side of every case is the oracle anyway.
std::vector<uint8_t> programPayload(const char* program) {
    PC1600Machine m;
    if (!bootIntoProNew0(m)) return {};
    const std::string text(program);
    if (!loadertest::loadBasic(m, std::vector<uint8_t>(text.begin(), text.end())).ok) return {};
    return readRange(m, toZ80(be16(m, 0xF865)), toZ80(be16(m, 0xF867)));
}

struct WorkAreaSnapshot {
    std::vector<std::pair<uint16_t, uint8_t>> bytes;
    std::vector<uint8_t> header1, header2;
};

WorkAreaSnapshot snapshot(PC1600Machine& m) {
    WorkAreaSnapshot w;
    const uint16_t ranges[][2] = {{0xF015, 0xF02D}, {0xF1C1, 0xF1DB}, {0xF864, 0xF8BE}, {0xFE3C, 0xFE42}};
    for (const auto& r : ranges)
        for (uint16_t a = r[0]; a < r[1]; ++a) w.bytes.push_back({a, m.memory().peek(a)});
    std::vector<uint8_t> i1 = m.debugSlotImage(1), i2 = m.debugSlotImage(2);
    w.header1.assign(i1.begin(), i1.begin() + std::min<size_t>(8, i1.size()));
    w.header2.assign(i2.begin(), i2.begin() + std::min<size_t>(8, i2.size()));
    return w;
}

void checkSameWorkArea(const WorkAreaSnapshot& typed, const WorkAreaSnapshot& fast, const char* label) {
    bool same = true;
    for (size_t i = 0; i < typed.bytes.size(); ++i) {
        if (editorOrRuntimeOnly(typed.bytes[i].first)) continue;
        if (typed.bytes[i].second != fast.bytes[i].second) {
            same = false;
            std::fprintf(stderr, "  %s: $%04X typed $%02X, fast $%02X\n", label, typed.bytes[i].first,
                         typed.bytes[i].second, fast.bytes[i].second);
        }
    }
    if (typed.header1 != fast.header1) { same = false; std::fprintf(stderr, "  %s: slot 1 header differs\n", label); }
    if (typed.header2 != fast.header2) { same = false; std::fprintf(stderr, "  %s: slot 2 header differs\n", label); }
    CHECK(same);
}

// A module of `size` bytes: 8 KB is the bundled CE-155, 16 / 32 KB plain RAM.
std::unique_ptr<SoftwareDefinedCard> slotModule(size_t size, CardHost host) {
    auto module = size == 0x2000 ? bundledCard("ce155.module.yaml", host) : plainRamCard(size);
    CHECK(module != nullptr);
    return module;
}

// Boot with optional modules (sizes in bytes, 0 = none), go to PRO mode,
// type the setup lines.
bool prepare(PC1600Machine& m, size_t slot1Ram, size_t slot2Ram, const std::vector<std::string>& setup) {
    if (slot1Ram) {
        auto module = slotModule(slot1Ram, CardHost::PC1600Slot1);
        if (!module) return false;
        m.memory().attachSlot1Card(std::move(module));
    }
    if (slot2Ram) {
        auto module = slotModule(slot2Ram, CardHost::PC1600Slot2);
        if (!module) return false;
        m.memory().attachSlot2Card(std::move(module));
    }
    if (!bootPC1600(m)) return false;
    tapKey(m, "mode");  // RUN -> PRO
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    std::string err;
    for (const std::string& line : setup) {
        typeLine(m, line, /*pressEnter=*/true, &err);
        waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz) * 2);
    }
    return true;
}

void checkTypedVsFast(const char* label, size_t slot1Ram, size_t slot2Ram, const std::vector<std::string>& setup,
                      const char* program, const std::vector<uint8_t>& payload, int expectTitle = -1) {
    WorkAreaSnapshot typed, fast;
    {
        PC1600Machine m;
        if (!prepare(m, slot1Ram, slot2Ram, setup)) return;
        if (expectTitle >= 0) CHECK(m.programAreaTitle() == expectTitle);
        BasicTypeResult t = typeBasicProgramText(m, program);
        CHECK(t.ok && t.rejectedLines.empty());
        typed = snapshot(m);
    }
    PC1600Machine m;
    if (!prepare(m, slot1Ram, slot2Ram, setup)) return;
    BasicLoadResult r = loadertest::loadPayload(m, payload);
    CHECK(r.ok);
    if (!r.ok) {
        std::fprintf(stderr, "  %s: loader error: %s\n", label, r.error.c_str());
        return;
    }
    fast = snapshot(m);
    checkSameWorkArea(typed, fast, label);
    // And it runs: line 60 pokes a marker into the free top of the work area.
    const uint8_t zero = 0;
    m.pokeMemory(0xFF80, &zero, 1);
    runAndReadVarPtr(m);
    CHECK(m.memory().peek(0xFF80) == 77);
}

TEST(same_memory_as_typed_pc1600) {
    const std::vector<uint8_t> shortPayload = programPayload(kShortProgram);
    SKIP_UNLESS(!shortPayload.empty(), "PC-1600 ROM set not found");
    const std::vector<uint8_t> longPayload = programPayload(kLongProgram);
    auto check = [&](const char* label, size_t slot1Ram, size_t slot2Ram, const std::vector<std::string>& setup,
                     int expectTitle = -1) {
        checkTypedVsFast(label, slot1Ram, slot2Ram, setup, kShortProgram, shortPayload, expectTitle);
    };
    check("stock", 0, 0, {"NEW0"});
    check("CE-155", 0x2000, 0, {"NEW0"});
    // A 32 KB module folded into S0, the program start pushed near the end
    // of its first bank so the (long) program crosses into the second.
    checkTypedVsFast("S0 across banks", 0x8000, 0, {"NEW0", "NEW \"S0:\",&3F00"}, kLongProgram, longPayload);
    // The module as a program module, selected with TITLE.
    check("S1 program module", 0x8000, 0, {"NEW0", "INIT\"S1:\",\"P\"", "TITLE\"S1:\""});

    // MODE 1: the area the ROM sets up (PC15MAP), whatever TITLE said before.
    check("MODE 1, no module", 0, 0, {"MODE1", "NEW0"}, 0);
    check("MODE 1, CE-155", 0x2000, 0, {"MODE1", "NEW0"}, 0);
    // A one-bank program module in S1 becomes the MODE 1 area ...
    check("MODE 1, S1 one bank", 0x4000, 0, {"NEW0", "INIT\"S1:\",\"P\"", "TITLE\"S1:\"", "MODE1"}, 1);
    // ... a two-bank one is hidden and S0 is used ...
    check("MODE 1, S1 two banks", 0x8000, 0, {"NEW0", "INIT\"S1:\",\"P\"", "TITLE\"S1:\"", "MODE1"}, 0);
    // ... and with one-bank program modules in both slots S1 wins over TITLE "S2:".
    check("MODE 1, S1 over S2", 0x4000, 0x4000,
          {"NEW0", "INIT\"S1:\",\"P\"", "INIT\"S2:\",\"P\"", "TITLE\"S2:\"", "MODE1"}, 1);
}

std::string writeTempListing(const char* name, const std::string& text) {
    const std::filesystem::path p = std::filesystem::temp_directory_path() / name;
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << text;
    return p.string();
}

bool bootMode(PC1600Machine& m, bool mode1) {
    return prepare(m, 0, 0, mode1 ? std::vector<std::string>{"MODE1", "NEW0"} : std::vector<std::string>{"NEW0"});
}

// A listing is tokenized with the table of the current MODE: CALL is the
// PC-1600's Z-80 CALL (F282H) in MODE 0 and the PC-1500's CALL (F18AH, which
// the PC-1600 lists as XCALL) in MODE 1.
TEST(listing_tokenized_per_model_and_mode) {
    const std::string path = writeTempListing("pc1600_mode_call.bas", "10 CALL &4000\n");
    for (bool mode1 : {false, true}) {
        PC1600Machine m;
        if (!bootMode(m, mode1)) return;
        BasicLoadResult r = loadertest::loadBasicFile(m, path);
        CHECK(r.ok);
        if (!r.ok) continue;
        const uint8_t hi = m.memory().peek(static_cast<uint16_t>(r.baseAddr + 3));
        const uint8_t lo = m.memory().peek(static_cast<uint16_t>(r.baseAddr + 4));
        CHECK(hi == (mode1 ? 0xF1 : 0xF2) && lo == (mode1 ? 0x8A : 0x82));
    }
}

// Text the PC-1500 can't hold is refused in MODE 1, and nothing is written.
TEST(mode1_non_ascii_refused) {
    const std::string path = writeTempListing("pc1600_mode_umlaut.bas", "10 PRINT \"\xC3\xA4\xC3\xB6\xC3\xBC\"\n");
    {
        PC1600Machine m;
        if (!bootMode(m, false)) return;
        CHECK(loadertest::loadBasicFile(m, path).ok);
    }
    PC1600Machine m;
    if (!bootMode(m, true)) return;
    const WorkAreaSnapshot before = snapshot(m);
    BasicLoadResult r = loadertest::loadBasicFile(m, path);
    CHECK(!r.ok);
    CHECK(r.error.find("MODE 1") != std::string::npos);
    const WorkAreaSnapshot after = snapshot(m);
    bool same = true;
    for (size_t i = 0; i < before.bytes.size(); ++i)
        if (!editorOrRuntimeOnly(before.bytes[i].first) && before.bytes[i].second != after.bytes[i].second) same = false;
    CHECK(same);
}

// A CE-158 (PC-1500) transfer file: refused in MODE 0, loaded in MODE 1.
TEST(tokenized_file_per_machine_and_mode) {
    std::vector<uint8_t> payload = {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D};
    std::vector<uint8_t> f(27, 0x00);
    f[0] = 0x01; f[1] = 0x40; f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    const uint16_t wire = static_cast<uint16_t>(payload.size() - 1);
    f[0x17] = static_cast<uint8_t>(wire >> 8);
    f[0x18] = static_cast<uint8_t>(wire & 0xFF);
    f.insert(f.end(), payload.begin(), payload.end());
    PC1600Machine m;
    if (!bootMode(m, true)) return;
    BasicLoadResult r = loadertest::loadBasic(m, f);
    CHECK(r.ok);
    if (r.ok) CHECK(readRange(m, r.baseAddr, static_cast<uint16_t>(r.baseAddr + payload.size())) == payload);
}

// A tokenized PC-1600 file (.bbin) loads in both MODEs, and in MODE 0 lands
// byte for byte where its listing does.
TEST(test_pc1600_bbin_file) {
    const std::string bbin = "tests/fixtures/basic/lissajou-1600.bbin";
    const std::string bas = "tests/fixtures/basic/lissajou-1600.bas";
    std::vector<uint8_t> fromListing;
    {
        PC1600Machine m;
        if (!bootMode(m, false)) return;
        BasicLoadResult r = loadertest::loadBasicFile(m, bas);
        CHECK(r.ok);
        if (r.ok) fromListing = readRange(m, r.baseAddr, r.endAddr);
    }
    for (bool mode1 : {false, true}) {
        PC1600Machine m;
        if (!bootMode(m, mode1)) return;
        BasicLoadResult r = loadertest::loadBasicFile(m, bbin);
        CHECK(r.ok);
        if (!r.ok) std::fprintf(stderr, "  loader error: %s\n", r.error.c_str());
        if (r.ok && !mode1) CHECK(readRange(m, r.baseAddr, r.endAddr) == fromListing);
    }
}

// The loaders read MODE and TITLE from the machine: after boot MODE 0 and
// S0; `MODE1` typed on a stock machine is accepted (the ROM needs no module).
TEST(test_mode_and_title_queries) {
    PC1600Machine m;
    if (!bootPC1600(m)) return;
    CHECK(!m.mode1());
    CHECK(m.programAreaTitle() == 0);
    std::string err;
    typeLine(m, "MODE1", /*pressEnter=*/true, &err);
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    CHECK(m.mode1());
    CHECK(m.programAreaTitle() == 0);
    typeLine(m, "MODE0", /*pressEnter=*/true, &err);
    waitIdle(m, static_cast<uint64_t>(PC1600Machine::kTStateHz));
    CHECK(!m.mode1());
}

// No reset, no NEW, MODE and the selected program area as before.
TEST(nothing_else_changes_mode_area_kept) {
    for (bool mode1 : {false, true}) {
        PC1600Machine m;
        if (!bootMode(m, mode1)) return;
        const bool modeBefore = m.mode1();
        const int areaBefore = m.programAreaTitle();
        const std::string listing = "10 PRINT 1\n";
        CHECK(loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end())).ok);
        CHECK(m.mode1() == modeBefore && m.programAreaTitle() == areaBefore);
    }
}

// S1 / S2 in a memory module; a selected slot without a program
// module is refused, naming the area, and nothing is written.
TEST(basic_load_into_s1_s2_module) {
    PC1600Machine m;
    if (!prepare(m, 0, 0x8000, {"NEW0", "INIT\"S2:\",\"P\"", "TITLE\"S2:\""})) return;
    CHECK(m.programAreaTitle() == 2);
    const std::string listing = "10 PRINT 1\n";
    BasicLoadResult r = loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end()));
    CHECK(r.ok);
    if (!r.ok) std::fprintf(stderr, "  loader error: %s\n", r.error.c_str());
    CHECK(r.baseAddr >= 0x8000 && r.baseAddr < 0xC000);   // in the slot window
}

TEST(slot_without_program_module_refused) {
    PC1600Machine m;
    if (!bootIntoProNew0(m)) return;
    const uint8_t s1 = 1;
    m.pokeMemory(0xF1D5, &s1, 1);   // TITLE "S1:" with nothing in slot 1
    const WorkAreaSnapshot before = snapshot(m);
    const std::string listing = "10 PRINT 1\n";
    BasicLoadResult r = loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end()));
    CHECK(!r.ok && r.error.find("S1") != std::string::npos);
    const WorkAreaSnapshot after = snapshot(m);
    bool same = true;
    for (size_t i = 0; i < before.bytes.size(); ++i)
        if (before.bytes[i].second != after.bytes[i].second) same = false;
    CHECK(same);
}

// No program area set up: the hint to run NEW first.
TEST(uninitialised_pointer_hint_pc1600) {
    PC1600Machine m;
    if (!bootIntoProNew0(m)) return;
    const uint8_t bad[2] = {0xFF, 0xFF};
    m.pokeMemory(0xF865, bad, 2);   // BASPRG_ST as a fresh RAM leaves it
    const std::string listing = "10 PRINT 1\n";
    BasicLoadResult r = loadertest::loadBasic(m, std::vector<uint8_t>(listing.begin(), listing.end()));
    CHECK(!r.ok && r.error.find("run NEW") != std::string::npos);
}

