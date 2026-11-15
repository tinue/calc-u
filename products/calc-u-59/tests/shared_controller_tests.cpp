// Shared/SessionController: the flows both apps run through it — model
// switches, presets, the TI-58C memory file, freezes, the ASM overlay and
// the fixes that came with moving them out of the frontends.

#include <algorithm>
#include "TestHarness.hpp"
#include "FakePlatform.hpp"
#include "TestRoms.hpp"
#include "../app/SessionController.hpp"
#include <chrono>
#include <deque>
#include <mutex>

using namespace cu59;
using namespace std::chrono_literals;
using testroms::FakePlatform;

namespace {

// The controllers run cooperatively (M-33: the threaded runner is gone):
// the test is their host and ticks them at 60 Hz on a clock of its own.
std::chrono::steady_clock::time_point g_now = std::chrono::steady_clock::time_point{} + std::chrono::hours(1);
constexpr auto kFrame = std::chrono::microseconds(16'667);

void frame(SessionController& c, FakePlatform& p) {
    g_now += kFrame;
    c.tick(g_now);
    p.pump();
}

void runFor(SessionController& c, FakePlatform& p, std::chrono::milliseconds d) {
    const auto end = g_now + d;
    while (g_now < end) frame(c, p);
}

template <class F> bool pumpUntil(SessionController& c, FakePlatform& p, F cond, std::chrono::milliseconds timeout = 3000ms) {
    const auto end = g_now + timeout;
    p.pump();
    while (g_now < end) {
        if (cond()) return true;
        frame(c, p);
    }
    return cond();
}

std::string catalog() { return testroms::readFile("roms/solid-state/cuecards.txt").value_or(""); }

std::vector<uint8_t> bytes(const std::string& s) { return std::vector<uint8_t>(s.begin(), s.end()); }

bool waitForScanLoop(SessionController& c, FakePlatform& p) {
    return pumpUntil(c, p, [&] { return c.session()->access().machine().isKeyReady(); });
}

} // namespace

TEST(controller_loads_a_preset_for_another_model) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.loadStateFile(bytes("MODEL: TI-58\nREGISTERS:\n05 = 3.25\n"));
    CHECK(c.model() == Model::TI58);
    CHECK(c.session()->isRunning());
    CHECK_EQ(testroms::mem(c.session()->access().machine()).readDataReg(5), 3.25);
    CHECK(p.errors.empty());
}

TEST(controller_reports_parse_errors_and_partition_errors) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI58));
    c.loadStateFile(bytes("PARTITION: 959\n"));  // beyond the TI-58's 479
    CHECK_EQ(p.errors.size(), size_t{1});
    c.loadStateFile(std::nullopt);
    CHECK_EQ(p.errors.back(), std::string("State file load error: Cannot read file."));
    c.loadStateFile(bytes("\xff\xfe"));
    CHECK(p.errors.back().find("not valid UTF-8") != std::string::npos);
}

TEST(controller_load_while_frozen_ends_running_without_tracing) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.freeze(FreezeOwner::Calculator);
    CHECK(c.isFrozen());
    CHECK(pumpUntil(c, p, [&] { return !c.debug().cpuHistory().empty(); }));
    c.loadStateFile(bytes("REGISTERS:\n01 = 7\n"));
    CHECK(!c.isFrozen());
    CHECK(!c.session()->debugState().frozen);
    CHECK(c.session()->isRunning());
    CHECK_EQ(testroms::dbg(c.session()->access().machine()).traceFlags(), static_cast<uint32_t>(TRACE_NONE));
}

TEST(controller_keystroke_controls_drive_the_gui) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    // Lbl A 1 + 1 = R/S, armed FREEZE ON START: A freezes before the program's first step.
    c.loadStateFile(bytes("PROGRAM:\n76 11 01 85 01 95 91\n"
                          "KEYSTROKES:\nPrinterTrace: on\nCutPaper:\nDebugPanel: CPU\n"
                          "FreezeOnStart: Calculator\n11\n"));
    auto tick = [&](auto cond) {
        return pumpUntil(c, p, [&] { return cond(); }, 10000ms);
    };
    CHECK(tick([&] { return c.isFrozen(); }));
    CHECK(c.printerTrace());
    CHECK(p.panels == (std::vector<DebugPanel>{DebugPanel::Cpu}));
    CHECK(tick([&] { return !c.keystrokesPlaying(); }));
    c.loadStateFile(bytes("SKIP-RESET: on\n"));  // retired: an error, then a normal load
    CHECK(!p.errors.empty() && p.errors.back().find("SKIP-RESET: no longer exists") != std::string::npos);
    CHECK(!c.isFrozen());
}

TEST(controller_constant_memory_keeps_the_loaded_card) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI58C));
    c.loadStateFile(bytes("CUECARD:\nTemplate: MagnetCard\nTitle: KEEP ME\nREGISTERS:\n03 = 1\n"));
    CHECK(p.mem && p.mem->find("KEEP ME") != std::string::npos);
    // Unchanged RAM: the debounced persist does not write again.
    int writes = p.memWrites;
    auto t = g_now;
    c.session()->stop();
    c.session()->sync([] {});
    c.tick(t);
    c.tick(t + 600ms);
    CHECK_EQ(p.memWrites, writes);
}

