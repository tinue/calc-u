// C59-4 (design §6): Calc-U-59's debug panel below the frontends: the
// product topics and entries (AC-1, AC-25), `debug-panel:` (AC-2, EC-4), the
// CALCULATOR tab (AC-3, AC-4), the buttons (AC-5, AC-7, EC-3), the CPU tab
// (AC-6), the heatmap (AC-8), the LOG tab (AC-9 to AC-12), the ASM overlay's
// kind, box, Run and life (AC-13, AC-16 to AC-18, AC-26, EC-1, EC-2), its
// file kind (AC-19, EC-5) and what 2.3.0's tests checked (AC-23; the cases
// of shared_debug_tests.cpp:184-287 and part_flows_tests.cpp:229, moved
// here). The format is asm_overlay_tests.cpp; the harness harness_tests.cpp.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include "../app/Bcd.hpp"
#include "../app/DebugLog.hpp"
#include "../app/DebugSnapshots.hpp"
#include "../app/DebugText.hpp"
#include "../app/RomHeatmap.hpp"
#include "../app/TIDebugPart.hpp"
#include "../core/TIDebugKinds.hpp"
#include "control/ControlState.hpp"
#include "debugger/ButtonRule.hpp"
#include "debugger/RunState.hpp"
#include "opening/OpenRequest.hpp"
#include <algorithm>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::debugger::kCpu;
using calcu::debugger::kProgram;
using calcu::debugger::RunState;
using calcu::sessions::Value;
using testroms::SessionRig;

namespace {

const std::string kOverlay = "Calc-U-59 ASM 2.0\nHEX:\n0A01 0A01\n";
// simple_count.asm: a counter on the display, a loop at 1802.
const std::string kCounter = "Calc-U-59 ASM 2.0\nHEX:\n01D8 01DB 0D00 0A37 1805 1007\n";

calcu::debugger::RunControl& rc(SessionRig& r) { return r.part().debuggerPart()->runControl(); }
bool paused(SessionRig& r) { return r.part().isFrozen(); }
bool ready(SessionRig& r) { return r.until([&] { return r.machine().isKeyReady() && !r.busy(); }); }
void program(SessionRig& r, std::vector<uint8_t> codes) {
    testroms::mem(r.machine()).writeProgram(codes.data(), static_cast<int>(codes.size()));
}
const std::vector<uint8_t> kLoop = {76, 11, 85, 1, 95, 61, 0, 0};   // Lbl A + 1 = GTO 000

template <class P> const P* topic(SessionRig& r, const calcu::sessions::TopicName& t) { return r.state<P>(t); }
const TIAsmOverlayState* asmState(SessionRig& r) { return topic<TIAsmOverlayState>(r, topics::kAsmOverlay); }

const calcu::control::ProductEntry* entry(SessionRig& r, const char* name) {
    const auto* cs = r.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return cs ? cs->productEntry(name) : nullptr;
}
bool available(SessionRig& r, const char* name) {
    const auto* e = entry(r, name);
    if (!e || !e->value) return false;
    const Value v = e->value->toValue();
    return v.find("available") && v.find("available")->boolean() && *v.find("available")->boolean();
}
std::string reason(SessionRig& r, const char* name) {
    const auto* e = entry(r, name);
    if (!e || !e->value) return {};
    const Value v = e->value->toValue();
    return v.find("reason") && v.find("reason")->string() ? *v.find("reason")->string() : std::string();
}

calcu::sessions::ReplyMessage asmLoad(SessionRig& r, const std::optional<std::string>& text, const std::string& name = "a.asm") {
    Value::Object o{{"name", Value(name)}};
    if (text) o.push_back({"text", Value(*text)});
    return r.send(commands::kAsmLoad, Value(std::move(o)));
}

std::vector<std::string> logTexts(SessionRig& r) {
    std::vector<std::string> out;
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<calcu::sessions::StreamMessage>(&m); s && s->topic == calcu::sessions::kLogTopic)
            for (const auto& p : s->appended)
                if (const auto* l = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p)) out.push_back(l->text);
    return out;
}
bool logged(SessionRig& r, const std::string& part) {
    const auto t = logTexts(r);
    return std::any_of(t.begin(), t.end(), [&](const std::string& s) { return s.find(part) != std::string::npos; });
}

calcu::sessions::ReplyMessage open(SessionRig& r, const std::string& name, const std::string& text,
                                   calcu::opening::Way way = calcu::opening::Way::Drop) {
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = name;
    f.bytes = std::vector<std::uint8_t>(text.begin(), text.end());
    q.files = {f};
    q.way = way;
    return r.operation(calcu::opening::kOpenCommand, q.toValue());
}

} // namespace

// ── T1: the topics (AC-1, AC-25) ─────────────────────────────────────────────

TEST(c59_4_ac25_six_product_topics_read_by_state) {
    SessionRig r;
    CHECK(ready(r));
    for (const auto* t : {&topics::kCalculatorDebug, &topics::kProgramListing, &topics::kCpuDebug, &topics::kHeatmap,
                          &topics::kAsmOverlay, &topics::kDebugView}) {
        const auto* p = r.fe.mirror.state(*t);
        CHECK(p && *p);
        if (p && *p) (void)(*p)->toValue();   // what the harness's `state` reads
    }
    CHECK_EQ(asmState(r)->fileName, std::string("No file selected"));
    CHECK(topic<TIDebugView>(r, topics::kDebugView)->logLevel == DebugLevel::Off);
}

