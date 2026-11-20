// The core through the machine interface: every model boots to key readiness, keys reach the
// firmware, the glyphs, blink periods and duty hold, and the TI-58C RAM, card
// reader, printer and breakpoint paths respond (the last through the adapters).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/Bcd.hpp"
#include "../core/TIRomText.hpp"

#include <algorithm>
#include <vector>

using cu59::ModelId;
using cu59::kTI59;
using cu59::kTI58;
using cu59::kTI58C;
using namespace testroms;

namespace {

// Matrix codes (the key positions of docs/formats/preset.md §8.1): 2 = 83, 3 = 84, + = 85, = = 95, CLR = 25.
constexpr int kKey2 = 83, kKey3 = 84, kPlus = 85, kEquals = 95, kClr = 25;
constexpr int kKey0 = 92, kRecip = 35;

} // namespace

TEST(rom_sentinels_and_sizes) {
    // The five files per model decode into 6144 words and 64 constant rows
    // inside the core; a TI-59 image with a wrong sentinel is refused.
    for (ModelId m : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        auto images = romSet()->imagesFor(*tiCatalogue().model(m), std::nullopt);
        CHECK_EQ(images.size(), size_t{5});
        std::vector<std::string_view> roms, constants;
        for (size_t i = 0; i < images.size(); ++i)
            (i < 3 ? roms : constants).emplace_back(reinterpret_cast<const char*>(images[i].bytes.data()), images[i].bytes.size());
        std::string error;
        TIRoms d = decodeRomSet(m == ModelId{kTI59} ? MachineVariant::TI59 : m == ModelId{kTI58} ? MachineVariant::TI58 : MachineVariant::TI58C,
                                roms, constants, error);
        CHECK(error.empty());
        CHECK_EQ(d.words.size(), size_t{6144});
        CHECK_EQ(d.constants.size(), size_t{64 * 16});
        if (m == ModelId{kTI59} && d.words.size() == 6144) {
            CHECK_EQ(d.words[0], 0x0A01);
            CHECK_EQ(d.words[6143], 0x1987);
        }
    }
}

TEST(rom_missing_file_throws) {
    // A missing file is found by the lookup at the start: there is no ROM
    // set, so no machine can be built.
    testroms::TempDir dir;
    auto r = cu59::lookUpRoms(calcu::roms::DirectoryBundle(dir.path()));
    CHECK(!r.ok());
    CHECK(r.set == nullptr);
    CHECK_EQ(r.missing.files.size(), size_t{24});
}

TEST(module_master_library_parses) {
    auto lib = cu59::moduleImage(*testroms::romSet(), "ML.rom.yaml");
    CHECK(lib.has_value());
    if (lib) CHECK(lib->size() > 4000 && lib->size() <= 5000);
}

TEST(boot_reaches_key_readiness_all_models) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        auto m = testroms::makeMachine(model);
        CHECK(testroms::runToScanLoop(*m, model));
        CHECK_EQ(testroms::displayText(*m), std::string("0"));
    }
}

TEST(two_plus_three_equals_five) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58C}}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        for (int k : {kClr, kKey2, kPlus, kKey3, kEquals}) testroms::tap(*m, model, k);
        CHECK_EQ(testroms::displayText(*m), std::string("5"));
    }
}

// Reading the keycode ROM (PRG SOURCE 8), then Del: the ROM leaves other data
// in B, the display's control register, and blinks. The five pictures of a real
// TI-59 after 9 Op 17 STO 00 Pgm 01 A CLR D.MS LRN SST and one to five Dels,
// without the dark positions, with o for the degree sign. They fix the rules for
// nibbles 2, 4 and 5, which depend on the zero suppression, and for a leftmost
// zero (the fourth picture, nibble 9).
TEST(control_nibble_glyphs) {
    const ModelId model = ModelId{kTI59};
    auto m = testroms::makeMachine(model);
    testroms::runToScanLoop(*m, model);
    // The picture once two polls agree: the first after a key still holds what
    // was driven before it.
    auto settledText = [&] {
        std::string text, previous;
        for (int i = 0; i < 100 && (text.empty() || text != previous); ++i) {
            previous = text;
            run(*m, static_cast<uint32_t>(instructionHz(model) * 0.05));
            text = testroms::displayText(*m);
        }
        return text;
    };
    for (int k : {64, 21, 64, 82, 62, 42, 92, 92, 21, 31, 92, 82, 11, 25, 21, 83, 31})
        testroms::tap(*m, model, k);
    CHECK_EQ(settledText(), std::string("00082"));  // 000 82, the first step of the keycode ROM
    testroms::tap(*m, model, 41);                   // SST
    for (const char* picture : {"----00", "0", "-0-\"0000'o0", "0000000\"0", "-00oo'o0"}) {
        testroms::tap(*m, model, 21);
        testroms::tap(*m, model, 51);               // Del
        CHECK_EQ(settledText(), std::string(picture));
    }
}