TEST(controller_model_switch_keeps_printer_and_log_resets_freeze) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.setPrinterConnected(false);
    c.dumpVars();
    c.armPcChange();
    CHECK(c.debugState().pcChangeArmed);
    CHECK(c.startMachine(Model::TI58));
    CHECK(!c.printerConnected());
    CHECK(testroms::moduleIn(c.session()->access().machine()) == "ML");
    CHECK_EQ(c.session()->access().machine().plugged().byPlace.size(), size_t{1});   // the module only
    CHECK(!c.debugState().pcChangeArmed && !c.isFrozen());
    CHECK(c.logText().find("──") != std::string::npos);  // the dump survived the switch
    CHECK(c.cardState() == SessionController::CardState::NoCard);
}

TEST(controller_reset_keeps_the_memory_and_clean_reset_clears_it) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(waitForScanLoop(c, p));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t reg[16];
    c.session()->access().memory().writeRAMReg(90, marked);
    c.resetMachine();
    CHECK(waitForScanLoop(c, p));
    c.session()->access().memory().copyRAMReg(90, reg);
    CHECK(std::equal(std::begin(marked), std::end(marked), reg));
    c.cleanResetMachine();
    CHECK(pumpUntil(c, p, [&] {
        c.session()->access().memory().copyRAMReg(90, reg);
        return reg[0] == 0;
    }));
}

// 2.3.0 ran a plain Reset's reset routine with the run loop stopped, so only a
// clean reset could be frozen at its first instruction. Since C59-1 (AC-6) both
// return at once and the ROM's start-up runs in the run loop, so an armed
// FREEZE ON START fires on either (reported with M-19: no ledger row yet).
TEST(controller_cpu_freeze_on_start_sees_reset_and_clean_reset) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(waitForScanLoop(c, p));
    for (bool clean : {false, true}) {
        c.armScanLoopExit();
        runFor(c, p, 50ms);  // the armed freeze sees the scan loop
        if (clean) c.cleanResetMachine(); else c.resetMachine();
        CHECK(pumpUntil(c, p, [&] { return c.isFrozen(); }));
        CHECK(c.debugState().owner == FreezeOwner::Cpu);
        c.resume();
        CHECK(waitForScanLoop(c, p));
    }
}

TEST(controller_bad_module_changes_nothing) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(!c.selectModule("ZZ"));
    CHECK_EQ(c.moduleID(), std::string("ML"));
    CHECK(p.errors.empty());   // not a ROM error: cuecards.txt lists no module ZZ (LIB-12 AC-10)
}

TEST(controller_resume_before_the_frozen_view_drops_it) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.freeze(FreezeOwner::Cpu);
    c.resume();  // before the frozen view arrives
    runFor(c, p, 200ms);  // the freeze job ran (it is queued before the loop)
    p.changes = 0;
    p.pump();
    CHECK(!c.isFrozen());
    CHECK(c.debug().cpuHistory().empty());
    CHECK((p.changes & SessionController::CpuChanged) == 0);
}

TEST(controller_events_of_a_replaced_machine_are_ignored) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.freeze(FreezeOwner::Calculator);
    c.session()->sync([] {});  // the old machine's frozen view is queued
    CHECK(c.startMachine(Model::TI58));
    p.pump();
    CHECK(!c.isFrozen());
    CHECK(c.debug().cpuHistory().empty());
}

TEST(controller_asm_overlay) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    c.runAsm();
    CHECK_EQ(c.asmStatus(), std::string("No ASM overlay loaded."));
    c.loadAsm(std::string("HEX:\n0x0A01\n"), "a.asm");
    CHECK_EQ(c.asmStatus(), std::string("Machine not initialized yet."));
    CHECK(c.startMachine(Model::TI59));
    c.loadAsm(std::nullopt, "a.asm");
    CHECK_EQ(c.asmStatus(), std::string("Could not read ASM file."));
    c.loadAsm(std::string("HEX:\n0x0A01 0x0A01\n"), "a.asm");
    CHECK_EQ(c.asmStatus(), std::string("Loaded 2 word(s) at 0x1800."));
    CHECK(waitForScanLoop(c, p));
    c.armScanLoopExit();
    c.runAsm();
    CHECK(c.asmActive());
    CHECK(c.isFrozen() && c.debugState().owner == FreezeOwner::Cpu);
    CHECK(!c.session()->isRunning());
    CHECK(!c.debug().cpuHistory().empty());
    c.clearAsm();
    CHECK(!c.asmActive() && !c.isFrozen() && c.session()->isRunning());
    CHECK_EQ(c.asmStatus(), std::string("ASM overlay cleared."));
}

