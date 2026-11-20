#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "media/Conformance.hpp"
#include "sessions/MachineSession.hpp"

// The media kind, the conformance check of the media places, and a
// pass-through place's fitted medium (media.arm and media.feed).

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

TEST(media_place_states_kind_passage_sides) {
    MediaFixture seg;
    const PlaceState* reader = seg.place("reader");
    CHECK(reader && reader->kind == "test-card" && reader->passThrough && (reader->sides == std::vector<int>{0, 1}));
    MediaFixture dot(onDot());
    const PlaceState* drive = dot.place("disk");   // its kind's eject word: one place of the kind
    CHECK(drive && drive->kind == "test-disk" && !drive->passThrough && (drive->sides == std::vector<int>{0}));
    CHECK(dot.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    CHECK(dot.place("drive") && dot.place("hub-disk"));   // two of the kind: by their IDs
}

TEST(media_place_insert_eject_side_content_revision) {
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

TEST(media_place_waits_for_medium) {
    MediaFixture f;
    f.ok("media.refresh");
    CHECK(!f.place("reader")->waits);
    f.reader()->waits = true;
    f.ok("media.refresh");
    CHECK(f.place("reader")->waits);
    CHECK(f.session->machineSession().waitsForInput());
}

TEST(write_protect_where_the_medium_has_one) {
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

TEST(media_place_capability_per_media_place) {
    MediaFixture f(onDot());
    CHECK(f.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    CHECK(checkMediaPlaces(*f.machine(), mt::mediaTestCatalogue()).empty());
    // A machine that does not offer its media places is reported.
    auto bare = mt::buildTestMachine(mt::kSeg);
    const auto lines = checkMediaPlaces(*bare, mt::testCatalogue());
    CHECK(lines.size() == 1 && lines[0] == "media place: reader of the machine has no capability");
}

// ── A pass-through place: the binding is the fitted medium (media.arm and
// media.feed) ──

namespace {
FixtureOptions withCards() {
    FixtureOptions o;
    o.prepare = [](auto& s, auto&) {
        s.write("Prog.testcard.yaml", card("Prog", "p").text());
        s.write("Other.testcard.yaml", card("Other", "o").text());
    };
    return o;
}
Value at(const std::string& entry) { return Value::object({{"place", "reader"}, {"entry", entry}}); }
Value side(int s) { return Value::object({{"place", "reader"}, {"side", s}}); }
void pass(MediaFixture& f) {
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    f.ok("media.refresh");
}
// The topic's `passing` of the reader as a frontend reads it.
std::optional<std::int64_t> passingValue(MediaFixture& f) {
    const Value v = f.state()->toValue();
    const Value* places = v.find("places");
    if (!places || !places->elements()) return std::nullopt;
    for (const Value& p : *places->elements())
        if (const Value* w = p.find("place"); w && w->string() && *w->string() == "reader")
            if (const Value* s = p.find("passing"); s && s->integer()) return *s->integer();
    return std::nullopt;
}
} // namespace

TEST(fitted_card_fed_by_each_side_stays_fitted) {
    MediaFixture f(withCards());
    CHECK(f.ok("media.arm", at("saved:Prog.testcard.yaml")));
    CHECK(f.place("reader")->holds && f.place("reader")->holds->name == "Prog" && !f.place("reader")->passing);
    CHECK(!f.reader()->side() && f.reader()->inserts == 0);   // fitting changes nothing in the machine
    for (int s : {0, 1, 0}) {   // either corner, again and again
        CHECK(f.ok("media.feed", side(s)));
        CHECK(f.reader()->side() == std::optional<int>(s) && f.place("reader")->passing == std::optional<int>(s));
        CHECK(f.place("reader")->holds->side == std::optional<int>(s));
        CHECK(passingValue(f) == std::optional<std::int64_t>(s));
        pass(f);
        CHECK(!f.reader()->side() && !f.place("reader")->passing && !passingValue(f));
        CHECK(f.place("reader")->holds && f.place("reader")->holds->name == "Prog" && !f.place("reader")->holds->side);
    }
    CHECK(f.reader()->inserts == 3);
    CHECK(!f.ok("media.feed", side(2)));   // the place's sides only
}

TEST(nothing_fitted_or_fed_while_a_card_passes) {
    MediaFixture f(withCards());
    CHECK(f.ok("media.arm", at("saved:Prog.testcard.yaml")));
    CHECK(f.ok("media.feed", side(1)));
    const std::string passing = "A test card is passing through reader now.";
    CHECK_EQ(f.refusal("media.arm", at("saved:Other.testcard.yaml")), passing);
    CHECK_EQ(f.refusal("media.choose", at("saved:Other.testcard.yaml")), passing);
    CHECK_EQ(f.refusal("media.start-new", Value::object({{"place", "reader"}, {"template", "implicit:Fresh"}, {"name", "New"}})),
             passing);
    CHECK_EQ(f.refusal("media.feed", side(0)), passing);   // the passing card is not pulled out
    CHECK(f.reader()->side() == std::optional<int>(1) && f.reader()->inserts == 1 && f.place("reader")->holds->name == "Prog");
    pass(f);
    CHECK(f.ok("media.arm", at("saved:Other.testcard.yaml")));   // once it has passed, another takes its place
    CHECK(f.place("reader")->holds->name == "Other");
    MediaFixture e(withCards());   // nothing fitted: nothing to feed
    CHECK_EQ(e.refusal("media.feed", side(0)), std::string("Nothing is fitted for reader."));
}

TEST(eject_takes_passing_card_out_still_fitted) {
    MediaFixture f(withCards());
    CHECK(f.ok("media.arm", at("saved:Prog.testcard.yaml")));
    CHECK(f.ok("media.feed", side(0)));
    CHECK(f.ok("media.eject", Value::object({{"place", "reader"}})));   // out of the reader
    CHECK(!f.reader()->side() && f.place("reader")->holds && f.place("reader")->holds->name == "Prog");
    CHECK(f.ok("media.feed", side(1)));   // still fitted: fed again
    pass(f);
    CHECK(f.ok("media.eject", Value::object({{"place", "reader"}})));   // none passing: the fitted card goes
    CHECK(!f.place("reader")->holds);
}

TEST(choose_fits_does_not_feed) {
    MediaFixture f(withCards());
    CHECK(f.ok("media.choose", at("saved:Prog.testcard.yaml")));
    CHECK(f.place("reader")->holds && f.place("reader")->holds->name == "Prog");
    CHECK(!f.reader()->side() && f.reader()->inserts == 0);
    CHECK(f.ok("media.choose", at("saved:Other.testcard.yaml")));   // it takes the place of the other
    CHECK(f.place("reader")->holds->name == "Other" && f.reader()->inserts == 0);
}
