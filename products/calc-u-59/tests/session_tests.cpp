// LIB-3's Calc-U-59 column (design §6, P59-2 to P59-5): Calc-U-59 on the
// library's app session, driven as a frontend drives it (AppSessionRig.hpp).

#include "TestHarness.hpp"
#include "AppSessionRig.hpp"
#include <fstream>

using namespace cu59;
using namespace std::chrono_literals;
using namespace calcu::sessions;
using testroms::SessionRig;

namespace {

bool hasNotice(SessionRig& r, const std::string& sentence) {
    for (const auto& m : r.messages)
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == kNoticesTopic)
            for (const auto& p : s->appended)
                if (const auto* n = payloadAs<Notice>(p); n && n->sentence == sentence) return true;
    return false;
}

bool hasLog(SessionRig& r, const std::string& line) {
    for (const auto& m : r.messages)
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == kLogTopic)
            for (const auto& p : s->appended)
                if (const auto* n = payloadAs<LogLine>(p); n && n->text == line) return true;
    return false;
}

std::uint64_t serialOf(SessionRig& r, const TopicName& t) {
    const PayloadPtr* p = r.mirror.state(t);
    if (!p || !*p) return 0;
    // Every interim payload is a TISnapshot: the serial says which set it was.
    if (const auto* x = payloadAs<TIModelState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TILedState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TICueCardState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIPrinterState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TICardReaderState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIRunState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TITraceState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIDebugLiveState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIDebugCpuState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIDebugHeatmapState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIDebugLogState>(*p)) return x->serial;
    if (const auto* x = payloadAs<TIDebugAsmState>(*p)) return x->serial;
    return 0;
}

} // namespace

// AC-29: what the fourteen change flags announced is in the mirror; each
// flag's action moves its topic on.
TEST(lib3_ac29_mirror_holds_every_change_flag) {
    SessionRig r;
    for (const auto* t : {&topics::kModel, &topics::kLed, &topics::kCueCard, &topics::kPrinter, &topics::kCardReader,
                          &topics::kRunState, &topics::kTrace, &topics::kDebugLive, &topics::kDebugCpu,
                          &topics::kDebugHeatmap, &topics::kDebugAsm})
        CHECK(r.mirror.state(*t) != nullptr);
    CHECK(r.state<TIModelState>(topics::kModel)->model == Model::TI59);
    CHECK_EQ(r.state<TIModelState>(topics::kModel)->moduleID, std::string("ML"));
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));

    auto moves = [&](const TopicName& t, auto action) {
        const auto before = serialOf(r, t);
        action();
        r.runFor(100ms);
        return serialOf(r, t) != before;
    };
    // ModelChanged
    CHECK(moves(topics::kModel, [&] { r.send(commands::kStartMachine, Value::object({{"model", 1}})); }));
    CHECK(r.state<TIModelState>(topics::kModel)->model == Model::TI58);
    // DisplayChanged
    CHECK(moves(topics::kLed, [&] { r.tap(83); }));   // 5
    // CueCardChanged, CardChanged
    CHECK(moves(topics::kCueCard, [&] { r.send(commands::kStartMachine, Value::object({{"model", 0}})); }));
    CHECK(moves(topics::kCardReader, [&] {
        r.send(commands::kFitBlankCard, Value::object({{"path", (r.folder.path / "a.u59").string()}, {"name", "a.u59"}}));
    }));
    CHECK(r.state<TICardReaderState>(topics::kCardReader)->fitted);
    // PrinterChanged, PaperCut
    CHECK(moves(topics::kPrinter, [&] { r.send(commands::kTogglePrinterTrace); }));
    CHECK(r.state<TIPrinterState>(topics::kPrinter)->trace);
    CHECK(moves(topics::kPrinter, [&] { r.send(commands::kCutPaper); }));
    // RunStateChanged
    CHECK(moves(topics::kRunState, [&] { r.send(commands::kFullSpeed, Value::object({{"on", true}})); }));
    CHECK(r.state<TIRunState>(topics::kRunState)->fullSpeed);
    r.send(commands::kFullSpeed, Value::object({{"on", false}}));
    // LiveChanged, ListingChanged
    CHECK(moves(topics::kDebugLive, [&] { r.send(commands::kLiveDebug, Value::object({{"on", true}})); }));
    // CpuChanged
    CHECK(moves(topics::kDebugCpu, [&] { r.send(commands::kCpuDebug, Value::object({{"on", true}})); }));
    // HeatmapChanged
    CHECK(moves(topics::kDebugHeatmap, [&] { r.send(commands::kClearHeatmap); }));
    // LogChanged
    CHECK(moves(topics::kDebugLog, [&] {
        r.send(commands::kDumpVars);
        r.runFor(1100ms);   // the LOG pane at 1 Hz
    }));
    // AsmChanged
    CHECK(moves(topics::kDebugAsm, [&] { r.send(commands::kClearAsm); }));
    // TraceChanged
    CHECK(moves(topics::kTrace, [&] { r.send(commands::kManualTrace, Value::object({{"on", true}})); }));
    CHECK(r.state<TITraceState>(topics::kTrace)->manualTrace);
    r.send(commands::kManualTrace, Value::object({{"on", false}}));
    // The freeze is run state too.
    CHECK(moves(topics::kRunState, [&] { r.send(commands::kFreeze, Value::object({{"owner", 0}})); }));
    CHECK(r.state<TIRunState>(topics::kRunState)->debug.frozen);
}