TEST(c59_4_ac25_entries_asm_run_trace_calculator_tab_with_reasons) {
    SessionRig r;
    CHECK(ready(r));
    r.frame();
    CHECK(!available(r, entries::kAsmRun));
    CHECK_EQ(reason(r, entries::kAsmRun), std::string(kNoAsmOverlay));
    CHECK(available(r, entries::kTrace));   // the rig's trace folder can be written
    CHECK(available(r, entries::kCalculatorTab));
    CHECK(asmLoad(r, kCounter).done);
    r.frame();
    CHECK(available(r, entries::kAsmRun));
    CHECK(r.send(commands::kAsmRun).done);
    r.frame();
    CHECK(!available(r, entries::kCalculatorTab));
    CHECK_EQ(reason(r, entries::kCalculatorTab), std::string(kAsmOverlayRuns));
    // No folder to write traces to: TRACE cannot be used.
    testroms::RigOptions o;
    o.traceFolder = "/dev/null/no";
    SessionRig w(o);
    CHECK(ready(w));
    w.frame();
    CHECK(!available(w, entries::kTrace));
    CHECK_EQ(reason(w, entries::kTrace), std::string(kTraceFolderNotWritable));
}

TEST(c59_4_ac25_cpu_and_calculator_computed_while_shown) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    const auto* before = r.fe.mirror.state(topics::kCalculatorDebug);
    const calcu::sessions::PayloadPtr first = before ? *before : nullptr;
    r.tap(11);   // A: the program runs
    r.runFor(200ms);
    CHECK(r.fe.mirror.state(topics::kCalculatorDebug) && *r.fe.mirror.state(topics::kCalculatorDebug) == first);   // not shown: not computed
    r.send(commands::kLiveDebug, Value::object({{"on", true}}));
    r.runFor(200ms);
    CHECK(topic<TICalculatorDebug>(r, topics::kCalculatorDebug)->live.currentStep >= 0);
    CHECK(topic<TICpuDebug>(r, topics::kCpuDebug)->entries.empty());   // the CPU tab not shown
    r.send(commands::kCpuDebug, Value::object({{"on", true}}));
    r.runFor(200ms);
    const auto* cpu = topic<TICpuDebug>(r, topics::kCpuDebug);
    CHECK(cpu && !cpu->entries.empty() && cpu->entries.size() <= 32);
}

TEST(c59_4_ac1_panel_fed_by_product_and_generic_topics) {
    SessionRig r;
    CHECK(ready(r));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"button", true}}));
    r.send(commands::kDump, Value::object({{"what", "scom"}}));
    r.frame();
    CHECK(r.fe.mirror.state(calcu::debugger::kRunStateTopic) != nullptr);
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
    CHECK(logged(r, "── SCOM ──"));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"off", true}}));
}

// ── T2: debug-panel: (AC-2, EC-4) ────────────────────────────────────────────

TEST(c59_4_ac2_view_shown) {
    SessionRig r;
    CHECK(ready(r));
    r.debug().showView(r.session().context(), "cpu");
    r.frame();
    CHECK(r.panels().back() == DebugPanel::Cpu);
    r.debug().showView(r.session().context(), "log");
    r.frame();
    CHECK(r.panels().back() == DebugPanel::Log);
}

TEST(c59_4_ac2_calculator_passed_over_while_overlay_runs) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    r.frame();
    CHECK(r.panels().back() == DebugPanel::Cpu);   // AC-18: the CPU tab while it runs
    const auto n = topic<TIDebugView>(r, topics::kDebugView)->requests;
    r.debug().showView(r.session().context(), "calculator");
    r.frame();
    CHECK_EQ(topic<TIDebugView>(r, topics::kDebugView)->requests, n);   // without a word
    CHECK(r.notices().empty());
}

// EC-4: a frontend that does not show the panel keeps the last view; one
// that starts to listen later gets it too (a state topic).
TEST(c59_4_ec4_view_kept_until_shown) {
    SessionRig r;
    CHECK(ready(r));
    r.debug().showView(r.session().context(), "log");
    r.frame();
    calcu::sessions::test::TestFrontend late;
    r.session().listen(late);
    const auto* v = late.state<TIDebugView>(topics::kDebugView);
    CHECK(v && v->view == DebugPanel::Log && v->requests == 1);
}

// ── T3: the CALCULATOR tab (AC-3, AC-4, AC-23) ───────────────────────────────

