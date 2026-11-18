// The queue: a press waits for the one before it, its gap and, where the
// profile says, key readiness.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

TEST(next_press_after_up_gap_and_readiness) {
    TypingRig r;
    r.run(100);
    bool ready = true;
    r.m->readiness = [&ready](const TypingTestMachine&) { return ready; };
    const std::size_t from = r.m->keyEvents().size();
    const auto k = r.key({"1", "2"});
    const auto t = r.ticks();
    ready = false;   // the machine works on the key
    r.run(3000);     // 1 up at +1000; +1500 the gap is over; still not ready
    CHECK_EQ(r.log(from).size(), std::size_t(2));
    ready = true;
    const auto readyAt = r.ticks();
    r.run(3000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    if (log.size() == 4) {
        CHECK(tickOf(log[1]) >= t + 1000 && tickOf(log[1]) < t + 1005);
        CHECK(tickOf(log[2]) >= readyAt && tickOf(log[2]) < readyAt + 55);   // the next poll
    }
    CHECK(r.reply(k) && r.reply(k)->done);
}

TEST(text_typed_with_profile_cadence) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.paste("1A2");
    const auto t = r.ticks();
    r.run(20'000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(8));
    if (log.size() != 8) return;
    const std::vector<std::string> keysInOrder = {"1 down", "1 up",      "shift down", "shift up",
                                                  "a down", "a up",      "2 down",     "2 up"};
    for (std::size_t i = 0; i < 8; ++i) CHECK(log[i].find(keysInOrder[i]) != std::string::npos);
    auto near = [](calcu::machine::Ticks got, calcu::machine::Ticks want) { return got >= want && got < want + 5; };
    CHECK_EQ(tickOf(log[0]), t);
    CHECK(near(tickOf(log[1]), t + 1000));
    CHECK(near(tickOf(log[2]), tickOf(log[1]) + 500));          // the gap
    CHECK(near(tickOf(log[4]), tickOf(log[3]) + 500 + 300));    // and 300 more after shift
    CHECK(near(tickOf(log[6]), tickOf(log[5]) + 500));
}

TEST(waits_without_limit_until_dropped) {
    TypingRig r;
    r.run(100);
    r.m->readiness = [](const TypingTestMachine&) { return false; };   // a program that never stops
    const std::size_t from = r.m->keyEvents().size();
    const auto k = r.key({"1"});
    r.run(2'000'000);   // 20 s
    CHECK_EQ(r.log(from).size(), std::size_t(0));
    CHECK(r.reply(k) == nullptr);
    CHECK_EQ(r.part.engine().waiting(), std::size_t(1));
    r.screenDown("b");   // live input: what waits is dropped
    const auto* rep = r.reply(k);
    CHECK(rep && !rep->done && rep->reason == kStoppedByKey);
}

TEST(first_press_bound_starts_all_the_same) {
    auto p = testProfile();
    p.queuedWaitsForReadiness = false;   // the PC-1600's
    p.firstPressOfStep = StartRule::KeyReadiness;
    p.firstPressBound = 5000;
    TypingRig r(calcu::sessions::Hosting::VirtualTime, p);
    r.run(100);
    r.m->readiness = [](const TypingTestMachine&) { return false; };
    const std::size_t from = r.m->keyEvents().size();
    const auto k = r.key({"1", "2"});
    const auto t = r.ticks();
    r.run(20'000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    if (log.size() == 4) {
        CHECK(tickOf(log[0]) >= t + 5000 && tickOf(log[0]) < t + 5005);
        CHECK(tickOf(log[2]) >= tickOf(log[1]) + 500 && tickOf(log[2]) < tickOf(log[1]) + 505);   // no readiness
    }
    CHECK(r.reply(k) && r.reply(k)->done);
}
