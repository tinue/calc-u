// LIB-3's Calc-U-59 column (design §6, P59-2 to P59-5): Calc-U-59 on the
// library's app session, driven as a frontend drives it (AppSessionRig.hpp).

#include "TestHarness.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "control/ControlState.hpp"
#include "media/MediaTopics.hpp"
#include "SessionRig.hpp"
#include "../app/Bcd.hpp"
#include "../app/TIPresetKeys.hpp"
#include <fstream>

using namespace cu59;
using namespace std::chrono_literals;
using namespace calcu::sessions;
using testroms::SessionRig;

namespace {

bool hasNotice(SessionRig& r, const std::string& sentence) {
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == kNoticesTopic)
            for (const auto& p : s->appended)
                if (const auto* n = payloadAs<Notice>(p); n && n->sentence == sentence) return true;
    return false;
}

bool hasLog(SessionRig& r, const std::string& line) {
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == kLogTopic)
            for (const auto& p : s->appended)
                if (const auto* n = payloadAs<LogLine>(p); n && n->text == line) return true;
    return false;
}

std::uint64_t serialOf(SessionRig& r, const TopicName& t) {
    const PayloadPtr* p = r.fe.mirror.state(t);
    if (!p || !*p) return 0;
    // Every interim payload is a TISnapshot: the serial says which set it was.
    if (const auto* x = payloadAs<TILedState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TICueCardState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIPrinterState>(*p)) return x->serial;
    // The debug panel's topics (C59-4 AC-25): a serial, or a count of sets.
    if (const auto* x = payloadAs<TICpuDebug>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIHeatmap>(*p)) return x->serial;
    return reinterpret_cast<std::uintptr_t>(p->get());
}

// The control state's model and what is plugged into `place` (LIB-4).
Value controlValue(SessionRig& r) {
    const auto* c = r.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return c ? c->toValue() : Value();
}
std::string controlModel(SessionRig& r) {
    const Value v = controlValue(r);
    const Value* m = v.find("model");
    return m && m->string() ? *m->string() : std::string();
}
std::string pluggedIn(SessionRig& r, const std::string& place) {
    const Value v = controlValue(r);
    const Value* places = v.find("places");
    if (!places || !places->elements()) return {};
    for (const auto& p : *places->elements()) {
        const Value* at = p.find("place");
        if (!at || !at->string() || *at->string() != place) continue;
        if (const Value* fs = p.find("fitting"); fs && fs->elements())
            for (const auto& f : *fs->elements())
                if (const Value* pl = f.find("plugged"); pl && pl->boolean() && *pl->boolean())
                    if (const Value* id = f.find("id"); id && id->string()) return *id->string();
    }
    return {};
}

} // namespace

