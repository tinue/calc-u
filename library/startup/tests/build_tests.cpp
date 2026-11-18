// The test product and MachineControl::build.
#include "TestHarness.hpp"
#include "TestHook.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;
using calcu::machine::KeyId;
using calcu::pacing::Speed;

namespace {
Value named(const char* name) { return Value::object({{"name", name}}); }

bool holds(TestMachine& m, const PlacePath& place, const calcu::machine::PeripheralId& id,
           std::optional<VariantId> variant = std::nullopt) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == place && e.peripheral == id && (!variant || e.variant == variant)) return true;
    return false;
}
} // namespace

TEST(test_product_machines_build_each_test_model) {
    RigOptions o;
    o.start = false;
    Rig r(o);
    for (const auto& model : {mt::kSeg, mt::kDot}) {
        const int before = r.product.builds();
        const auto refused = r.startup->control().build(r.session->context(), r.product.builtInDefault(model), BuildOrigin::Start);
        CHECK(!refused);
        CHECK_EQ(r.product.builds(), before + 1);
        CHECK(r.machine() && r.machine()->model() == model);
    }
    CHECK(holds(*r.machine(), at({"a"}), mt::kPrinter) == false);
}

TEST(one_build_with_model_variant_and_plugs) {
    RigOptions o;
    o.start = false;
    Rig r(o);
    MachinePlan plan{mt::kDot, VariantId{"V2"}, {plug("a", mt::kPrinter), plug("card-1", mt::kModule, VariantId{"8k"})}, nullptr};
    CHECK(!r.startup->control().build(r.session->context(), plan, BuildOrigin::Start));
    CHECK_EQ(r.product.builds(), 1);   // one machine with all of it, in one step
    TestMachine* m = r.machine();
    CHECK(m && m->variant() == std::optional<VariantId>(VariantId{"V2"}));
    if (!m) return;
    CHECK(holds(*m, at({"a"}), mt::kPrinter));
    CHECK(holds(*m, at({"card-1"}), mt::kModule, VariantId{"8k"}));
    const auto* roms = m->romsAt(at({"card-1"}));   // from the RomSet
    CHECK(roms && roms->size() == 1 && (*roms)[0].fileName == "test-module-8k.rom" && (*roms)[0].bytes.size() == 8192);
    CHECK_EQ(m->tickCount(), Ticks{0});   // from a cold boot: nothing ran
}

TEST(no_power_cycle_for_what_it_was_built_with) {
    Rig r;   // test-seg with the printer in `printer`
    CHECK(r.done(r.startCall));
    TestMachine* m = r.machine();
    CHECK(m && holds(*m, at({"printer"}), mt::kPrinter));
    if (m) CHECK_EQ(m->operationsTold(at({"printer"})), std::uint64_t{0});
}

TEST(clock_given_host_time_at_build) {
    RigOptions o;
    o.settings = "[startup]\nmodel = test-dot\n";
    Rig r(o);
    TestMachine* m = r.machine();
    CHECK(m && m->clock());
    if (!m || !m->clock()) return;
    CHECK(m->clock()->setCalls >= 1);   // PeripheralsPart at the new machine
    // The host's time of the rig: 2019-12-20 (50 years of 365 days), whatever the zone's hour.
    const auto f = m->clock()->read();
    CHECK(f.month == 12 || f.month == 1);
}

TEST(ready_when_key_ready) {
    Rig r;
    CHECK(r.done(r.startCall));
    TestMachine* m = r.machine();
    CHECK(m && m->isKeyReady());
    if (!m) return;
    // Over at the first run piece (1/60 s) after readiness at 50 ticks.
    const Ticks piece = calcu::sessions::OperationContext::runPiece(mt::kSegRate);
    CHECK(m->tickCount() >= mt::kStartUpTicks && m->tickCount() <= piece + 5);
}

TEST(refused_build_changes_nothing) {
    Rig r;
    TestMachine* before = r.machine();
    const int builds = r.product.builds();
    auto& control = r.startup->control();
    const auto noVariant = control.build(r.session->context(), MachinePlan{mt::kSeg, VariantId{"V1"}, {}, nullptr}, BuildOrigin::ModelSwitch);
    CHECK(noVariant && noVariant->reason == "The test-seg has no variant \"V1\"." && !noVariant->place);
    const auto noModel = control.build(r.session->context(), MachinePlan{ModelId{"TI-59"}, {}, {}, nullptr}, BuildOrigin::ModelSwitch);
    CHECK(noModel && noModel->reason == "There is no model \"TI-59\".");
    CHECK(r.machine() == before);
    CHECK_EQ(r.product.builds(), builds);
    CHECK(control.lastBuild() && control.lastBuild()->origin == BuildOrigin::Start);
}

