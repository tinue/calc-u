// LIB-6 L4: building the machine (AC-10, AC-52, AC-55, EC-2, EC-10).
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
namespace machine = calcu::machine;

namespace {
bool pluggedAt(mt::TestMachine* m, const char* place, const machine::PeripheralId& id,
               std::optional<machine::VariantId> v = std::nullopt) {
    if (!m) return false;
    for (const auto& e : m->plugged().byPlace)
        if (machine::placeText(e.place) == place && e.peripheral == id && (!v || e.variant == v)) return true;
    return false;
}
} // namespace

TEST(ac10_one_machine_from_cold_boot_with_its_places) {
    Rig r;
    CHECK(r.done(r.load(header() + "keys:\n  - key: 2 3\n")));   // the machine before has pressed keys
    const int builds = r.machines.builder_.inner.builds;
    const CallId c = r.load(header("test-dot") + "a: test-hub\ncard-1: test-module:8k\nmodel: x\n");   // refused
    CHECK(!r.done(c));
    const CallId d = r.load("format-version: 2\nmodel: test-dot:V2\na: test-hub\ncard-1: test-module:8k\n");
    CHECK(r.done(d));
    CHECK_EQ(r.machines.builder_.inner.builds, builds + 1);   // one machine, in one step
    CHECK(r.machine()->model() == mt::kDot);
    CHECK(r.machine()->variant() == machine::VariantId{"V2"});
    CHECK(pluggedAt(r.machine(), "a", mt::kHub));
    CHECK(pluggedAt(r.machine(), "card-1", mt::kModule, machine::VariantId{"8k"}));
    CHECK_EQ(r.machine()->presses(), std::uint64_t(0));   // its memory from a cold boot
    const auto& last = r.startupPart->control().lastBuild();
    CHECK(last && last->origin == calcu::startup::BuildOrigin::Preset);   // D-24
    // The model's default variant when none is named.
    CHECK(r.done(r.load(header("test-dot"))));
    CHECK(r.machine()->variant() == machine::VariantId{"V1"} || !r.machine()->variant());
}

TEST(ac10_old_machine_gone_items_start_at_key_readiness) {
    RigOptions o;
    o.startUp = 2'000;   // the new machine's start-up
    Rig r(o);
    mt::TestMachine* before = r.machine();
    const CallId c = r.load(header() + "keys:\n  - key: 2\n");
    CHECK(r.done(c));
    CHECK(r.machine() != before);
    const auto d = r.downs("2");
    CHECK(d.size() == 1 && d[0] >= 2'000);   // the first step at key readiness
}

TEST(ac52_machine_refusal_reason_and_key_line_old_machine_kept) {
    Rig r;
    mt::TestMachine* before = r.machine();
    // The builder refuses the printer and the UART together: no place named.
    const CallId c = r.load(header() + "printer: test-printer\naux: test-uart\nkeys:\n  - key: 2\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 2: The test-seg could not be built: "));
    CHECK(hasLine({r.reason(c)}, mt::kTogether));
    CHECK(r.machine() == before);
    CHECK(r.downs("2").empty());
    CHECK(hasLine(r.notices(), mt::kTogether));
    // A refusal that names its place gives the line of the key: the reader
    // knows a peripheral that the machine's catalogue does not.
    machine::Catalogue more = mt::makeTestCatalogue();
    machine::PeripheralDescriptor ghost;
    ghost.id = machine::PeripheralId{"test-ghost"};
    ghost.fits = {machine::PinOutId{"test.bus"}};
    more.addPeripheral(ghost);
    RigOptions o;
    o.readCatalogue = &more;
    Rig g(o);
    const CallId e = g.load(header() + "printer: test-printer\naux: test-ghost\n");
    CHECK(!g.done(e));
    CHECK(hasLine({g.reason(e)}, "t.test-seg, line 4: test-ghost is not a known peripheral."));
}

TEST(ac55_build_only_ends_without_items_or_readiness) {
    RigOptions o;
    o.startUp = mt::TestStartUp::kNever;   // never ready: build only does not wait for it
    Rig r(o);
    const std::string path = r.write(header() + "printer: test-printer\nkeys:\n  - key: 2\n");
    const CallId c = r.send("load-preset", Value::object({{"ref", path}, {"buildOnly", true}}));
    CHECK(r.done(c));
    CHECK(pluggedAt(r.machine(), "printer", mt::kPrinter));
    CHECK_EQ(r.ticks(), Ticks{0});
    CHECK(r.machine()->keyEvents().empty());
    CHECK_EQ(r.rules.seg.afterBuilds, 0);
}

TEST(ec2_header_only_ends_at_key_readiness) {
    RigOptions o;
    o.startUp = 3'000;
    Rig r(o);
    const CallId c = r.load(header() + "printer: test-printer\n");
    CHECK(r.done(c));
    CHECK(r.machine()->isKeyReady());
    CHECK(r.ticks() >= 3'000 && r.ticks() < 3'000 + 2'000);   // one run piece at most
}

TEST(ec10_no_key_readiness_fails_before_any_item) {
    RigOptions o;
    o.startUp = mt::TestStartUp::kNever;
    Rig r(o);
    const CallId c = r.load(header() + "keys:\n  - key: 2\n");
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, "t.test-seg, line 2: The test-seg did not report key readiness after it was built."));
    CHECK(r.machine()->keyEvents().empty());   // no step was carried out
    CHECK_EQ(r.rules.seg.afterBuilds, 0);
    CHECK(r.ticks() >= 100'000);   // LIB-18 EC-7: the bound of 1 s
}
