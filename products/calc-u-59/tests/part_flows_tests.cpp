// The product's flows in cu59::TIAppPart on the library's app session: model
// switches, presets, the TI-58C memory file, freezes, the ASM overlay, the
// places of C59-6, cards and cue cards. Ported in M-34 from 2.3.0's
// SessionController tests (shared_controller_tests.cpp and the controller
// half of shared_cooperative_tests.cpp, a FakePlatform and the controller's
// cooperative batches): the same checks, on the part as every frontend hosts
// it (SessionRig.hpp, PartRig), paced by the library's pacer.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include "../app/CardFile.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../app/ModuleImage.hpp"
#include "../app/TextUtil.hpp"
#include "../core/PC100C.hpp"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

using namespace cu59;
using namespace std::chrono_literals;
using testroms::PartRig;

namespace {

std::vector<uint8_t> bytes(const std::string& s) { return std::vector<uint8_t>(s.begin(), s.end()); }

bool waitForScanLoop(PartRig& r) {
    return r.runUntil([&] { return r.part().emulator()->access().machine().isKeyReady(); }, 3000ms);
}

bool cardIdle(PartRig& r) { return r.part().cardState() == CardState::NoCard; }

// The run control of the part's session (LIB-10; the freeze until M-53).
calcu::debugger::RunControl& rc(PartRig& r) { return r.part().debuggerPart()->runControl(); }
using calcu::debugger::kCpu;
using calcu::debugger::kProgram;

const std::string kMem = "cards/ti58c.mem";

} // namespace

TEST(part_loads_a_preset_for_another_model) {
    PartRig r;
    auto& c = r.part();
    c.loadStateFile(bytes("MODEL: TI-58\nREGISTERS:\n05 = 3.25\n"));
    CHECK(c.model() == Model::TI58);
    CHECK(!c.isFrozen());
    CHECK_EQ(testroms::mem(c.emulator()->access().machine()).readDataReg(5), 3.25);
    CHECK(r.errors().empty());
}

TEST(part_reports_parse_errors_and_partition_errors) {
    PartRig r(Model::TI58);
    auto& c = r.part();
    c.loadStateFile(bytes("PARTITION: 959\n"));  // beyond the TI-58's 479
    CHECK_EQ(r.errors().size(), size_t{1});
    c.loadStateFile(std::nullopt);
    CHECK_EQ(r.errors().back(), std::string("State file load error: Cannot read file."));
    c.loadStateFile(bytes("\xff\xfe"));
    CHECK(r.errors().back().find("not valid UTF-8") != std::string::npos);
}

TEST(part_load_while_frozen_ends_running_without_tracing) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kProgram);
    CHECK(r.runUntil([&] { return c.isFrozen() && !c.debug().cpuHistory().empty(); }, 3000ms));
    c.loadStateFile(bytes("REGISTERS:\n01 = 7\n"));
    CHECK(!c.isFrozen());
    r.run(20ms);
    CHECK_EQ(testroms::hw(c.emulator()->access().machine()).traceFlags(), static_cast<uint32_t>(TRACE_NONE));
}

TEST(part_keystroke_controls_drive_the_gui) {
    PartRig r;
    auto& c = r.part();
    // Lbl A 1 + 1 = R/S, armed FREEZE ON START: A freezes before the program's first step.
    c.loadStateFile(bytes("PROGRAM:\n76 11 01 85 01 95 91\n"
                          "KEYSTROKES:\nPrinterTrace: on\nCutPaper:\nDebugPanel: CPU\n"
                          "FreezeOnStart: Calculator\n11\n"));
    CHECK(r.runUntil([&] { return c.isFrozen(); }, 10000ms));
    CHECK(c.printerTrace());
    r.run(20ms);
    CHECK(r.panels() == (std::vector<DebugPanel>{DebugPanel::Cpu}));
    CHECK(r.runUntil([&] { return !c.keystrokesPlaying(); }, 10000ms));
    c.loadStateFile(bytes("SKIP-RESET: on\n"));  // retired: an error, then a normal load
    CHECK(!r.errors().empty() && r.errors().back().find("SKIP-RESET: no longer exists") != std::string::npos);
    CHECK(!c.isFrozen());
}

TEST(part_constant_memory_keeps_the_loaded_card) {
    PartRig r(Model::TI58C);
    auto& c = r.part();
    c.loadStateFile(bytes("CUECARD:\nTemplate: MagnetCard\nTitle: KEEP ME\nREGISTERS:\n03 = 1\n"));
    CHECK(r.p.cards.count(kMem) && r.p.cards[kMem].find("KEEP ME") != std::string::npos);
    // Unchanged RAM: the debounced persist does not write again.
    const int writes = r.p.writes[kMem];
    rc(r).pause(kCpu);
    CHECK(r.runUntil([&] { return c.isFrozen(); }, 3000ms));
    r.run(700ms);
    CHECK_EQ(r.p.writes[kMem], writes);
}

