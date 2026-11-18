// app/ debug model: program source decoding, module image, X, the step
// views; what the debug panel shows is in debug_panel_tests.cpp.

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/Bcd.hpp"
#include "../app/DebugLog.hpp"
#include "../app/DebugSnapshots.hpp"
#include "../app/DebugText.hpp"
#include <algorithm>
#include <filesystem>

using namespace cu59;

namespace {
/// X as the CALCULATOR tab shows it after tapping `keys` (matrix codes) on a fresh TI-59.
std::string xAfter(std::initializer_list<int> keys) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    for (int k : keys) testroms::tap(*m, Model::TI59, k);
    std::vector<uint8_t> romCache;
    auto s = buildLiveSnapshot(*m, ModuleImage{}, romCache, calcViewState(*m)).snap;
    return xRegisterLine(s.xValue, s.xRaw);
}
} // namespace

// Matrix codes: digits 0 92, 1 82, 2 83, 3 84, 5 73, 7 62; . 93, +/- 94, EE 52,
// INV 22, = 95, + 85, / 55, √x 34.
TEST(x_register_entry_and_result) {
    CHECK_EQ(xAfter({}), std::string("X: 0"));
    // Number entry: the image in A.
    CHECK_EQ(xAfter({82, 83, 93, 73}), std::string("X: 12.5"));
    CHECK_EQ(xAfter({82, 83, 93, 73, 94}), std::string("X: -12.5"));
    CHECK_EQ(xAfter({92, 93, 92, 92, 73}), std::string("X: 0.005"));
    CHECK_EQ(xAfter({82, 93, 73, 52, 84}), std::string("X: 1500"));                 // EE display form
    CHECK_EQ(xAfter({82, 83, 94, 52, 84, 94}), std::string("X: -0.012"));
    CHECK_EQ(xAfter({82, 93, 73, 52, 84, 22, 52, 83}), std::string("X: 1520"));     // INV EE keeps the exponent
    CHECK_EQ(xAfter({82, 85, 83}), std::string("X: 2"));
    CHECK_EQ(xAfter({73, 94, 85, 84, 95, 62}), std::string("X: 7"));                // fB's stale minus is ignored
    // Result on display: C, all 13 digits.
    CHECK_EQ(xAfter({83, 34}), std::string("X: 1.414213562373"));
    CHECK_EQ(xAfter({82, 55, 84, 95}), std::string("X: 0.3333333333333"));
    CHECK_EQ(xAfter({82, 52, 73, 94, 95}), std::string("X: 1e-05"));
    CHECK_EQ(xAfter({73, 94, 85, 84, 95}), std::string("X: -2"));
    CHECK_EQ(xAfter({82, 55, 92, 95}), std::string("X: 9.999999999999e+99"));        // overflow clamp
}

namespace {
/// The CALCULATOR tab's current step and X after each debugger STEP through
/// `prog` (started with A, so it begins with LBL A), until step `last` is next.
std::vector<std::string> stepThrough(const std::vector<uint8_t>& prog, int last, Model model = Model::TI59) {
    auto m = testroms::makeMachine(model);
    testroms::runToScanLoop(*m, model);
    testroms::mem(*m).writeProgram(prog.data(), prog.size());
    testroms::press(*m, 11);  // A
    testroms::stepToBoundary(*m);
    testroms::release(*m, 11);
    std::vector<uint8_t> romCache;
    std::vector<std::string> steps;
    for (int i = 0; i < 40; ++i) {
        testroms::stepToBoundary(*m);
        auto state = calcViewState(*m);
        CHECK(state.atStep && state.cpu.pc == testroms::dbg(*m).pc());  // STEP stopped at the boundary
        auto snap = buildLiveSnapshot(*m, ModuleImage{}, romCache, state).snap;
        steps.push_back(std::to_string(snap.currentStep) + " " + xRegisterLine(snap.xValue, snap.xRaw));
        if (snap.currentStep >= last) break;
    }
    return steps;
}
} // namespace

// Each STEP stops at the step dispatch and shows the next step, one byte at a
// time (operands included), as LRN does after each SST: the GTO completes in
// its last byte and lands on its target, with X intact.
TEST(step_shows_the_next_step) {
    // LBL A, 1, +, GTO 00 17 (GTO at 004, operands at 005/006), Nop…, 2, =, R/S
    std::vector<uint8_t> prog = {76, 11, 1, 85, 61, 0, 17};
    prog.resize(17, 68);
    prog.insert(prog.end(), {2, 95, 91});
    const std::vector<std::string> gto = {"3 X: 1", "4 X: 1", "5 X: 1", "6 X: 1", "17 X: 1", "18 X: 2", "19 X: 3"};
    CHECK(stepThrough(prog, 19) == gto);
    // An entry with EE inside a program: A is realigned before each dispatch.
    // LBL A, 1, ., 5, EE, 3, +, 2, =, R/S
    CHECK(stepThrough({76, 11, 1, 93, 5, 52, 3, 85, 2, 95, 91}, 10) ==
          (std::vector<std::string>{"3 X: 1", "4 X: 1", "5 X: 1.5", "6 X: 1.5", "7 X: 1500", "8 X: 1500", "9 X: 2",
                                    "10 X: 1502"}));
    // P→R from a program runs a keycode sub-program from the constant ROM; its
    // launch leaves X normalized in A for the sub-program's first step.
    // LBL A, 3, x⇄t, 4, P→R, R/S
    auto pr = stepThrough({76, 11, 3, 32, 4, 37, 91}, 285);
    CHECK(std::find(pr.begin(), pr.end(), "284 X: 4") != pr.end());
    CHECK(std::none_of(pr.begin(), pr.end(), [](const std::string& s) { return s.find("X: A=") != std::string::npos; }));
    // The TI-58C's ROM has its dispatch elsewhere (082B).
    CHECK(stepThrough(prog, 19, Model::TI58C) == gto);
}

