// The product's flows in cu59::TIAppPart on the library's app session: model
// switches, presets, freezes, the ASM overlay, the places of C59-6, cards
// and cue cards. Ported in M-34 from 2.3.0's SessionController tests; since
// M-54 on the library's commands (LIB-18: model, reset, cold-boot, attach,
// detach; LIB-6: load-preset {content}, run-step) and presets of format 2.
// The card cases moved to card_flows_tests.cpp in M-55 (C59-7 T9, AC-27).

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "opening/OpenRequest.hpp"
#include "TestRoms.hpp"
#include "../app/KeyboardLayout.hpp"
#include "../app/ModuleImage.hpp"
#include "../app/TextUtil.hpp"
#include "../core/PC100C.hpp"
#include "control/ControlState.hpp"
#include "peripherals/PeripheralsPart.hpp"
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

bool waitForScanLoop(PartRig& r) {
    return r.runUntil([&] { return !r.busy() && r.part().emulator()->access().machine().isKeyReady(); }, 3000ms);
}

// The run control of the part's session (LIB-10).
calcu::debugger::RunControl& rc(PartRig& r) { return r.part().debuggerPart().runControl(); }
using calcu::debugger::kCpu;
using calcu::debugger::kProgram;

// A preset of format 2 for `model`, with the Master Library module and,
// unless `printer` is false, the PC-100C; `body` its sections.
std::string preset(const std::string& body, const std::string& model = "TI-59", bool printer = true) {
    return "format-version: 2\nmodel: " + model + "\nslot-1: { name: ML }\n" + (printer ? "printer: PC-100C\n" : "") + body;
}

// A file dropped on the calculator: LIB-13's `open {way: drop}` (M-56);
// true when it was opened.
bool drop(PartRig& r, const std::optional<std::string>& content, const std::string& path, const std::string& name) {
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = name;
    if (!path.empty()) f.path = path;   // none: as its content (LIB-6 EC-5)
    if (content) f.bytes = std::vector<std::uint8_t>(content->begin(), content->end());
    q.files = {f};
    q.way = calcu::opening::Way::Drop;
    return r.operation(calcu::opening::kOpenCommand, q.toValue()).done;
}

// Keys at regular speed through LIB-5's `key` command (a user's keys, as
// 2.3.0's KEYSTROKES pressed them at regular speed; LIB-6's run-step runs at
// full speed); the reply comes when the last key is up.
bool keys(PartRig& r, std::initializer_list<const char*> names) {
    using calcu::sessions::Value;
    Value::Array a;
    for (const char* n : names) a.emplace_back(n);
    const auto reply = r.operation(calcu::typing::kKey, Value::object({{"names", Value(std::move(a))}}), 30000ms);
    return reply.done;
}

double reg(PartRig& r, int n) { return testroms::mem(r.part().emulator()->machine()).readDataReg(n); }
int step(PartRig& r, int s) { return int(testroms::mem(r.part().emulator()->machine()).readProgramStep(s)); }

} // namespace

// LIB-18 AC-24 (design P59-3): a preset that names another model builds it;
// `[startup] last-model` follows (AC-9).
TEST(lib18_ac24_preset_for_another_model_builds_it_last_model_follows) {
    PartRig r;
    auto& c = r.part();
    const auto reply = r.load(preset("data:\n  05: 3.25\n", "TI-58"));
    CHECK(reply.done);
    CHECK(c.model() == Model::TI58);
    CHECK(!c.isFrozen());
    CHECK_EQ(reg(r, 5), 3.25);
    CHECK_EQ(r.settings.text("startup", "last-model"), std::string("TI-58"));
    CHECK(r.errors().empty());
}

// LIB-6 AC-38 (design P3; was part_reports_parse_errors_and_partition_errors):
// any error refuses the file whole, with its first error and its line; the
// machine stays as it was.
TEST(lib6_ac38_first_error_refuses_the_file_machine_untouched) {
    PartRig r(Model::TI58);
    auto& c = r.part();
    const auto ticksBefore = r.ticks();
    auto reply = r.load(preset("partition: 959\ndata:\n  77: 1\n", "TI-58"));   // beyond the TI-58's 479; R77 too
    CHECK(!reply.done);
    CHECK(reply.reason.find("line 5") != std::string::npos);   // the first error only
    CHECK(reply.reason.find("line 7") == std::string::npos);
    CHECK(c.model() == Model::TI58 && r.ticks() >= ticksBefore);
    reply = r.load("\xff\xfe");
    CHECK(!reply.done && reply.reason.find("UTF-8") != std::string::npos);
    reply = r.load("PARTITION: 479\n");   // 2.3.0: outdated, with the hint (row 59-1)
    CHECK(!reply.done && reply.reason.find("Calc-U-59") != std::string::npos);
}

