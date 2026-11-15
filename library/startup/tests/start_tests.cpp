// LIB-18 design L3: the start (AC-3, AC-4, AC-5, EC-1, EC-2, EC-6), with
// the stand-in for LIB-6's loader.
#include "Files.hpp"
#include "TestHarness.hpp"
#include "TestPresetLoader.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;
using calcu::sessions::Hosting;

namespace {
bool holds(TestMachine& m, const char* place, const calcu::machine::PeripheralId& id) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == at({place}) && e.peripheral == id) return true;
    return false;
}

// A preset of the stand-in: test-seg with the UART in `aux`.
TestPresetLoader::Preset segWithUart() {
    TestPresetLoader::Preset p;
    p.plan = MachinePlan{mt::kSeg, std::nullopt, {plug("aux", mt::kUart)}, nullptr};
    return p;
}
TestPresetLoader::Preset dotV2() {
    TestPresetLoader::Preset p;
    p.plan = MachinePlan{mt::kDot, VariantId{"V2"}, {}, nullptr};
    return p;
}
} // namespace

TEST(ac3_startup_model_named) {
    RigOptions o;
    o.settings = "[startup]\nmodel = test-dot\nlast-model = test-seg\n";
    Rig r(o);
    CHECK(r.done(r.startCall));
    CHECK(r.machine() && r.machine()->model() == mt::kDot);
}

TEST(ac3_last_gives_last_model) {
    RigOptions o;
    o.settings = "[startup]\nmodel = last\nlast-model = test-dot\n";
    Rig r(o);
    CHECK(r.machine() && r.machine()->model() == mt::kDot);
}

TEST(ac3_nothing_recorded_gives_fallback) {
    RigOptions o;
    o.settings = "";
    Rig r(o);
    CHECK(r.machine() && r.machine()->model() == mt::kSeg);   // the product's fallback model
    // The last used model is kept in [startup] last-model.
    CHECK(calcu::testing::readText(r.iniPath).find("last-model = test-seg") != std::string::npos);
    Rig none;   // defaults only
    CHECK(none.machine() && none.machine()->model() == mt::kSeg);
}

TEST(ac4_startup_preset_loaded) {
    TestPresetLoader loader;
    loader.presets["seg.preset.yaml"] = segWithUart();
    RigOptions o;
    o.settings = "[preset]\ntest-seg = seg.preset.yaml\n";
    o.loader = &loader;
    Rig r(o);
    CHECK(r.done(r.startCall));
    CHECK_EQ(loader.calls.size(), std::size_t{1});
    if (!loader.calls.empty()) {
        CHECK_EQ(loader.calls[0].first, std::string("seg.preset.yaml"));
        CHECK(loader.calls[0].second == std::optional<ModelId>(mt::kSeg));
    }
    CHECK(r.machine() && holds(*r.machine(), "aux", mt::kUart) && !holds(*r.machine(), "printer", mt::kPrinter));
    CHECK_EQ(r.product.defaultsAsked, 0);
    CHECK(r.startup->control().lastBuild() && r.startup->control().lastBuild()->origin == BuildOrigin::StartupPreset);
    CHECK(r.notices().empty());
}

TEST(ac4_no_startup_preset_builtin_default) {
    TestPresetLoader loader;
    loader.presets["seg.preset.yaml"] = segWithUart();
    RigOptions o;
    o.settings = "[preset]\ntest-dot = seg.preset.yaml\n";   // only another model has one
    o.loader = &loader;
    Rig r(o);
    CHECK(loader.calls.empty());
    CHECK(r.machine() && holds(*r.machine(), "printer", mt::kPrinter));   // ADR 0021
    CHECK(r.startup->control().lastBuild() && r.startup->control().lastBuild()->origin == BuildOrigin::Start);
}

TEST(ac4_no_loader_builtin_default) {   // D-9
    RigOptions o;
    o.settings = "[preset]\ntest-seg = seg.preset.yaml\n";
    Rig r(o);
    CHECK(r.done(r.startCall));
    CHECK(r.machine() && holds(*r.machine(), "printer", mt::kPrinter));
    CHECK(r.notices().empty());
}

TEST(ac5_start_is_one_operation_busy_until_ready) {
    RigOptions o;
    o.hosting = Hosting::Paced;
    Rig r(o);
    CHECK(r.fe.busy() && r.fe.busy()->operationRuns);   // at once
    CHECK(r.reply(r.startCall) == nullptr);             // its reply at its end
    CHECK(r.machine() && !r.machine()->isKeyReady());
    r.callWhileBusy();
    CHECK(r.done(r.startCall));
    CHECK(r.fe.busy() && !r.fe.busy()->operationRuns);
    CHECK(r.machine() && r.machine()->isKeyReady());
    CHECK_EQ(r.product.builds(), 1);
}

TEST(ac5_one_build_when_model_named) {   // LIB-15 AC-16
    RigOptions o;
    o.start = false;
    Rig r(o);
    const CallId c = r.send("start", Value::object({{"model", "TEST-DOT"}}));
    CHECK(r.done(c));
    CHECK_EQ(r.product.builds(), 1);
    CHECK(r.machine() && r.machine()->model() == mt::kDot);
}

