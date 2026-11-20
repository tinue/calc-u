// The TI-59, TI-58 and TI-58C behind the machine interface. The golden runs
// are golden_runs_tests.cpp, the core's own cases core_tests.cpp, the seam the
// CTest CalcU59CoreSeam.
//
// The expected values that cannot come from the golden runs:
// - Ticks: TMC0501::getStepWeight is 4 in the IDLE state and 1 otherwise,
//   never 4 on the TI-58C; one instruction is 32 ticks, so 32 and 128.
// - Partition (golden runs ti59-reset-partition and
//   ti58-reset-partition): the ROM's default partition is 479.59 on the TI-59
//   (60 program registers) and 239.29 on the TI-58 (30); "4 Op 17" makes the
//   TI-59 639.39 (80), "2 Op 17" the TI-58 319.19 (40). A Reset keeps the
//   partition.
// - Constant memory: a TI-58C at "2 Op 17" (319.19, 40)
//   and Fix 2.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../core/TIKeys.hpp"
#include "../core/TIMachineTesting.hpp"
#include "../core/TIRomRanges.hpp"
#include <algorithm>
#include <random>
#include <set>

using cu59::ModelId;
using cu59::kTI59;
using cu59::kTI58;
using cu59::kTI58C;
using calcu::machine::KeyCall;
using calcu::machine::KeyId;
using calcu::machine::Ticks;
using namespace testroms;

namespace {

const ModelId kModels[] = {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}};

TIMachine& ti(Machine& m) { return static_cast<TIMachine&>(m); }

std::vector<uint8_t> ram(Machine& m) {
    std::vector<uint8_t> r(120 * 16);
    mem(m).serialiseRAM(r.data());
    return r;
}

std::vector<uint8_t> scom(Machine& m) {
    const CpuFrame f = dbg(m).snapshotCPU();
    return std::vector<uint8_t>(&f.SCOM[0][0], &f.SCOM[0][0] + 256);
}

/// Keys by name: digits and the few named ones the tests need; Op, Fix,
/// Prt, Pause are second functions (2nd + key).
void keys(Machine& m, ModelId model, std::initializer_list<int> codes) {
    for (int c : codes) tap(m, model, c);
}
constexpr int kOp = 64, k2nd = 21, kFix = 53, kClr = 25, kSto = 42, kRcl = 43;

/// 1–9 written to registers 0 .. n-1 (RAM), as a fill to look for afterwards.
void fill(Machine& m, int regs) {
    for (int r = 0; r < regs; ++r) {
        uint8_t n[16];
        for (int i = 0; i < 16; ++i) n[i] = static_cast<uint8_t>((r + i) % 9 + 1);
        mem(m).writeRAMReg(r, n);
    }
}

bool keyMatrixClear(Machine& m) {
    // The keyboard rows 1–9 on the five keyboard K-lines; the card switch
    // (bit 4) and the printer line are not keys.
    for (int row = 1; row <= 9; ++row)
        if (TIMachineTesting::keyLines(ti(m), row) & 0b1101110) return false;
    return true;
}

} // namespace

// ── Keys in the descriptor ──────────────────────────────────────────────────