TEST(refusal_names_the_refused_place) {
    Rig r;
    auto& control = r.startup->control();
    const auto unfit = control.build(r.session->context(), MachinePlan{mt::kSeg, {}, {plug("slot-1", mt::kHub)}, nullptr}, BuildOrigin::Preset);
    CHECK(unfit && unfit->place == std::optional<PlacePath>(at({"slot-1"})));
    if (unfit) CHECK_EQ(unfit->reason, std::string("The Test hub does not fit slot-1."));
    const auto noPlace = control.build(r.session->context(), MachinePlan{mt::kSeg, {}, {plug("card-1", mt::kModule)}, nullptr}, BuildOrigin::Preset);
    CHECK(noPlace && noPlace->place == std::optional<PlacePath>(at({"card-1"})));
    if (noPlace) CHECK_EQ(noPlace->reason, std::string("The test-seg has no place card-1."));
    const auto unknown = control.build(r.session->context(), MachinePlan{mt::kSeg, {}, {plug("aux", calcu::machine::PeripheralId{"CE-150"})}, nullptr}, BuildOrigin::Preset);
    CHECK(unknown && unknown->place == std::optional<PlacePath>(at({"aux"})));
    CHECK_EQ(r.product.builds(), 1);
}

TEST(last_build_gives_plan_and_origin) {
    Rig r;
    auto& control = r.startup->control();
    CHECK(control.lastBuild() && control.lastBuild()->origin == BuildOrigin::Start);
    if (control.lastBuild()) {
        CHECK(control.lastBuild()->plan.model == mt::kSeg);
        CHECK_EQ(control.lastBuild()->plan.peripherals.size(), std::size_t{1});
        CHECK(control.lastBuild()->plan.peripherals[0].roms.empty());   // the plan as given
    }
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK(control.lastBuild() && control.lastBuild()->origin == BuildOrigin::ModelSwitch &&
          control.lastBuild()->plan.model == mt::kDot);
    CHECK(r.done(r.send("variant", named("V2"))));
    CHECK(control.lastBuild() && control.lastBuild()->origin == BuildOrigin::VariantSwitch &&
          control.lastBuild()->plan.variant == std::optional<VariantId>(VariantId{"V2"}));
}

TEST(old_machine_and_its_memory_gone) {
    Rig r;
    CHECK(r.done(r.send("key-down", Value::object({{"key", "1"}}))));
    CHECK(r.done(r.send("key-up", Value::object({{"key", "1"}}))));
    CHECK_EQ(r.machine()->presses(), std::uint64_t{1});
    const auto gen = r.session->machineSession().generation();
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK(r.done(r.send("model", named("test-seg"))));
    CHECK(r.session->machineSession().generation() > gen);
    CHECK_EQ(r.machine()->presses(), std::uint64_t{0});   // a new machine, its memory empty
    CHECK_EQ(r.product.builds(), 3);
}

TEST(full_speed_and_held_keys_do_not_carry_over) {
    TestHook hook;
    RigOptions o;
    o.hook = &hook;
    Rig r(o);
    CHECK(r.done(r.send("speed", Value::object({{"full", true}}))));
    CHECK(r.fe.busy() && r.fe.busy()->speed == Speed::Full);
    CHECK(r.done(r.send("key-down", Value::object({{"key", "1"}}))));   // held on the old machine
    hook.held = {KeyId{"2"}};                                              // held by the typing engine
    const int drops = hook.drops;
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK(r.fe.busy() && r.fe.busy()->speed == Speed::Regular);   // full speed does not carry over
    CHECK(r.machine()->keysDown().empty());
    CHECK(hook.held.empty());
    CHECK_EQ(hook.drops, drops + 1);
}

TEST(text_being_typed_dropped_with_the_old_machine) {
    TestHook hook;
    RigOptions o;
    o.hook = &hook;
    Rig r(o);
    hook.waiting = "123";
    CHECK(r.done(r.send("model", named("test-dot"))));
    CHECK(hook.waiting.empty());
    CHECK(r.machine()->model() == mt::kDot);
}