// AC-29: what the fourteen change flags announced is in the mirror; each
// flag's action moves its topic on.
TEST(lib3_ac29_mirror_holds_every_change_flag) {
    SessionRig r;
    for (const auto* t : {&calcu::control::kControlStateTopic, &topics::kLed, &topics::kCueCard, &topics::kPrinter, &calcu::media::kMediaTopic,
                          &topics::kCalculatorDebug, &topics::kCpuDebug,
                          &topics::kHeatmap, &topics::kAsmOverlay, &topics::kDebugView})
        CHECK(r.fe.mirror.state(*t) != nullptr);
    // The model and the module: LIB-4's control state (C59-8: c59.model went).
    CHECK_EQ(controlModel(r), std::string("TI-59"));
    CHECK_EQ(pluggedIn(r, "slot-1"), std::string("ML"));
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));

    auto moves = [&](const TopicName& t, auto action) {
        const auto before = serialOf(r, t);
        action();
        r.runFor(100ms);
        return serialOf(r, t) != before;
    };
    // ModelChanged
    CHECK(moves(calcu::control::kControlStateTopic, [&] { r.model("TI-58"); }));
    CHECK_EQ(controlModel(r), std::string("TI-58"));
    // DisplayChanged
    CHECK(moves(topics::kLed, [&] { r.tap(83); }));   // 5
    // CueCardChanged, CardChanged
    CHECK(moves(topics::kCueCard, [&] { r.model("TI-59"); }));
    {   // CardChanged: the library's `media` topic (M-55, LIB-7 59)
        const PayloadPtr before = *r.fe.mirror.state(calcu::media::kMediaTopic);
        r.send(calcu::media::kStartNewCommand,
               Value::object({{"place", "card-reader"}, {"template", "implicit:Fresh card"}, {"name", "a"}}));
        r.runFor(100ms);
        CHECK(*r.fe.mirror.state(calcu::media::kMediaTopic) != before);
        const auto* media = r.state<calcu::media::MediaState>(calcu::media::kMediaTopic);
        CHECK(media && media->place("card-reader") && media->place("card-reader")->holds);
        CHECK(moves(topics::kCueCard, [&] {}) || r.part().cardOnShow());
    }
    // PrinterChanged (TRACE through the paper panel's set-control, LIB-8 P-E2)
    CHECK(moves(topics::kPrinter, [&] {
        r.send(commands::kSetControl, Value::object({{"peripheral", "PC-100C"}, {"control", "TRACE"}, {"on", true}}));
    }));
    CHECK(r.state<TIPrinterState>(topics::kPrinter)->trace);
    // RunStateChanged: the library's `speed` (c59.full-speed and the interim
    // topic c59.run-state went in M-59); the part follows the commanded speed.
    r.send(calcu::sessions::kSpeedCommand, Value::object({{"full", true}}));
    CHECK(r.part().fullSpeed());
    r.send(calcu::sessions::kSpeedCommand, Value::object({{"full", false}}));
    CHECK(!r.part().fullSpeed());
    // What the debug panel shows: its product topics since M-56 (C59-4 AC-25).
    CHECK(moves(topics::kCpuDebug, [&] { r.send(commands::kCpuDebug, Value::object({{"on", true}})); }));
    CHECK(moves(topics::kHeatmap, [&] { r.send(commands::kHeatmapReset); }));
    CHECK(moves(topics::kAsmOverlay, [&] { r.send(commands::kAsmRun); }));   // "No ASM overlay loaded."
    CHECK(moves(topics::kDebugView, [&] { r.send(commands::kLogLevel, Value::object({{"level", "info"}})); }));
    // The trace and the pause are the library's topics since M-53 (LIB-10).
    r.send(calcu::debugger::kTraceCommand, Value::object({{"button", true}}));
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
    r.send(calcu::debugger::kTraceCommand, Value::object({{"off", true}}));
    r.send(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}}));
    CHECK(r.until([&] { return r.part().isFrozen(); }));
    CHECK(r.state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic)->paused);
}

// AC-30: an error is a notice, a diagnostic a line of the log.
TEST(lib3_ac30_error_is_notice_diagnostic_is_log) {
    SessionRig r;
    r.part().error("Preset load error", "Cannot read file.");
    r.frame();
    CHECK(hasNotice(r, "Preset load error: Cannot read file."));
    r.part().diagnostic("a developer line");
    r.frame();
    CHECK(hasLog(r, "a developer line"));
}

// AC-30: the card and constant-memory files are the session's, in the save
// folder, without a frontend.
TEST(lib3_ac30_card_and_constant_memory_files_without_frontend) {
    SessionRig r(Model::TI58C);   // built from the file (C59-3 AC-7): it writes it at the first change
    r.runFor(1s);
    const auto mem = r.folder.path() / TIFileAccess::kConstantMemoryFile;
    CHECK_EQ(std::string(TIFileAccess::kConstantMemoryFile), std::string("ti58c.memory.yaml"));
    CHECK(testroms::fs::exists(mem));
    CHECK_EQ(r.part().files().savePath("x"), (r.folder.path() / "x").string());
    // The cards: the library's media part on the same save folder (M-55).
    SessionRig c;
    const std::string text = writeMagneticCardFile(TIMagneticCardData{}, "x", "", false, std::nullopt, "");
    const auto imported = c.part().mediaPart().importBytes(c.session().context(), "x.magnetcard.yaml", text, {});
    CHECK(imported.ok);
    CHECK(testroms::fs::exists(c.folder.path() / "x.magnetcard.yaml"));
}

// AC-30: the module ID is no longer saved (row 59-9).
TEST(lib3_ac30_module_id_not_saved) {
    SessionRig r;
    CHECK(r.attach("slot-1", "LE").done);
    CHECK_EQ(pluggedIn(r, "slot-1"), std::string("LE"));
    CHECK(testroms::fs::is_empty(r.folder.path()));   // nothing written for it
}

