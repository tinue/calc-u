// LIB-18 design L5: attach, detach, replace (AC-10, AC-11, AC-12, EC-5;
// D-14, D-15). test-seg: slot-1 (one module, power cycle), printer and aux
// (bus, nothing follows); test-dot: a, b (chain, power cycle), card-1 and
// card-2 (one module over both, power cycle); switched off by `off`.
#include "TestHarness.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;
using calcu::sessions::Hosting;

namespace {
Value attachArgs(const char* place, const char* peripheral, const char* variant = nullptr) {
    Value::Object o{{"place", place}, {"peripheral", peripheral}};
    if (variant) o.push_back({"variant", variant});
    return Value(std::move(o));
}
Value placeArg(const char* place) { return Value::object({{"place", place}}); }
RigOptions dotStart(Hosting h = Hosting::VirtualTime) {
    RigOptions o;
    o.settings = "[startup]\nmodel = test-dot\n";
    o.hosting = h;
    return o;
}
bool holds(TestMachine& m, const PlacePath& place, const calcu::machine::PeripheralId& id,
           std::optional<VariantId> variant = std::nullopt) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place == place && e.peripheral == id && (!variant || e.variant == variant)) return true;
    return false;
}
void press(Rig& r, const char* key) {
    r.send("key-down", Value::object({{"key", key}}));
    r.send("key-up", Value::object({{"key", key}}));
}
} // namespace

TEST(ac10_attach_is_one_operation_over_at_key_readiness) {
    Rig r(dotStart(Hosting::Paced));
    r.callWhileBusy();
    const CallId c = r.send("attach", attachArgs("a", "test-printer"));
    CHECK(r.reply(c) == nullptr);   // an operation: its reply at its end
    CHECK(r.session->busy());
    CHECK(holds(*r.machine(), at({"a"}), mt::kPrinter));   // the change is made first
    CHECK(!r.machine()->isKeyReady());                      // then the power cycle runs
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK(r.machine()->isKeyReady());
    CHECK_EQ(r.machine()->operationsTold(at({"a"})), std::uint64_t{1});
}

TEST(ac10_attach_named_and_default_variant) {
    Rig seg;
    CHECK(seg.done(seg.send("attach", attachArgs("slot-1", "test-module", "8k"))));
    CHECK(holds(*seg.machine(), at({"slot-1"}), mt::kModule, VariantId{"8k"}));
    Rig dot(dotStart());
    CHECK(dot.done(dot.send("attach", attachArgs("card-1", "TEST-MODULE"))));   // any case; the default variant
    CHECK(holds(*dot.machine(), at({"card-1"}), mt::kModule, VariantId{"4k"}));
    const CallId bad = dot.send("attach", attachArgs("card-2", "test-module", "2k"));
    CHECK(!dot.done(bad));
    CHECK_EQ(dot.reason(bad), std::string("The Test module has no variant 2k."));
}

TEST(ac10_detach_place) {
    Rig r;   // the printer in `printer`
    const CallId c = r.send("detach", placeArg("printer"));
    CHECK(r.done(c));
    CHECK(r.machine()->plugged().byPlace.empty());
}

TEST(ac10_attach_to_full_single_place_is_one_replace_one_power_cycle) {
    Rig r(dotStart());
    CHECK(r.done(r.send("attach", attachArgs("card-1", "test-module"))));
    const int plugs = r.machine()->plugCalls, unplugs = r.machine()->unplugCalls;
    const Ticks t0 = r.machine()->tickCount();
    CHECK(r.done(r.send("attach", attachArgs("card-1", "test-module", "8k"))));
    TestMachine* m = r.machine();
    CHECK(holds(*m, at({"card-1"}), mt::kModule, VariantId{"8k"}));
    CHECK_EQ(m->plugged().byPlace.size(), std::size_t{1});
    CHECK_EQ(m->plugCalls, plugs + 1);
    CHECK_EQ(m->unplugCalls, unplugs + 1);
    CHECK_EQ(m->operationsTold(at({"card-1"})), std::uint64_t{1});   // one power cycle
    CHECK(m->tickCount() > t0 && m->isKeyReady());
    // The same peripheral and variant again: nothing to do (D-14).
    const Ticks t1 = m->tickCount();
    CHECK(r.done(r.send("attach", attachArgs("card-1", "test-module", "8k"))));
    CHECK_EQ(m->plugCalls, plugs + 1);
    CHECK_EQ(m->tickCount(), t1);
}