TEST(part_load_while_frozen_ends_running_without_tracing) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kProgram);
    CHECK(r.runUntil([&] { return c.isFrozen() && !r.debug().model().cpuHistory().empty(); }, 3000ms));
    CHECK(r.load(preset("data:\n  01: 7\n")).done);
    CHECK(!c.isFrozen());
    r.run(20ms);
    CHECK_EQ(testroms::hw(c.emulator()->access().machine()).traceFlags(), static_cast<uint32_t>(TRACE_NONE));
}

// LIB-10 AC-35 (G11): a new machine disarms pause-on-start. LIB-18 AC-20: a
// model switch builds the built-in default (Master Library, no printer).
TEST(lib10_ac35_part_model_switch_keeps_log_resets_freeze) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    r.debug().dump("vars");
    rc(r).pauseOnStart(kProgram);
    CHECK(c.runState().armed == kProgram);
    CHECK(r.model("TI-58").done);
    CHECK(c.model() == Model::TI58);
    CHECK(!c.printerConnected());   // nothing of the old machine is carried
    CHECK(testroms::moduleIn(c.emulator()->access().machine()) == "ML");
    CHECK_EQ(c.emulator()->access().machine().plugged().byPlace.size(), size_t{1});   // the module only
    r.run(20ms);
    CHECK(!c.runState().armed && !c.isFrozen());
    bool dumped = false;   // the dump is in LIB-3's log (C59-4 AC-10), which the switch keeps
    for (const auto& m : r.fe.log)
        if (const auto* l = std::get_if<calcu::sessions::StreamMessage>(&m); l && l->topic == calcu::sessions::kLogTopic)
            for (const auto& p : l->appended)
                if (const auto* x = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p)) dumped = dumped || x->text.find("──") != std::string::npos;
    CHECK(dumped);
    CHECK(!c.cardFitted());
}

TEST(part_reset_keeps_the_memory_and_clean_reset_clears_it) {
    PartRig r;
    auto& c = r.part();
    CHECK(waitForScanLoop(r));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t regs[16];
    testroms::mem(c.emulator()->access().machine()).writeRAMReg(90, marked);
    CHECK(r.reset().done);
    CHECK(waitForScanLoop(r));
    testroms::mem(c.emulator()->access().machine()).copyRAMReg(90, regs);
    CHECK(std::equal(std::begin(marked), std::end(marked), regs));
    CHECK(r.coldBoot().done);
    testroms::mem(c.emulator()->access().machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
}

// 2.3.0 ran a plain Reset's reset routine with the run loop stopped, so only a
// clean reset could be frozen at its first instruction. Since C59-1 (AC-6) both
// return at once and the ROM's start-up runs in the run loop, so an armed
// FREEZE ON START fires on either (ledger row 59-34; LIB-10 Q-1, user
// 2026-11-16: it replaces AC-30's Reset clause for Calc-U-59). Since M-53 the
// run control's pause-on-start cpu (TILevelRules); since M-54 LIB-18's
// `reset` and `cold-boot`.
TEST(lib10_ac30_part_cpu_freeze_on_start_sees_reset_and_clean_reset) {
    PartRig r;
    auto& c = r.part();
    CHECK(waitForScanLoop(r));
    for (bool clean : {false, true}) {
        rc(r).pauseOnStart(kCpu);
        r.run(50ms);  // the armed start sees the scan loop
        r.post(clean ? calcu::startup::kColdBootCommand : calcu::startup::kResetCommand);
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
    CHECK(!r.attach("slot-1", "ZZ").done);
    CHECK_EQ(c.moduleID(), std::string("ML"));
    CHECK(r.errors().empty());   // a refusal, no notice (LIB-12 AC-10)
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
    CHECK(r.debug().model().cpuHistory().empty());
    const auto sets = r.stateSets(mark);
    CHECK(std::find(sets.begin(), sets.end(), topics::kCpuDebug) == sets.end());
}

TEST(part_pause_of_a_replaced_machine_is_dropped) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kProgram);
    r.run(20ms);
    CHECK(r.model("TI-58").done);
    r.run(20ms);
    CHECK(!c.isFrozen());
    CHECK(r.debug().model().cpuHistory().empty());
}

