// LIB-18 design L6: Reset, Cold boot, power cycle; readiness (AC-13, AC-14,
// EC-3, EC-7; D-4, D-5).
#include "TestHarness.hpp"
#include "TestHook.hpp"
#include "TestProductMachines.hpp"

using namespace calcu::startup;
using namespace calcu::startup::test;
using calcu::machine::KeyId;
using calcu::sessions::Hosting;

namespace {
const char* const kRestarts[] = {"reset", "cold-boot", "power-cycle"};
void press(Rig& r, const char* key) {
    r.send("key-down", Value::object({{"key", key}}));
    r.send("key-up", Value::object({{"key", key}}));
}
} // namespace

TEST(ac13_reset_cold_boot_power_cycle_are_operations_over_at_key_readiness) {
    RigOptions o;
    o.hosting = Hosting::Paced;
    Rig r(o);
    r.callWhileBusy();
    for (const char* name : kRestarts) {
        CHECK(r.machine()->isKeyReady());
        const CallId c = r.send(name);
        CHECK(r.reply(c) == nullptr);   // an operation of the session
        CHECK(r.fe.busy() && r.fe.busy()->operationRuns);
        CHECK(!r.machine()->isKeyReady());   // the machine's operation started
        r.callWhileBusy();
        CHECK(r.done(c));
        CHECK(r.machine()->isKeyReady());
    }
    // The memory: Reset and power cycle keep it, Cold boot empties it.
    Rig v;
    press(v, "1");
    CHECK(v.done(v.send("reset")));
    CHECK_EQ(v.machine()->presses(), std::uint64_t{1});
    CHECK(v.done(v.send("power-cycle")));
    CHECK_EQ(v.machine()->presses(), std::uint64_t{1});
    CHECK(v.done(v.send("cold-boot")));
    CHECK_EQ(v.machine()->presses(), std::uint64_t{0});
}

TEST(ac13_plugs_stay) {   // LIB-2 AC-10
    Rig r;   // the printer in `printer`
    for (const char* name : kRestarts) CHECK(r.done(r.send(name)));
    TestMachine* m = r.machine();
    CHECK_EQ(m->plugged().byPlace.size(), std::size_t{1});
    CHECK_EQ(m->operationsTold(at({"printer"})), std::uint64_t{3});
    CHECK_EQ(r.product.builds(), 1);
}

TEST(ac13_clock_given_host_time_at_end) {   // LIB-2 AC-23
    RigOptions o;
    o.settings = "[startup]\nmodel = test-dot\n";
    Rig r(o);
    for (const char* name : kRestarts) {
        const int before = r.machine()->clock()->setCalls;
        CHECK(r.done(r.send(name)));
        CHECK_EQ(r.machine()->clock()->setCalls, before + 1);
    }
}

TEST(ac14_text_and_held_keys_dropped) {   // LIB-5 EC-5
    TestHook hook;
    RigOptions o;
    o.hook = &hook;
    Rig r(o);
    int drops = hook.drops;
    for (const char* name : kRestarts) {
        hook.waiting = "12";
        hook.held = {KeyId{"1"}};
        CHECK(r.done(r.send(name)));
        CHECK(hook.waiting.empty() && hook.held.empty());
        CHECK_EQ(hook.drops, ++drops);
    }
    // The power cycle after an attach drops them too.
    hook.waiting = "3";
    CHECK(r.done(r.send("attach", Value::object({{"place", "slot-1"}, {"peripheral", "test-module"}}))));
    CHECK(hook.waiting.empty());
    CHECK_EQ(hook.drops, drops + 1);
}

TEST(ec3_ready_at_a_prompt_ends_the_wait) {   // C1600-1 AC-21
    RigOptions o;
    // A start-up that ends at a prompt after 5000 ticks: ready there.
    o.prepare = [](TestProductMachines& p) { p.defaults[mt::kSeg].content = std::make_shared<mt::TestStartUp>(5'000); };
    Rig r(o);
    CHECK(r.done(r.startCall));
    const Ticks piece = calcu::sessions::OperationContext::runPiece(mt::kSegRate);
    CHECK(r.machine()->isKeyReady());
    CHECK(r.machine()->tickCount() >= 5'000 && r.machine()->tickCount() <= 5'000 + piece + 5);
    CHECK(r.logLines().empty());
}

TEST(ec7_never_ready_ends_after_bound_with_log_line_machine_left) {
    RigOptions o;
    o.prepare = [](TestProductMachines& p) {
        p.defaults[mt::kSeg].content = std::make_shared<mt::TestStartUp>(mt::TestStartUp::kNever);
        p.bounds[mt::kSeg] = calcu::machine::Seconds(0.01);   // 1000 ticks
    };
    Rig r(o);
    CHECK(r.done(r.startCall));   // the operation ends as done (D-5)
    TestMachine* m = r.machine();
    CHECK(m && !m->isKeyReady());   // left as it is
    const Ticks piece = calcu::sessions::OperationContext::runPiece(mt::kSegRate);
    CHECK(m && m->tickCount() >= 1'000 && m->tickCount() <= 1'000 + piece + 5);
    const auto lines = r.logLines();
    CHECK(lines.size() == 1 && lines[0] == "The test-seg was not ready for keys after 0.01 seconds; it is left as it is.");
    CHECK(r.notices().empty());
    // The same bound for a reset.
    CHECK(r.done(r.send("reset")));
    CHECK_EQ(r.logLines().size(), std::size_t{1});
}
