// The library's app session in Calc-U-1600: operations in parts with live
// updates, the buzzer only at regular speed, and 0.8.1's callbacks as
// messages. That the product has no session of its own and the Qt6 app
// sends no suspend is the CTest CalcU1600NoOwnSession; the Web's suspend is
// frontends/web/tests/suspend.spec.js. On a scripted clock; SKIP
// without ROMs.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "../app/tests/SessionFixture.hpp"
#include "Machine/SharpRoms.hpp"
#include "../core/Connector/PlotterPaperOutput.hpp"
#include "support/RawPlotPoints.hpp"
#include "TestHarness.hpp"
#include "control/ControlState.hpp"
#include "debugger/RunState.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = cu1600::commands;
namespace t = cu1600::topics;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;
using Model = calcu::machine::ModelId;

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

// A preset (the Lissajous example: it plots and waits for the
// program to end) runs in parts:
// busy over several calls, the display and the paper updated during it,
// every call within the allowance of a call at full speed.
TEST(preset_in_parts_busy_display_and_paper_update) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    f.readCost = std::chrono::microseconds(500);   // the host's work inside a call
    f.messages.clear();
    const sessions::CallId id = f.send(calcu::presets::kLoadPresetCommand, Value::object({{"ref", "examples:plotter/lissajou-1500.pc1500a"}}));
    // What the command's own flush sent (the start of the operation, before
    // its first call): the armed machine.
    const int controlsAtSend = countState(f.messages, 0, calcu::control::kControlStateTopic);
    std::vector<CallRecord> calls;
    std::optional<sessions::ReplyMessage> reply;
    while (!(reply = f.takeReply(id)) && calls.size() < 20000) {
        const std::size_t from = f.messages.size();
        const auto before = f.now + std::chrono::milliseconds(17);
        f.call(17);
        CallRecord r;
        r.busy = f.busy();
        r.displayMessages = countState(f.messages, from, sessions::kDisplayTopic);
        r.controlMessages = countState(f.messages, from, calcu::control::kControlStateTopic);
        r.wallMs = std::chrono::duration<double, std::milli>(f.now - before).count();
        // The CE-150's paper in the session: its marks so far.
        r.plotRevision = 0;
        for (const auto& [owner, paper] : f.part().paperPart().papers())
            if (owner == "CE-150") r.plotRevision = sharp::rawPlotPoints(*paper).size();
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
    std::printf("  preset in parts: %zu calls, %ld busy, %d display and %d control-state messages while busy, worst call %.1f ms\n",
                calls.size(), static_cast<long>(busyCalls), displays, controls, worstMs);
    CHECK(busyCalls > 10);
    CHECK(displays > 5);       // the display kept updating
    CHECK(controls >= 1);      // the armed machine (the CE-150 attached) was shown during it
    CHECK(lastRevisionWhileBusy + 1 > firstRevision);   // the paper grew while it ran
    CHECK(worstMs <= 14.0 + 3.0);   // a call's allowance at full speed (14 ms), plus the slice that crosses it
    CHECK(!f.busy());
}

// The buzzer through the audio sink at regular speed, nothing
// while the display is held for full speed or an operation runs.
TEST(buzzer_heard_at_regular_speed_only) {
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
    f.typeLine("BEEP 3");
    f.runFor(4.0);
    CHECK(startedFrom(mark));
    CHECK(samplesFrom(mark) > 0);
    for (const auto& m : f.messages)
        if (const auto* s = std::get_if<sessions::SoundStart>(&m)) CHECK(s->sampleRate == 48000);

    // Full speed (the display held): nothing.
    f.readCost = std::chrono::milliseconds(1);   // a call at full speed runs until its allowance is used
    CHECK(f.ok(sessions::kSpeedCommand, Value::object({{"full", true}})));
    f.typeLine("BEEP 3");
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
    const sessions::CallId id = f.send(calcu::startup::kResetCommand);
    std::size_t duringOp = 0;
    while (!f.takeReply(id)) {
        const std::size_t from = f.messages.size();
        f.call();
        if (f.busy()) duringOp += samplesFrom(from);
    }
    CHECK(duringOp == 0);
}

