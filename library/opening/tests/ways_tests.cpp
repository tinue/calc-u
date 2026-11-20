// The ways in and the drag, at the session; each frontend's wiring is
// tested in its product.
#include "OpenRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::opening;
using namespace calcu::opening::test;

TEST(every_way_in_same_outcome_but_limit) {
    const Way ways[] = {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness};
    for (Way w : ways) {
        OpenRigOptions o;
        o.model = "test-dot";
        o.adjust = [](OpenSetup& s) { s.maxBytes[Way::Drop] = 1000; };
        OpenRig r(o);
        // The same three files by every way: a kind, a medium, nothing.
        const auto* k = r.opened(file("p.lst", "abc"), w);
        CHECK(k && k->done && r.replyOf(k) && r.replyOf(k)->as == OpenedAs::FileKind);
        CHECK(r.listing.ways.size() == 1 && r.listing.ways[0] == w);   // the kind knows the way in
        calcu::testing::TempFolder outside("calcu-open-ways");
        outside.write("w.testdisk.yaml", disk("W", "abc").text());
        const auto* m = r.opened(r.onDisk(outside, "w.testdisk.yaml"), w);
        CHECK(m && m->done && r.replyOf(m) && r.replyOf(m)->detail == "disk");
        const auto* n = r.opened(file("x.jpg", "abc"), w);
        CHECK(n && !n->done && n->reason == "Test-Product cannot open x.jpg.");
        // Only the limit depends on the way in.
        const auto* big = r.opened(file("big.lst", std::string(1001, 'x')), w);
        CHECK(big && big->done == (w != Way::Drop));
    }
}

TEST(would_take_by_tests_1_to_3_and_kind_names) {
    OpenRig r;
    CHECK(r.wouldTake("game.test-seg"));         // 2
    CHECK(r.wouldTake("GAME.TEST-DOT"));
    CHECK(r.wouldTake("w.testdisk.yaml"));      // 3
    CHECK(r.wouldTake("prog.lst"));          // a kind's name
    CHECK(!r.wouldTake(".game.test-seg"));       // 1c
    CHECK(!r.wouldTake("photo.jpg"));        // nothing by its name
}

TEST(would_take_ignores_content) {
    OpenRig r;
    // A blob and a preset by its model line are told by their content: a drag of their name is not taken.
    CHECK(!r.wouldTake("data.bin"));
    CHECK(!r.wouldTake("notes.txt"));
}

TEST(size_at_drag_held_against_limit) {
    OpenRigOptions o;
    o.adjust = [](OpenSetup& s) { s.maxBytes[Way::Drop] = 100; };
    OpenRig r(o);
    CHECK(r.wouldTake("game.test-seg", 100));
    CHECK(!r.wouldTake("game.test-seg", 101));
    CHECK(r.wouldTake("game.test-seg"));   // no size known: by the name
}