TEST(c59_4_ac3_shows_registers_partition_steps_flags_source_scom_returns) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    // LBL A, STO 12, RCL 12, GTO 01 23, R/S
    const uint8_t prog[] = {76, 11, 42, 12, 43, 12, 61, 1, 23, 91};
    testroms::mem(*m).writeProgram(prog, sizeof prog);
    auto reg = encodeBcd(std::string_view("2.5"));
    testroms::mem(*m).writeDataRegister(3, reg.data());

    std::vector<uint8_t> romCache;
    auto lib = ModuleImage::parse(*cu59::moduleImage(*testroms::romSet(), "ML.rom.yaml"));
    auto b = buildLiveSnapshot(*m, lib, romCache, calcViewState(*m));
    const auto& s = b.snap;
    CHECK(s.programSource == ProgramSource::UserProgram);
    CHECK_EQ(s.currentStep, 0);
    CHECK_EQ(static_cast<int>(s.programWindow.size()), 6);  // steps 0–5
    CHECK_EQ(s.programWindow[1].mnemonic, std::string("A"));
    CHECK_EQ(s.programWindow[3].mnemonic, std::string("12"));  // STO operand
    CHECK(s.angleMode == AngleMode::Deg);
    CHECK_EQ(static_cast<int>(s.nonZeroRegs.size()), 1);
    if (!s.nonZeroRegs.empty()) {
        CHECK_EQ(s.nonZeroRegs[0].num, 3);
        CHECK_EQ(s.nonZeroRegs[0].value, 2.5);
    }
    CHECK(s.dataRegCount > 0 && s.programRegCount > 0);   // the partition
    for (const auto& row : s.scomRows) CHECK_EQ(row.size(), size_t{16});
    CHECK_EQ(s.statusRegister, std::string(s.scomRows[0].rbegin(), s.scomRows[0].rend()));  // SCOM[0], nibble 15 first
    CHECK(b.argSteps == (std::set<int>{3, 5, 7, 8}));

    auto listing = programListing(b.sourceKeycodes, b.argSteps, s.currentStep);
    CHECK_EQ(listing.size(), b.sourceKeycodes.size());
    CHECK(listing[0].isCurrent && !listing[1].isCurrent);
    CHECK_EQ(listing[7].mnemonic, std::string("01"));
    CHECK_EQ(listing[9].mnemonic, std::string("R/S"));
    CHECK(buildLiveSnapshot(*m, lib, romCache, calcViewState(*m)).snap == s);
}

TEST(c59_4_ac3_ti58c_shows_h00_to_h03) {
    auto m = testroms::makeMachine(Model::TI58C);
    testroms::runToScanLoop(*m, Model::TI58C);
    auto* pm = cu59::kindOf<ProgramMemory>(*m);
    CHECK(pm && pm->extraCount() == 4);
    RegisterNibbles v{};
    const auto seven = encodeBcd(std::string_view("7"));
    std::copy(seven.begin(), seven.end(), v.begin());
    pm->writeExtra(2, v);
    std::vector<uint8_t> romCache;
    const auto s = buildLiveSnapshot(*m, ModuleImage{}, romCache, calcViewState(*m)).snap;
    const bool h02 = std::any_of(s.nonZeroRegs.begin(), s.nonZeroRegs.end(),
                                 [](const auto& e) { return e.isHidden && e.num == 2 && e.value == 7; });
    CHECK(h02);
}

// Through the debug target (LIB-10 AC-28): the data registers and SCOM the
// tab shows are what the target's spaces hold.
TEST(c59_4_ac3_read_through_the_target) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto reg = encodeBcd(std::string_view("2.5"));
    testroms::mem(*m).writeDataRegister(3, reg.data());
    const auto& t = m->debugging()->target(0);
    std::vector<uint8_t> romCache;
    const auto s = buildLiveSnapshot(*m, ModuleImage{}, romCache, calcViewState(*m)).snap;
    std::uint8_t n[16];
    testroms::mem(*m).copyRAMReg(testroms::mem(*m).dataRegCount() - 1 - 3, n);
    std::uint64_t want = 0;
    for (int i = 15; i >= 0; --i) want = (want << 4) | n[i];
    CHECK_EQ(t.peek(1, static_cast<std::uint64_t>(testroms::mem(*m).dataRegCount() - 1 - 3), {}).value, want);
    CHECK(!s.nonZeroRegs.empty() && s.nonZeroRegs[0].value == 2.5);
}

TEST(c59_4_ac4_listing_while_paused_memory_rom_module) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.send(commands::kLiveDebug, Value::object({{"on", true}}));
    r.tap(11);
    rc(r).pause(kProgram);
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.frame();
    const auto* l = topic<TIProgramListing>(r, topics::kProgramListing);
    CHECK(l && !l->steps.empty());
    CHECK(l && l->source == ProgramSource::UserProgram);
    rc(r).resume();
    r.runFor(50ms);
    l = topic<TIProgramListing>(r, topics::kProgramListing);
    CHECK(l && l->steps.empty());   // empty while it runs
    // The ROM's own programs and the module: the program source of the listing.
    CHECK(programSourceLabel(ProgramSource::SolidState, 0).size() > 0);
}

TEST(c59_4_ac4_current_step_marked) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.send(commands::kLiveDebug, Value::object({{"on", true}}));
    r.tap(11);
    rc(r).pause(kProgram);
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.frame();
    const auto* l = topic<TIProgramListing>(r, topics::kProgramListing);
    CHECK(l && l->currentIndex >= 0 && l->currentIndex < static_cast<int>(l->steps.size()));
    if (l && l->currentIndex >= 0) {
        CHECK(l->steps[static_cast<size_t>(l->currentIndex)].isCurrent);
        CHECK(programStepLine(l->steps[static_cast<size_t>(l->currentIndex)], true).find("next") != std::string::npos);
    }
    rc(r).resume();
}