// While a program runs, the view is built from the last latched boundary, so
// sampling at arbitrary instructions (as the 60 Hz refresh does) never shows X
// mid-operation; the live CPU state at those samples often would.
TEST(running_view_shows_only_step_boundaries) {
    for (Model model : {Model::TI59, Model::TI58C}) {
        auto m = testroms::makeMachine(model);
        testroms::runToScanLoop(*m, model);
        // LBL A, 2, √x, ×, 3, =, 1/x, +, 7, =, GTO 00 02
        const uint8_t prog[] = {76, 11, 2, 34, 65, 3, 95, 35, 85, 7, 95, 61, 0, 2};
        testroms::mem(*m).writeProgram(prog, sizeof prog);
        testroms::press(*m, 11);  // A
        testroms::run(*m, 2000);
        testroms::release(*m, 11);
        std::vector<uint8_t> romCache;
        int raw = 0, liveRaw = 0;
        for (int i = 0; i < 3000; ++i) {
            testroms::run(*m, 37);
            raw += !buildLiveSnapshot(*m, ModuleImage{}, romCache, calcViewState(*m)).snap.xRaw.empty();
            TMC0501::StepBoundary live{testroms::dbg(*m).snapshotCPU(), testroms::card(*m).libExecPC(), false, 0};
            liveRaw += !buildLiveSnapshot(*m, ModuleImage{}, romCache, live).snap.xRaw.empty();
        }
        CHECK_EQ(raw, 0);
        CHECK(liveRaw > 100);
    }
}

// At idle the cursor is what LRN shows after SST: the counter + 1, or the
// counter itself after RST or a jump (fB[11]).
TEST(idle_cursor_matches_lrn_after_sst) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    std::vector<uint8_t> prog = {1, 85, 61, 0, 17};  // 1 + GTO 00 17 Nop… 2 = R/S
    prog.resize(17, 68);
    prog.insert(prog.end(), {2, 95, 91});
    testroms::mem(*m).writeProgram(prog.data(), prog.size());
    std::vector<uint8_t> romCache;
    auto next = [&] {
        return buildLiveSnapshot(*m, ModuleImage{}, romCache, calcViewState(*m))
            .snap.currentStep;
    };
    testroms::tap(*m, Model::TI59, 81);  // RST
    CHECK_EQ(next(), 0);
    const int expected[] = {1, 2, 3, 4, 17, 18};
    for (int e : expected) {
        testroms::tap(*m, Model::TI59, 41);  // SST
        CHECK_EQ(next(), e);
    }
}

TEST(program_source_decoding) {
    CHECK(programSourceFrom(0, 0x0100) == ProgramSource::UserProgram);
    CHECK(programSourceFrom(1, 0x0100) == ProgramSource::SolidState);
    CHECK(programSourceFrom(9, 0x0100) == ProgramSource::SolidState);   // bit 0 wins over bit 3
    CHECK(programSourceFrom(8, 0x0100) == ProgramSource::Rom);
    CHECK(programSourceFrom(2, 0x0100) == ProgramSource::FastMode);
    CHECK(programSourceFrom(4, 0x1193) == ProgramSource::FastMode);     // only flag 2 is the return window
    CHECK(programSourceFrom(2, 0x1194) == ProgramSource::SolidStateReturn);
    CHECK(programSourceFrom(2, 0x1500) == ProgramSource::SolidStateReturn);
    CHECK(programSourceFrom(2, 0x1507) == ProgramSource::FastMode);
    CHECK(stepsFromScomPC(ProgramSource::Rom) && !stepsFromScomPC(ProgramSource::SolidState));
}