TEST(ac10_switched_off_machine_nothing_follows) {   // AC-33 in the library form; OQ-3
    Rig r(dotStart());
    press(r, "off");
    CHECK(!r.machine()->readDisplay().on);
    const Ticks t0 = r.machine()->tickCount();
    CHECK(r.done(r.send("attach", attachArgs("a", "test-printer"))));
    CHECK(holds(*r.machine(), at({"a"}), mt::kPrinter));
    CHECK_EQ(r.machine()->operationsTold(at({"a"})), std::uint64_t{0});   // no power cycle
    CHECK_EQ(r.machine()->tickCount(), t0);
    CHECK(!r.machine()->readDisplay().on);   // it stays off
}

TEST(ac11_memory_through_power_cycle_and_new_plug_found) {
    Rig r(dotStart());
    press(r, "a");
    press(r, "b");
    CHECK_EQ(r.machine()->presses(), std::uint64_t{2});
    CHECK(r.done(r.send("attach", attachArgs("a", "test-printer"))));
    CHECK_EQ(r.machine()->presses(), std::uint64_t{2});   // what the model keeps through a power cycle
    CHECK_EQ(r.machine()->operationsTold(at({"a"})), std::uint64_t{1});   // the new plug was told the power cycle
    CHECK(r.done(r.send("detach", placeArg("a"))));
    CHECK_EQ(r.machine()->presses(), std::uint64_t{2});
}

TEST(ac12_refusal_changes_nothing_no_power_cycle_reason) {
    Rig r;   // the test printer in `printer`
    const Ticks t0 = r.machine()->tickCount();
    const CallId c = r.send("attach", attachArgs("aux", "test-uart"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string(mt::kTogether));   // the machine's reason
    CHECK_EQ(r.machine()->plugged().byPlace.size(), std::size_t{1});
    CHECK_EQ(r.machine()->operationsTold(at({"printer"})), std::uint64_t{0});
    CHECK_EQ(r.machine()->tickCount(), t0);
}

TEST(ac12_full_place_refused_by_can_replace_before_the_operation) {
    RigOptions o;
    o.hosting = Hosting::Paced;
    Rig r(o);
    r.callWhileBusy();
    const CallId first = r.send("attach", attachArgs("slot-1", "test-module"));
    r.callWhileBusy();
    CHECK(r.done(first));
    CHECK(holds(*r.machine(), at({"slot-1"}), mt::kModule));
    const Ticks t0 = r.machine()->tickCount();
    const CallId c = r.send("attach", attachArgs("slot-1", "test-hub"));
    CHECK(r.reply(c) != nullptr && !r.done(c));   // at once: no operation started
    CHECK_EQ(r.reason(c), std::string("The Test hub does not fit slot-1."));
    CHECK(!r.session->busy());
    CHECK(holds(*r.machine(), at({"slot-1"}), mt::kModule));
    CHECK_EQ(r.machine()->tickCount(), t0);
}

TEST(ec5_detach_chain_one_power_cycle) {   // LIB-2 EC-3
    Rig r(dotStart());
    CHECK(r.done(r.send("attach", attachArgs("card-1", "test-module"))));
    CHECK(r.done(r.send("attach", attachArgs("a", "test-hub"))));
    CHECK(r.done(r.send("attach", attachArgs("a/jack", "test-printer"))));   // a place on a peripheral
    CHECK(holds(*r.machine(), at({"a", "jack"}), mt::kPrinter));
    const auto told = r.machine()->operationsTold(at({"card-1"}));
    CHECK(r.done(r.send("detach", placeArg("a"))));
    CHECK_EQ(r.machine()->plugged().byPlace.size(), std::size_t{1});   // the hub and what is on it
    CHECK_EQ(r.machine()->operationsTold(at({"card-1"})), told + 1);  // one power cycle
}

TEST(detach_empty_place_done) {   // LIB-2 EC-2; D-14
    Rig r;
    const Ticks t0 = r.machine()->tickCount();
    const int unplugs = r.machine()->unplugCalls;
    CHECK(r.done(r.send("detach", placeArg("aux"))));
    CHECK_EQ(r.machine()->tickCount(), t0);
    CHECK_EQ(r.machine()->unplugCalls, unplugs);
}