TEST(controller_reload_and_memory_card) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    // Part 1 writes R00 to a memory card; part 2, reloaded after it, reads it back.
    c.loadStateFile(bytes("PROGRAM:\n76 11 04 96 91\nREGISTERS:\n00 = 42\n"  // Lbl A 4 Write R/S
                          "KEYSTROKES:\nArmCard: mem:M\n11\n"
                          "PROGRAM:\n76 11 47 04 22 96 43 00 91\n"           // Lbl A CMs 4 INV Write RCL 00 R/S
                          "KEYSTROKES:\nArmCard: mem:M\n11\n"));
    auto tickUntil = [&](auto cond) {
        return pumpUntil(c, p, [&] { return cond(); }, 20000ms);
    };
    CHECK(tickUntil([&] { return c.keystrokesPlaying(); }));
    CHECK(tickUntil([&] { return !c.keystrokesPlaying(); }));
    CHECK(waitForScanLoop(c, p));
    CHECK_EQ(testroms::mem(c.session()->access().machine()).readDataReg(0), 42.0);
    CHECK_EQ(int(testroms::mem(c.session()->access().machine()).readProgramStep(2)), 47);  // part 2 is in
    CHECK(p.errors.empty());
}

// ── C59-6 (design T7, T10): the app session's part of the places ────────────

namespace {
// TRACE as the PC-100C in the machine has it; nullopt without a PC-100C.
std::optional<bool> traceSwitch(SessionController& c) {
    auto a = c.session()->access();
    auto* controls = testroms::printerControls(a.machine());
    if (!controls) return std::nullopt;
    for (const auto& i : controls->list())
        if (i.id == PC100C::kTrace) return i.on;
    return std::nullopt;
}
} // namespace

TEST(c59_6_ac6_trace_kept_by_app_session) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(!c.printerTrace());                       // off when the product starts
    CHECK(traceSwitch(c) == std::optional<bool>(false));
    c.togglePrinterTrace();
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
    // Not saved: the Platform has no setting for it; a new app session starts off.
    SessionController again(p, testroms::romSet(), catalog(), "ML");
    CHECK(again.startMachine(Model::TI59));
    CHECK(!again.printerTrace());
}

TEST(c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    c.togglePrinterTrace();
    CHECK(c.printerTrace());
    c.loadStateFile(bytes("REGISTERS:\n01 = 1\n"));
    CHECK(!c.printerTrace() && traceSwitch(c) == std::optional<bool>(false));
    c.loadStateFile(bytes("KEYSTROKES:\nPrinterTrace: on\n"));
    CHECK(pumpUntil(c, p, [&] { return c.printerTrace(); }, 5000ms));
    CHECK(traceSwitch(c) == std::optional<bool>(true));
}

TEST(c59_6_ac1_paper_kept_across_detach_and_attach) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(c.printerConnected());
    c.loadStateFile(bytes("KEYSTROKES:\n82 21 94\n"));   // 1 2nd Prt
    CHECK(pumpUntil(c, p, [&] { return !c.printerLines().empty() && !c.keystrokesPlaying(); },
                    5000ms));
    const auto lines = c.printerLines();
    CHECK(lines.size() == 1 && lines[0] == "          1.        ");
    c.setPrinterConnected(false);
    CHECK(!c.printerConnected());
    CHECK(testroms::paper(c.session()->access().machine()) == nullptr);
    CHECK(c.printerLines() == lines);
    c.setPrinterConnected(true);
    CHECK(testroms::paper(c.session()->access().machine()) != nullptr);
    CHECK(c.printerLines() == lines);              // the paper is back with the PC-100C
    CHECK(c.startMachine(Model::TI58));
    CHECK(c.printerLines() == lines);              // and through a model switch
}

TEST(c59_6_ac11_change_module_power_cycles) {
    FakePlatform p;
    SessionController c(p, testroms::romSet(), catalog(), "ML");
    CHECK(c.startMachine(Model::TI59));
    CHECK(waitForScanLoop(c, p));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t reg[16];
    c.session()->access().memory().writeRAMReg(90, marked);
    c.changeModule("LE");
    CHECK_EQ(c.moduleID(), std::string("LE"));
    CHECK(testroms::moduleIn(c.session()->access().machine()) == "LE");
    CHECK(c.session()->moduleImage() == *cu59::moduleImage(*testroms::romSet(), "LE.rom.yaml"));
    // The power cycle: the TI-59 loses its program and registers (row 59-11).
    CHECK(waitForScanLoop(c, p));
    c.session()->access().memory().copyRAMReg(90, reg);
    CHECK_EQ(int(reg[0]), 0);
    // The printer too.
    c.session()->access().memory().writeRAMReg(90, marked);
    c.setPrinterConnected(false);
    CHECK(waitForScanLoop(c, p));
    c.session()->access().memory().copyRAMReg(90, reg);
    CHECK_EQ(int(reg[0]), 0);
    // An empty slot.
    c.changeModule("");
    CHECK(c.moduleID().empty() && testroms::moduleIn(c.session()->access().machine()).empty());
    CHECK(c.session()->moduleImage().empty());
    CHECK(waitForScanLoop(c, p));
    CHECK(p.errors.empty());
}