TEST(ac5_one_build_when_preset_named) {
    TestPresetLoader loader;
    loader.presets["dot.preset.yaml"] = dotV2();
    RigOptions o;
    o.start = false;
    o.loader = &loader;
    Rig r(o);
    const CallId c = r.send("start", Value::object({{"preset", "dot.preset.yaml"}}));
    CHECK(r.done(c));
    CHECK_EQ(r.product.builds(), 1);
    CHECK_EQ(loader.calls.size(), std::size_t{1});
    if (!loader.calls.empty()) CHECK(!loader.calls[0].second);   // for any model
    CHECK(r.machine() && r.machine()->variant() == std::optional<VariantId>(VariantId{"V2"}));
}

TEST(ac5_unknown_model_notice_and_startup_model) {   // LIB-15 EC-4
    RigOptions o;
    o.start = false;
    Rig r(o);
    const CallId c = r.send("start", Value::object({{"model", "TI-59"}}));
    CHECK(r.done(c));
    const auto n = r.notices();
    CHECK(n.size() == 1 && n[0] == "There is no model \"TI-59\"; the startup model is used.");
    CHECK(r.machine() && r.machine()->model() == mt::kSeg);
    CHECK_EQ(r.product.builds(), 1);
}

TEST(ac5_second_start_refused) {   // D-8
    Rig r;
    const CallId again = r.send("start");
    CHECK(!r.done(again));
    CHECK_EQ(r.reason(again), std::string(kStartedAlready));
    CHECK_EQ(r.product.builds(), 1);
}

TEST(ec1_preset_gone_outdated_or_other_model_gives_default_and_notice) {
    TestPresetLoader loader;
    TestPresetLoader::Preset outdated = segWithUart();
    outdated.script = TestPresetLoader::Script::Outdated;
    loader.presets["old.preset.yaml"] = outdated;
    loader.presets["dot.preset.yaml"] = dotV2();
    const std::pair<const char*, const char*> cases[] = {
        {"gone.preset.yaml", "the file gone.preset.yaml is gone"},
        {"old.preset.yaml", "the file old.preset.yaml is outdated"},
        {"dot.preset.yaml", "it is for the test-dot"},
    };
    for (const auto& [ref, why] : cases) {
        RigOptions o;
        o.settings = std::string("[preset]\ntest-seg = ") + ref + "\n";
        o.loader = &loader;
        Rig r(o);
        CHECK(r.done(r.startCall));
        CHECK(r.machine() && r.machine()->model() == mt::kSeg && holds(*r.machine(), "printer", mt::kPrinter));
        CHECK_EQ(r.product.builds(), 1);
        const auto n = r.notices();
        CHECK_EQ(n.size(), std::size_t{1});
        if (!n.empty())
            CHECK_EQ(n[0], "The startup preset of the test-seg could not be loaded: " + std::string(why) +
                               ". The test-seg starts as its built-in default machine.");
    }
}

TEST(ec2_last_built_by_preset_uses_its_startup_preset) {
    TestPresetLoader loader;
    loader.presets["dot.preset.yaml"] = dotV2();
    TestPresetLoader::Preset start = dotV2();
    start.plan.variant = VariantId{"V1"};
    loader.presets["dot-start.preset.yaml"] = start;
    std::string ini;
    {
        RigOptions o;
        o.settings = "[startup]\nmodel = last\n[preset]\ntest-dot = dot-start.preset.yaml\n";
        o.loader = &loader;
        o.start = false;
        Rig first(o);
        CHECK(first.done(first.send("start", Value::object({{"preset", "dot.preset.yaml"}}))));   // a preset builds the test-dot
        ini = first.iniPath;
        first.folder_.write("keep", "");   // the folder stays while `first` lives
        // The second start reads the same file.
        loader.calls.clear();
        RigOptions again;
        again.iniPath = ini;
        again.loader = &loader;
        Rig second(again);
        CHECK(second.machine() && second.machine()->model() == mt::kDot);
        CHECK_EQ(loader.calls.size(), std::size_t{1});
        if (!loader.calls.empty()) CHECK_EQ(loader.calls[0].first, std::string("dot-start.preset.yaml"));
        CHECK(second.machine() && second.machine()->variant() == std::optional<VariantId>(VariantId{"V1"}));
    }
}

TEST(ec6_preset_fails_while_running_machine_stays_notice) {
    TestPresetLoader loader;
    TestPresetLoader::Preset failing = segWithUart();
    failing.script = TestPresetLoader::Script::FailAfterBuild;
    loader.presets["fail.preset.yaml"] = failing;
    RigOptions o;
    o.settings = "[preset]\ntest-seg = fail.preset.yaml\n";
    o.loader = &loader;
    Rig r(o);
    CHECK(r.done(r.startCall));
    CHECK(r.machine() && holds(*r.machine(), "aux", mt::kUart));   // as far as the preset built it
    CHECK_EQ(r.product.defaultsAsked, 0);
    CHECK_EQ(r.product.builds(), 1);
    const auto n = r.notices();
    CHECK(n.size() == 1 && n[0] == "The startup preset of the test-seg stopped: the tape broke.");
}