TEST(the_45_key_ids_by_row_with_spellings) {
    const std::vector<std::vector<std::string>> rows = {
        {"A", "B", "C", "D", "E"},     {"2nd", "INV", "lnx", "CE", "CLR"}, {"LRN", "x⇄t", "x²", "√x", "1/x"},
        {"SST", "STO", "RCL", "SUM", "yˣ"}, {"BST", "EE", "(", ")", "÷"}, {"GTO", "7", "8", "9", "×"},
        {"SBR", "4", "5", "6", "-"},   {"RST", "1", "2", "3", "+"},      {"R/S", "0", ".", "+/-", "="},
    };
    for (ModelId model : kModels) {
        const auto& keys = tiCatalogue().model(model)->keys;
        CHECK_EQ(keys.size(), size_t{45});
        for (size_t i = 0; i < keys.size(); ++i) CHECK(keys[i].id.value == rows[i / 5][i % 5]);
        auto spellings = [&](const char* id) {
            for (const auto& k : keys)
                if (k.id.value == id) return k.otherSpellings;
            return std::vector<std::string>{};
        };
        CHECK(spellings("x⇄t") == (std::vector<std::string>{"X⇄T", "x<>t"}));
        CHECK(spellings("x²") == (std::vector<std::string>{"X²", "x^2"}));
        CHECK(spellings("√x") == (std::vector<std::string>{"√X", "sqrt"}));
        CHECK(spellings("yˣ") == (std::vector<std::string>{"Yˣ", "y^x"}));
        CHECK(spellings("÷") == (std::vector<std::string>{"/"}));
        CHECK(spellings("×") == (std::vector<std::string>{"x", "*"}));
        for (const auto& k : keys) {   // no key is x2
            CHECK(k.id.value != "x2");
            CHECK(std::find(k.otherSpellings.begin(), k.otherSpellings.end(), "x2") == k.otherSpellings.end());
        }
    }
}

TEST(machine_no_printer_keys) {
    for (ModelId model : kModels) {
        auto m = cu59::buildMachine(model, *romSet());
        for (const char* k : {"PRINT", "ADV", "TRACE"}) {
            CHECK(!m->descriptor().hasKey(KeyId{k}));
            CHECK(m->keyDown(KeyId{k}) == KeyCall::UnknownKey);
        }
    }
}

// ── Emulated time, build ────────────────────────────────────────────────────

TEST(instruction_ticks_per_model) {
    for (ModelId model : kModels) {
        auto m = makeMachine(model);
        CHECK_EQ(m->advance(1), Ticks{32});   // the start-up: active
        CHECK(runToScanLoop(*m, model));
        std::set<Ticks> seen;
        for (int i = 0; i < 2000; ++i) {
            const bool idle = (dbg(*m).snapshotCPU().cpuFlags & 0x0001) != 0;   // FLG_IDLE
            const Ticks t = m->advance(1);
            seen.insert(t);
            if (!idle) CHECK_EQ(t, Ticks{32});
        }
        if (model == ModelId{kTI58C}) CHECK(seen == (std::set<Ticks>{32}));
        else CHECK(seen.count(128) == 1 && std::all_of(seen.begin(), seen.end(), [](Ticks t) { return t == 32 || t == 128; }));
    }
}

TEST(overshoot_below_one_instruction) {
    auto m = makeMachine(ModelId{kTI59});
    std::vector<Ticks> ns;
    for (Ticks n = 1; n <= 10'000; ++n) ns.push_back(n);
    std::minstd_rand rng(59);
    for (int i = 0; i < 200; ++i) ns.push_back(1 + rng() % 200'000);
    for (Ticks n : ns) {
        const Ticks before = m->tickCount();
        const Ticks ran = m->advance(n);
        CHECK(ran >= n && ran - n <= 127);
        CHECK_EQ(ran, m->tickCount() - before);   // reports what it ran
    }
    CHECK_EQ(m->advance(0), Ticks{0});
}

TEST(tick_count_goes_on_across_operations) {
    for (ModelId model : kModels) {
        auto m = makeMachine(model);
        CHECK_EQ(m->tickCount(), Ticks{0});
        runToScanLoop(*m, model);
        for (int op = 0; op < 3; ++op) {
            const Ticks before = m->tickCount();
            CHECK(before > 0);
            if (op == 0) m->reset(); else if (op == 1) m->powerCycle(); else m->coldBoot();
            CHECK_EQ(m->tickCount(), before);   // not back to 0
            runToScanLoop(*m, model);
            CHECK(m->tickCount() > before);
        }
    }
}

TEST(idle_advance_by_32_runs_one_of_128) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}}) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        int idleOnes = 0;
        for (int i = 0; i < 2000; ++i) {
            const Ticks t = m->advance(32);
            CHECK(t == 32 || t == 128);   // one instruction either way: it stops at ≥ 32
            if (t == 128) ++idleOnes;
        }
        CHECK(idleOnes > 0);
    }
}