// ── C59-6 (design T7, T10) and LIB-18 AC-21: the app session's part of the places ──

namespace {
// TRACE on, as the paper panel sends it (LIB-2's set-control; Part E simplification E-3).
calcu::sessions::Value traceOnArgs() {
    return calcu::sessions::Value::object({{"peripheral", "PC-100C"}, {"control", "TRACE"}, {"on", true}});
}
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

// C59-6 AC-12 (row 59-64; user, 2026-11-17, Q-3 of
// docs/reviews/e-simplification.md): without a PC-100C plugged in, LIB-2's
// set-control for one of its controls is refused.
TEST(c59_6_ac12_set_control_without_a_pc100c_refused) {
    PartRig r;
    CHECK(!traceSwitch(r.part()));   // the built-in default has no printer (row 59-10)
    const auto reply = r.send(calcu::peripherals::kSetControlCommand, traceOnArgs());
    CHECK(!reply.done);
    CHECK_EQ(reply.reason, std::string("No PC-100C is plugged in."));
    CHECK(!r.part().printerTrace());
}

// LIB-18 AC-21, AC-23 (design P59-4, D-27; was c59_6_ac6_trace_kept_by_app_session):
// TRACE keeps its position through Reset, Cold boot, a model switch and a
// detach; an attach puts it into the new PC-100C.
TEST(lib18_ac21_trace_kept_through_detach_and_attach) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(!c.printerTrace());                       // off when the product starts
    CHECK(traceSwitch(c) == std::optional<bool>(false));
    CHECK(r.send(calcu::peripherals::kSetControlCommand, traceOnArgs()).done);   // LIB-2 (E-3)
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.reset().done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.coldBoot().done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.model("TI-58C").done);                 // a new machine, without a printer (AC-18)
    CHECK(c.printerTrace() && !traceSwitch(c));
    CHECK(r.attach("printer", "PC-100C").done);     // the remembered position goes into it
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.detach("printer").done);                // detached: the position stays the session's
    CHECK(c.printerTrace() && !traceSwitch(c));
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    // Not saved: nothing holds it; a new app session starts off.
    PartRig again;
    CHECK(!again.part().printerTrace());
}

// C59-6 AC-6, row 59-25 (was c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it):
// a preset's build has TRACE off unless it says `printer-trace: on`.
TEST(c59_6_ac6_preset_sets_trace_off_unless_printer_trace_on) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(r.send(calcu::peripherals::kSetControlCommand, traceOnArgs()).done);   // LIB-2 (E-3)
    CHECK(c.printerTrace());
    CHECK(r.load(preset("data:\n  01: 1\n")).done);
    r.run(20ms);
    CHECK(!c.printerTrace() && traceSwitch(c) == std::optional<bool>(false));
    CHECK(r.load(preset("keys:\n  - printer-trace: on\n")).done);
    r.run(20ms);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
}

