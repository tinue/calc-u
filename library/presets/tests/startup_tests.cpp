// The start-up with the real loader: PresetsPart is the
// startup::PresetLoader.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;

namespace {
RigOptions startingWith(const std::string& ref) {
    RigOptions o;
    o.settings = "[preset]\ntest-seg = " + ref + "\n";
    return o;
}
} // namespace

TEST(startup_preset_built_by_the_loader) {
    calcu::testing::TempFolder t("calcu-start");
    t.write("start.test-seg", header() + "printer: test-printer\nkeys:\n  - key: 2 3\n");
    Rig r(startingWith(t / "start.test-seg"));
    CHECK(r.done(r.startCall));
    CHECK_EQ(r.downs("3").size(), std::size_t(1));
    const auto& last = r.startupPart->control().lastBuild();
    CHECK(last && last->origin == calcu::startup::BuildOrigin::StartupPreset);
    CHECK(r.notices().empty());
}

TEST(startup_preset_outdated_gives_default_and_notice) {
    calcu::testing::TempFolder t("calcu-start");
    t.write("old.test-seg", "MODEL: TI-59\nKEYSTROKES:\n");
    Rig r(startingWith(t / "old.test-seg"));
    CHECK(r.done(r.startCall));
    CHECK(r.machine() && r.machine()->plugged().byPlace.empty());   // the built-in default
    const auto n = r.notices();
    CHECK(n.size() == 1 && hasLine(n, "The startup preset of the test-seg could not be loaded: ") &&
          hasLine(n, "earlier release"));
    // A preset of another model is not the startup preset of this one.
    t.write("dot.test-dot", header("test-dot"));
    Rig d(startingWith(t / "dot.test-dot"));
    CHECK(hasLine(d.notices(), "is a preset for the test-dot"));
}

TEST(startup_preset_failing_while_run_keeps_machine) {
    calcu::testing::TempFolder t("calcu-start");
    t.write("fails.test-seg", header() + "printer: test-printer\nkeys:\n  - key: 2\n  - expect: 9\n  - key: 3\n");
    Rig r(startingWith(t / "fails.test-seg"));
    CHECK(r.done(r.startCall));
    bool printer = false;
    for (const auto& e : r.machine()->plugged().byPlace) printer = printer || e.peripheral == mt::kPrinter;
    CHECK(printer);   // the machine as far as it got
    CHECK_EQ(r.downs("2").size(), std::size_t(1));
    CHECK(r.downs("3").empty());
    CHECK(hasLine(r.notices(), "The startup preset of the test-seg stopped: "));
}