TEST(new_machine_equals_cold_booted) {
    for (ModelId model : kModels) {
        auto fresh = makeMachine(model);
        auto used = makeMachine(model);
        runToScanLoop(*used, model);
        keys(*used, model, {62, kSto, 92, 82});   // 7 STO 01
        fill(*used, 60);
        used->coldBoot();
        for (Machine* m : {fresh.get(), used.get()}) {
            CHECK(runToScanLoop(*m, model));
            run(*m, 20000);
        }
        CHECK(ram(*fresh) == ram(*used));
        CHECK(scom(*fresh) == scom(*used));
        CHECK_EQ(displayText(*fresh), displayText(*used));
        CHECK_EQ(displayText(*fresh), std::string("0"));
    }
}

// ── Reset, power cycle, cold boot ───────────────────────────────────────────

TEST(reset_keeps_ram_and_partition_ti59_ti58) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}}) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        const int defaultRegs = model == ModelId{kTI59} ? 60 : 30;
        CHECK_EQ(mem(*m).partitionProgramRegs(), defaultRegs);
        keys(*m, model, {model == ModelId{kTI59} ? 72 : 83, k2nd, kOp, 82, 62});   // 4 / 2 Op 17
        const int regs = model == ModelId{kTI59} ? 80 : 40;
        CHECK_EQ(mem(*m).partitionProgramRegs(), regs);
        keys(*m, model, {62, kSto, 92, 72});   // 7 STO 04
        fill(*m, 20);                          // and program steps 0–159
        const auto before = ram(*m);
        m->reset();
        CHECK(runToScanLoop(*m, model));
        CHECK(ram(*m) == before);
        CHECK_EQ(mem(*m).partitionProgramRegs(), regs);
        keys(*m, model, {kRcl, 92, 72});       // RCL 04: the register addressing is right
        CHECK_EQ(displayText(*m), std::string("7"));
        CHECK_EQ(mem(*m).partitionProgramRegs(), regs);
    }
}

TEST(reset_equals_power_cycle_ti58c) {
    auto a = makeMachine(ModelId{kTI58C}), b = makeMachine(ModelId{kTI58C});
    for (Machine* m : {a.get(), b.get()}) {
        runToScanLoop(*m, ModelId{kTI58C});
        keys(*m, ModelId{kTI58C}, {72, 82, kSto, 92, 73});   // 41 STO 05
    }
    a->reset();
    b->powerCycle();
    for (Machine* m : {a.get(), b.get()}) runToScanLoop(*m, ModelId{kTI58C});
    CHECK(ram(*a) == ram(*b));
    CHECK(scom(*a) == scom(*b));
    CHECK_EQ(a->tickCount(), b->tickCount());
    CHECK_EQ(dbg(*a).pc(), dbg(*b).pc());
}

TEST(power_cycle_loses_memory_ti59_ti58) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}}) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        keys(*m, model, {72, k2nd, kOp, 82, 62});   // 4 Op 17
        fill(*m, 60);
        m->powerCycle();
        CHECK(runToScanLoop(*m, model));
        auto fresh = makeMachine(model);
        runToScanLoop(*fresh, model);
        CHECK(ram(*m) == ram(*fresh));   // as the start-up leaves a machine whose memory was lost
        CHECK_EQ(mem(*m).partitionProgramRegs(), model == ModelId{kTI59} ? 60 : 30);
    }
}

TEST(power_cycle_keeps_constant_memory_ti58c) {
    auto m = makeMachine(ModelId{kTI58C});
    runToScanLoop(*m, ModelId{kTI58C});
    keys(*m, ModelId{kTI58C}, {72, 82, kSto, 92, 73});   // 41 STO 05
    for (int h = 1; h <= 3; ++h) {                    // H01–H03 (RAM 61–63); H00 is the ROM's control register
        uint8_t n[16] = {};
        n[15] = static_cast<uint8_t>(h);
        mem(*m).writeRAMReg(cu59::kExtraRegisterBase + h, n);
    }
    const auto before = ram(*m);
    m->powerCycle();
    CHECK(runToScanLoop(*m, ModelId{kTI58C}));
    const auto after = ram(*m);
    CHECK(std::equal(before.begin(), before.begin() + 64 * 16, after.begin()));   // all of its RAM, H00–H03 included
    keys(*m, ModelId{kTI58C}, {kRcl, 92, 73});
    CHECK_EQ(displayText(*m), std::string("41"));
}

