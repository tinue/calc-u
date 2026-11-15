// LIB-3 Calc-U-1600 column (M-31; LIB-3 design §6, P16-2, P16-4, P16-5):
// operations in parts with live updates (AC-35), the buzzer only at regular
// speed (AC-37), and today's callbacks as messages (AC-38). AC-34 and the
// Qt6 half of AC-36 are the CTest CalcU1600NoOwnSession; the Web half of
// AC-36 is frontends/web/tests/suspend.spec.js. On a scripted clock; SKIP
// without ROMs.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "../app/tests/SessionFixture.hpp"
#include "Machine/SharpRoms.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = shared::commands;
namespace t = shared::topics;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;
using shared::Model;

Value toggle(const char* p) { return Value::object({{"peripheral", p}}); }

// What arrived for `topic` while an operation ran, per host call.
struct CallRecord {
    bool busy = false;
    int displayMessages = 0;
    int controlMessages = 0;
    double wallMs = 0;
    std::uint64_t plotRevision = 0;
};

int countState(const std::vector<sessions::Message>& ms, std::size_t from, const sessions::TopicName& topic) {
    int n = 0;
    for (std::size_t i = from; i < ms.size(); ++i)
        if (const auto* s = std::get_if<sessions::StateMessage>(&ms[i]); s && s->topic == topic) ++n;
    return n;
}

// LIB-3 AC-35: a preset (the Lissajous example: it plots and waits for the
// program to end) runs in parts:
// busy over several calls, the display and the paper updated during it,
// every call within the allowance of a call at full speed.
TEST(lib3_ac35_preset_in_parts_busy_display_and_paper_update) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    f.readCost = std::chrono::microseconds(500);   // the host's work inside a call
    f.messages.clear();
    const sessions::CallId id = f.send(c::kLoadPreset, Value::object({{"path", "examples:plotter/lissajou-1500.pc1500a"}}));
    // What the command's own flush sent (the start of the operation, before
    // its first call): the armed machine.
    const int controlsAtSend = countState(f.messages, 0, t::kControlState);
    std::vector<CallRecord> calls;
    std::optional<sessions::ReplyMessage> reply;
    while (!(reply = f.takeReply(id)) && calls.size() < 20000) {
        const std::size_t from = f.messages.size();
        const auto before = f.now + std::chrono::milliseconds(17);
        f.call(17);
        CallRecord r;
        r.busy = f.busy();
        r.displayMessages = countState(f.messages, from, sessions::kDisplayTopic);
        r.controlMessages = countState(f.messages, from, t::kControlState);
        r.wallMs = std::chrono::duration<double, std::milli>(f.now - before).count();
        r.plotRevision = f.part().plotRevision(shared::PaperKind::CE150);
        calls.push_back(r);
    }
    CHECK(reply && reply->done);
    if (reply && !reply->done) std::fprintf(stderr, "  preset: %s\n", reply->reason.c_str());
    const auto busyCalls = std::count_if(calls.begin(), calls.end(), [](const CallRecord& r) { return r.busy; });
    int displays = 0, controls = controlsAtSend;
    double worstMs = 0;
    std::uint64_t firstRevision = 0, lastRevisionWhileBusy = 0;
    for (const CallRecord& r : calls) {
        if (!r.busy) continue;
        displays += r.displayMessages;
        controls += r.controlMessages;
        worstMs = std::max(worstMs, r.wallMs);
        if (!firstRevision) firstRevision = r.plotRevision + 1;
        lastRevisionWhileBusy = r.plotRevision;
    }
    std::printf("  AC-35: %zu calls, %ld busy, %d display and %d control-state messages while busy, worst call %.1f ms\n",
                calls.size(), static_cast<long>(busyCalls), displays, controls, worstMs);
    CHECK(busyCalls > 10);
    CHECK(displays > 5);       // the display kept updating
    CHECK(controls >= 1);      // the armed machine (the CE-150 attached) was shown during it
    CHECK(lastRevisionWhileBusy + 1 > firstRevision);   // the paper grew while it ran
    CHECK(worstMs <= 14.0 + 3.0);   // LIB-17 AC-5's 14 ms, plus the slice that crosses it
    CHECK(!f.busy());
}