// Everything 0.8.1's callbacks announced
// (Shared/MachineSession.hpp:69-93, Shared/AppSession.hpp:62-84) arrives
// as messages a test frontend keeps, under the owners' names: the
// library's `control-state`, `media`, `run-state`, `notices`, `log`,
// `busy`, `roms-missing` and serial links; the product's own `c1600.lamps`.
TEST(mirror_holds_every_callback) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");

    // romMissing -> the roms-missing topic; the start is refused.
    {
        SessionFixture f("calcu-session-mirror-missing");
        fs::remove(fs::path(f.paths->bundledResourcesDir()) / "PC1600-P2-B6-new.bin");
        f.open();
        const sessions::ReplyMessage r = f.run(c::kStart);
        CHECK(!r.done);
        const auto* m = f.state<calcu::startup::RomsMissingState>(calcu::startup::kRomsMissingTopic);
        CHECK(m && !m->missing.files.empty());
        // The start builds nothing: its reason names the file.
        CHECK(r.reason.find("PC1600-P2-B6-new.bin") != std::string::npos);
        CHECK(!f.part().hasLiveMachine());
    }

    SessionFixture f;
    f.readCost = std::chrono::microseconds(500);
    f.open();
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

    // machineReplaced, modelSwitched -> the control state's model and a new
    // machine; machineAboutToChange -> the library's run state (the debugger
    // is a part).
    const auto generation = f.session().machineSession().generation();
    CHECK(f.model("PC-1600"));
    const auto* after = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    CHECK(after && after->model == calcu::machine::ModelId{"PC-1600"});
    CHECK(f.session().machineSession().generation() > generation);
    const auto* run = f.state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
    CHECK(run && !run->paused);

    // runDebuggerSlice -> the run control is the gate of every advance: a
    // pause stops emulated time, a resume lets it go on.
    CHECK(f.ok(calcu::debugger::kPauseCommand, Value::object({{"level", "cpu"}})));
    const auto ticks = f.part().machine()->tickCount();
    f.runFor(0.5);
    CHECK_EQ(f.part().machine()->tickCount(), ticks);
    CHECK(f.ok(calcu::debugger::kResumeCommand));
    f.runFor(0.5);
    CHECK(f.part().machine()->tickCount() > ticks);

    // presetArmed -> display and control state during the operation;
    // presetLog -> log; moduleChanged -> the topic `media`.
    mark = f.messages.size();
    const fs::path modules = f.root / "modules.pc1600";
    {
        std::ofstream out(modules);
        out << "format-version: 2\nmodel: PC-1600\nprinter: CE-1600P\nslot-1: { name: CE-1600M }\n"
               "keys:\n  - key: mode\n  - type: NEW0\n  - key: mode\n  - type: PRINT MEM\n";
    }
    const sessions::CallId id = f.send(calcu::presets::kLoadPresetCommand, Value::object({{"ref", modules.generic_string()}}));
    int controlWhileBusy = countState(f.messages, mark, calcu::control::kControlStateTopic);   // the command's own flush: armed
    while (!f.takeReply(id)) {
        const std::size_t from = f.messages.size();
        f.call();
        if (f.busy()) controlWhileBusy += countState(f.messages, from, calcu::control::kControlStateTopic);
    }
    CHECK(controlWhileBusy >= 1);
    CHECK(!f.listener.mirror().takeAppended(sessions::kLogTopic).empty());
    CHECK(f.held("slot-1") && f.held("slot-1")->name == "CE-1600M");
    {
        const auto* cs = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        const calcu::control::PlaceEntry* printer = cs ? cs->place(calcu::machine::PlacePath{{calcu::machine::PlaceName{"printer"}}}) : nullptr;
        const calcu::control::PeripheralChoice* p = printer ? calcu::control::choice(*printer, calcu::machine::PeripheralId{"CE-1600P"}) : nullptr;
        CHECK(p && p->plugged);
    }

    // attachFailed -> the refusal of `attach`: the CE-158 refused beside the
    // CE-1600P.
    const sessions::ReplyMessage ce158 = f.run(
        calcu::startup::kAttachCommand, Value::object({{"place", "interface"}, {"peripheral", "CE-158"}}));
    CHECK(!ce158.done && ce158.reason.find("cannot be used together") != std::string::npos);
    // bayError -> the refusal of `media.choose`: a module that does not exist.
    (void)f.notices();
    CHECK(!f.choose("slot-1", "No such module").done);

    // tapesChanged -> the topic `media`; the tape status -> c1600.lamps.
    CHECK(f.part().tapeInterfaceAttached());
    CHECK(f.run(calcu::media::kStartNewCommand, Value::object({{"place", "cassette"}, {"template", f.entry("cassette", "Empty")}, {"name", "mirror"}})).done);
    f.call();
    CHECK(f.state<cu1600::LampsPayload>(t::kLamps)->tape.mode == TapeDeck::Mode::Record);

    // serialLinksRelinked -> c1600.serial-links.
    const auto moves = f.state<cu1600::SerialLinksPayload>(t::kSerialLinks)->moves;
    f.part().refreshSerialLinkDirectory();
    f.call();
    CHECK(f.state<cu1600::SerialLinksPayload>(t::kSerialLinks)->moves == moves + 1);

    // hostDirectories -> the place host-drive available or not.
    const calcu::machine::PlacePath hostDrive{{calcu::machine::PlaceName{"host-drive"}}};
    auto hostDriveAvailable = [&](SessionFixture& s) {
        const auto* cs = s.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
        const calcu::control::PlaceEntry* p = cs ? cs->place(hostDrive) : nullptr;
        return p && p->available.now;
    };
    CHECK(hostDriveAvailable(f));
    SessionFixture web("calcu-session-mirror-web");
    web.hostDirectories = false;
    CHECK(web.start("PC-1600"));
    CHECK(!hostDriveAvailable(web));
}

// Reset, Cold boot and a model switch stop what runs; keys pressed while an
// operation runs are dropped.
TEST(keys_dropped_and_reset_stops_an_operation) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.readCost = std::chrono::milliseconds(1);
    const sessions::CallId preset = f.send(calcu::presets::kLoadPresetCommand, Value::object({{"ref", "tests/fixtures/presets/maxed-out-mem.pc1600"}}));
    f.call();
    CHECK(f.busy());
    f.paste("PRINT 1");
    CHECK(f.engine().idle());   // dropped
    const sessions::ReplyMessage reset = f.run(calcu::startup::kColdBootCommand);
    CHECK(reset.done);
    const auto stopped = f.takeReply(preset);
    CHECK(stopped && !stopped->done && stopped->reason == sessions::kStopped);
    f.paste("PRINT 1");
    CHECK(!f.engine().idle());   // taken again
}

} // namespace
