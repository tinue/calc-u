#include "MediaFixture.hpp"
#include "TestHarness.hpp"

// LIB-7 L4 (design §6): the topic `media` (AC-7).

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {
FixtureOptions onDot() {
    FixtureOptions o;
    o.model = "test-dot";
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "", false).text());
        s.write("Work.testdisk.yaml", disk("Work", "w").text());
        s.write("Mine.testdisk.yaml", disk("Mine", "m", true).text());
    };
    return o;
}
Value choose(const std::string& place, const std::string& entry) { return Value::object({{"place", place}, {"entry", entry}}); }
} // namespace

TEST(ac7_kinds_templates_instances_with_place) {
    MediaFixture f(onDot());
    const MediaState* s = f.state();
    CHECK(s && s->kinds.size() == 3);
    const KindState* cards = s->kindState("test-card");
    CHECK(cards && cards->templates.size() == 1 && cards->templates[0].name == "Fresh" &&
          cards->templates[0].origin == Origin::Implicit);
    const KindState* disks = s->kindState("test-disk");
    CHECK(disks && disks->templates.size() == 2 && disks->instances.size() == 1);
    CHECK(disks->instances[0].name == "Work" && !disks->instances[0].inPlace && !disks->instances[0].outdated);
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    disks = f.state()->kindState("test-disk");
    CHECK(disks->instances[0].inPlace == std::optional<std::string>("disk"));
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    disks = f.state()->kindState("test-disk");
    CHECK(!disks->instances[0].inPlace && disks->templates[0].inPlace == std::optional<std::string>("disk"));
}

TEST(ac7_place_holds_and_waits) {
    MediaFixture f(onDot());
    CHECK(!f.place("disk")->holds);
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    const PlaceState* p = f.place("disk");
    CHECK(p->holds && p->holds->name == "Work" && !p->holds->isTemplate && p->holds->side == std::optional<int>(0));
    CHECK(!p->waits);
    f.drive()->waits = true;
    CHECK(f.ok("media.refresh"));
    CHECK(f.place("disk")->waits);
}

TEST(ac7_reread_after_own_write_and_on_refresh) {
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    CHECK(f.ok("media.name-and-save", Value::object({{"place", "disk"}, {"name", "Fresh copy"}})));
    const KindState* disks = f.state()->kindState("test-disk");   // listed without a refresh
    CHECK(disks && disks->instances.size() == 2);
    CHECK(std::any_of(disks->instances.begin(), disks->instances.end(),
                      [](const KindState::Item& i) { return i.name == "Fresh copy" && i.inPlace == std::optional<std::string>("disk"); }));
}

TEST(ac7_outside_change_not_seen_until_refresh) {
    MediaFixture f(onDot());
    f.saveFile("Later.testdisk.yaml", disk("Later", "l"));
    f.call();
    f.ok("media.eject", Value::object({{"place", "disk"}}));
    CHECK(f.state()->kindState("test-disk")->instances.size() == 1);   // not watched
    CHECK(f.ok("media.refresh"));                                      // a picker or the list opened
    CHECK(f.state()->kindState("test-disk")->instances.size() == 2);
}