// LIB-3 AC-37: the buzzer through the audio sink at regular speed, nothing
// while the display is held for full speed or an operation runs.
TEST(lib3_ac37_buzzer_heard_at_regular_speed_only) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.services.offered.insert(sessions::HostService::AudioSink);
    CHECK(f.start("PC-1600"));
    auto samplesFrom = [&](std::size_t from) {
        std::size_t n = 0;
        for (std::size_t i = from; i < f.messages.size(); ++i)
            if (const auto* s = std::get_if<sessions::SoundSamples>(&f.messages[i])) n += s->mono.size();
        return n;
    };
    auto startedFrom = [&](std::size_t from) {
        return std::any_of(f.messages.begin() + static_cast<std::ptrdiff_t>(from), f.messages.end(),
                           [](const sessions::Message& m) { return std::holds_alternative<sessions::SoundStart>(m); });
    };
    CHECK(samplesFrom(0) == 0);   // the start's boot was an operation: nothing heard

    // Regular speed: heard, through a SoundStart with the machine's rate.
    std::size_t mark = f.messages.size();
    f.part().typing().typeCommand("BEEP 3");
    f.runFor(4.0);
    CHECK(startedFrom(mark));
    CHECK(samplesFrom(mark) > 0);
    for (const auto& m : f.messages)
        if (const auto* s = std::get_if<sessions::SoundStart>(&m)) CHECK(s->sampleRate == 48000);

    // Full speed (the display held): nothing.
    f.readCost = std::chrono::milliseconds(1);   // a call at full speed runs until its allowance is used
    CHECK(f.ok(sessions::kSpeedCommand, Value::object({{"full", true}})));
    f.part().typing().typeCommand("BEEP 3");
    f.call();
    mark = f.messages.size();
    for (int i = 0; i < 60; ++i) f.call();
    CHECK(samplesFrom(mark) == 0);
    CHECK(f.ok(sessions::kSpeedCommand, Value::object({{"full", false}})));
    f.readCost = {};
    f.runFor(2.0);
    f.readCost = std::chrono::milliseconds(1);

    // An operation (a reset): nothing while it runs.
    mark = f.messages.size();
    const sessions::CallId id = f.send(c::kReset, Value::object({{"all", false}}));
    std::size_t duringOp = 0;
    while (!f.takeReply(id)) {
        const std::size_t from = f.messages.size();
        f.call();
        if (f.busy()) duringOp += samplesFrom(from);
    }
    CHECK(duringOp == 0);
}

// The debugger's hooks as SharpDebuggerGate sees them (no message).
struct CountingDebugger : shared::SharpDebugger {
    int aboutToChange = 0, replaced = 0, slices = 0;
    void machineAboutToChange() override { ++aboutToChange; }
    void machineReplaced() override { ++replaced; }
    bool runDebuggerSlice(std::uint64_t) override {
        ++slices;
        return false;   // the machine runs the slice as without a debugger
    }
};

