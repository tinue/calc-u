#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "media/Conformance.hpp"
#include "sessions/MachineSession.hpp"

// LIB-7 L3 (design §6): the media kind (AC-6) and LIB-1's conformance check
// of the media places (LIB-1 D-1, AC-40).

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {
FixtureOptions onDot() {
    FixtureOptions o;
    o.model = "test-dot";
    return o;
}
} // namespace

TEST(ac6_place_states_kind_passage_sides) {
    MediaFixture seg;
    const PlaceState* reader = seg.place("reader");
    CHECK(reader && reader->kind == "test-card" && reader->passThrough && (reader->sides == std::vector<int>{0, 1}));
    MediaFixture dot(onDot());
    const PlaceState* drive = dot.place("disk");   // its kind's eject word: one place of the kind
    CHECK(drive && drive->kind == "test-disk" && !drive->passThrough && (drive->sides == std::vector<int>{0}));
    CHECK(dot.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    CHECK(dot.place("drive") && dot.place("hub-disk"));   // two of the kind: by their IDs
}

TEST(ac6_insert_eject_side_content_revision) {
    MediaFixture f(onDot());
    MediaPlace& p = *f.drive();
    CHECK(!p.side() && !p.content());
    CHECK(p.insert(contentFrom("x"), 0).ok);
    CHECK(p.side() == std::optional<int>(0) && contentOf(p.content()) == "x");
    const auto r = p.revision();
    f.drive()->write("y");
    CHECK(p.revision() != r && contentOf(p.content()) == "y");
    CHECK_EQ(contentOf(p.eject()), std::string("y"));
    CHECK(!p.side());
    CHECK(!p.insert(std::make_shared<mt::TestModuleDefinition>("other"), 0).ok);   // not the kind's content: a refusal
}

TEST(ac6_waits_for_medium) {
    MediaFixture f;
    f.ok("media.refresh");
    CHECK(!f.place("reader")->waits);
    f.reader()->waits = true;
    f.ok("media.refresh");
    CHECK(f.place("reader")->waits);
    CHECK(f.media->waitsForMedium(*f.machine()));
    CHECK(f.session->machineSession().waitsForInput());   // LIB-3 AC-22
}

TEST(ac6_write_protect_where_the_medium_has_one) {
    MediaFixture f;
    f.saveFile("Bank.testmodule.yaml", moduleFile("Bank", "m", true));
    CHECK(f.done(f.load(header() + "slot-1: { name: Bank }\n")));
    CHECK(f.place("slot-1")->askWriteProtect);   // a module with the switch still off
    MediaFixture d(onDot());
    d.saveFile("Blank.testdisk.yaml", disk("Blank", "", true));
    CHECK(d.ok("media.refresh"));
    CHECK(d.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Blank.testdisk.yaml"}})));
    CHECK(!d.place("disk")->askWriteProtect);
    CHECK(!d.drive()->writeProtected());   // a disk has no switch
}

TEST(ac40_media_place_capability_per_media_place) {
    MediaFixture f(onDot());
    CHECK(f.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    CHECK(checkMediaPlaces(*f.machine(), mt::mediaTestCatalogue()).empty());
    // A machine that does not offer its media places is reported.
    auto bare = mt::buildTestMachine(mt::kSeg);
    const auto lines = checkMediaPlaces(*bare, mt::testCatalogue());
    CHECK(lines.size() == 1 && lines[0] == "media place: reader of the machine has no capability");
}