// Error-blink period (0 1/x) of the TI-59 ROM in seconds: 0x0655 SET.IDLE
// re-lights the display once per lit pass; a lit phase starts at the first one
// after a dark phase (≥ 2000 ticks without it).
static double errorBlinkPeriod(ModelId model, bool printer) {
    auto m = testroms::makeMachine(model, printer);   // the PC-100C in its place or not
    testroms::runToScanLoop(*m, model);
    for (int k : {kKey0, kRecip}) testroms::tap(*m, model, k);
    std::vector<uint64_t> litStarts;
    uint64_t lastSetIdle = 0;
    for (int i = 0; i < 400'000 && litStarts.size() < 4; ++i) {
        m->advance(1);
        if (dbg(*m).pc() != 0x0655) continue;
        uint64_t t = m->tickCount() / 32;   // weighted instructions
        if (lastSetIdle == 0 || t - lastSetIdle > 2000) litStarts.push_back(t);
        lastSetIdle = t;
    }
    CHECK_EQ(litStarts.size(), size_t{4});
    if (litStarts.size() < 4) return 0;
    return double(litStarts[3] - litStarts[1]) / 2 / instructionHz(model);
}

// The error blink of a real TI-59 (0 1/x), filmed next to a stopwatch on the
// PC-100C: 0.899 s per period; stand-alone, 0.449 s. The printer's KP line at
// D0 sends every idle pass through the confirmation scan (0x0659–0x0660).
TEST(error_blink_period_ti59) {
    double period = errorBlinkPeriod(ModelId{kTI59}, true);
    CHECK(period > 0.899 * 0.98 && period < 0.899 * 1.02);
    period = errorBlinkPeriod(ModelId{kTI59}, false);
    CHECK(period > 0.449 * 0.98 && period < 0.449 * 1.02);
}

// A real TI-58's error blink (Pavel's unit, 384.6 kHz): 0.6801 s per period
// stand-alone, 1.2122 s on the PC-100C. Every idle pass detours through the
// TI-58's card veto (digit 7, the 0x0459 card poll); with the printer also
// through the KP·D0 confirmation scan. Emulated: 0.681 s and 1.215 s.
TEST(error_blink_period_ti58) {
    double period = errorBlinkPeriod(ModelId{kTI58}, false);
    CHECK(period > 0.6801 * 0.99 && period < 0.6801 * 1.01);
    period = errorBlinkPeriod(ModelId{kTI58}, true);
    CHECK(period > 1.2122 * 0.99 && period < 1.2122 * 1.01);
}

// The TI-58C blink counts idle passes at full speed, so its length in
// instructions is fixed by the ROM path alone; a real unit's blink in seconds
// gives that unit's clock, to which its other measurements are normalised.
static double ti58cErrorBlinkTicks(bool printer) {
    auto m = testroms::makeMachine(ModelId{kTI58C}, printer);
    testroms::runToScanLoop(*m, ModelId{kTI58C});
    for (int k : {kKey0, kRecip}) testroms::tap(*m, ModelId{kTI58C}, k);
    const double hz = instructionHz(ModelId{kTI58C});
    // The multiplex leaves short gaps in a lit phase; a lit phase starts after
    // ≥ 10 ms without any driven digit.
    const uint64_t gap = static_cast<uint64_t>(0.010 * hz);
    std::vector<uint64_t> litStarts;
    uint64_t lastLit = 0;
    m->readDisplay();
    for (int i = 0; i < 40'000 && litStarts.size() < 4; ++i) {
        run(*m, 5);
        auto d = m->readDisplay();
        bool lit = false;
        for (const auto& p : positions(d)) lit = lit || p.brightness.value_or(0) > 0.0f;
        if (!lit) continue;
        uint64_t t = m->tickCount() / 32;
        if (lastLit == 0 || t - lastLit > gap) litStarts.push_back(t);
        lastLit = t;
    }
    CHECK_EQ(litStarts.size(), size_t{4});
    if (litStarts.size() < 4) return 0;
    return double(litStarts[3] - litStarts[1]) / 2;
}