// LIB-3 AC-38: everything 0.8.1's callbacks announced
// (Shared/MachineSession.hpp:69-93, Shared/AppSession.hpp:62-84) arrives
// as messages a test frontend keeps (the interim names of design §5).
TEST(lib3_ac38_mirror_holds_every_callback) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");

    // romMissing -> LIB-12's roms-missing topic; the start is refused.
    {
        SessionFixture f("calcu-session-ac38-missing");
        fs::remove(fs::path(f.paths->bundledResourcesDir()) / "PC1600-P2-B6-new.bin");
        f.open();
        const sessions::ReplyMessage r = f.run(c::kStart);
        CHECK(!r.done);
        const auto* m = f.state<shared::RomsMissingPayload>(t::kRomsMissing);
        CHECK(m && !m->missing.files.empty());
        CHECK(r.reason == calcu::roms::describe(m->missing));
        CHECK(!f.part().hasLiveMachine());
    }

    SessionFixture f;
    f.readCost = std::chrono::microseconds(500);
    CountingDebugger debugger;
    f.open();
    f.part().setDebugger(&debugger);
    std::size_t mark = 0;
    CHECK(f.start("PC-1500A"));

    // busyChanged -> busy topic: on, then off.
    {
        std::vector<bool> runs;
        for (const auto& m : f.messages)
            if (const auto* s = std::get_if<sessions::StateMessage>(&m))
                if (const auto* b = sessions::payloadAs<sessions::BusyState>(s->state)) runs.push_back(b->operationRuns);
        CHECK(runs.size() >= 2);
        CHECK(std::find(runs.begin(), runs.end(), true) != runs.end() && runs.back() == false);
    }

    // machineReplaced, modelSwitched -> c1600.machine; machineAboutToChange
    // -> no message, the debugger's hook.
    const shared::MachinePayload before = *f.state<shared::MachinePayload>(t::kMachine);
    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1600"}})));
    const shared::MachinePayload* after = f.state<shared::MachinePayload>(t::kMachine);
    CHECK(after && after->model == Model::PC1600);
    CHECK(after && after->builds == before.builds + 1 && after->modelSwitches == before.modelSwitches + 1);
    CHECK(debugger.aboutToChange >= 1 && debugger.replaced >= 2);

    // runDebuggerSlice -> no message: every advance goes through the gate.
    const int slices = debugger.slices;
    f.runFor(0.5);
    CHECK(debugger.slices > slices);

    // presetArmed -> display and control state during the operation;
    // presetLog -> log; moduleChanged -> c1600.machine.
    mark = f.messages.size();
    const auto moduleChanges = after->moduleChanges;
    const sessions::CallId id = f.send(c::kLoadPreset, Value::object({{"path", "tests/fixtures/presets/startup-pc1600.pc1600"}}));
    int controlWhileBusy = countState(f.messages, mark, t::kControlState);   // the command's own flush: armed
    while (!f.takeReply(id)) {
        const std::size_t from = f.messages.size();
        f.call();
        if (f.busy()) controlWhileBusy += countState(f.messages, from, t::kControlState);
    }
    CHECK(controlWhileBusy >= 1);
    CHECK(!f.listener.mirror().takeAppended(sessions::kLogTopic).empty());
    CHECK(f.state<shared::MachinePayload>(t::kMachine)->moduleChanges != moduleChanges);
    CHECK(f.state<shared::ControlStatePayload>(t::kControlState)->state.ce1600p.attached);

    // notice, attachFailed -> notices: the CE-158 refused beside the CE-1600P (ledger 1600-22).
    (void)f.notices();
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-158")));
    const auto notices = f.notices();
    CHECK(!notices.empty() && notices.back().find("could not be attached") != std::string::npos);
    // bayError -> notices: a module that does not exist.
    CHECK(f.ok(c::kSelectModule, Value::object({{"slot", 1}, {"name", "No such module"}})));
    CHECK(!f.notices().empty());

    // tapesChanged, tape status -> c1600.tapes.
    CHECK(f.part().tapeInterfaceAttached());
    CHECK(f.part().tapes().recordNew("ac38", nullptr));
    f.call();
    CHECK(f.state<shared::TapesPayload>(t::kTapes)->status.mode == TapeDeck::Mode::Record);

    // serialLinksRelinked -> c1600.serial-links.
    const auto moves = f.state<shared::SerialLinksPayload>(t::kSerialLinks)->moves;
    f.part().refreshSerialLinkDirectory();
    f.call();
    CHECK(f.state<shared::SerialLinksPayload>(t::kSerialLinks)->moves == moves + 1);

    // hostDirectories -> the frontend states a folder picker (AC-26).
    CHECK(f.state<shared::ControlStatePayload>(t::kControlState)->state.hostDriveOffered);
    SessionFixture web("calcu-session-ac38-web");
    web.hostDirectories = false;
    CHECK(web.start("PC-1600"));
    CHECK(!web.state<shared::ControlStatePayload>(t::kControlState)->state.hostDriveOffered);
    f.part().setDebugger(nullptr);
}

// Reset, Cold boot and a model switch stop what runs; keys pressed while an
// operation runs are dropped (ledger 1600-9; LIB-18 AC-16).
TEST(ledger_1600_9_keys_dropped_and_reset_stops_an_operation) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.readCost = std::chrono::milliseconds(1);
    const sessions::CallId preset = f.send(c::kLoadPreset, Value::object({{"path", "tests/fixtures/presets/startup-pc1600.pc1600"}}));
    f.call();
    CHECK(f.busy());
    f.part().typing().pasteText("PRINT 1");
    CHECK(!f.part().typing().pasteActive());   // dropped
    const sessions::ReplyMessage reset = f.run(c::kReset, Value::object({{"all", true}}));
    CHECK(reset.done);
    const auto stopped = f.takeReply(preset);
    CHECK(stopped && !stopped->done && stopped->reason == sessions::kStopped);
    f.part().typing().pasteText("PRINT 1");
    CHECK(f.part().typing().pasteActive());   // taken again
}

} // namespace