// AC-30: the step debug-panel: is a product message.
TEST(lib3_ac30_show_debug_panel_is_product_message) {
    SessionRig r;
    r.post(calcu::presets::kLoadPresetCommand,
           Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - debug-panel: cpu\n"}, {"name", "p.ti59"}}));
    CHECK(r.until([&] { return !r.panels().empty(); }));
    CHECK(!r.panels().empty() && r.panels()[0] == DebugPanel::Cpu);   // the product topic debug-view (C59-4 AC-2)
}

// AC-32: a preset runs as an operation: busy says so, a second is refused.
TEST(lib3_ac32_preset_load_is_busy_and_refuses_second) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(calcu::presets::kLoadPresetCommand,
                             Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - key: 1 + 1 =\n  - let-it-finish:\n"},
                                            {"name", "a.ti59"}}));
    CHECK(r.busy());
    CHECK(r.reply(id) == nullptr);
    const ReplyMessage second = r.send(calcu::presets::kLoadPresetCommand,
                                       Value::object({{"content", "format-version: 2\nmodel: TI-59\n"}, {"name", "b.ti59"}}));
    CHECK(!second.done);
    CHECK_EQ(second.reason, std::string("Another operation is running."));
    CHECK(r.until([&] { return r.reply(id) != nullptr; }));
    CHECK(r.reply(id)->done);
    CHECK(!r.busy());
    r.runFor(300ms);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("2"));   // 1 + 1 =
}

// AC-32: connecting or disconnecting the printer (a power cycle) is an
// operation: LIB-18's attach and detach (row 59-39).
TEST(lib3_ac32_printer_attach_is_operation) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(calcu::startup::kAttachCommand, Value::object({{"place", "printer"}, {"peripheral", "PC-100C"}}));
    CHECK(r.busy());
    CHECK(r.until([&] { return r.reply(id) != nullptr; }));
    CHECK(r.reply(id)->done && !r.busy());
    r.frame();
    CHECK(r.state<TIPrinterState>(topics::kPrinter)->connected);
    CHECK(r.machine().isKeyReady());
}

// AC-32: the display and the paper keep updating while a preset runs.
TEST(lib3_ac32_updates_during_preset) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(calcu::presets::kLoadPresetCommand,
                             Value::object({{"content", "format-version: 2\nmodel: TI-59\nprinter: PC-100C\nkeys:\n"
                                                        "  - regular-speed:\n  - key: 1 2nd Prt\n  - key: 2 2nd Prt\n  - let-it-finish:\n"},
                                            {"name", "p.ti59"}}));
    int ledSetsWhileBusy = 0, paperWhileBusy = 0;
    std::uint64_t led = serialOf(r, topics::kLed);
    // The paper: the library's stream `paper PC-100C` (LIB-8, M-53).
    auto paperMessages = [&] {
        int n = 0;
        for (const auto& m : r.fe.log)
            if (const auto* x = std::get_if<StreamMessage>(&m); x && x->topic == calcu::paper::paperTopic("PC-100C")) ++n;
        return n;
    };
    int paper = paperMessages();
    while (r.reply(id) == nullptr && r.now < calcu::pacing::WallTime(std::chrono::hours(2))) {
        r.frame();
        if (!r.busy()) continue;
        if (serialOf(r, topics::kLed) != led) ++ledSetsWhileBusy, led = serialOf(r, topics::kLed);
        if (paperMessages() != paper) ++paperWhileBusy, paper = paperMessages();
    }
    CHECK(ledSetsWhileBusy > 0);
    CHECK(paperWhileBusy > 0);
}

// AC-33: no machine has sound, and none is asked for even of a frontend
// that offers an audio sink.
TEST(lib3_ac33_no_sound_messages_and_no_sink_stated) {
    HostServices services;
    services.offered = {HostService::AudioSink};
    SessionRig r(Model::TI59, services);
    CHECK(!r.machine().descriptor().hasSound);
    r.tap(83);
    r.runFor(1s);
    for (const auto& m : r.fe.log) {
        CHECK(!std::holds_alternative<SoundStart>(m));
        CHECK(!std::holds_alternative<SoundSamples>(m));
        CHECK(!std::holds_alternative<ServiceAsk>(m));
    }
}

// C59-5 AC-8, C59-3 AC-6: suspend writes the TI-58C's memory at once and runs
// nothing; continue runs again.
TEST(c59_5_ac8_suspend_persists_and_stops_continue_runs) {
    SessionRig r(Model::TI58C);
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.runFor(1s);   // the first change written
    const auto mem = r.folder.path() / TIFileAccess::kConstantMemoryFile;
    testroms::fs::remove(mem);
    cu59::programMemoryOf(r.machine())->writeRegister(1, *cu59::parseDecimal("42"));   // a change, no time passes
    r.send(kSuspendCommand);
    CHECK(testroms::fs::exists(mem));
    const auto t = r.ticks();
    r.runFor(500ms);
    CHECK_EQ(r.ticks(), t);
    r.send(kContinueCommand);
    r.runFor(500ms);
    CHECK(r.ticks() > t);
}
