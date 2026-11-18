// Model and variant switches.
#include "Files.hpp"
#include "TestHarness.hpp"
#include "TestPresetLoader.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;

namespace {
Value named(const char* name) { return Value::object({{"name", name}}); }
Value attachArgs(const char* place, const char* peripheral, const char* variant = nullptr) {
    Value::Object o{{"place", place}, {"peripheral", peripheral}};
    if (variant) o.push_back({"variant", variant});
    return Value(std::move(o));
}
RigOptions dotStart() {
    RigOptions o;
    o.settings = "[startup]\nmodel = test-dot\n";
    return o;
}
TestPresetLoader::Preset dotV2() {
    TestPresetLoader::Preset p;
    p.plan = MachinePlan{mt::kDot, VariantId{"V2"}, {}, nullptr};
    return p;
}
std::string lastModel(Rig& r) { return r.settings->text("startup", "last-model"); }
} // namespace

TEST(model_switch_from_startup_preset) {
    TestPresetLoader loader;
    loader.presets["dot.preset.yaml"] = dotV2();
    RigOptions o;
    o.settings = "[preset]\ntest-dot = dot.preset.yaml\n";
    o.loader = &loader;
    Rig r(o);
    CHECK(r.machine() && r.machine()->model() == mt::kSeg);
    const CallId c = r.send("model", named("test-dot"));
    CHECK(r.done(c));
    CHECK(r.machine() && r.machine()->variant() == std::optional<VariantId>(VariantId{"V2"}));
    CHECK(loader.calls.size() == 1 && loader.calls[0].second == std::optional<ModelId>(mt::kDot));
    CHECK(r.startup->control().lastBuild() && r.startup->control().lastBuild()->origin == BuildOrigin::StartupPreset);
}

TEST(model_switch_builtin_default) {
    Rig r;
    const CallId c = r.send("model", named("test-dot"));
    CHECK(r.done(c));
    TestMachine* m = r.machine();
    CHECK(m && m->model() == mt::kDot && m->variant() == std::optional<VariantId>(VariantId{"V1"}));
    CHECK(m && m->plugged().byPlace.empty());   // the old machine's printer is not carried over
    CHECK(m && m->isKeyReady());
    CHECK_EQ(r.product.builds(), 2);
    CHECK(r.startup->control().lastBuild() && r.startup->control().lastBuild()->origin == BuildOrigin::ModelSwitch);
}

TEST(variant_switch_keeps_plugs_in_places) {
    Rig r(dotStart());
    CHECK(r.done(r.send("attach", attachArgs("a", "test-printer"))));
    CHECK(r.done(r.send("attach", attachArgs("card-1", "test-module", "8k"))));
    const CallId c = r.send("variant", named("v2"));   // any case
    CHECK(r.done(c));
    TestMachine* m = r.machine();
    CHECK(m && m->variant() == std::optional<VariantId>(VariantId{"V2"}));
    if (!m) return;
    const auto p = m->plugged().byPlace;
    CHECK_EQ(p.size(), std::size_t{2});
    if (p.size() == 2) {
        CHECK(p[0].place == at({"a"}) && p[0].peripheral == mt::kPrinter);
        CHECK(p[1].place == at({"card-1"}) && p[1].peripheral == mt::kModule &&
              p[1].variant == std::optional<VariantId>(VariantId{"8k"}));
    }
    CHECK_EQ(m->operationsTold(at({"a"})), std::uint64_t{0});   // built with them: no power cycle
}

TEST(variant_switch_memory_empty_no_preset) {
    TestPresetLoader loader;
    TestPresetLoader::Preset v1 = dotV2();
    v1.plan.variant = VariantId{"V1"};
    loader.presets["dot.preset.yaml"] = v1;
    RigOptions o = dotStart();
    o.settings = "[startup]\nmodel = test-dot\n[preset]\ntest-dot = dot.preset.yaml\n";
    o.loader = &loader;
    Rig r(o);
    CHECK(r.done(r.send("key-down", Value::object({{"key", "a"}}))));
    CHECK(r.done(r.send("key-up", Value::object({{"key", "a"}}))));
    CHECK_EQ(r.machine()->presses(), std::uint64_t{1});
    CHECK(r.done(r.send("variant", named("V2"))));
    CHECK_EQ(r.machine()->presses(), std::uint64_t{0});   // its own memory starts empty
    CHECK_EQ(loader.calls.size(), std::size_t{1});        // the start's only: no startup preset
    CHECK(r.startup->control().lastBuild() && r.startup->control().lastBuild()->origin == BuildOrigin::VariantSwitch);
}

TEST(same_model_done_nothing_built) {
    Rig r;
    const auto gen = r.session->machineSession().generation();
    const CallId c = r.send("model", named("TEST-SEG"));
    CHECK(r.done(c));
    CHECK_EQ(r.product.builds(), 1);
    CHECK_EQ(r.session->machineSession().generation(), gen);
}

TEST(same_variant_done) {
    Rig r(dotStart());
    const auto gen = r.session->machineSession().generation();
    CHECK(r.done(r.send("variant", named("v1"))));
    CHECK_EQ(r.product.builds(), 1);
    CHECK_EQ(r.session->machineSession().generation(), gen);
}

TEST(last_model_after_command_preset_and_start) {
    TestPresetLoader loader;
    TestPresetLoader::Preset seg;
    seg.plan = MachinePlan{mt::kSeg, std::nullopt, {}, nullptr};
    loader.presets["seg.preset.yaml"] = seg;
    RigOptions o;
    o.settings = "[startup]\nmodel = last\n";
    o.loader = &loader;
    o.start = false;
    Rig r(o);
    auto inFile = [&] {
        const std::string t = calcu::testing::readText(r.iniPath);
        const auto at = t.find("last-model = ");
        return at == std::string::npos ? std::string() : t.substr(at + 13, t.find('\n', at) - at - 13);
    };
    CHECK(r.done(r.send("start")));
    CHECK_EQ(inFile(), std::string("test-seg"));   // the start
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK_EQ(inFile(), std::string("test-dot"));   // a command
    r.settings->set("preset", "test-seg", "seg.preset.yaml");
    CHECK(r.done(r.send("model", named("test-seg"))));
    CHECK_EQ(loader.calls.size(), std::size_t{1});
    CHECK_EQ(inFile(), std::string("test-seg"));   // a preset
    CHECK_EQ(lastModel(r), std::string("test-seg"));
}

TEST(unwritable_settings_keep_it_for_the_run) {
    RigOptions o;
    o.settings = "[startup]\nmodel = last\nlast-model = test-seg\n";
    o.formatVersion = "9";   // another format version: not written in this run
    Rig r(o);
    const std::string before = calcu::testing::readText(r.iniPath);
    CHECK(!r.settings->writable());
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK_EQ(lastModel(r), std::string("test-dot"));   // kept until the product quits
    CHECK_EQ(calcu::testing::readText(r.iniPath), before);
}

TEST(variant_not_of_the_model_refused) {
    Rig r;
    const CallId c = r.send("variant", named("V1"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("The test-seg has no variant \"V1\"."));
    CHECK_EQ(r.product.builds(), 1);
}

TEST(unknown_model_refused) {
    Rig r;
    const CallId c = r.send("model", named("TI-59"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("There is no model \"TI-59\"."));
    CHECK_EQ(r.product.builds(), 1);
}