// Pavel's TI-58C measured PHI 194.7 kHz on the scope (389.4 kHz clock) and
// blinks 0.9185 s stand-alone, 0.7234 s on the PC-100C (faster with the
// printer, unlike the TI-59 and TI-58). At its clock that is 11,177 and 8,803
// instructions. (Another unit blinks 0.896 s
// stand-alone, as at 400 kHz.)
TEST(error_blink_period_ti58c) {
    const double unitHz = 389400.0 / 32;
    double measured = 0.9185 * unitHz;
    double ticks = ti58cErrorBlinkTicks(false);
    CHECK(ticks > measured * 0.99 && ticks < measured * 1.01);
    measured = 0.7234 * unitHz;
    ticks = ti58cErrorBlinkTicks(true);
    CHECK(ticks > measured * 0.99 && ticks < measured * 1.01);
}

// readDisplay() reports how long each position was driven: a steady IDLE display
// reads 1.0 for lit positions and 0 for suppressed ones, a computing CPU 0.
TEST(display_duty_normalised) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58C}}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        for (int k : {kKey2, kKey3}) testroms::tap(*m, model, k);  // shows "23"
        m->readDisplay();
        run(*m, 20000);
        auto d = m->readDisplay();
        int lit = 0, dark = 0;
        for (const auto& p : positions(d)) {
            const float duty = p.brightness.value_or(-1);
            if (duty > 0.95f && duty <= 1.0f) ++lit;
            if (duty == 0.0f) ++dark;                           // leading zeros, exponent: not driven
        }
        CHECK_EQ(lit, 2);
        CHECK_EQ(dark, 10);
        auto again = m->readDisplay();                          // no cycle since: the same output
        for (size_t i = 0; i < 12; ++i) CHECK(positions(again)[i].brightness == positions(d)[i].brightness);
    }
}

TEST(ti58c_keeps_memory) {
    auto m = testroms::makeMachine(ModelId{kTI58C});
    testroms::runToScanLoop(*m, ModelId{kTI58C});
    std::vector<uint8_t> a(1920), b(1920);
    mem(*m).serialiseRAM(a.data());
    uint8_t reg[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6};
    mem(*m).writeRAMReg(5, reg);
    mem(*m).serialiseRAM(b.data());
    CHECK(a != b);
    mem(*m).deserialiseRAM(a.data());
    std::vector<uint8_t> c(1920);
    mem(*m).serialiseRAM(c.data());
    CHECK(a == c);
}

// The fast-mode entry of CHIP 10/1980 (Neef): LRN in the module falls into the
// reset routine, whose RAM.OP op 2 clears the whole RAM. Checked on a real TI-59:
// program and registers gone, t still there.
TEST(start_up_clears_memory_except_ti58c) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        const uint8_t entry[] = {2, 4, 0, 42, 0, 36, 2, 71, 40, 0, 97, 99, 99, 0, 0, 0, 1, 2, 3};
        mem(*m).writeProgram(entry, static_cast<int>(sizeof(entry)));
        auto eleven = cu59::encodeBcd("11");
        mem(*m).writeDataRegister(1, eleven.data());
        for (int key : {62, 32, 25, 81, 91}) testroms::tap(*m, model, key);  // 7 x<>t CLR RST R/S
        run(*m, 100000);
        std::vector<uint8_t> ram(1920), zero(1920, 0);
        mem(*m).serialiseRAM(ram.data());
        CHECK(ram == zero);
        for (int key : {81, 32}) testroms::tap(*m, model, key);  // RST x<>t
        CHECK_EQ(testroms::displayText(*m), std::string("7"));
    }
}

TEST(start_up_clears_memory_except_ti58c_power_cycle) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        auto eleven = cu59::encodeBcd("11");
        mem(*m).writeRAMReg(5, eleven.data());
        m->powerCycle();
        testroms::runToScanLoop(*m, model);
        uint8_t reg[16];
        mem(*m).copyRAMReg(5, reg);
        CHECK_EQ(std::equal(eleven.begin(), eleven.end(), reg), model == ModelId{kTI58C});
    }
}

TEST(card_insert_and_eject) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    // The mode (read/write) is set later by the ROM's card opcodes, not by insertion.
    card(*m).insertCard(nullptr, 0);  // blank card
    CHECK(card(*m).isCardPresent());
    card(*m).cardEject();
    CHECK(!card(*m).isCardPresent());
    CHECK_EQ(card(*m).cardMode(), 0);
}