// LIB-10 AC-35 (G11): a new machine disarms pause-on-start.
TEST(lib10_ac35_part_model_switch_keeps_printer_and_log_resets_freeze) {
    PartRig r;
    auto& c = r.part();
    c.setPrinterConnected(false);
    c.dumpVars();
    rc(r).pauseOnStart(kProgram);
    CHECK(c.runState().armed == kProgram);
    CHECK(c.startMachine(Model::TI58));
    CHECK(!c.printerConnected());
    CHECK(testroms::moduleIn(c.emulator()->access().machine()) == "ML");
    CHECK_EQ(c.emulator()->access().machine().plugged().byPlace.size(), size_t{1});   // the module only
    r.run(20ms);
    CHECK(!c.runState().armed && !c.isFrozen());
    CHECK(c.logText().find("──") != std::string::npos);  // the dump survived the switch
    CHECK(c.cardState() == CardState::NoCard);
}

TEST(part_reset_keeps_the_memory_and_clean_reset_clears_it) {
    PartRig r;
    auto& c = r.part();
    CHECK(waitForScanLoop(r));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t reg[16];
    c.emulator()->access().memory().writeRAMReg(90, marked);
    c.resetMachine();
    CHECK(waitForScanLoop(r));
    c.emulator()->access().memory().copyRAMReg(90, reg);
    CHECK(std::equal(std::begin(marked), std::end(marked), reg));
    c.cleanResetMachine();
    CHECK(r.runUntil([&] {
        c.emulator()->access().memory().copyRAMReg(90, reg);
        return reg[0] == 0;
    }, 3000ms));
}

// 2.3.0 ran a plain Reset's reset routine with the run loop stopped, so only a
// clean reset could be frozen at its first instruction. Since C59-1 (AC-6) both
// return at once and the ROM's start-up runs in the run loop, so an armed
// FREEZE ON START fires on either (ledger row 59-34; LIB-10 Q-1, user
// 2026-11-16: it replaces AC-30's Reset clause for Calc-U-59). Since M-53 the
// run control's pause-on-start cpu (TILevelRules).
TEST(lib10_ac30_part_cpu_freeze_on_start_sees_reset_and_clean_reset) {
    PartRig r;
    auto& c = r.part();
    CHECK(waitForScanLoop(r));
    for (bool clean : {false, true}) {
        rc(r).pauseOnStart(kCpu);
        r.run(50ms);  // the armed start sees the scan loop
        if (clean) c.cleanResetMachine(); else c.resetMachine();
        CHECK(r.runUntil([&] { return c.isFrozen(); }, 3000ms));
        CHECK(c.runState().owner == kCpu);
        CHECK(c.runState().reason == calcu::debugger::RunState::Reason::PauseOnStart);
        rc(r).resume();
        CHECK(waitForScanLoop(r));
    }
}

TEST(part_bad_module_changes_nothing) {
    PartRig r;
    auto& c = r.part();
    CHECK(!c.selectModule("ZZ"));
    CHECK_EQ(c.moduleID(), std::string("ML"));
    CHECK(r.errors().empty());   // not a ROM error: cuecards.txt lists no module ZZ (LIB-12 AC-10)
}

// LIB-10 AC-35: a resume before the pause landed: no pause, no frozen view.
TEST(lib10_ac35_part_resume_before_the_frozen_view_drops_it) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kCpu);
    rc(r).resume();  // before the pause lands
    r.run(200ms);  // the freeze job ran (it is queued before the run mark)
    const std::size_t mark = r.fe.log.size();
    r.run(50ms);
    CHECK(!c.isFrozen());
    CHECK(c.debug().cpuHistory().empty());
    const auto sets = r.stateSets(mark);
    CHECK(std::find(sets.begin(), sets.end(), topics::kDebugCpu) == sets.end());
}

TEST(part_events_of_a_replaced_machine_are_ignored) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kProgram);
    r.run(20ms);
    CHECK(c.startMachine(Model::TI58));
    r.run(20ms);
    CHECK(!c.isFrozen());
    CHECK(c.debug().cpuHistory().empty());
}

// The session always has a machine (makeAppSession starts one), so 2.3.0's
// "Machine not initialized yet." before the first machine has no case here.
TEST(part_asm_overlay) {
    PartRig r;
    auto& c = r.part();
    c.runAsm();
    CHECK_EQ(c.asmStatus(), std::string("No ASM overlay loaded."));
    c.loadAsm(std::nullopt, "a.asm");
    CHECK_EQ(c.asmStatus(), std::string("Could not read ASM file."));
    c.loadAsm(std::string("HEX:\n0x0A01 0x0A01\n"), "a.asm");
    CHECK_EQ(c.asmStatus(), std::string("Loaded 2 word(s) at 0x1800."));
    CHECK(waitForScanLoop(r));
    rc(r).pauseOnStart(kCpu);
    c.runAsm();
    CHECK(c.asmActive());
    CHECK(c.isFrozen() && c.runState().owner == kCpu);
    r.run(20ms);
    CHECK(!c.debug().cpuHistory().empty());
    c.clearAsm();
    CHECK(!c.asmActive() && !c.isFrozen());
    CHECK_EQ(c.asmStatus(), std::string("ASM overlay cleared."));
}

