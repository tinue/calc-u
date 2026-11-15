#include "MediaFixture.hpp"
#include "TestHarness.hpp"

// LIB-7 L5 (design §6): the pickers and choosing (AC-8, AC-9, AC-10).

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions slotFixture() {
    FixtureOptions o;
    o.prepare = [](auto& s, auto& b) {
        b.write("modules/Zeta.testmodule.yaml", moduleFile("Zeta", "z").text());
        b.write("modules/Alpha.testmodule.yaml", moduleFile("Alpha", "a").text());
        TestFile mine = moduleFile("Mine", "m", true);
        mine.description = "My own template";
        s.write("Mine.testmodule.yaml", mine.text());
        s.write("b.testmodule.yaml", moduleFile("B", "b").text());
        s.write("a.testmodule.yaml", moduleFile("a", "a").text());
    };
    return o;
}

FixtureOptions onDot() {
    FixtureOptions o;
    o.model = "test-dot";
    o.hosting = sessions::Hosting::Paced;
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "").text());
        s.write("Work.testdisk.yaml", disk("Work", "w").text());
        s.write("Other.testdisk.yaml", disk("Other", "o").text());
        TestFile old = disk("Old", "x");
        old.version = 1;
        s.write("Old.testdisk.yaml", old.text());
    };
    return o;
}

Value choose(const std::string& place, const std::string& entry) { return Value::object({{"place", place}, {"entry", entry}}); }

std::vector<std::string> labels(const PlaceState* p) {
    std::vector<std::string> out;
    for (const auto& e : p->picker) out.push_back(std::string(groupText(e.group)) + ":" + e.label);
    return out;
}

} // namespace

TEST(ac8_order_empty_templates_instances_modules) {
    MediaFixture f(slotFixture());
    const std::vector<std::string> want = {"empty:",
                                           "bundled-template:Alpha",
                                           "bundled-template:Test memory module",
                                           "bundled-template:Zeta",
                                           "user-template:Mine",
                                           "instance:a",
                                           "instance:B",
                                           "module:Test module",
                                           "module:Test ROM module"};
    CHECK(labels(f.place("slot-1")) == want);
}

TEST(ac8_tooltip_is_description) {
    MediaFixture f(slotFixture());
    for (const auto& e : f.place("slot-1")->picker) {
        if (e.label == "Mine") CHECK(e.tooltip == std::optional<std::string>("My own template"));
        if (e.label == "Zeta") CHECK(!e.tooltip);
    }
}

TEST(ac8_listing_label_and_sort_key) {
    FixtureOptions o = slotFixture();
    // As Calc-U-59's modules: the description as the label, no tooltip, by menu-sort.
    o.listing.label = [](const machine::PeripheralDescriptor& d) { return "Described " + d.name; };
    o.listing.tooltip = [](const machine::PeripheralDescriptor&) { return std::nullopt; };
    o.listing.sortKey = [](const machine::PeripheralDescriptor& d) { return d.id == mt::kRomModule ? 1 : 2; };
    MediaFixture f(o);
    const auto l = labels(f.place("slot-1"));
    CHECK(l.size() == 9 && l[7] == "module:Described Test ROM module" && l[8] == "module:Described Test module");
    CHECK(!f.place("slot-1")->picker[7].tooltip);
}

TEST(ac9_outdated_entry_disabled_with_hint) {
    MediaFixture f(onDot());
    const PlaceState* p = f.place("disk");
    const auto it = std::find_if(p->picker.begin(), p->picker.end(), [](const PickerEntry& e) { return e.label == "Old"; });
    CHECK(it != p->picker.end());
    if (it == p->picker.end()) return;
    CHECK(!it->canChoose);
    CHECK(it->hint == std::optional<std::string>("The test disk format version 1 is outdated: the current version is 2, "
                                                  "described in library/media/tests/TestKinds.hpp."));
    CHECK(f.state()->kindState("test-disk")->instances.size() == 3);
    CHECK(!f.ok("media.choose", choose("disk", "saved:Old.testdisk.yaml")) || !f.place("disk")->holds);
}

TEST(ac9_outdated_logged_once_per_start) {
    MediaFixture f(onDot());
    const auto lines = f.logLines();
    CHECK(std::count_if(lines.begin(), lines.end(), [](const std::string& l) {
              return l.find("Old.testdisk.yaml is outdated") != std::string::npos;
          }) == 1);
    CHECK(f.ok("media.refresh"));
    CHECK(!hasLine(f.logLines(), "Old.testdisk.yaml"));
}

TEST(ac10_old_medium_written_first) {
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("unsaved yet");
    f.call();   // within the half second: not written yet
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("w"));
    CHECK(f.ok("media.choose", choose("disk", "saved:Other.testdisk.yaml")));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("unsaved yet"));
    CHECK_EQ(f.drive()->bytes(), std::string("o"));
}

TEST(ac10_holding_place_new_medium_in_at_once) {
    MediaFixture f(onDot());
    const auto generation = f.session->machineSession().generation();
    const auto ticks = f.machine()->tickCount();
    const CallId c = f.send("media.choose", choose("disk", "saved:Work.testdisk.yaml"));
    CHECK(f.done(c) && !f.session->busy());   // a plain command, no operation, no power cycle
    CHECK_EQ(f.drive()->bytes(), std::string("w"));
    CHECK(f.session->machineSession().generation() == generation && f.machine()->tickCount() == ticks);
}

TEST(ac10_module_slot_is_a_replace_by_lib18) {
    MediaFixture f(slotFixture());
    CHECK(f.done(f.load(header() + "slot-1: test-module\n")));
    const int unplugs = f.machine()->unplugCalls;
    CHECK(f.ok("media.choose", choose("slot-1", "saved:b.testmodule.yaml")));
    CHECK(f.module() && f.module()->bytes() == "b");
    CHECK_EQ(f.machine()->unplugCalls, unplugs + 1);   // one replace
    CHECK(f.machine()->operationsTold(startup::test::at({"slot-1"})) == 1);   // the power cycle of the slot (LIB-18 AC-21, AC-29)
    CHECK(f.machine()->isKeyReady());
    const PlaceState* p = f.place("slot-1");
    CHECK(p->holds && p->holds->name == "B" && !p->holds->isTemplate);
}

TEST(ac10_unreadable_leaves_place_empty_with_notice) {
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.writeSaved("Other.testdisk.yaml", "garbage\n");   // after the picker was filled
    CHECK(f.ok("media.choose", choose("disk", "saved:Other.testdisk.yaml")));
    CHECK(!f.place("disk")->holds && !f.drive()->side());
    const auto n = f.notices();
    CHECK(hasLine(n, "The test disk “Other” cannot be read: It is not a test-disk file."));
}
