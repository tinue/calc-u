#include "MediaFixture.hpp"
#include "TestHarness.hpp"

// The pickers and choosing.

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

// The picker of a place as the topic sends it: "<group>:<label>".
std::vector<std::string> labels(const MediaFixture& f, const std::string& place) {
    std::vector<std::string> out;
    const Value v = f.state()->toValue();
    for (const Value& p : *v.find("places")->elements()) {
        if (*p.find("place")->string() != place) continue;
        for (const Value& e : *p.find("picker")->elements())
            out.push_back(*e.find("group")->string() + ":" + *e.find("label")->string());
    }
    return out;
}

} // namespace

TEST(picker_order_empty_templates_instances_modules) {
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
    CHECK(labels(f, "slot-1") == want);
}

TEST(picker_tooltip_is_description) {
    MediaFixture f(slotFixture());
    for (const auto& e : f.place("slot-1")->picker) {
        if (e.label == "Mine") CHECK(e.tooltip == std::optional<std::string>("My own template"));
        if (e.label == "Zeta") CHECK(!e.tooltip);
    }
}

TEST(picker_listing_label_and_sort_key) {
    FixtureOptions o = slotFixture();
    // As Calc-U-59's modules: the description as the label, no tooltip, by menu-sort.
    o.listing.label = [](const machine::PeripheralDescriptor& d) { return "Described " + d.name; };
    o.listing.tooltip = [](const machine::PeripheralDescriptor&) { return std::nullopt; };
    o.listing.sortKey = [](const machine::PeripheralDescriptor& d) { return d.id == mt::kRomModule ? 1 : 2; };
    MediaFixture f(o);
    const auto l = labels(f, "slot-1");
    CHECK(l.size() == 9 && l[7] == "module:Described Test ROM module" && l[8] == "module:Described Test module");
    CHECK(!f.place("slot-1")->picker[7].tooltip);
}

TEST(file_of_another_version_not_listed) {
    MediaFixture f(onDot());
    const PlaceState* p = f.place("disk");
    CHECK(std::none_of(p->picker.begin(), p->picker.end(), [](const PickerEntry& e) { return e.label == "Old"; }));
    CHECK(f.state()->kindState("test-disk")->instances.size() == 2);
    CHECK(!f.ok("media.choose", choose("disk", "saved:Old.testdisk.yaml")));
}

TEST(unreadable_logged_once_per_start) {
    MediaFixture f(onDot());
    const auto lines = f.logLines();
    CHECK(std::count_if(lines.begin(), lines.end(), [](const std::string& l) {
              return l.find("Old.testdisk.yaml cannot be read: Its version is not known.") != std::string::npos;
          }) == 1);
    CHECK(f.ok("media.refresh"));
    CHECK(!hasLine(f.logLines(), "Old.testdisk.yaml"));
}

TEST(choose_writes_old_medium_first) {
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("unsaved yet");
    f.call();   // within the half second: not written yet
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("w"));
    CHECK(f.ok("media.choose", choose("disk", "saved:Other.testdisk.yaml")));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("unsaved yet"));
    CHECK_EQ(f.drive()->bytes(), std::string("o"));
}

TEST(holding_place_new_medium_in_at_once) {
    MediaFixture f(onDot());
    const auto generation = f.session->machineSession().generation();
    const auto ticks = f.machine()->tickCount();
    const CallId c = f.send("media.choose", choose("disk", "saved:Work.testdisk.yaml"));
    CHECK(f.done(c) && !f.session->busy());   // a plain command, no operation, no power cycle
    CHECK_EQ(f.drive()->bytes(), std::string("w"));
    CHECK(f.session->machineSession().generation() == generation && f.machine()->tickCount() == ticks);
}

TEST(module_slot_choice_is_a_replace_by_machine_control) {
    MediaFixture f(slotFixture());
    CHECK(f.done(f.load(header() + "slot-1: test-module\n")));
    const int unplugs = f.machine()->unplugCalls;
    CHECK(f.ok("media.choose", choose("slot-1", "saved:b.testmodule.yaml")));
    CHECK(f.module() && f.module()->bytes() == "b");
    CHECK_EQ(f.machine()->unplugCalls, unplugs + 1);   // one replace
    CHECK(f.machine()->operationsTold(startup::test::at({"slot-1"})) == 1);   // the power cycle of the slot
    CHECK(f.machine()->isKeyReady());
    const PlaceState* p = f.place("slot-1");
    CHECK(p->holds && p->holds->name == "B" && !p->holds->isTemplate);
}

TEST(unreadable_choice_leaves_place_empty_with_notice) {
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.writeSaved("Other.testdisk.yaml", "garbage\n");   // after the picker was filled
    CHECK(f.ok("media.choose", choose("disk", "saved:Other.testdisk.yaml")));
    CHECK(!f.place("disk")->holds && !f.drive()->side());
    const auto n = f.notices();
    CHECK(hasLine(n, "The test disk “Other” cannot be read: It is not a test-disk file."));
}

// A module slot that no kind of medium
// fits, named by the listing, has a picker: the empty slot and the modules,
// by the listing's order; choosing one replaces the module.
TEST(picker_slot_without_media_listed_by_the_listing) {
    FixtureOptions o;
    o.listing.moduleSlots = {machine::PlaceName{"a"}};
    o.listing.label = [](const machine::PeripheralDescriptor& d) { return "Listed " + d.id.value; };
    o.model = "test-dot";
    MediaFixture f(o);
    const PlaceState* a = f.place("a");
    CHECK(a && a->slot && a->kind.empty());
    if (!a) return;
    CHECK(!a->picker.empty() && a->picker.front().group == PickerEntry::Group::Empty);
    bool modules = a->picker.size() > 1;
    for (std::size_t i = 1; i < a->picker.size(); ++i)
        modules = modules && a->picker[i].group == PickerEntry::Group::Module && a->picker[i].label.rfind("Listed ", 0) == 0;
    CHECK(modules);
    if (a->picker.size() < 2) return;
    const std::string entry = a->picker[1].id;
    CHECK(f.ok("media.choose", choose("a", entry)));
    CHECK(f.place("a")->holds.has_value());
    CHECK(f.ok("media.choose", choose("a", "")));   // the empty entry: a detach
    CHECK(!f.place("a")->holds);
    // Without the listing's slot, no place `a`.
    FixtureOptions plain;
    plain.model = "test-dot";
    MediaFixture g(plain);
    CHECK(!g.place("a"));
}