TEST(cold_boot_from_zeroed_memory) {
    for (ModelId model : kModels) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        keys(*m, model, {62, kSto, 92, 82});
        fill(*m, 60);
        m->coldBoot();
        CHECK(runToScanLoop(*m, model));
        auto fresh = makeMachine(model);
        runToScanLoop(*fresh, model);
        CHECK(ram(*m) == ram(*fresh));
        if (model == ModelId{kTI58C}) {   // its ROM never clears the RAM: zero is what was loaded
            const auto r = ram(*m);
            CHECK(std::all_of(r.begin() + cu59::kExtraRegisterBase * 16 + 16, r.begin() + 64 * 16, [](uint8_t b) { return b == 0; }));
        }
    }
}

TEST(constant_memory_in_place_before_start_up) {
    // Partition 319.19 and Fix 2 come back on a TI-58C built with its
    // constant memory.
    auto a = makeMachine(ModelId{kTI58C});
    runToScanLoop(*a, ModelId{kTI58C});
    keys(*a, ModelId{kTI58C}, {83, k2nd, kOp, 82, 62, kClr, k2nd, kFix, 83});   // 2 Op 17 (319.19), CLR, Fix 2
    CHECK_EQ(mem(*a).partitionProgramRegs(), 40);
    auto content = std::make_shared<TIConstantMemory>();
    mem(*a).serialiseRAM(content->ram.data());
    auto b = cu59::buildMachine(ModelId{kTI58C}, *romSet(), {}, content);
    CHECK(ram(*b) == ram(*a));   // in place before the processor starts
    CHECK(runToScanLoop(*b, ModelId{kTI58C}));
    CHECK_EQ(mem(*b).partitionProgramRegs(), 40);
    for (Machine* m : {a.get(), b.get()}) {
        keys(*m, ModelId{kTI58C}, {kClr, 82, 55, 84, 95});   // 1 ÷ 3 =
        m->readDisplay();   // a fresh window: what is shown now
        run(*m, 20000);
    }
    CHECK_EQ(displayText(*b), std::string("033"));   // 0.33: Fix 2 came back
    CHECK_EQ(displayText(*a), std::string("033"));
}

TEST(no_key_down_and_switched_on_display_after_each_operation) {
    for (ModelId model : kModels) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        for (int op = 0; op < 3; ++op) {
            m->keyDown(KeyId{"5"});
            m->keyDown(KeyId{"CLR"});
            CHECK(!keyMatrixClear(*m));
            if (op == 0) m->reset(); else if (op == 1) m->powerCycle(); else m->coldBoot();
            CHECK(keyMatrixClear(*m));
            CHECK(runToScanLoop(*m, model));
            run(*m, 20000);
            CHECK_EQ(displayText(*m), std::string("0"));
        }
    }
}

TEST(restored_before_key_readiness) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}}) {
        auto m = makeMachine(model);
        runToScanLoop(*m, model);
        keys(*m, model, {72, k2nd, kOp, 82, 62});   // a partition other than the default
        fill(*m, 20);
        const auto before = ram(*m);
        const int regs = mem(*m).partitionProgramRegs();
        m->reset();
        bool sawCleared = false;
        for (int i = 0; i < 200'000 && !m->isKeyReady(); ++i) {
            CHECK(TIMachineTesting::restorePending(ti(*m)));
            m->advance(1);
            sawCleared = sawCleared || ram(*m) != before;
        }
        CHECK(sawCleared);   // the ROM's start-up did clear it
        CHECK(m->isKeyReady());
        CHECK(!TIMachineTesting::restorePending(ti(*m)));
        CHECK(ram(*m) == before);
        CHECK_EQ(mem(*m).partitionProgramRegs(), regs);
    }
}