// PRINT held down prints the display, on the text layer and as the dots of
// its characters.
TEST(print_gives_a_line) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    testroms::tap(*m, ModelId{kTI59}, kKey2);
    testroms::printerControl(*m, PC100C::kPrint, true);
    run(*m, 3000);
    testroms::printerControl(*m, PC100C::kPrint, false);
    run(*m, 200000);
    auto lines = testroms::printedLines(*m);
    CHECK(!lines.empty());
    if (!lines.empty()) CHECK_EQ(lines[0], std::string("          2.        "));
    // The dots: the "2" and the "." of the font, at full darkness, a complete line.
    const auto& paper = testroms::paperTap(*m).paper;
    std::size_t expected = 0;
    for (int code : {3, 32})
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 5; ++c) expected += (pc100cpaper::kFont[code][r] >> c) & 1u;
    CHECK_EQ(paper.marks().size(), expected);
    for (const auto& mk : paper.marks()) CHECK_EQ(static_cast<int>(std::get<calcu::paper::Dot>(mk).darkness), 20);
    CHECK_EQ(paper.reach().to, pc100cpaper::kLine);
}

// The printer interrupt cuts a line mid-row: the head fires a row's columns
// right to left, 64 instructions apart (core/PC100C.hpp kColumnInstructions).
// examples/debug/printer-interrupt-sweep.ti58c on the TI-58C, one line per k: the
// interrupt comes 2049 + 16·(k − 21) cycles after the print starts, roughly.
// Expected values are what a real TI-58C + PC-100A printed (2026-10-25 scans,
// docs/developer/Core-Architecture.md "Dot timing within a line").
static PC100C::CodeLine sweepLine(int k, std::vector<calcu::paper::Mark>* marks = nullptr) {
    const ModelId model = ModelId{kTI58C};
    auto m = testroms::makeMachine(model);
    testroms::runToScanLoop(*m, model);
    // 000 INV SBR, 001–004 Nop, 005 Op 05 Int, 008 1F, 009 RTN;
    // 100: RCL 01 +/- INV log SBR 005 R/S (prints 10^-k's Int cut short).
    std::vector<uint8_t> p(240, 0);
    const uint8_t head[] = {92, 68, 68, 68, 68, 69, 5, 59, 23, 92};
    const uint8_t drv[] = {43, 1, 94, 22, 28, 71, 0, 5, 91};
    std::copy(std::begin(head), std::end(head), p.begin());
    std::copy(std::begin(drv), std::end(drv), p.begin() + 100);
    mem(*m).writeProgram(p.data(), static_cast<int>(p.size()));
    uint8_t r[16];
    mem(*m).copyRAMReg(1, r);  // steps 008–015: step 008's digits become 1F
    r[0] = 0xF; r[1] = 1;
    mem(*m).writeRAMReg(1, r);
    auto k01 = cu59::encodeBcd(std::to_string(k));
    mem(*m).writeRAMReg(59 - 1, k01.data());  // R01
    for (int key : {61, 82, 92, 92, 91}) testroms::tap(*m, model, key);  // GTO 100 R/S
    run(*m, static_cast<uint32_t>(instructionHz(model)));
    auto lines = testroms::printedCodeLines(*m);
    if (marks) *marks = testroms::paperTap(*m).paper.marks();
    return lines.empty() ? PC100C::CodeLine{} : lines[0];
}

// A line cut short by the printer interrupt has
// the rows of the scan of a real printout, and its marks only those rows.
TEST(interrupt_rows_against_the_scan) {
    struct Case { int k, rows, columns; };
    for (Case c : {Case{21, 4, 0}, {22, 4, 1}, {26, 4, 2}, {30, 4, 3}, {36, 4, 4}, {39, 4, 4},
                   {40, 4, 5}, {60, 5, 1}, {63, 5, 2}, {69, 5, 3}}) {
        std::vector<calcu::paper::Mark> marks;
        PC100C::CodeLine l = sweepLine(c.k, &marks);
        CHECK_EQ(static_cast<int>(l.rowCount), c.rows);
        CHECK_EQ(static_cast<int>(l.partialColumns), c.columns);
        // The marks of the first line: no dot below its last drawn row.
        const int drawn = pc100cpaper::drawnRows(c.rows, c.columns);
        for (const auto& mk : marks) {
            const auto& d = std::get<calcu::paper::Dot>(mk);
            if (d.at.y >= pc100cpaper::kLine) continue;   // a later line
            CHECK(d.at.y < drawn * calcu::paper::kPerUnit);
        }
    }
}

TEST(breakpoint_sets_step_bit31) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    testroms::runToScanLoop(*m, ModelId{kTI59});
    uint16_t target = dbg(*m).pc();
    TI59Machine& hw = testroms::hw(*m);   // the core's own breakpoints (headless tools)
    hw.setTraceFlags(hw.traceFlags() | TRACE_BREAKPOINTS);
    hw.addBreakpoint(target);
    bool hit = false;
    for (int i = 0; i < 20000 && !hit; ++i) hit = (hw.step() & 0x80000000u) != 0;
    CHECK(hit);
}