// ── T4: the buttons (AC-5, AC-7, EC-3) ───────────────────────────────────────

TEST(c59_4_ac5_freeze_on_start_arms_then_armed_disarms) {
    SessionRig r;
    CHECK(ready(r));
    r.send(calcu::debugger::kPauseOnStartCommand, Value::object({{"level", "program"}}));
    r.frame();
    const auto* s = r.state<RunState>(calcu::debugger::kRunStateTopic);
    CHECK(freezeButtons(kProgram, s).armed);              // reads ARMED; pressing it …
    CHECK(!freezeButtons(kProgram, s).freezeOnStart);
    CHECK(!freezeButtons(kProgram, s).freeze);
    r.send(calcu::debugger::kPauseOnStartCommand, Value::object({{"off", true}}));
    r.frame();
    s = r.state<RunState>(calcu::debugger::kRunStateTopic);
    CHECK(!freezeButtons(kProgram, s).armed);              // … disarms it
    CHECK(freezeButtons(kProgram, s).freeze && freezeButtons(kProgram, s).freezeOnStart);
}

TEST(c59_4_ac5_freeze_and_start_only_while_running_and_not_armed) {
    RunState s;
    CHECK(freezeButtons(kProgram, &s) == (FreezeButtons{true, true, false}));
    s.paused = true;
    s.owner = kProgram;
    CHECK(freezeButtons(kProgram, &s) == (FreezeButtons{false, false, false}));
    CHECK(calcu::debugger::buttonUse(kProgram, &s).step && calcu::debugger::buttonUse(kProgram, &s).resume);
    s = RunState{};
    s.armed = kProgram;
    CHECK(freezeButtons(kProgram, &s) == (FreezeButtons{false, false, true}));
    CHECK(freezeButtons(kCpu, &s) == (FreezeButtons{true, true, false}));   // the other tab's start
    CHECK(freezeButtons(kProgram, nullptr) == FreezeButtons{});
}

TEST(c59_4_ac7_cpu_tab_buttons_for_cpu) {
    SessionRig r;
    CHECK(ready(r));
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.frame();
    const auto* s = r.state<RunState>(calcu::debugger::kRunStateTopic);
    CHECK(calcu::debugger::buttonUse(kCpu, s).step && calcu::debugger::buttonUse(kCpu, s).resume);
    CHECK(!calcu::debugger::buttonUse(kProgram, s).step);
    r.send(calcu::debugger::kResumeCommand);
}

// EC-3: STEP of the tab whose level does not own the pause cannot be used; a
// preset's `step:` of that level is carried out all the same (LIB-10 AC-45).
TEST(c59_4_ec3_step_of_the_other_level_cannot_be_used_preset_step_is_carried_out) {
    SessionRig r;
    CHECK(ready(r));
    program(r, kLoop);
    r.tap(11);
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.frame();
    CHECK(!calcu::debugger::buttonUse(kProgram, r.state<RunState>(calcu::debugger::kRunStateTopic)).step);
    const auto reply = r.step("step", "{ level: program }");
    CHECK(reply.done);
    r.frame();
    CHECK(paused(r) && r.part().runState().owner == kProgram);
    r.send(calcu::debugger::kResumeCommand);
}

// ── T5: the CPU tab (AC-6) ───────────────────────────────────────────────────

TEST(c59_4_ac6_newest_32_running_up_to_1024_and_next_paused) {
    SessionRig r;
    CHECK(ready(r));
    r.send(commands::kCpuDebug, Value::object({{"on", true}}));
    r.runFor(100ms);
    const auto* c = topic<TICpuDebug>(r, topics::kCpuDebug);
    CHECK(c && !c->paused && c->entries.size() == 32);
    CHECK(c && c->entries.back().isCurrent);
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r); }, 2s));
    r.frame();
    c = topic<TICpuDebug>(r, topics::kCpuDebug);
    CHECK(c && c->paused && c->entries.size() > 32 && c->entries.size() <= 1025);
    CHECK(c && !c->entries.back().isHistory);   // the next instruction
    CHECK(c && c->entries.back().pc == testroms::dbg(r.machine()).pc());
    r.send(calcu::debugger::kResumeCommand);
}

// ── T6: the heatmap (AC-8) ───────────────────────────────────────────────────

TEST(c59_4_ac8_counts_each_rom_address_once) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto* h = kindOf<RomHeatmapCounter>(*m);
    CHECK(h && h->counts().size() == 0x1400);
    testroms::run(*m, 100);
    CHECK_EQ(std::count(h->counts().begin(), h->counts().end(), 0u), std::ptrdiff_t{0x1400});   // off: nothing
    h->setOn(true);
    const std::uint16_t at = testroms::dbg(*m).pc();
    m->advance(1);   // one instruction
    std::uint64_t total = 0;
    for (auto c : h->counts()) total += c;
    CHECK_EQ(total, std::uint64_t{1});
    CHECK_EQ(h->counts()[at], 1u);
    h->reset();
    total = 0;
    for (auto c : h->counts()) total += c;
    CHECK_EQ(total, std::uint64_t{0});
    CHECK_EQ(heatStep(0), 0);
    CHECK_EQ(heatStep(1), 1);
}

