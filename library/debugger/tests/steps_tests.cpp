// LIB-10 L13 (design §6): the preset steps of AC-19 on LIB-6's step place;
// with them LIB-6 AC-19's debugger steps and AC-63's `debug-panel:`.
#include "DebugRig.hpp"
#include "TestHarness.hpp"
#include <filesystem>

using namespace calcu::debugger;
using namespace calcu::debugger::test;
namespace presets = calcu::presets;
namespace formats = calcu::formats;

namespace {
std::string keys(const std::string& items, const std::string& model = "test-seg") {
    return pt::header(model) + "keys:\n" + items;
}
} // namespace

TEST(ac19_each_step_does_what_its_ac_says) {
    DebugRig r;
    const CallId l = r.load(keys("  - trace: s.bin\n"
                                 "  - pause: program\n"
                                 "  - step: { level: cpu, count: 2 }\n"
                                 "  - debug-panel: cpu\n"
                                 "  - pause-on-start: cpu\n"
                                 "  - resume:\n"));
    CHECK(r.done(l));
    const auto log = r.logLines();
    CHECK(pt::hasLine(log, "line 4 trace: s.bin: done"));
    CHECK(pt::hasLine(log, "line 5 pause: program: done"));   // finished when the pause had landed
    CHECK(pt::hasLine(log, "line 6 step: cpu 2: done"));
    CHECK(pt::hasLine(log, "line 7 debug-panel: cpu: done"));
    CHECK(pt::hasLine(log, "line 8 pause-on-start: cpu: done"));
    CHECK(pt::hasLine(log, "line 9 resume: done"));
    CHECK(r.shownViews == std::vector<std::string>{"cpu"});   // the frontend was told
    CHECK(!r.paused());
    CHECK(r.runState()->armed == kCpu);
    CHECK(!r.traceState()->running);   // ended with its preset (AC-22)
    CHECK(std::filesystem::exists(std::filesystem::path(r.traces / "s.bin")));
    // pause-on-start: off; a pause that the preset leaves.
    CHECK(r.done(r.load(keys("  - pause-on-start: off\n  - pause: cpu\n"))));
    CHECK(!r.runState()->armed);
    CHECK(r.paused() && r.runState()->owner == kCpu);
}

TEST(ac19_level_view_count_out_of_range_refused_at_read) {
    DebugRig r;
    const std::pair<const char*, const char*> cases[] = {
        {"  - pause: basic\n", "pause: takes a level"},
        {"  - pause-on-start: basic\n", "pause-on-start: takes a level"},
        {"  - step: { level: basic }\n", "step: takes a level"},
        {"  - step: { level: cpu, count: 0 }\n", "count from 1 to 100000"},
        {"  - step: { level: cpu, count: 100001 }\n", "count from 1 to 100000"},
        {"  - step: { colour: red }\n", "Unknown key colour"},
        {"  - debug-panel: memory\n", "debug-panel: takes a view of the debug panel: cpu, log."},
    };
    for (const auto& [item, message] : cases) {
        const std::uint64_t gen = r.session->machineSession().generation();
        const CallId l = r.load(keys(std::string("  - wait: 1ms\n") + item));
        CHECK(!r.done(l));
        CHECK(r.reason(l).find(message) != std::string::npos);
        CHECK(r.reason(l).find("line 5") != std::string::npos);
        CHECK_EQ(r.session->machineSession().generation(), gen);   // refused when read: nothing built
    }
    CHECK(r.done(r.load(keys("  - step: { level: cpu, count: 100000 }\n  - step:\n"))));   // in range; 1 without
}

TEST(ac19_step_not_paused_logs_and_goes_on) {
    DebugRig r;
    const CallId l = r.load(keys("  - step: { level: cpu, count: 5 }\n  - wait: 1ms\n"));
    CHECK(r.done(l));
    CHECK(pt::hasLine(r.logLines(), "step: ignored, the machine is not paused"));   // OQ-4
    CHECK(!r.paused());
}

TEST(ac19_undefined_steps_refused_for_a_model) {
    DebugRig r;   // test-dot stands for a Sharp model: the run-control steps and debug-panel: are [undefined]
    for (const char* item : {"  - pause: cpu\n", "  - resume:\n", "  - step:\n", "  - pause-on-start: cpu\n",
                             "  - debug-panel: cpu\n"}) {
        const CallId l = r.load(keys(item, "test-dot"), "t.test-dot");
        CHECK(!r.done(l));
        CHECK(r.reason(l).find("is not defined yet for the test-dot by the preset format.") != std::string::npos);
    }
}

TEST(ac19_trace_defined_for_every_model) {
    DebugRig r;
    const CallId l = r.load(keys("  - trace: d.bin\n  - wait: 1ms\n", "test-dot"), "t.test-dot");
    CHECK(r.done(l));
    CHECK(std::filesystem::exists(std::filesystem::path(r.traces / "d.bin")));
}

TEST(ac19_no_step_waits_for_a_running_machine) {
    // In virtual time a step that waited for the machine to run would fail
    // with "The machine is paused." (LIB-6 AC-19): every one is carried out
    // on the paused machine.
    DebugRig r;
    const CallId l = r.load(keys("  - pause: cpu\n"
                                 "  - trace: n.bin\n"
                                 "  - debug-panel: log\n"
                                 "  - pause-on-start: program\n"
                                 "  - trace: off\n"
                                 "  - pause: program\n"
                                 "  - resume:\n"));
    CHECK(r.done(l));
    for (const char* verb : {"pause", "resume", "step", "pause-on-start", "debug-panel", "trace"}) {
        const presets::StepDef* d = r.presetsPart->vocabulary().step(verb);
        CHECK(d != nullptr);
        if (!d) continue;
        CHECK(!d->command);   // the harness has commands of its own (§3.9)
        formats::YamlNode v;
        v.type = formats::YamlNode::Type::Scalar;
        v.scalar = std::string(verb) == "trace" ? "x.bin" : "cpu";
        if (std::string(verb) == "step" || std::string(verb) == "resume") {
            v.type = formats::YamlNode::Type::Null;
            v.scalar.clear();
        }
        std::string error;
        auto item = presets::readStep(verb, v, r.presetsPart->readSetup(), *r.machine(), &error);
        CHECK(item != nullptr);
        if (item) CHECK(!item->needsRunningMachine());
    }
}

TEST(ac19_debug_panel_is_a_frontend_step) {
    DebugRig r;
    const presets::StepDef* d = r.presetsPart->vocabulary().step("debug-panel");
    CHECK(d && d->frontend);   // LIB-6 AC-63: never fails
    r.debugger.runControl();
    // Where nobody has the panel it is passed over.
    DebuggerSetup bare;
    DebuggerPart part(bare);
    presets::Vocabulary v;
    addDebuggerSteps(v, part);
    CHECK(v.step("debug-panel")->frontend);
    CHECK(!v.step("pause")->frontend && !v.step("trace")->frontend);
    // The rig's frontend shows it; a preset with it never fails.
    CHECK(r.done(r.load(keys("  - debug-panel: log\n"))));
    CHECK(r.shownViews == std::vector<std::string>{"log"});
}
