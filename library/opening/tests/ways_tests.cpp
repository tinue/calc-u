// The ways in and the drag, at the session; each frontend's wiring is
// tested in its product.
#include "TestHarness.hpp"
#include "TestOpeners.hpp"

using namespace calcu::opening;
using namespace calcu::opening::test;

TEST(every_way_in_same_outcome_but_limit) {
    const Way ways[] = {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness};
    for (Way w : ways) {
        OpenRig r([](OpenSetup& s) { s.maxBytes[Way::Drop] = 4; });
        r.rig.build(mt::kDot);
        // The same three files by every way: a kind, a medium, nothing.
        const auto* k = r.opened(file("p.lst", "abc"), w);
        CHECK(k && k->done && r.replyOf(k) && r.replyOf(k)->as == OpenedAs::FileKind);
        CHECK(r.listing.ways.size() == 1 && r.listing.ways[0] == w);   // the kind knows the way in
        const auto* m = r.opened(file("w.tdisk.yaml", "abc", "/m/w.tdisk.yaml"), w);
        CHECK(m && m->done && r.replyOf(m) && r.replyOf(m)->detail == "drive");
        const auto* n = r.opened(file("x.jpg", "abc"), w);
        CHECK(n && !n->done && n->reason == "Test-Product cannot open x.jpg.");
        // Only the limit depends on the way in.
        const auto* big = r.opened(file("big.lst", "abcdefgh"), w);
        CHECK(big && big->done == (w != Way::Drop));
    }
}

TEST(would_take_by_tests_1_to_3_and_kind_names) {
    OpenRig r;
    r.rig.build();
    CHECK(r.wouldTake("game.tseg"));         // 2
    CHECK(r.wouldTake("GAME.TDOT"));
    CHECK(r.wouldTake("w.tdisk.yaml"));      // 3
    CHECK(r.wouldTake("prog.lst"));          // a kind's name
    CHECK(!r.wouldTake(".game.tseg"));       // 1c
    CHECK(!r.wouldTake("photo.jpg"));        // nothing by its name
}

TEST(would_take_ignores_content) {
    OpenRig r;
    r.rig.build();
    // A blob and a preset by its model line are told by their content: a drag of their name is not taken.
    CHECK(!r.wouldTake("data.bin"));
    CHECK(!r.wouldTake("notes.txt"));
}

TEST(size_at_drag_held_against_limit) {
    OpenRig r([](OpenSetup& s) { s.maxBytes[Way::Drop] = 100; });
    r.rig.build();
    CHECK(r.wouldTake("game.tseg", 100));
    CHECK(!r.wouldTake("game.tseg", 101));
    CHECK(r.wouldTake("game.tseg"));   // no size known: by the name
}