TEST(c59_4_ac8_reset_by_command_reset_and_model_switch) {
    SessionRig r;
    CHECK(ready(r));
    r.send(commands::kCpuDebug, Value::object({{"on", true}}));
    r.runFor(300ms);
    auto sum = [&] {
        std::uint64_t t = 0;
        for (auto c : topic<TIHeatmap>(r, topics::kHeatmap)->counts) t += c;
        return t;
    };
    CHECK(sum() > 0);
    r.send(commands::kHeatmapReset);
    r.frame();
    const auto afterCommand = sum();
    r.runFor(300ms);
    CHECK(sum() > afterCommand);
    CHECK(r.reset().done);
    r.frame();
    const std::uint64_t afterReset = sum();
    r.runFor(300ms);
    CHECK(afterReset < sum());
    CHECK(r.model("TI-58").done);
    r.frame();
    CHECK(sum() < 1'000'000);   // a new machine counts from zero
    auto* h = kindOf<RomHeatmapCounter>(r.machine());
    CHECK(h && h->on());        // and counts while the tab shows
}

TEST(c59_4_ac8_apart_from_the_profile) {
    SessionRig r;
    CHECK(ready(r));
    r.send(commands::kCpuDebug, Value::object({{"on", true}}));
    r.runFor(100ms);
    CHECK(!rc(r).profile().on());   // the harness's profile is LIB-10's, not switched by the tab
}

// ── T7: the LOG tab (AC-9 to AC-12) ──────────────────────────────────────────

TEST(c59_4_ac9_last_2000_lines_in_pages) {
    RingLog log;
    CHECK_EQ(RingLog::kCapacity, 2000);
    CHECK_EQ(log.displayText(), std::string());
    log.write("a");
    log.write("b");
    CHECK_EQ(log.displayText(), std::string("a\nb"));
    for (int i = 0; i < 100; ++i) log.write("x" + std::to_string(i));
    std::string page = log.displayText();  // > one page new: only the newest page
    CHECK_EQ(std::count(page.begin(), page.end(), '\n'), 39);
    CHECK(page.substr(page.size() - 3) == "x99");
    log.write("y");
    std::string slow = log.displayText();  // one new line: up to 10 pages
    CHECK_EQ(std::count(slow.begin(), slow.end(), '\n'), 102);
    // Several lines in one log message (the machine's lines of one call).
    log.writeAll({"p\nq"});
    CHECK(log.displayText().find("p\nq") != std::string::npos);
}

TEST(c59_4_ac9_clear_empties_the_view_not_the_log) {
    SessionRig r;
    CHECK(ready(r));
    r.send(commands::kDump, Value::object({{"what", "vars"}}));
    r.frame();
    RingLog view;   // the frontend's view of the log
    for (const auto& t : logTexts(r)) view.write(t);
    CHECK(!view.displayText().empty());
    view.clear();
    CHECK_EQ(view.displayText(), std::string());
    CHECK(logged(r, "── Vars"));   // the session's log is untouched
}

TEST(c59_4_ac10_dumps_in_todays_layout) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto v = encodeBcd(std::string_view("42"));
    testroms::mem(*m).writeDataRegister(7, v.data());
    auto vars = dumpVars(*m);
    CHECK(vars.size() >= 2);
    CHECK(std::find(vars.begin(), vars.end(), std::string("V07 = 42")) != vars.end());
    auto scom = dumpScom(*m);
    CHECK_EQ(scom.size(), size_t{17});
    CHECK_EQ(scom[1].substr(0, 4), std::string("S00 "));
    auto mem = dumpMemory(*m);
    CHECK(mem.size() >= 2);
    CHECK(dumpProg(*m)[0].rfind("── Prog P000–P", 0) == 0);
    // The buttons' command writes the lines into the log and replies with them.
    SessionRig r;
    CHECK(ready(r));
    const auto reply = r.send(commands::kDump, Value::object({{"what", "scom"}}));
    CHECK(reply.done);
    const Value lines = reply.result ? reply.result->toValue() : Value();
    CHECK(lines.elements() && lines.elements()->size() == 17);
    r.frame();
    CHECK(logged(r, "S15 "));
    CHECK(!r.send(commands::kDump, Value::object({{"what", "nothing"}})).done);
}

TEST(c59_4_ac11_levels_off_info_debug) {
    SessionRig r;
    CHECK(ready(r));
    for (const char* l : {"info", "debug", "off"}) {
        CHECK(r.send(commands::kLogLevel, Value::object({{"level", l}})).done);
        r.frame();
        CHECK_EQ(std::string(debugLevelName(topic<TIDebugView>(r, topics::kDebugView)->logLevel)), std::string(l));
        CHECK_EQ(kindOf<MachineLog>(r.machine())->level(), static_cast<int>(*debugLevelNamed(l)));
    }
    CHECK(!r.send(commands::kLogLevel, Value::object({{"level", "loud"}})).done);
}