// LIB-8 AC-29 (C59-6 AC-1): the PC-100C's paper stays the PaperPart's while
// it is detached, and it is back, printed on below, when it is attached
// again on another model.
TEST(lib8_ac29_paper_back_on_attach_on_any_model) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(c.printerConnected());
    CHECK(keys(r, {"1", "2nd", "Prt"}));
    CHECK(r.runUntil([&] { return !r.printedText().empty(); }, 5000ms));
    CHECK(waitForScanLoop(r));   // the line printed and fed
    const auto lines = r.printedText();
    CHECK(lines.size() == 1 && lines[0] == "          1.        ");
    r.run(100ms);
    const calcu::paper::Reach reach = r.pc100cPaper()->reach();
    CHECK_EQ(reach.to, pc100cpaper::kLine);
    CHECK(r.detach("printer").done);
    CHECK(!c.printerConnected());
    CHECK(testroms::paper(c.emulator()->access().machine()) == nullptr);
    r.run(20ms);
    CHECK(r.printedText() == lines);
    CHECK(r.model("TI-58C").done);
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(testroms::paper(c.emulator()->access().machine()) != nullptr);
    CHECK(r.printedText() == lines);              // the paper is back with the PC-100C
    CHECK(waitForScanLoop(r));
    CHECK(keys(r, {"2", "2nd", "Prt"}));
    CHECK(r.runUntil([&] { return r.printedText().size() == 2; }, 5000ms));
    CHECK(waitForScanLoop(r));
    r.run(100ms);
    const auto& text = r.pc100cPaper()->text();
    CHECK(text.size() == 2 && text[1].text == "          2.        ");
    CHECK_EQ(text[1].at.y, pc100cpaper::kLine);   // printed on below the first line
}

// C59-6 AC-11, row 59-11 (LIB-18 AC-21, design P59-4): a module change and
// the printer power-cycle the machine, now through `attach` and `detach`.
TEST(c59_6_ac11_change_module_power_cycles) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(waitForScanLoop(r));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t regs[16];
    testroms::mem(c.emulator()->access().machine()).writeRAMReg(90, marked);
    CHECK(r.attach("slot-1", "LE").done);   // a replace
    CHECK_EQ(c.moduleID(), std::string("LE"));
    CHECK(testroms::moduleIn(c.emulator()->access().machine()) == "LE");
    // The power cycle: the TI-59 loses its program and registers (row 59-11).
    CHECK(waitForScanLoop(r));
    testroms::mem(c.emulator()->access().machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
    // The printer too.
    testroms::mem(c.emulator()->access().machine()).writeRAMReg(90, marked);
    CHECK(r.detach("printer").done);
    CHECK(waitForScanLoop(r));
    testroms::mem(c.emulator()->access().machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
    // An empty slot.
    CHECK(r.detach("slot-1").done);
    CHECK(c.moduleID().empty() && testroms::moduleIn(c.emulator()->access().machine()).empty());
    CHECK(waitForScanLoop(r));
    CHECK(r.errors().empty());
}

// ── Presets, cards, cue cards ────────────────────────────────────────────────

// C59-2 AC-7 (was part_plays_a_preset): a preset's keys and its elapsed time.
TEST(c59_2_ac7_preset_plays_its_keys) {
    PartRig r;
    const auto reply = r.load(preset("keys:\n  - regular-speed:\n  - key: CLR 2 + 3 =\n  - elapsed-time: start\n"
                                     "  - wait: 50ms\n  - elapsed-time: report\n"));
    CHECK(reply.done);
    r.run(500ms);
    CHECK_EQ(testroms::displayText(r.part().emulator()->machine()), std::string("5"));
    bool elapsed = false;   // elapsed-time report: a line of the session's log (LIB-6 AC-23)
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kLogTopic)
            for (const auto& p : s->appended)
                if (const auto* l = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p))
                    elapsed = elapsed || l->text.find("0:00.05") != std::string::npos;
    CHECK(elapsed);
    CHECK(r.errors().empty());
}

// C59-2 AC-7 (was part_reload_keeps_the_program): sections after keys change
// only what they list.
TEST(c59_2_ac7_reload_keeps_the_program) {
    PartRig r;
    CHECK(r.load(preset("program:\n  keycodes: |\n    85 01\nkeys:\n  - key: CLR\ndata:\n  05: 3.25\n")).done);
    CHECK_EQ(step(r, 1), 1);
    CHECK_EQ(reg(r, 5), 3.25);
}

TEST(part_shows_the_library_program_card) {
    PartRig r;
    r.run(50ms);
    CHECK(r.step("key", "Pgm 0 1").done);
    r.run(1s);
    CHECK(r.part().cueCard().has_value());
    if (r.part().cueCard()) CHECK_EQ(r.part().cueCard()->id, std::string("ML-01"));
}