TEST(part_reload_and_memory_card) {
    PartRig r;
    auto& c = r.part();
    // Part 1 writes R00 to a memory card; part 2, reloaded after it, reads it back.
    c.loadStateFile(bytes("PROGRAM:\n76 11 04 96 91\nREGISTERS:\n00 = 42\n"  // Lbl A 4 Write R/S
                          "KEYSTROKES:\nArmCard: mem:M\n11\n"
                          "PROGRAM:\n76 11 47 04 22 96 43 00 91\n"           // Lbl A CMs 4 INV Write RCL 00 R/S
                          "KEYSTROKES:\nArmCard: mem:M\n11\n"));
    CHECK(r.runUntil([&] { return c.keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !c.keystrokesPlaying(); }));
    CHECK(waitForScanLoop(r));
    CHECK_EQ(testroms::mem(c.emulator()->access().machine()).readDataReg(0), 42.0);
    CHECK_EQ(int(testroms::mem(c.emulator()->access().machine()).readProgramStep(2)), 47);  // part 2 is in
    CHECK(r.errors().empty());
}

// ── C59-6 (design T7, T10): the app session's part of the places ────────────

namespace {
// TRACE as the PC-100C in the machine has it; nullopt without a PC-100C.
std::optional<bool> traceSwitch(TIAppPart& c) {
    auto a = c.emulator()->access();
    auto* controls = testroms::printerControls(a.machine());
    if (!controls) return std::nullopt;
    for (const auto& i : controls->list())
        if (i.id == PC100C::kTrace) return i.on;
    return std::nullopt;
}
} // namespace

TEST(c59_6_ac6_trace_kept_by_app_session) {
    PartRig r;
    auto& c = r.part();
    CHECK(!c.printerTrace());                       // off when the product starts
    CHECK(traceSwitch(c) == std::optional<bool>(false));
    c.setPrinterControl("TRACE", true);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    c.resetMachine();
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    c.cleanResetMachine();
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(c.startMachine(Model::TI58C));           // a model switch: a new machine, the same switch
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    c.setPrinterConnected(false);                  // detached: the position stays with the PC-100C
    CHECK(c.printerTrace() && !traceSwitch(c));
    c.setPrinterConnected(true);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    // Not saved: nothing holds it; a new app session starts off.
    PartRig again;
    CHECK(!again.part().printerTrace());
}

TEST(c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it) {
    PartRig r;
    auto& c = r.part();
    c.setPrinterControl("TRACE", true);
    CHECK(c.printerTrace());
    c.loadStateFile(bytes("REGISTERS:\n01 = 1\n"));
    CHECK(!c.printerTrace() && traceSwitch(c) == std::optional<bool>(false));
    c.loadStateFile(bytes("KEYSTROKES:\nPrinterTrace: on\n"));
    CHECK(r.runUntil([&] { return c.printerTrace(); }, 5000ms));
    CHECK(traceSwitch(c) == std::optional<bool>(true));
}

// LIB-8 AC-29 (C59-6 AC-1): the PC-100C's paper stays the PaperPart's while
// it is detached, and it is back, printed on below, when it is attached
// again on another model (was c59_6_ac1_paper_kept_across_detach_and_attach).
TEST(lib8_ac29_paper_back_on_attach_on_any_model) {
    PartRig r;
    auto& c = r.part();
    CHECK(c.printerConnected());
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\n"));   // 1 2nd Prt
    CHECK(r.runUntil([&] { return !r.printedText().empty() && !c.keystrokesPlaying(); }, 5000ms));
    const auto lines = r.printedText();
    CHECK(lines.size() == 1 && lines[0] == "          1.        ");
    r.run(100ms);
    const calcu::paper::Reach reach = r.pc100cPaper()->reach();
    CHECK_EQ(reach.to, pc100cpaper::kLine);
    c.setPrinterConnected(false);
    CHECK(!c.printerConnected());
    CHECK(testroms::paper(c.emulator()->access().machine()) == nullptr);
    r.run(20ms);
    CHECK(r.printedText() == lines);
    CHECK(c.startMachine(Model::TI58C));
    c.setPrinterConnected(true);
    CHECK(testroms::paper(c.emulator()->access().machine()) != nullptr);
    CHECK(r.printedText() == lines);              // the paper is back with the PC-100C
    CHECK(waitForScanLoop(r));
    c.loadStateFile(bytes("KEYSTROKES:\n83 21 94\n"));   // 2 2nd Prt; no PRINTER: line, the paper stays
    CHECK(r.runUntil([&] { return r.printedText().size() == 2 && !c.keystrokesPlaying(); }, 5000ms));
    r.run(100ms);
    const auto& text = r.pc100cPaper()->text();
    CHECK(text.size() == 2 && text[1].text == "          2.        ");
    CHECK_EQ(text[1].at.y, pc100cpaper::kLine);   // printed on below the first line
}

TEST(c59_6_ac11_change_module_power_cycles) {
    PartRig r;
    auto& c = r.part();
    CHECK(waitForScanLoop(r));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t reg[16];
    c.emulator()->access().memory().writeRAMReg(90, marked);
    c.changeModule("LE");
    CHECK_EQ(c.moduleID(), std::string("LE"));
    CHECK(testroms::moduleIn(c.emulator()->access().machine()) == "LE");
    CHECK(c.emulator()->moduleImage() == *cu59::moduleImage(*testroms::romSet(), "LE.rom.yaml"));
    // The power cycle: the TI-59 loses its program and registers (row 59-11).
    CHECK(waitForScanLoop(r));
    c.emulator()->access().memory().copyRAMReg(90, reg);
    CHECK_EQ(int(reg[0]), 0);
    // The printer too.
    c.emulator()->access().memory().writeRAMReg(90, marked);
    c.setPrinterConnected(false);
    CHECK(waitForScanLoop(r));
    c.emulator()->access().memory().copyRAMReg(90, reg);
    CHECK_EQ(int(reg[0]), 0);
    // An empty slot.
    c.changeModule("");
    CHECK(c.moduleID().empty() && testroms::moduleIn(c.emulator()->access().machine()).empty());
    CHECK(c.emulator()->moduleImage().empty());
    CHECK(waitForScanLoop(r));
    CHECK(r.errors().empty());
}