// AC-30: an error is a notice, a diagnostic a line of the log.
TEST(lib3_ac30_error_is_notice_diagnostic_is_log) {
    SessionRig r;
    r.send(commands::kLoadState, Value::object({}));   // no bytes: the file could not be read
    CHECK(hasNotice(r, "State file load error: Cannot read file."));
    const NoticeParts n = splitNotice("State file load error: Cannot read file.");
    CHECK_EQ(n.title, std::string("State file load error"));
    CHECK_EQ(n.message, std::string("Cannot read file."));
    r.part().diagnostic("a developer line");
    r.frame();
    CHECK(hasLog(r, "a developer line"));
}

// AC-30: the card and constant-memory files are the session's, in the save
// folder, without a frontend.
TEST(lib3_ac30_card_and_constant_memory_files_without_frontend) {
    SessionRig r(Model::TI58C);
    r.send(commands::kLoadState, Value::object({{"bytes", r.bytes("REGISTERS:\n03 = 7\n")}}));
    r.until([&] { return !r.busy(); });
    const auto mem = r.folder.path / TIFileAccess::kConstantMemoryFile;
    CHECK(testroms::fs::exists(mem));
    TIFileAccess& p = r.part().files();
    CHECK_EQ(p.cardFilePath("x.u59"), (r.folder.path / "x.u59").string());
    CHECK(!p.readCard(p.cardFilePath("x.u59")).raw);
    CHECK(p.writeCard(p.cardFilePath("x.u59"), "CARD"));
    CHECK(p.readCard(p.cardFilePath("x.u59")).raw == std::optional<std::vector<uint8_t>>(std::vector<uint8_t>{'C', 'A', 'R', 'D'}));
}

// AC-30: the module ID is no longer saved (row 59-9).
TEST(lib3_ac30_module_id_not_saved) {
    SessionRig r;
    r.send(commands::kChangeModule, Value::object({{"id", "LE"}}));
    r.until([&] { return !r.busy(); });
    CHECK_EQ(r.state<TIModelState>(topics::kModel)->moduleID, std::string("LE"));
    CHECK(testroms::fs::is_empty(r.folder.path));   // nothing written for it
}

// AC-30: KEYSTROKES DebugPanel: is a product message.
TEST(lib3_ac30_show_debug_panel_is_product_message) {
    SessionRig r;
    r.send(commands::kLoadState, Value::object({{"bytes", r.bytes("KEYSTROKES:\nDebugPanel: CPU\n")}}));
    std::vector<PayloadPtr> got;
    CHECK(r.until([&] {
        for (auto& p : r.mirror.takeAppended(topics::kShowDebugPanel)) got.push_back(p);
        return !got.empty();
    }));
    CHECK(!got.empty() && payloadAs<TIShowDebugPanel>(got[0])->panel == DebugPanel::Cpu);
}

// AC-32: a preset runs as an operation: busy says so, a second is refused.
TEST(lib3_ac32_preset_load_is_busy_and_refuses_second) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(commands::kLoadState, Value::object({{"bytes", r.bytes("KEYSTROKES:\n82 85 82 95\n")}}));
    CHECK(r.busy());
    CHECK(r.reply(id) == nullptr);
    const ReplyMessage second = r.send(commands::kLoadState, Value::object({{"bytes", r.bytes("REGISTERS:\n01 = 1\n")}}));
    CHECK(!second.done);
    CHECK_EQ(second.reason, std::string("Another operation is running."));
    CHECK(r.until([&] { return r.reply(id) != nullptr; }));
    CHECK(r.reply(id)->done);
    CHECK(!r.busy());
    r.runFor(300ms);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("2"));   // 1 + 1 =
}

// AC-32: connecting or disconnecting the printer (a power cycle) is an operation.
TEST(lib3_ac32_printer_toggle_is_operation) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(commands::kTogglePrinterConnection);
    CHECK(r.busy());
    CHECK(!r.state<TIPrinterState>(topics::kPrinter)->connected);
    CHECK(r.until([&] { return r.reply(id) != nullptr; }));
    CHECK(r.reply(id)->done && !r.busy());
    CHECK(r.machine().isKeyReady());
}

// AC-32: the display and the paper keep updating while a preset runs.
TEST(lib3_ac32_updates_during_preset) {
    SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const CallId id = r.post(commands::kLoadState, Value::object({{"bytes", r.bytes("KEYSTROKES:\n82 21 94\n83 21 94\n")}}));
    int ledSetsWhileBusy = 0, paperWhileBusy = 0;
    std::uint64_t led = serialOf(r, topics::kLed), paper = serialOf(r, topics::kPrinter);
    while (r.reply(id) == nullptr && r.now < calcu::pacing::WallTime(std::chrono::hours(2))) {
        r.frame();
        if (!r.busy()) continue;
        if (serialOf(r, topics::kLed) != led) ++ledSetsWhileBusy, led = serialOf(r, topics::kLed);
        if (serialOf(r, topics::kPrinter) != paper) ++paperWhileBusy, paper = serialOf(r, topics::kPrinter);
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
    for (const auto& m : r.messages) {
        CHECK(!std::holds_alternative<SoundStart>(m));
        CHECK(!std::holds_alternative<SoundSamples>(m));
        CHECK(!std::holds_alternative<ServiceAsk>(m));
    }
}

// C59-5 AC-8: suspend writes the TI-58C's memory and runs nothing; continue runs again.
TEST(c59_5_ac8_suspend_persists_and_stops_continue_runs) {
    SessionRig r(Model::TI58C);
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    const auto mem = r.folder.path / TIFileAccess::kConstantMemoryFile;
    testroms::fs::remove(mem);
    r.part().emulator()->access().memory();   // the machine is there
    r.tap(83);
    r.send(kSuspendCommand);
    CHECK(testroms::fs::exists(mem));
    const auto t = r.ticks();
    r.runFor(500ms);
    CHECK_EQ(r.ticks(), t);
    r.send(kContinueCommand);
    r.runFor(500ms);
    CHECK(r.ticks() > t);
}