TEST(module_image_program_table) {
    auto raw = cu59::moduleImage(*testroms::romSet(), "ML.rom.yaml");
    CHECK(raw.has_value());
    if (!raw) return;
    std::string err;
    auto img = ModuleImage::parse(*raw, &err);
    CHECK(err.empty());
    CHECK_EQ(img.keycodes.size(), raw->size());
    CHECK(img.programRanges.size() >= 20);
    for (size_t i = 1; i < img.programRanges.size(); ++i)
        CHECK(img.programRanges[i - 1].second <= img.programRanges[i].first);
    auto first = img.programRanges[0];
    auto step = img.libraryStep(static_cast<uint16_t>(first.first + 3));
    CHECK(step && step->program == 1 && step->step == 3);
    CHECK(!img.libraryStep(0xFFFF));
    CHECK_EQ(img.keycodes[0], static_cast<uint8_t>(((*raw)[0] >> 4) * 10 + ((*raw)[0] & 0xF)));  // BCD-decoded

    auto bad = ModuleImage::parse({0x02, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x20}, &err);
    CHECK(bad.programRanges.empty());
    CHECK(!err.empty());
}

TEST(debug_panel_text) {
    CHECK_EQ(debugPanelFontSize(300), 9);
    CHECK_EQ(debugPanelFontSize(400), 11);
    CHECK_EQ(debugPanelFontSize(800), 13);
}

TEST(mobile_layout) {
    CHECK(mobileLayout(440, 956, false) == MobileLayout::Paged);               // iPhone portrait
    CHECK(mobileLayout(832, 419, false) == MobileLayout::CalcPrinter);         // iPhone landscape (safe area)
    CHECK(mobileLayout(880, 645, false) == MobileLayout::CalcPrinterDebugger); // iPhone Duo open (safe area)
    CHECK(mobileLayout(880, 645, true) == MobileLayout::Paged);                // Duo laptop pose
    CHECK(mobileLayout(669, 951, false) == MobileLayout::Paged);               // Duo open, tall
    CHECK(mobileLayout(1133, 744, false) == MobileLayout::CalcPrinterDebugger); // iPad mini landscape
    CHECK_EQ(mobilePrinterWidth(MobileLayout::CalcPrinterDebugger, 880).max, 220.0); // Duo: narrow tape
    CHECK_EQ(mobilePrinterWidth(MobileLayout::CalcPrinterDebugger, 1133).max, 360.0); // iPad
    CHECK_EQ(mobilePrinterWidth(MobileLayout::CalcPrinter, 832).min, 290.0);
    CHECK(!mobileBarLabels(1133)); // iPad mini landscape: icons
    CHECK(mobileBarLabels(1366));  // iPad Pro 13" landscape: labels
    CHECK_EQ(mobileTopMargin(0), 32.0);  // iPhone Duo open
    CHECK_EQ(mobileTopMargin(24), 0.0);  // iPad: status bar inset
    CHECK_EQ(partitionLine(60, 60), std::string("Steps: 480, Registers: 60 (479.59)"));
    CHECK_EQ(partitionLine(120, 0), std::string("Steps: 960, Registers: 0 (959.)"));
    CHECK_EQ(angleModeLabel(std::nullopt), std::string("?"));
    CHECK_EQ(angleModeLabel(AngleMode::Grad), std::string("GRAD"));
    CHECK_EQ(registerLine({5, 2.5, false}), std::string("R05 = 2.5"));
    CHECK_EQ(registerLine({2, 1731371734.99909, true}), std::string("H02 = 1731371734.999"));  // 13 digits
    CHECK_EQ(hirLine(1, 3.14159265), std::string("1: 3.1416"));
    CHECK_EQ(tRegisterLine(0), std::string("T: 0"));
    CHECK_EQ(xRegisterLine(-2.5, ""), std::string("X: -2.5"));
    CHECK_EQ(xRegisterLine(0, "C=000000000000000a"), std::string("X: C=000000000000000a"));
    CHECK(xRegisterColor(0, "") != xRegisterColor(1, "") && xRegisterColor(1, "") != xRegisterColor(1, "A=f"));
    CHECK_EQ(returnLevelColor(0, false), 0x409940u);
    CHECK_EQ(returnLevelColor(2, true), returnLevelColor(1, true));
    CHECK_EQ(returnLevelColor(5, false), 0xCCCCCCu);
    CHECK_EQ(currentStepMarker(true), std::string("← next"));
    CHECK_EQ(programStepLine({38, 42, "STO", false}, true), std::string("038  42  STO"));
    CHECK_EQ(programStepLine({7, 5, "5", true}, false), std::string("007  05  5  ← running"));
    CHECK_EQ(returnLevelLine(1, 38), std::string("L1:038"));
    InspectorEntry e{};
    e.pc = 0x0837;
    e.opcode = 0x0A12;
    e.disasm = "BRC 0x0840";
    CHECK_EQ(inspectorLine(e), std::string("0837  0A12  BRC 0x0840"));
    CHECK_EQ(stepHighlightColor(ProgramSource::FastMode), stepHighlightColor(ProgramSource::UserProgram));
    CHECK(condFlag(0x0800) && !condFlag(0x0001) && idleFlag(0x0001));
    uint8_t n[16] = {0x15, 0xA};  // high bits are masked
    CHECK_EQ(registerNibbles(n), std::string("00000000000000A5"));
    CHECK_EQ(hexBin16(0x1F0A), std::string("1F0A [0001 1111 0000 1010]"));
}
