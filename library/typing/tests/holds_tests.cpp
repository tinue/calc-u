// LIB-5 design §6, L2: E1, the minimum hold (AC-1).
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

TEST(ac1_early_key_up_carried_out_at_min_hold) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.screenDown("a");
    const auto t0 = r.ticks();
    r.run(10);
    r.screenUp("a");
    CHECK_EQ(r.log(from).size(), std::size_t(1));   // still down
    r.run(2000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK_EQ(log.at(0), std::to_string(t0) + " a down");
    // At the minimum, within one instruction (3 or 5 ticks).
    CHECK(tickOf(log.at(1)) >= t0 + 1000 && tickOf(log.at(1)) < t0 + 1005);
}

TEST(ac1_key_up_never_dropped) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.screenDown("b");
    const auto t0 = r.ticks();
    r.screenUp("b");   // at once
    r.send("focus-lost");
    r.screenUp("b");   // again: holds nothing (EC-1)
    r.run(3000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK(log.size() == 2 && log[1].find("b up") != std::string::npos && tickOf(log[1]) >= t0 + 1000);
    CHECK(r.part.engine().idle());
}

TEST(ac1_typed_press_held_its_cadence_at_least_the_minimum) {
    auto p = testProfile();
    p.typed = Cadence{200, 0};   // shorter than the minimum: the minimum holds
    TypingRig r(calcu::sessions::Hosting::VirtualTime, p);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.key({"2"});
    r.run(3000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK(log.size() == 2 && tickOf(log[1]) - tickOf(log[0]) >= 1000 && tickOf(log[1]) - tickOf(log[0]) < 1005);
}