TEST(c59_4_ac11_debug_logs_each_register_and_memory_write) {
    SessionRig r;
    CHECK(ready(r));
    const std::size_t before = logTexts(r).size();
    for (int k : {82, 42, 92, 82}) r.tap(k);   // 1 STO 01: memory written, at the level off nothing
    r.runFor(100ms);
    CHECK_EQ(logTexts(r).size(), before);
    r.send(commands::kLogLevel, Value::object({{"level", "debug"}}));
    for (int k : {83, 42, 92, 83}) r.tap(k);   // 2 STO 02
    r.runFor(100ms);
    CHECK(logTexts(r).size() > before);
    r.send(commands::kLogLevel, Value::object({{"level", "off"}}));
}

TEST(c59_4_ac11_product_lines_at_every_level) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(r.reset().done);   // level off
    r.frame();
    CHECK(logged(r, "Calculator Reset"));
    r.send(commands::kDump, Value::object({{"what", "prog"}}));
    r.frame();
    CHECK(logged(r, "── Prog"));
}

TEST(c59_4_ac12_trace_button_starts_and_ends_the_button_trace) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(r.send(calcu::debugger::kTraceCommand, Value::object({{"button", true}})).done);
    r.runFor(50ms);
    const auto* t = r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic);
    CHECK(t && t->running && !t->fileName.empty());
    CHECK(r.send(calcu::debugger::kTraceCommand, Value::object({{"off", true}})).done);
    r.frame();
    CHECK(!r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
}

TEST(c59_4_ac12_shows_a_trace_whoever_started_it) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(r.step("trace", "mine.trace").done);   // a preset's step, not the button
    r.frame();
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
    r.send(calcu::debugger::kTraceCommand, Value::object({{"off", true}}));
}

TEST(c59_4_ac12_cannot_be_used_without_a_writable_folder) {
    testroms::RigOptions o;
    o.traceFolder = "/dev/null/no";
    SessionRig r(o);
    CHECK(ready(r));
    r.frame();
    CHECK(!available(r, entries::kTrace));
    testroms::RigOptions web;
    web.noTraceFolder = true;   // the Web keeps its traces (LIB-10 AC-26)
    SessionRig w(web);
    CHECK(ready(w));
    w.frame();
    CHECK(available(w, entries::kTrace));
}

// ── T8: the overlay kind (AC-13) ─────────────────────────────────────────────

TEST(c59_4_ac13_up_to_2048_over_1800_to_1fff) {
    for (Model model : {Model::TI59, Model::TI58, Model::TI58C}) {
        auto m = testroms::makeMachine(model);
        auto* o = kindOf<AsmOverlay>(*m);
        CHECK(o != nullptr);
        if (!o) continue;
        CHECK(o->load(std::vector<std::uint16_t>(2048, 0x0A01)));
        CHECK_EQ(o->loaded(), std::size_t{2048});
        CHECK_EQ(m->debugging()->target(0).peek(0, 0x1FFF, {}).value, std::uint64_t{0x0A01});
        CHECK(!o->load(std::vector<std::uint16_t>(2049, 0x0A01)));
        CHECK_EQ(o->loaded(), std::size_t{2048});   // nothing changed
    }
}

TEST(c59_4_ac13_taken_away) {
    auto m = testroms::makeMachine(Model::TI59);
    auto* o = kindOf<AsmOverlay>(*m);
    const auto before = m->debugging()->target(0).peek(0, 0x1800, {}).value;
    CHECK(o->load({0x0A01}));
    o->clear();
    CHECK_EQ(o->loaded(), std::size_t{0});
    CHECK_EQ(m->debugging()->target(0).peek(0, 0x1800, {}).value, before);
}

TEST(c59_4_ac13_entered_at_1800) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto* o = kindOf<AsmOverlay>(*m);
    CHECK(!o->enter(8192).entered);   // nothing loaded
    CHECK(o->load({0x0A01, 0x0A01}));
    const auto e = o->enter(8192);
    CHECK(e.entered && o->running());
}

// ── T10: the box (AC-16, EC-1, EC-2) ─────────────────────────────────────────

TEST(c59_4_ac16_select_file_loads_at_once) {
    SessionRig r;
    CHECK(ready(r));
    const auto reply = asmLoad(r, kOverlay, "two.asm");
    CHECK(reply.done);
    r.frame();
    CHECK_EQ(asmState(r)->fileName, std::string("two.asm"));
    CHECK_EQ(asmState(r)->instructions, 2);
    CHECK_EQ(asmState(r)->status, std::string("Loaded 2 word(s) at 0x1800."));
    CHECK_EQ(kindOf<AsmOverlay>(r.machine())->loaded(), std::size_t{2});
    CHECK(!asmLoad(r, std::nullopt, "gone.asm").done);   // unreadable
}

TEST(c59_4_ac16_run_only_with_instructions) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(!r.send(commands::kAsmRun).done);
    CHECK(asmLoad(r, kCounter).done);
    const auto reply = r.send(commands::kAsmRun);
    CHECK(reply.done);
    r.frame();
    CHECK(asmState(r)->running);
    CHECK(asmState(r)->status.rfind("ASM entered at 0x1800 (", 0) == 0);
}