// ── Key readiness ───────────────────────────────────────────────────────────

TEST(key_ready_in_scan_loop_each_model_with_and_without_printer) {
    for (ModelId model : kModels) {
        for (bool printer : {false, true}) {
            auto m = makeMachine(model, printer);
            CHECK(runToScanLoop(*m, model));
            run(*m, 1000);
            bool steady = true;
            for (int i = 0; i < 20'000; ++i) {
                m->advance(1);
                steady = steady && m->isKeyReady();
            }
            CHECK(steady);
        }
    }
}

TEST(not_key_ready_while_a_program_runs) {
    // Lbl A GTO A; with Pause; with Prt (printer on).
    const std::vector<std::vector<uint8_t>> programs = {{76, 11, 61, 11}, {76, 11, 66, 61, 11}, {76, 11, 99, 61, 11}};
    for (ModelId model : kModels) {
        for (size_t p = 0; p < programs.size(); ++p) {
            auto m = makeMachine(model, p == 2);
            runToScanLoop(*m, model);
            mem(*m).writeProgram(programs[p].data(), static_cast<int>(programs[p].size()));
            press(*m, 11);
            run(*m, static_cast<uint32_t>(instructionHz(model) * 0.1));
            release(*m, 11);
            run(*m, static_cast<uint32_t>(instructionHz(model) * 0.1));
            bool ready = false;
            for (int i = 0; i < static_cast<int>(instructionHz(model) * 2); ++i) {
                m->advance(1);
                ready = ready || m->isKeyReady();
            }
            CHECK(!ready);
        }
    }
}

// ── Keys ────────────────────────────────────────────────────────────────────

TEST(each_key_id_presses_its_row_and_column) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    for (const auto& k : tiCatalogue().model(ModelId{kTI59})->keys) {
        const auto code = cu59::matrixCodeOf(k.id);
        CHECK(code.has_value());
        if (!code) continue;
        const int row = *code / 10, col = *code % 10;
        CHECK(cu59::keyIdAt(*code) == k.id);
        const uint8_t bit = static_cast<uint8_t>(1u << kTIKeyLineBits[col]);
        CHECK(m->keyDown(k.id) == KeyCall::Done);
        CHECK_EQ(TIMachineTesting::keyLines(ti(*m), row) & 0b1101110, int(bit));
        for (int other = 1; other <= 9; ++other)
            if (other != row) CHECK_EQ(TIMachineTesting::keyLines(ti(*m), other) & 0b1101110, 0);
        CHECK(m->keyUp(k.id) == KeyCall::Done);
        CHECK(keyMatrixClear(*m));
    }
}

TEST(two_keys_down_both_in_the_matrix) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    m->keyDown(KeyId{"2"});
    m->keyDown(KeyId{"3"});
    CHECK_EQ(TIMachineTesting::keyLines(ti(*m), 8) & 0b1101110, (1 << kTIKeyLineBits[3]) | (1 << kTIKeyLineBits[4]));
    m->keyDown(KeyId{"A"});
    CHECK_EQ(TIMachineTesting::keyLines(ti(*m), 1) & 0b1101110, 1 << kTIKeyLineBits[1]);
    run(*m, 3000);   // the ROM decides; the machine keeps both
    CHECK_EQ(TIMachineTesting::keyLines(ti(*m), 8) & 0b1101110, (1 << kTIKeyLineBits[3]) | (1 << kTIKeyLineBits[4]));
}

TEST(unknown_key_refused_state_unchanged) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    std::vector<uint8_t> lines;
    for (int c = 0; c < 16; ++c) lines.push_back(TIMachineTesting::keyLines(ti(*m), c));
    const Ticks t = m->tickCount();
    for (const char* k : {"x2", "PRINT", "", "x<>t", "sqrt"}) {   // other spellings are not IDs
        CHECK(m->keyDown(KeyId{k}) == KeyCall::UnknownKey);
        CHECK(m->keyUp(KeyId{k}) == KeyCall::UnknownKey);
    }
    for (int c = 0; c < 16; ++c) CHECK_EQ(TIMachineTesting::keyLines(ti(*m), c), lines[static_cast<size_t>(c)]);
    CHECK_EQ(m->tickCount(), t);
}

