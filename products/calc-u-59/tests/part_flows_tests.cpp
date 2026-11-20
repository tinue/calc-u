// The product's flows in cu59::TIAppPart on the library's app session: model
// switches, presets, freezes, the ASM overlay, the printer and module places,
// cards and cue cards, on the library's commands (model, reset, cold-boot,
// attach, detach; load-preset {content}, run-step) and presets of format 2.
// The card cases are in card_flows_tests.cpp.

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
    return r.runUntil([&] { return !r.busy() && r.part().machine()->isKeyReady(); }, 3000ms);
}

// The run control of the part's session.
calcu::debugger::RunControl& rc(PartRig& r) { return r.part().debuggerPart().runControl(); }
using calcu::debugger::kCpu;
using calcu::debugger::kProgram;

// A preset of format 2 for `model`, with the Master Library module and,
// unless `printer` is false, the PC-100C; `body` its sections.
std::string preset(const std::string& body, const std::string& model = "TI-59", bool printer = true) {
    return "format-version: 2\nmodel: " + model + "\nslot-1: { name: ML }\n" + (printer ? "printer: PC-100C\n" : "") + body;
}

// A file dropped on the calculator: the library's `open {way: drop}`;
// true when it was opened.
bool drop(PartRig& r, const std::optional<std::string>& content, const std::string& path, const std::string& name) {
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = name;
    if (!path.empty()) f.path = path;   // none: as its content
    if (content) f.bytes = std::vector<std::uint8_t>(content->begin(), content->end());
    q.files = {f};
    q.way = calcu::opening::Way::Drop;
    return r.operation(calcu::opening::kOpenCommand, q.toValue()).done;
}

// Keys at regular speed through the library's `key` command (a user's keys,
// pressed at regular speed; the preset's run-step runs at
// full speed); the reply comes when the last key is up.
bool keys(PartRig& r, std::initializer_list<const char*> names) {
    using calcu::sessions::Value;
    Value::Array a;
    for (const char* n : names) a.emplace_back(n);
    const auto reply = r.operation(calcu::typing::kKey, Value::object({{"names", Value(std::move(a))}}), 30000ms);
    return reply.done;
}

double reg(PartRig& r, int n) { return testroms::mem(*r.part().machine()).readDataReg(n); }
int step(PartRig& r, int s) { return int(testroms::mem(*r.part().machine()).readProgramStep(s)); }

} // namespace

// A preset that names another model builds it; `[startup] last-model` follows.
TEST(preset_for_another_model_builds_it_last_model_follows) {
    PartRig r;
    auto& c = r.part();
    const auto reply = r.load(preset("data:\n  05: 3.25\n", "TI-58"));
    CHECK(reply.done);
    CHECK(c.model() == ModelId{kTI58});
    CHECK(!c.isFrozen());
    CHECK_EQ(reg(r, 5), 3.25);
    CHECK_EQ(r.settings.text("startup", "last-model"), std::string("TI-58"));
    CHECK(r.errors().empty());
}

// Any error refuses the file whole, with its first error and its line; the
// machine stays as it was.
TEST(first_error_refuses_the_file_machine_untouched) {
    PartRig r(ModelId{kTI58});
    auto& c = r.part();
    const auto ticksBefore = r.ticks();
    auto reply = r.load(preset("partition: 959\ndata:\n  77: 1\n", "TI-58"));   // beyond the TI-58's 479; R77 too
    CHECK(!reply.done);
    CHECK(reply.reason.find("line 5") != std::string::npos);   // the first error only
    CHECK(reply.reason.find("line 7") == std::string::npos);
    CHECK(c.model() == ModelId{kTI58} && r.ticks() >= ticksBefore);
    reply = r.load("\xff\xfe");
    CHECK(!reply.done && reply.reason.find("UTF-8") != std::string::npos);
    reply = r.load("partition: 479\n");   // no format-version
    CHECK(!reply.done && reply.reason.find("format-version: is required") != std::string::npos);
}

TEST(part_load_while_frozen_ends_running_without_tracing) {
    PartRig r;
    auto& c = r.part();
    rc(r).pause(kProgram);
    CHECK(r.runUntil([&] { return c.isFrozen() && !r.debug().model().cpuHistory().empty(); }, 3000ms));
    CHECK(r.load(preset("data:\n  01: 7\n")).done);
    CHECK(!c.isFrozen());
    r.run(20ms);
    CHECK_EQ(testroms::hw(*c.machine()).traceFlags(), static_cast<uint32_t>(TRACE_NONE));
}