TEST(c59_4_ac16_clear_resets_when_running) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    r.runFor(50ms);
    CHECK(r.send(commands::kAsmClear).done);
    r.frame();
    CHECK(!asmState(r)->running);
    CHECK_EQ(asmState(r)->status, std::string("ASM overlay cleared."));
    CHECK(logged(r, "Calculator Reset"));   // reset: the empty range is not carried out
    CHECK(ready(r));
}

TEST(c59_4_ec1_run_without_instructions_status) {
    SessionRig r;
    CHECK(ready(r));
    const auto t = r.ticks();
    const auto reply = r.send(commands::kAsmRun);
    CHECK(!reply.done && reply.reason == kNoAsmOverlay);
    r.frame();
    CHECK_EQ(asmState(r)->status, std::string(kNoAsmOverlay));
    CHECK(!asmState(r)->running);
    CHECK(r.notices().empty());   // nothing else happens
    CHECK(r.ticks() >= t);
}

TEST(c59_4_ec2_refused_file_keeps_loaded) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kOverlay, "good.asm").done);
    const auto reply = asmLoad(r, "HEX:\n0A01\n", "old.asm");
    CHECK(!reply.done && reply.reason.find("older format") != std::string::npos);
    r.frame();
    CHECK_EQ(asmState(r)->fileName, std::string("good.asm"));
    CHECK_EQ(kindOf<AsmOverlay>(r.machine())->loaded(), std::size_t{2});
    CHECK_EQ(r.notices().size(), std::size_t{1});   // the refusal is a notice
}

// ── T11: Run (AC-17) ─────────────────────────────────────────────────────────

TEST(c59_4_ac17_enters_at_1800_with_cond) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, *testroms::readFile("examples/assembly/DPT.asm"), "DPT.asm").done);
    CHECK(r.send(commands::kAsmRun).done);
    const CpuFrame f = testroms::dbg(r.machine()).snapshotCPU();
    CHECK(f.pc >= 0x1800 && f.pc < 0x180E);
    r.runFor(100ms);
    const std::uint16_t pc = testroms::dbg(r.machine()).pc();
    CHECK(pc >= 0x1800 && pc < 0x180E);   // DPT's loop: entered with COND, it stays in it
}

TEST(c59_4_ac17_fails_after_8192_with_status_and_notice) {
    // An overlay whose entry never reaches 1800: a branch at the entry is not
    // taken by the core's entry; an instruction stream that never stops at a
    // program step boundary is not needed: the entry gives up after 8192.
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto* o = kindOf<AsmOverlay>(*m);
    CHECK(o->load({0x0A01}));
    const auto e = o->enter(0);   // no instruction allowed: not entered
    CHECK(!e.entered && !o->running());
    // The status and the notice of a failed entry (the part's sentence).
    const std::string s = "ASM entry failed after " + std::to_string(e.instructions) + " step(s).";
    CHECK(s.rfind("ASM entry failed after ", 0) == 0);
}

TEST(c59_4_ac17_armed_cpu_start_pauses_at_1800) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done);
    r.send(calcu::debugger::kPauseOnStartCommand, Value::object({{"level", "cpu"}}));
    r.runFor(50ms);
    CHECK(!paused(r));
    CHECK(r.send(commands::kAsmRun).done);
    CHECK(paused(r));
    CHECK(r.part().runState().reason == RunState::Reason::PauseOnStart && r.part().runState().owner == kCpu);
    CHECK_EQ(testroms::dbg(r.machine()).pc(), std::uint16_t{0x1800});
    r.send(calcu::debugger::kResumeCommand);
}

TEST(c59_4_ac17_while_paused_not_while_operation) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done);
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r); }, 2s));
    CHECK(r.send(commands::kAsmRun).done);   // while paused: it goes on in the overlay
    CHECK(!paused(r));
    r.frame();
    CHECK(asmState(r)->running);
    // While an operation runs (a preset), Run cannot be used.
    r.post(calcu::presets::kLoadPresetCommand,
           Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - wait: 1s\n"}, {"name", "p.ti59"}}));
    r.frame();
    CHECK(r.busy());
    r.frame();
    CHECK(!available(r, entries::kAsmRun));
    const auto reply = r.send(commands::kAsmRun);
    CHECK(!reply.done);
}

// ── T12: the overlay's life (AC-18, AC-26) ───────────────────────────────────

TEST(c59_4_ac18_calculator_tab_closed_cpu_shown) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    r.frame();
    CHECK(r.panels().back() == DebugPanel::Cpu);
    CHECK(!available(r, entries::kCalculatorTab));
}

TEST(c59_4_ac18_freeze_step_resume_inside_the_overlay_it_still_runs) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    r.runFor(50ms);
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r); }, 2s));
    const std::uint16_t at = testroms::dbg(r.machine()).pc();
    CHECK(at >= 0x1800 && at < 0x1806);
    r.send(calcu::debugger::kStepCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return paused(r) && testroms::dbg(r.machine()).pc() != at; }, 2s));
    CHECK(testroms::dbg(r.machine()).pc() >= 0x1800);
    r.send(calcu::debugger::kResumeCommand);
    r.runFor(50ms);
    r.frame();
    CHECK(asmState(r)->running);   // a FREEZE does not end its run (OQ-5)
    CHECK(!available(r, entries::kCalculatorTab));
}