// ── Display output ──────────────────────────────────────────────────────────

TEST(twelve_positions_rightmost_first_segments_and_brightness) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    keys(*m, ModelId{kTI59}, {82, 83, 84});   // 123
    m->readDisplay();
    run(*m, 20000);
    const auto out = m->readDisplay();
    const auto& ps = positions(out);
    CHECK_EQ(ps.size(), size_t{12});
    std::vector<int> lit;
    for (size_t i = 0; i < ps.size(); ++i) {
        CHECK_EQ(ps[i].lit.size(), size_t{8});
        CHECK(ps[i].brightness.has_value());
        CHECK(*ps[i].brightness >= 0.0f && *ps[i].brightness <= 1.0f);
        CHECK(ps[i].segmentBrightness[kTIDecimalPoint].has_value());   // DP: its own brightness
        for (int s = 0; s < 7; ++s) CHECK(!ps[i].segmentBrightness[static_cast<size_t>(s)].has_value());
        if (*ps[i].brightness > 0) lit.push_back(static_cast<int>(i));
    }
    CHECK_EQ(lit.size(), size_t{3});
    if (lit.size() == 3) {   // index 0 is the rightmost: the lowest index is the last digit
        CHECK(glyphText(segmentBits(ps[static_cast<size_t>(lit[0])])) == "3");
        CHECK(glyphText(segmentBits(ps[static_cast<size_t>(lit[2])])) == "1");
    }
    // The decimal point: 1.5 lights the DP of the "1" only.
    keys(*m, ModelId{kTI59}, {kClr, 82, 93, 73});
    m->readDisplay();
    run(*m, 20000);
    const auto dp = m->readDisplay();
    int dps = 0;
    for (const auto& p : positions(dp))
        if (p.lit[kTIDecimalPoint]) {
            ++dps;
            CHECK(*p.segmentBrightness[kTIDecimalPoint] > 0.0f);
            CHECK(glyphText(segmentBits(p)) == "1");
        }
    CHECK_EQ(dps, 1);
}

TEST(digit_minus_degree_quote_apostrophe_blank_and_zero_suppression) {
    for (int d = 0; d < 10; ++d)
        for (uint8_t ctrl : {0, 1, 8, 9}) CHECK_EQ(segmentsFor(static_cast<uint8_t>(d), ctrl, false), kDigitSegments[d]);
    CHECK_EQ(segmentsFor(0, 6, false), kSegmentsMinus);
    CHECK_EQ(segmentsFor(3, 5, false), kSegmentsDegree);
    CHECK_EQ(segmentsFor(0, 2, false), kSegmentsQuote);
    CHECK_EQ(segmentsFor(0, 4, false), kSegmentsApostrophe);
    for (uint8_t ctrl : {3, 7, 10, 15}) CHECK_EQ(segmentsFor(5, ctrl, false), 0);
    CHECK_EQ(segmentsFor(8, 0, true), 0);   // zero suppression
    // On the machine: -23 shows the minus left of the digits; the leading
    // zeros are suppressed, dark and with no segment lit.
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    keys(*m, ModelId{kTI59}, {83, 84, 94});
    m->readDisplay();
    run(*m, 20000);
    const auto out = m->readDisplay();
    std::string text;
    for (int i = 11; i >= 0; --i) {
        const auto& p = positions(out)[static_cast<size_t>(i)];
        if (p.brightness.value_or(0) > 0) text += glyphText(segmentBits(p));
        else CHECK_EQ(segmentBits(p), 0);
    }
    CHECK_EQ(text, std::string("-23"));
}