// A new machine disarms pause-on-start. A model switch builds the built-in
// default (Master Library, no printer).
TEST(part_model_switch_keeps_log_resets_freeze) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    r.debug().dump("vars");
    rc(r).pauseOnStart(kProgram);
    CHECK(c.runState().armed == kProgram);
    CHECK(r.model("TI-58").done);
    CHECK(c.model() == ModelId{kTI58});
    CHECK(!c.printerConnected());   // nothing of the old machine is carried
    CHECK(testroms::moduleIn(*c.machine()) == "ML");
    CHECK_EQ(c.machine()->plugged().byPlace.size(), size_t{1});   // the module only
    r.run(20ms);
    CHECK(!c.runState().armed && !c.isFrozen());
    bool dumped = false;   // the dump is in the session's log, which the switch keeps
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
    testroms::mem(*c.machine()).writeRAMReg(90, marked);
    CHECK(r.reset().done);
    CHECK(waitForScanLoop(r));
    testroms::mem(*c.machine()).copyRAMReg(90, regs);
    CHECK(std::equal(std::begin(marked), std::end(marked), regs));
    CHECK(r.coldBoot().done);
    testroms::mem(*c.machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
}

// A plain Reset and a clean reset both return at once and the ROM's start-up
// runs in the run loop, so an armed FREEZE ON START (the run control's pause-on-start cpu, TILevelRules) fires on either
// the library's `reset` or `cold-boot`.
TEST(part_cpu_freeze_on_start_sees_reset_and_clean_reset) {
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
    CHECK(r.errors().empty());   // a refusal, no notice
}

// A resume before the pause landed: no pause, no frozen view.
TEST(part_resume_before_the_frozen_view_drops_it) {
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

// ── The app session's part of the printer and module places ─────────────────

namespace {
// TRACE on, as the paper panel sends it (the library's set-control).
calcu::sessions::Value traceOnArgs() {
    return calcu::sessions::Value::object({{"peripheral", "PC-100C"}, {"control", "TRACE"}, {"on", true}});
}
// TRACE as the PC-100C in the machine has it; nullopt without a PC-100C.
std::optional<bool> traceSwitch(TIAppPart& c) {
    calcu::machine::Machine& m = *c.machine();
    auto* controls = testroms::printerControls(m);
    if (!controls) return std::nullopt;
    for (const auto& i : controls->list())
        if (i.id == PC100C::kTrace) return i.on;
    return std::nullopt;
}
} // namespace

// Without a PC-100C plugged in, the library's set-control for one of its
// controls is refused.
TEST(set_control_without_a_pc100c_refused) {
    PartRig r;
    CHECK(!traceSwitch(r.part()));   // the built-in default has no printer
    const auto reply = r.send(calcu::peripherals::kSetControlCommand, traceOnArgs());
    CHECK(!reply.done);
    CHECK_EQ(reply.reason, std::string("No PC-100C is plugged in."));
    CHECK(!r.part().printerTrace());
}

// TRACE keeps its position through Reset, Cold boot, a model switch and a
// detach; an attach puts it into the new PC-100C.
TEST(trace_kept_through_detach_and_attach) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(!c.printerTrace());                       // off when the product starts
    CHECK(traceSwitch(c) == std::optional<bool>(false));
    CHECK(r.send(calcu::peripherals::kSetControlCommand, traceOnArgs()).done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.reset().done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.coldBoot().done);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
    CHECK(r.model("TI-58C").done);                 // a new machine, without a printer
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

// A preset's build has TRACE off unless it says `printer-trace: on`.
TEST(preset_sets_trace_off_unless_printer_trace_on) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(r.send(calcu::peripherals::kSetControlCommand, traceOnArgs()).done);
    CHECK(c.printerTrace());
    CHECK(r.load(preset("data:\n  01: 1\n")).done);
    r.run(20ms);
    CHECK(!c.printerTrace() && traceSwitch(c) == std::optional<bool>(false));
    CHECK(r.load(preset("keys:\n  - printer-trace: on\n")).done);
    r.run(20ms);
    CHECK(c.printerTrace() && traceSwitch(c) == std::optional<bool>(true));
}