// C59-2 AC-7 (was part_reload_after_the_first_run): the second stage uses the
// first one's register and fits its cue card.
TEST(c59_2_ac7_reload_after_the_first_run) {
    PartRig r;
    CHECK(r.load(preset("program:\n  keycodes: |\n    76 11 01 02 03 42 08 91\n"   // Lbl A 123 STO 08 R/S
                        "keys:\n  - key: A\n"
                        "program:\n  keycodes: |\n    76 11 43 08 85 01 95 91\n"      // Lbl A RCL 08 + 1 = R/S
                        "data:\n  07: 9\ncue-card:\n  title: Part 2\n"
                        "keys:\n  - key: A\n  - let-it-finish:\n")).done);
    r.run(300ms);
    calcu::machine::Machine& m = r.part().emulator()->machine();
    CHECK_EQ(testroms::displayText(m), std::string("124"));  // part 2 used part 1's R08
    CHECK_EQ(testroms::mem(m).readDataReg(7), 9.0);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Part 2");
    CHECK(r.errors().empty());
}

// LIB-6 §11.2 (until LIB-13 59's `open`, M-56): a dropped preset is LIB-6's
// load-preset by its content.
TEST(lib13_ac16_card_on_the_ti59_and_preset_by_content) {
    PartRig r;
    CHECK(drop(r, preset("program:\n  keycodes: |\n    76 11\nkeys:\n  - key: CLR\n", "TI-58"), "", "p.ti58"));
    CHECK(r.runUntil([&] { return !r.busy(); }));
    CHECK(r.part().model() == Model::TI58);
    CHECK_EQ(step(r, 1), 11);
    CHECK(r.errors().empty());
    CHECK(!drop(r, std::nullopt, "", "p.ti58"));
    CHECK(!r.errors().empty());  // reported, as a failed Open is
}


// ── LIB-8's Calc-U-59 column in the app session (design §11.6, E8-6, E8-7) ──

// AC-29: detached, the panel's owner is none (the control state's paper
// panel; release prep 3: `c59.printer` went) and the papers topic still lists
// the PC-100C's paper, empty of nothing it had.
TEST(lib8_ac29_detached_panel_owner_empty) {
    PartRig r;
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(r.step("key", "1 2nd Prt").done);
    CHECK(r.runUntil([&] { return !r.printedText().empty(); }, 5000ms));
    CHECK(r.detach("printer").done);
    r.run(50ms);
    const auto* control = r.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    CHECK(control && control->panels.size() == 1 && !control->panels[0].owner);
    CHECK(control && control->controls.empty());   // the PC-100C's PRINT, ADV, TRACE went with it
    const auto* papers = r.state<calcu::paper::PaperList>(calcu::paper::kPapersTopic);
    CHECK(papers && papers->papers.size() == 1 && papers->papers[0].owner == "PC-100C");
    CHECK_EQ(r.printedText().size(), std::size_t{1});   // kept while detached
    // Without a name the paper commands mean the printer plugged in: none (EC-8).
    const auto reply = r.send(calcu::paper::kPaperCutCommand, calcu::sessions::Value::object({{"copy", false}}));
    CHECK(!reply.done && reply.reason == calcu::paper::kNoPrinterPlugged);
}

// AC-8, LIB-6 AC-41: a user's load of a preset that names the printer starts
// with empty paper.
TEST(lib8_ac8_preset_naming_printer_starts_empty) {
    PartRig r;
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(r.step("key", "1 2nd Prt").done);
    CHECK(r.runUntil([&] { return !r.printedText().empty(); }, 5000ms));
    CHECK(r.load(preset("data:\n  01: 1\n")).done);
    r.run(50ms);
    CHECK(r.printedText().empty());
    CHECK(r.pc100cPaper()->empty());
}

// AC-22: the step cut-paper: cuts the paper of the printer plugged in, without a copy.
TEST(lib8_ac22_preset_cut_paper) {
    PartRig r;
    CHECK(r.load(preset("keys:\n  - regular-speed:\n  - key: 1 2nd Prt\n  - cut-paper:\n  - key: 2 2nd Prt\n"
                        "  - let-it-finish:\n")).done);
    r.run(300ms);
    CHECK(r.errors().empty());
    const auto text = r.printedText();
    CHECK(text.size() == 1 && text[0] == "          2.        ");
    CHECK_EQ(r.pc100cPaper()->text()[0].at.y, calcu::paper::Coord{0});   // y counts from the cut
}
