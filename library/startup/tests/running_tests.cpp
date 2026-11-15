// LIB-18 design L7: while an operation runs (AC-16; D-6, D-7). The running
// operation is the sessions' test part's `tp.op`, or a startup preset of the
// stand-in loader that runs long.
#include "TestHarness.hpp"
#include "TestPresetLoader.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;
using calcu::sessions::Hosting;
using calcu::sessions::kAnotherOperationRuns;
using calcu::sessions::kStopped;

namespace {
RigOptions paced() {
    RigOptions o;
    o.hosting = Hosting::Paced;
    return o;
}
// A rig whose start is over and whose `tp.op` runs long, started and stepped once.
CallId runLong(Rig& r) {
    r.callWhileBusy();
    r.part.operation = calcu::sessions::test::runningFor(50'000'000);
    const CallId op = r.send("tp.op");
    r.call();
    return op;
}
} // namespace

TEST(ac16_reset_cold_boot_model_stop_running_operation_first) {
    const std::pair<const char*, Value> commands[] = {
        {"reset", Value()},
        {"cold-boot", Value()},
        {"model", Value::object({{"name", "test-dot"}})},
    };
    for (const auto& [name, args] : commands) {
        Rig r(paced());
        const CallId op = runLong(r);
        CHECK(r.session->busy() && r.reply(op) == nullptr);
        const CallId c = r.send(name, args);
        CHECK(r.reply(op) && !r.done(op));   // ended first
        CHECK(r.session->busy());             // the new command's operation runs
        r.callWhileBusy();
        CHECK(r.done(c));
        CHECK(r.machine()->isKeyReady());
    }
}

TEST(ac16_stopped_command_replied_stopped) {
    // A startup preset that runs long (OQ-1: Reset is how the user stops it).
    TestPresetLoader loader;
    TestPresetLoader::Preset p;
    p.plan = MachinePlan{mt::kSeg, std::nullopt, {}, nullptr};
    p.steps = 50'000'000;
    loader.presets["long.preset.yaml"] = p;
    RigOptions o = paced();
    o.settings = "[preset]\ntest-seg = long.preset.yaml\n";
    o.loader = &loader;
    Rig r(o);
    r.call();
    CHECK(r.session->busy());
    const CallId reset = r.send("reset");
    CHECK(r.reply(r.startCall) && !r.done(r.startCall));
    CHECK_EQ(r.reason(r.startCall), std::string(kStopped));
    r.callWhileBusy();
    CHECK(r.done(reset));
    CHECK(r.machine() && r.machine()->isKeyReady());   // the preset's machine, reset
    CHECK_EQ(r.product.builds(), 1);
    CHECK(r.notices().empty());   // a stop is not a failure
}

TEST(ac16_other_commands_refused_while_running) {
    Rig r(paced());
    const CallId op = runLong(r);
    const std::pair<const char*, Value> commands[] = {
        {"variant", Value::object({{"name", "V1"}})},
        {"attach", Value::object({{"place", "aux"}, {"peripheral", "test-uart"}})},
        {"detach", Value::object({{"place", "aux"}})},   // would be Done (D-7)
        {"power-cycle", Value()},
        {"start", Value()},
    };
    for (const auto& [name, args] : commands) {
        const CallId c = r.send(name, args);
        CHECK(r.reply(c) && !r.done(c));
        if (std::string(name) != "start") CHECK_EQ(r.reason(c), std::string(kAnotherOperationRuns));
    }
    CHECK(r.session->busy() && r.reply(op) == nullptr);
    CHECK_EQ(r.product.builds(), 1);
}

TEST(ac16_model_to_running_model_leaves_operation_running) {   // D-6
    Rig r(paced());
    const CallId op = runLong(r);
    const CallId c = r.send("model", Value::object({{"name", "test-seg"}}));
    CHECK(r.done(c));
    CHECK(r.session->busy() && r.reply(op) == nullptr);
    const CallId bad = r.send("model", Value::object({{"name", "TI-59"}}));   // refused: stops nothing either
    CHECK(!r.done(bad));
    CHECK(r.session->busy() && r.reply(op) == nullptr);
}