// The PC-100C's paper stays the PaperPart's while
// it is detached, and it is back, printed on below, when it is attached
// again on another model.
TEST(paper_back_on_attach_on_any_model) {
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
    CHECK(testroms::paper(*c.machine()) == nullptr);
    r.run(20ms);
    CHECK(r.printedText() == lines);
    CHECK(r.model("TI-58C").done);
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(testroms::paper(*c.machine()) != nullptr);
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

// A module change and the printer power-cycle the machine, through `attach`
// and `detach`.
TEST(change_module_power_cycles) {
    PartRig r;
    auto& c = r.part();
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(waitForScanLoop(r));
    const uint8_t marked[16] = {1, 2, 3};
    uint8_t regs[16];
    testroms::mem(*c.machine()).writeRAMReg(90, marked);
    CHECK(r.attach("slot-1", "LE").done);   // a replace
    CHECK_EQ(c.moduleID(), std::string("LE"));
    CHECK(testroms::moduleIn(*c.machine()) == "LE");
    // The power cycle: the TI-59 loses its program and registers (row 59-11).
    CHECK(waitForScanLoop(r));
    testroms::mem(*c.machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
    // The printer too.
    testroms::mem(*c.machine()).writeRAMReg(90, marked);
    CHECK(r.detach("printer").done);
    CHECK(waitForScanLoop(r));
    testroms::mem(*c.machine()).copyRAMReg(90, regs);
    CHECK_EQ(int(regs[0]), 0);
    // An empty slot.
    CHECK(r.detach("slot-1").done);
    CHECK(c.moduleID().empty() && testroms::moduleIn(*c.machine()).empty());
    CHECK(waitForScanLoop(r));
    CHECK(r.errors().empty());
}

// ── Presets, cards, cue cards ────────────────────────────────────────────────

// A preset's keys and its elapsed time.
TEST(part_preset_plays_its_keys) {
    PartRig r;
    const auto reply = r.load(preset("keys:\n  - regular-speed:\n  - key: CLR 2 + 3 =\n  - elapsed-time: start\n"
                                     "  - wait: 50ms\n  - elapsed-time: report\n"));
    CHECK(reply.done);
    r.run(500ms);
    CHECK_EQ(testroms::displayText(*r.part().machine()), std::string("5"));
    bool elapsed = false;   // elapsed-time report: a line of the session's log
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kLogTopic)
            for (const auto& p : s->appended)
                if (const auto* l = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p))
                    elapsed = elapsed || l->text.find("0:00.05") != std::string::npos;
    CHECK(elapsed);
    CHECK(r.errors().empty());
}

// Sections after keys change only what they list.
TEST(part_reload_keeps_the_program) {
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

// The second stage uses the first one's register and fits its cue card.
TEST(part_reload_after_the_first_run) {
    PartRig r;
    CHECK(r.load(preset("program:\n  keycodes: |\n    76 11 01 02 03 42 08 91\n"   // Lbl A 123 STO 08 R/S
                        "keys:\n  - key: A\n"
                        "program:\n  keycodes: |\n    76 11 43 08 85 01 95 91\n"      // Lbl A RCL 08 + 1 = R/S
                        "data:\n  07: 9\ncue-card:\n  title: Part 2\n"
                        "keys:\n  - key: A\n  - let-it-finish:\n")).done);
    r.run(300ms);
    calcu::machine::Machine& m = *r.part().machine();
    CHECK_EQ(testroms::displayText(m), std::string("124"));  // part 2 used part 1's R08
    CHECK_EQ(testroms::mem(m).readDataReg(7), 9.0);
    CHECK(r.part().cueCard() && r.part().cueCard()->title == "Part 2");
    CHECK(r.errors().empty());
}

// A dropped preset is loaded by its content (the library's load-preset).
TEST(dropped_preset_loads_by_its_content) {
    PartRig r;
    CHECK(drop(r, preset("program:\n  keycodes: |\n    76 11\nkeys:\n  - key: CLR\n", "TI-58"), "", "p.ti58"));
    CHECK(r.runUntil([&] { return !r.busy(); }));
    CHECK(r.part().model() == ModelId{kTI58});
    CHECK_EQ(step(r, 1), 11);
    CHECK(r.errors().empty());
    CHECK(!drop(r, std::nullopt, "", "p.ti58"));
    CHECK(!r.errors().empty());  // reported, as a failed Open is
}


// ── The PC-100C's paper in the app session ─────────────────────────────────

// Detached, the panel's owner is none (the control state's paper panel) and
// the papers topic still lists the PC-100C's paper, empty of nothing it had.
TEST(detached_paper_panel_owner_empty) {
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
    // Without a name the paper commands mean the printer plugged in: none.
    const auto reply = r.send(calcu::paper::kPaperCutCommand, calcu::sessions::Value::object({{"copy", false}}));
    CHECK(!reply.done && reply.reason == calcu::paper::kNoPrinterPlugged);
}

// A user's load of a preset that names the printer starts with empty paper.
TEST(preset_naming_printer_starts_empty) {
    PartRig r;
    CHECK(r.attach("printer", "PC-100C").done);
    CHECK(r.step("key", "1 2nd Prt").done);
    CHECK(r.runUntil([&] { return !r.printedText().empty(); }, 5000ms));
    CHECK(r.load(preset("data:\n  01: 1\n")).done);
    r.run(50ms);
    CHECK(r.printedText().empty());
    CHECK(r.pc100cPaper()->empty());
}

// The step cut-paper: cuts the paper of the printer plugged in, without a copy.
TEST(preset_cut_paper) {
    PartRig r;
    CHECK(r.load(preset("keys:\n  - regular-speed:\n  - key: 1 2nd Prt\n  - cut-paper:\n  - key: 2 2nd Prt\n"
                        "  - let-it-finish:\n")).done);
    r.run(300ms);
    CHECK(r.errors().empty());
    const auto text = r.printedText();
    CHECK(text.size() == 1 && text[0] == "          2.        ");
    CHECK_EQ(r.pc100cPaper()->text()[0].at.y, calcu::paper::Coord{0});   // y counts from the cut
}