// ── Presets, cards, cue cards (from 2.3.0's cooperative SessionController tests) ──

TEST(part_plays_a_preset) {
    PartRig r;
    r.run(1s);
    CHECK(r.part().emulator()->machine().isKeyReady());
    r.load("KEYSTROKES:\n25 83 85 84 95\nZeroElapseTime:\nWait: 50ms\nReportElapseTime:\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(500ms);
    CHECK_EQ(testroms::displayText(r.part().emulator()->machine()), std::string("5"));
    CHECK(r.part().logText().find("Elapsed: 0:00.0") != std::string::npos);
    CHECK(r.errors().empty());
}

TEST(part_reload_keeps_the_program) {
    PartRig r;
    r.load("PROGRAM:\n85 01\nKEYSTROKES:\n25\nREGISTERS:\n05 = 3.25\n");  // CLR, then a reload
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 1);
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(5), 3.25);
}

TEST(part_writes_and_reads_a_card) {
    PartRig r;
    r.load("PROGRAM:\n76 11 01 02 03 91\nKEYSTROKES:\n82 21 91\n");  // Lbl A 1 2 3 R/S; 1 2nd Write
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);  // the ROM waits for a card; playback waits with it
    CHECK(r.part().keystrokesPlaying());
    CHECK(r.part().fitBlankCard("web:card", "card.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    CHECK_EQ(r.p.cards.count("web:card"), size_t{1});
    r.load("PARTITION: 479\n");  // a fresh machine: no program
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 0);
    const std::string& card = r.p.cards["web:card"];
    CHECK(r.part().fitCard(std::vector<uint8_t>(card.begin(), card.end()), "web:card", "card.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 11);
    CHECK(r.errors().empty());
}

TEST(part_shows_the_library_program_card) {
    PartRig r;
    r.run(50ms);
    r.load("KEYSTROKES:\n21 31 92 82\n");  // 2nd Pgm 01
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(1s);
    CHECK(r.part().cueCard().has_value());
    if (r.part().cueCard()) CHECK_EQ(r.part().cueCard()->id, std::string("ML-01"));
}




TEST(part_reload_after_the_first_run) {
    PartRig r;
    r.load("PROGRAM:\n76 11 01 02 03 42 08 91\n"  // Lbl A 123 STO 08 R/S
           "KEYSTROKES:\n11\n"
           "PROGRAM:\n76 11 43 08 85 01 95 91\n"  // Lbl A RCL 08 + 1 = R/S
           "REGISTERS:\n07 = 9\nCUECARD:\nTitle: Part 2\n"
           "KEYSTROKES:\n11\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(300ms);
    calcu::machine::Machine& m = r.part().emulator()->machine();
    CHECK_EQ(testroms::displayText(m), std::string("124"));  // part 2 used part 1's R08
    CHECK_EQ(testroms::mem(m).readDataReg(7), 9.0);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Part 2");
    CHECK(r.errors().empty());
}

TEST(part_program_writes_and_reads_a_memory_card) {
    PartRig r;
    r.load("PROGRAM:\n76 11 04 96 25 47 04 22 96 43 00 91\n"  // Lbl A 4 Write CLR CMs 4 INV Write RCL 00 R/S
           "REGISTERS:\n00 = 42\n"
           "KEYSTROKES:\nArmCard: mem:Scratch\nArmCard: mem:Scratch\n11\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }, 60s));
    r.run(300ms);
    CHECK_EQ(testroms::displayText(r.part().emulator()->machine()), std::string("42"));
    CHECK(r.p.cards.empty());  // a memory card never reaches the card folder
    CHECK(r.errors().empty());
}

TEST(part_program_reads_a_defined_card) {
    PartRig r;
    r.load("CARD: mem:Vars\nBank: 4\nREGISTERS:\n05 = 2.5\nCUECARD:\nTitle: Vars\nEND-CARD\n"
           "PROGRAM:\n76 11 04 22 96 43 05 91\n"  // Lbl A 4 INV Write RCL 05 R/S
           "KEYSTROKES:\nArmCard: mem:Vars\n11\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }, 60s));
    r.run(300ms);
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(5), 2.5);  // the full load cleared it; the card set it
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Vars");
    CHECK(r.errors().empty());
}

TEST(part_card_file_written_then_inserted) {
    PartRig r;
    r.load("PROGRAM:\n76 11 04 96 91\nREGISTERS:\n03 = 7\n"  // Lbl A 4 Write R/S
           "KEYSTROKES:\nArmCard: Results\n11\n");
    CHECK(r.runUntil([&] { return r.p.cards.count("cards/Results.U59") == 1; }, 60s));
    CHECK(r.part().logText().find("no card file Results.U59") != std::string::npos);
    r.load("REGISTERS:\n03 = 0\nKEYSTROKES:\nInsertCard: Results\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }, 60s));
    CHECK(cardIdle(r));  // playback waited for the card
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(3), 7.0);
    CHECK(r.errors().empty());
}

// Meusch's recipe (TI PPC Notes V5N9-10): card, RST R/S, card. The entry clears the
// RAM, so the table can only print if the second reading brought the program back.
TEST(part_fast_mode_entry_then_card_prints_the_table) {
    PartRig r;
    r.load(testroms::readFile("examples/historic/characters-fastmode.ti59").value());
    CHECK(r.runUntil([&] { return r.printedText().size() >= 8; }, 120s));
    CHECK(r.errors().empty());
    CHECK(testroms::mem(r.part().emulator()->machine()).readProgramStep(5) == 36);  // from the card, not the load
}

TEST(part_unused_card_is_logged_on_the_next_load) {
    PartRig r;
    r.load("KEYSTROKES:\nArmCard: mem:Never\n");
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(100ms);
    CHECK(r.part().logText().find("mem:Never") == std::string::npos);  // still armed after the script
    r.load("REGISTERS:\n01 = 1\n");
    r.run(1100ms);  // the LOG text refreshes at 1 Hz
    CHECK(r.part().logText().find("card mem:Never was never read or written") != std::string::npos);
}

TEST(part_cards_need_a_card_reader) {
    PartRig r;
    CHECK(r.part().startMachine(Model::TI58));
    r.load("KEYSTROKES:\nArmCard: Results\n");
    CHECK(!r.errors().empty() && r.errors().back().find("need a card reader") != std::string::npos);
}

TEST(part_insert_card_passes_an_unused_armed_card) {
    PartRig r;
    r.load("CARD: mem:Data\nBank: 4\nREGISTERS:\n03 = 7\nEND-CARD\n"
           "KEYSTROKES:\nArmCard: mem:NeverAsked\nInsertCard: mem:Data\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }, 30s));
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(3), 7.0);  // read at idle, as pushed in by hand
}

namespace {
/// A card holding the program Lbl A 1 2 3 R/S, as a blank card written by the machine.
std::string writtenCard(PartRig& r) {
    r.load("PROGRAM:\n76 11 01 02 03 91\nKEYSTROKES:\n82 21 91\n");  // 1 2nd Write
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    CHECK(r.part().fitBlankCard("first", "first.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    return r.p.cards["first"];
}
}  // namespace

TEST(drop_action_goes_by_name_and_model) {
    using A = DropAction;
    PartRig r;
    CHECK(r.part().dropAction("a.ti59") == A::LoadPreset);
    CHECK(r.part().dropAction("a.TI58") == A::LoadPreset);
    CHECK(r.part().dropAction("a.Ti58C") == A::LoadPreset);
    CHECK(r.part().dropAction("a.U59") == A::FitCard);
    CHECK(r.part().dropAction("a.bin") == A::FitCard);
    CHECK(r.part().dropAction(".a.u59") == A::NotAccepted);
    CHECK(r.part().dropAction(".a.ti59") == A::NotAccepted);
    CHECK(r.part().dropAction("a.txt") == A::NotAccepted);
    CHECK(r.part().dropAction("ti59") == A::NotAccepted);
    for (Model m : {Model::TI58, Model::TI58C}) {
        CHECK(r.part().startMachine(m));
        CHECK(r.part().dropAction("a.U59") == A::Ignore);
        CHECK(r.part().dropAction("a.ti59") == A::LoadPreset);
    }
}

TEST(dropped_card_is_fitted_and_read_at_idle) {
    PartRig r;
    const std::string card = writtenCard(r);
    r.load("PARTITION: 479\n");  // a fresh machine: no program
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 0);
    CHECK(r.part().openDroppedFile(bytes(card), "dropped", "dropped.U59"));
    CHECK(r.part().cardFitted());
    CHECK(cardIdle(r));  // not fed until a corner is tapped
    r.run(500ms);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 0);
    CHECK(r.part().feedSide(0));
    CHECK(r.part().cardState() == CardState::Swiping);
    CHECK(r.part().litCardCorner() == 0);
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 11);
    CHECK_EQ(r.p.cards.count("dropped"), size_t{0});  // read, not written
    CHECK(r.errors().empty());
}

TEST(dropped_card_is_overwritten_after_write) {
    PartRig r;
    const std::string card = writtenCard(r);
    r.load("PROGRAM:\n76 12 04 05 06 91\nKEYSTROKES:\n82 21 91\n");  // Lbl B …; 1 2nd Write
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    r.p.cards["dropped"] = card;
    CHECK(r.part().openDroppedFile(bytes(card), "dropped", "dropped.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    CHECK_EQ(r.p.cards.count("dropped"), size_t{1});
    CHECK(r.p.cards["dropped"] != card);

    r.load("PARTITION: 479\n");
    r.run(1s);
    const std::string overwritten = r.p.cards["dropped"];
    CHECK(r.part().openDroppedFile(bytes(overwritten), "dropped", "dropped.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 12);
}

TEST(dropped_card_is_ignored_without_a_reader) {
    PartRig r;
    const std::string card = writtenCard(r);
    for (Model m : {Model::TI58, Model::TI58C}) {
        CHECK(r.part().startMachine(m));
        CHECK(!r.part().openDroppedFile(bytes(card), "dropped", "dropped.U59"));
        r.run(500ms);
        CHECK(cardIdle(r));
    }
    CHECK(r.errors().empty());
}

TEST(dropped_card_replaces_the_fitted_one_but_not_during_a_pass) {
    PartRig r;
    const std::string card = writtenCard(r);
    CHECK(r.part().openDroppedFile(bytes(card), "one", "one.U59"));
    CHECK(r.part().openDroppedFile(bytes(card), "two", "two.U59"));
    CHECK_EQ(r.part().cardFileName(), std::string("two.U59"));
    CHECK(r.part().feedSide(0));
    CHECK(!r.part().openDroppedFile(bytes(card), "three", "three.U59"));
    CHECK_EQ(r.part().cardFileName(), std::string("two.U59"));
}

namespace {
const char* const kTwoWrites = "PROGRAM:\n76 11 01 02 03 91\nKEYSTROKES:\n82 21 91\n83 21 91\n";  // 1 2nd Write; 2 2nd Write
/// Run a preset's two key-driven writes, feeding `first` then `second` as the ROM waits.
void feedTwoWrites(PartRig& r, int first, int second) {
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    CHECK(r.part().feedSide(first));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    r.run(2s);
    CHECK(r.part().feedSide(second));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
}
}  // namespace

TEST(fresh_card_writes_both_sides_into_one_file) {
    PartRig r;
    // Loading a preset removes a fitted card, so fit it once the preset runs.
    r.load(kTwoWrites);
    CHECK(r.part().fitBlankCard("cards/Fresh.U59", "Fresh.U59"));
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Fresh");
    CHECK(r.part().cueCard() && !r.part().cueCard()->bankLeft && !r.part().cueCard()->bankRight);
    feedTwoWrites(r, 0, 1);
    const std::string& text = r.p.cards["cards/Fresh.U59"];
    CHECK(startsWith(text, "Calc-U-59 Card 2.0\n"));
    CHECK(text.find("Banks:") == std::string::npos);
    // The corners show what was written.
    CHECK(r.part().cueCard() && r.part().cueCard()->bankLeft == 1 && r.part().cueCard()->bankRight == 2);
    auto doc = decodeCardFile(bytes(text), "Fresh.U59");
    CHECK(doc.card && doc.card->sides[0] && doc.card->sides[1]);

    // Side 2 reads bank 2; the card stays fitted after the pass.
    const std::string saved = text;
    r.load("PARTITION: 479\n");
    r.run(1s);
    CHECK(!r.part().cardFitted());
    CHECK(r.part().fitCard(bytes(saved), "cards/Fresh.U59", "Fresh.U59"));
    CHECK(r.part().tapCard(0.9, kCardRect.y + 0.1 * kCardRect.h));
    CHECK(r.part().litCardCorner() == 1);
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(!r.part().litCardCorner());
    CHECK(r.part().cardFitted());
    CHECK(r.errors().empty());
}

TEST(a_side_can_hold_any_bank_and_the_corner_follows) {
    PartRig r;
    r.load("PROGRAM:\n76 11 01 02 03 91\nKEYSTROKES:\n82 21 91\n82 21 91\n");  // bank 1 twice
    CHECK(r.part().fitBlankCard("c", "c.U59"));
    feedTwoWrites(r, 1, 0);
    CHECK(r.part().cueCard() && r.part().cueCard()->bankLeft == 1 && r.part().cueCard()->bankRight == 1);
    CHECK(r.errors().empty());
}

TEST(version_1_card_stays_version_1_on_a_write) {
    PartRig r;
    const std::string card = writtenCard(r);
    auto oldDoc = decodeCardFile(bytes(card), "x");
    CHECK(oldDoc.card && oldDoc.card->sides[0]);
    CueCardContent cue;
    cue.title = "OLD CARD";
    cue.bankLeft = 1;
    cue.bankRight = 2;
    const std::string v1 = encodeCardText(*oldDoc.card->sides[0], &cue);
    r.p.cards["v1"] = v1;
    r.load("PROGRAM:\n76 12 04 05 06 91\nKEYSTROKES:\n82 21 91\n");
    CHECK(r.part().fitCard(bytes(v1), "v1", "v1.U59"));
    CHECK(r.part().cueCard() && r.part().cueCard()->bankLeft == 1 && r.part().cueCard()->bankRight == 2);
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    CHECK(r.part().feedSide(1));  // either corner: the one side
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    const std::string& text = r.p.cards["v1"];
    CHECK(text != v1);
    CHECK(startsWith(text, "Calc-U-59 Card 1.0\n"));
    CHECK(text.find("Banks: 1, 2") != std::string::npos);
    CHECK(r.part().cueCard() && r.part().cueCard()->bankRight == 2);
    CHECK(r.errors().empty());
}

TEST(write_keeps_the_side_changed_elsewhere_and_refuses_a_broken_file) {
    PartRig r;
    const std::string card = writtenCard(r);
    CardDocument other;
    other.cueCard = freshCardCue("Shared.U59");
    other.sides[1] = decodeCardFile(bytes(card), "x").card->sides[0];
    r.p.cards["s"] = encodeCardDocument(other);

    CardDocument mine;
    mine.cueCard = freshCardCue("Shared.U59");
    r.load("PROGRAM:\n76 12 04 05 06 91\nKEYSTROKES:\n82 21 91\n");
    CHECK(r.part().fitCard(bytes(encodeCardDocument(mine)), "s", "Shared.U59"));  // fitted before side 2 appeared
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    auto merged = decodeCardFile(bytes(r.p.cards["s"]), "Shared.U59");
    CHECK(merged.card && merged.card->sides[0] && merged.card->sides[1]);

    r.load("PROGRAM:\n76 12 04 05 06 91\nKEYSTROKES:\n82 21 91\n");
    CHECK(r.part().fitCard(bytes(r.p.cards["s"]), "s", "Shared.U59"));
    r.p.cards["s"] = "Calc-U-59 Card 2.0\nbroken";
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    r.run(2s);
    CHECK(r.part().feedSide(0));
    CHECK(r.runUntil([&] { return cardIdle(r); }));
    CHECK_EQ(r.p.cards["s"], std::string("Calc-U-59 Card 2.0\nbroken"));  // left alone, reported
    CHECK(!r.errors().empty());
}

TEST(fresh_card_replaces_the_file_of_that_name_at_its_first_write_only) {
    PartRig r;
    r.p.cards["same"] = "Calc-U-59 Card 2.0\nwhatever was here";
    r.load(kTwoWrites);
    CHECK(r.part().fitBlankCard("same", "same.U59"));
    CHECK_EQ(r.p.cards["same"], std::string("Calc-U-59 Card 2.0\nwhatever was here"));  // still there
    feedTwoWrites(r, 0, 1);
    CHECK(decodeCardFile(bytes(r.p.cards["same"]), "same.U59").card.has_value());
    CHECK(r.errors().empty());
}

TEST(blank_side_fed_at_idle_writes_nothing) {
    PartRig r;
    r.load("PARTITION: 479\n");
    r.run(1s);
    CHECK(r.part().fitBlankCard("blank", "blank.U59"));
    CHECK(r.part().feedSide(0));
    r.run(5s);
    r.part().ejectCard();
    r.run(500ms);
    CHECK_EQ(r.p.cards.count("blank"), size_t{0});
    CHECK(r.part().cardFitted());
    CHECK(r.errors().empty());
}

TEST(blank_side_fed_at_partition_0_writes_nothing) {
    PartRig r;
    r.load("KEYSTROKES:\n92 21 64 82 62\n");  // 0 2nd Op 17
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(1s);
    CHECK(r.part().fitBlankCard("blank", "blank.U59"));
    CHECK(r.part().feedSide(0));
    r.run(5s);
    r.part().ejectCard();
    r.run(500ms);
    CHECK_EQ(r.p.cards.count("blank"), size_t{0});
    CHECK(r.part().cardFitted());
}

TEST(module_cue_card_covers_the_fitted_card_and_it_comes_back) {
    PartRig r;
    r.load("KEYSTROKES:\nWait: 1s\n21 31 92 82\nWait: 3s\n21 31 92 92\n");  // Pgm 01, later Pgm 00
    CHECK(r.part().fitBlankCard("c", "Mine.U59"));
    auto mine = [&] { return r.part().cueCard() && r.part().cueCard()->title == "Mine"; };
    CHECK(mine());
    CHECK(r.runUntil([&] { return !mine(); }));
    CHECK(!r.part().tapCard(0.1, kCardRect.y + 0.1 * kCardRect.h));  // not on show
    CHECK(cardIdle(r));
    CHECK(r.part().cardFitted());
    CHECK(r.runUntil(mine));
    CHECK(r.errors().empty());
}

TEST(card_corners_hit_only_the_top_corners) {
    const double top = kCardRect.y + 0.1 * kCardRect.h;
    CHECK(cardCornerAt(0.01, top) == 0);
    CHECK(cardCornerAt(0.99, top) == 1);
    CHECK(!cardCornerAt(0.5, top));
    CHECK(!cardCornerAt(0.1, kCardRect.y + 0.3 * kCardRect.h));  // the title row
    CHECK(!cardCornerAt(0.1, kCardRect.y - 0.001));
    CHECK(!cardCornerAt(0.21, top));
}

TEST(preset_creates_a_full_two_sided_card) {
    for (const std::string& arms : {std::string("ArmCard: Results:1\nArmCard: Results:2\n"),
                                   std::string("ArmCard: Results:2\nArmCard: Results:1\n")}) {
        PartRig r;
        r.load("PROGRAM:\n76 11 01 96 02 96 91\n"  // Lbl A 1 Write 2 Write R/S
               "CUECARD:\nTitle: My Program\nA: run\nBanks: 3, 4\n"
               "KEYSTROKES:\n" + arms + "11\n");
        CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
        CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying() && r.p.cards.count("cards/Results.U59"); }, 60s));
        r.run(1s);
        const std::string& text = r.p.cards["cards/Results.U59"];
        CHECK(startsWith(text, "Calc-U-59 Card 2.0\n"));
        CHECK(text.find("Banks:") == std::string::npos);
        auto doc = decodeCardFile(bytes(text), "Results.U59");
        CHECK(doc.card && doc.card->sides[0] && doc.card->sides[1]);
        if (doc.card && doc.card->cueCard) {
            CHECK_EQ(doc.card->cueCard->title, std::string("My Program"));
            CHECK_EQ(doc.card->cueCard->labels[5], std::string("run"));
        }
        CHECK(r.part().cardFitted());
        CHECK(r.errors().empty());
    }
}

TEST(preset_cannot_feed_an_unreadable_file) {
    PartRig r;
    r.p.unreadableCards.insert("cards/Gone.U59");
    r.load("KEYSTROKES:\nInsertCard: Gone\n25\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    CHECK(!r.errors().empty());
    CHECK(!r.part().cardFitted());
    CHECK_EQ(r.p.cards.count("cards/Gone.U59"), size_t{0});
}

TEST(preset_side_on_a_version_1_file_feeds_its_one_side) {
    PartRig r;
    const std::string card = writtenCard(r);
    r.p.cards["cards/Old.U59"] = encodeCardText(*decodeCardFile(bytes(card), "x").card->sides[0], nullptr);
    r.load("KEYSTROKES:\nInsertCard: Old:2\n");
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.runUntil([&] { return !r.part().keystrokesPlaying(); }));
    r.run(1s);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 11);
    CHECK(r.errors().empty());
}

TEST(dropped_preset_loads_as_the_button_does) {
    PartRig r;
    CHECK(r.part().openDroppedFile(bytes("MODEL: TI-58\nPROGRAM:\n76 11\nKEYSTROKES:\n25\n"), "p", "p.ti58"));
    CHECK(r.part().model() == Model::TI58);
    CHECK_EQ(int(testroms::mem(r.part().emulator()->machine()).readProgramStep(1)), 11);
    CHECK(r.runUntil([&] { return r.part().keystrokesPlaying(); }));
    CHECK(r.errors().empty());
    CHECK(!r.part().openDroppedFile(std::nullopt, "p", "p.ti58"));
    CHECK(!r.errors().empty());  // reported, as a failed Open is
}


// ── LIB-8's Calc-U-59 column in the app session (design §11.6, E8-6, E8-7) ──

// AC-29: detached, the panel's owner is none (P-E1: `c59.printer` says
// not connected) and the papers topic still lists the PC-100C's paper, empty
// of nothing it had.
TEST(lib8_ac29_detached_panel_owner_empty) {
    PartRig r;
    auto& c = r.part();
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\n"));   // 1 2nd Prt
    CHECK(r.runUntil([&] { return !r.printedText().empty() && !c.keystrokesPlaying(); }, 5000ms));
    c.setPrinterConnected(false);
    r.run(50ms);
    const auto* printer = r.state<TIPrinterState>(topics::kPrinter);
    CHECK(printer && !printer->connected);
    const auto* papers = r.state<calcu::paper::PaperList>(calcu::paper::kPapersTopic);
    CHECK(papers && papers->papers.size() == 1 && papers->papers[0].owner == "PC-100C");
    CHECK_EQ(r.printedText().size(), std::size_t{1});   // kept while detached
    // Without a name the paper commands mean the printer plugged in: none (EC-8).
    const auto reply = r.send(calcu::paper::kPaperCutCommand, calcu::sessions::Value::object({{"copy", false}}));
    CHECK(!reply.done && reply.reason == calcu::paper::kNoPrinterPlugged);
}

// AC-8 (P-E4): a preset that names the printer starts with empty paper.
TEST(lib8_ac8_preset_naming_printer_starts_empty) {
    PartRig r;
    auto& c = r.part();
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\n"));
    CHECK(r.runUntil([&] { return !r.printedText().empty() && !c.keystrokesPlaying(); }, 5000ms));
    c.loadStateFile(bytes("PRINTER: on\nREGISTERS:\n01 = 1\n"));
    r.run(50ms);
    CHECK(r.printedText().empty());
    CHECK(r.pc100cPaper()->empty());
}

// AC-8 (D-29): a 2.3.0 preset without a PRINTER: line keeps the paper (ledger 59-48).
TEST(lib8_ac8_preset_without_printer_line_keeps_paper) {
    PartRig r;
    auto& c = r.part();
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\n"));
    CHECK(r.runUntil([&] { return !r.printedText().empty() && !c.keystrokesPlaying(); }, 5000ms));
    c.loadStateFile(bytes("REGISTERS:\n01 = 1\n"));
    r.run(50ms);
    CHECK_EQ(r.printedText().size(), std::size_t{1});
}

// AC-22 (P-E5): KEYSTROKES CutPaper: is the step cut-paper: the paper of
// the printer plugged in, without a copy.
TEST(lib8_ac22_keystrokes_cut_paper) {
    PartRig r;
    auto& c = r.part();
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\nWait: 1s\nCutPaper:\n83 21 94\nWait: 1s\n"));
    CHECK(r.runUntil([&] { return c.keystrokesPlaying(); }, 2000ms));
    CHECK(r.runUntil([&] { return !c.keystrokesPlaying(); }, 8000ms));
    r.run(300ms);
    CHECK(r.errors().empty());
    const auto text = r.printedText();
    CHECK(text.size() == 1 && text[0] == "          2.        ");
    CHECK_EQ(r.pc100cPaper()->text()[0].at.y, calcu::paper::Coord{0});   // y counts from the cut
}