TEST(c_indicator_apart_from_leftmost_position) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    // Waiting for a key: the number, C off.
    keys(*m, ModelId{kTI59}, {82});
    m->readDisplay();
    run(*m, 20000);
    auto waiting = m->readDisplay();
    CHECK_EQ(waiting.indicators.size(), size_t{1});
    CHECK(waiting.indicators[0].name == "C");
    CHECK(!waiting.indicators[0].on);
    bool shown = false;
    for (const auto& p : positions(waiting)) shown = shown || p.brightness.value_or(0) > 0;
    CHECK(shown);
    // A running program: the positions dark, C on.
    const uint8_t loop[] = {76, 11, 61, 11};
    mem(*m).writeProgram(loop, 4);
    press(*m, 11);
    run(*m, 2000);
    release(*m, 11);
    run(*m, 5000);
    m->readDisplay();
    run(*m, 5000);
    auto running = m->readDisplay();
    for (const auto& p : positions(running)) CHECK_EQ(p.brightness.value_or(1), 0.0f);
    CHECK(running.indicators[0].on && running.indicators[0].brightness.value_or(0) > 0.0f);
    // C and a minus in the leftmost position in one output, each with its own
    // brightness: the error blink of a negative result (1 +/- ÷ 0 =).
    auto e = makeMachine(ModelId{kTI59});
    runToScanLoop(*e, ModelId{kTI59});
    keys(*e, ModelId{kTI59}, {82, 94, 55, 92, 95});
    bool together = false;
    for (int i = 0; i < 200 && !together; ++i) {
        run(*e, 237);   // about 1/60 s
        const auto o = e->readDisplay();
        const auto& left = positions(o)[11];
        together = o.indicators[0].on && left.brightness.value_or(0) > 0 && segmentBits(left) == kSegmentsMinus;
        if (together) CHECK(o.indicators[0].brightness.has_value() && left.brightness.has_value());
    }
    CHECK(together);
}

TEST(no_sound_display_always_on) {
    for (ModelId model : kModels) {
        auto m = makeMachine(model);
        int16_t buf[64];
        CHECK(!m->descriptor().hasSound);
        CHECK_EQ(m->takeSound(buf), size_t{0});
        CHECK(m->readDisplay().on);
        runToScanLoop(*m, model);
        CHECK(m->readDisplay().on);
        CHECK_EQ(m->takeSound(buf), size_t{0});
        m->reset();
        CHECK(m->readDisplay().on);
    }
}

TEST(two_reads_without_advance_are_equal) {
    auto m = makeMachine(ModelId{kTI59});
    runToScanLoop(*m, ModelId{kTI59});
    keys(*m, ModelId{kTI59}, {83, 84});
    run(*m, 5000);
    const auto a = m->readDisplay();
    const auto b = m->readDisplay();
    for (size_t i = 0; i < 12; ++i) {
        CHECK(positions(a)[i].lit == positions(b)[i].lit);
        CHECK(positions(a)[i].brightness == positions(b)[i].brightness);
        CHECK(positions(a)[i].segmentBrightness == positions(b)[i].segmentBrightness);
    }
    CHECK(a.indicators[0].on == b.indicators[0].on && a.indicators[0].brightness == b.indicators[0].brightness);
}

// ── The card switch on the readerless models ────────────────────────────────

TEST(ti58_ti58c_find_their_card_switch_line) {
    for (ModelId model : {ModelId{kTI58}, ModelId{kTI58C}}) {
        auto m = makeMachine(model);
        for (int op = 0; op < 4; ++op) {
            if (op == 1) m->reset(); else if (op == 2) m->powerCycle(); else if (op == 3) m->coldBoot();
            CHECK(runToScanLoop(*m, model));
            if (model != ModelId{kTI58}) continue;
            bool veto = false;
            for (int i = 0; i < 20'000 && !veto; ++i) {
                m->advance(1);
                veto = dbg(*m).pc() >= kCardVetoLo && dbg(*m).pc() <= kCardVetoHi;
            }
            CHECK(veto);   // the idle pass goes through the card veto
        }
    }
}