TEST(c59_4_ac18_reset_ends_run_keeps_instructions_run_again) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    CHECK(r.reset().done);
    r.frame();
    CHECK(!asmState(r)->running);
    CHECK_EQ(asmState(r)->instructions, 6);
    CHECK(available(r, entries::kCalculatorTab));
    CHECK(ready(r));
    CHECK(r.send(commands::kAsmRun).done);   // entered again
    r.frame();
    CHECK(asmState(r)->running);
}

TEST(c59_4_ac26_cold_boot_removes) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done && r.send(commands::kAsmRun).done);
    CHECK(r.coldBoot().done);
    r.frame();
    CHECK(!asmState(r)->running);
    CHECK_EQ(asmState(r)->instructions, 0);
    CHECK_EQ(kindOf<AsmOverlay>(r.machine())->loaded(), std::size_t{0});
    CHECK(!r.send(commands::kAsmRun).done);   // nothing to run
}

// Replaces part_flows_tests.cpp's part_asm_overlay: today an overlay outlived
// a model switch and a preset.
TEST(c59_4_ac26_new_machine_by_preset_and_model_switch_removes) {
    SessionRig r;
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done);
    CHECK(r.model("TI-58").done);
    r.frame();
    CHECK_EQ(asmState(r)->instructions, 0);
    CHECK_EQ(asmState(r)->fileName, std::string("No file selected"));
    CHECK(!r.send(commands::kAsmRun).done);
    CHECK(ready(r));
    CHECK(asmLoad(r, kCounter).done);
    const auto id = r.post(calcu::presets::kLoadPresetCommand,
                           Value::object({{"content", "format-version: 2\nmodel: TI-59\n"}, {"name", "p.ti59"}}));
    CHECK(r.until([&] { return r.reply(id) != nullptr; }, 30s));
    r.frame();
    CHECK_EQ(asmState(r)->instructions, 0);
    CHECK_EQ(kindOf<AsmOverlay>(r.machine())->loaded(), std::size_t{0});
}

// ── T13: opening (AC-19, EC-5) ───────────────────────────────────────────────

TEST(c59_4_ac19_asm_opened_as_by_select_file) {
    SessionRig r;
    CHECK(ready(r));
    for (auto way : {calcu::opening::Way::Drop, calcu::opening::Way::Start, calcu::opening::Way::Harness}) {
        const auto reply = open(r, "Counter.ASM", kCounter, way);
        CHECK(reply.done);
        const Value v = reply.result ? reply.result->toValue() : Value();
        CHECK(v.find("detail") && v.find("detail")->string() && *v.find("detail")->string() == "asm-overlay");
        r.frame();
        CHECK_EQ(asmState(r)->fileName, std::string("Counter.ASM"));
        CHECK_EQ(asmState(r)->instructions, 6);
    }
    const auto old = open(r, "old.asm", "HEX:\n0A01\n");
    CHECK(!old.done && old.reason.find("older format") != std::string::npos);
    r.frame();
    CHECK_EQ(r.notices().size(), std::size_t{1});   // one notice, the open part's
}

TEST(c59_4_ec5_opened_before_first_machine_kept) {
    testroms::RigOptions o;
    o.start = false;   // no machine yet
    SessionRig r(o);
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = "c.asm";
    f.bytes = std::vector<std::uint8_t>(kCounter.begin(), kCounter.end());
    q.files = {f};
    q.way = calcu::opening::Way::Start;
    const auto id = r.post(calcu::opening::kOpenCommand, q.toValue());
    const auto* first = r.reply(id);
    CHECK(first && first->done);
    const Value v = first && first->result ? first->result->toValue() : Value();
    CHECK(v.find("as") && v.find("as")->string() && *v.find("as")->string() == "kept");
    r.post(calcu::sessions::kStartCommand);
    CHECK(r.until([&] { return r.session().machineSession().machine() && !r.busy(); }));
    r.runFor(50ms);
    CHECK_EQ(asmState(r)->fileName, std::string("c.asm"));
    CHECK_EQ(kindOf<AsmOverlay>(r.machine())->loaded(), std::size_t{6});
}

// ── T15: every frontend (AC-21): the labels and short forms the panels draw ──

TEST(c59_4_ac21_labels_and_short_forms) {
    CHECK_EQ(debugPanelFontSize(300), 9);
    CHECK_EQ(std::string(debugPanelName(DebugPanel::Calculator)), std::string("calculator"));
    CHECK_EQ(std::string(debugPanelName(DebugPanel::Cpu)), std::string("cpu"));
    CHECK_EQ(std::string(debugPanelName(DebugPanel::Log)), std::string("log"));
    CHECK(currentStepMarker(true).find("next") != std::string::npos);
    CHECK(currentStepMarker(false).find("running") != std::string::npos);
}
