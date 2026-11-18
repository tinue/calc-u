// Live input: held keys, keys held together, what live input drops, focus loss.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

namespace {
bool notReady(const TypingTestMachine&) { return false; }
std::size_t count(const std::vector<std::string>& log, const std::string& what) {
    std::size_t n = 0;
    for (const auto& l : log)
        if (l.find(what) != std::string::npos) ++n;
    return n;
}
} // namespace

TEST(live_key_down_at_once_when_not_ready) {
    TypingRig r;
    r.run(100);
    r.m->readiness = notReady;   // a running program
    const std::size_t from = r.m->keyEvents().size();
    const auto t = r.ticks();
    r.screenDown("a");
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    CHECK(r.log(from).size() > 0 && r.log(from)[0] == std::to_string(t) + " a down");
    r.hostDown(digit('7', "Digit7"));   // a host key too, held together with a
    CHECK_EQ(r.log(from).size(), std::size_t(2));
    CHECK(r.log(from).size() > 1 && r.log(from)[1] == std::to_string(t) + " 7 down");
}

TEST(live_key_after_engine_hold_and_gap) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.key({"1"});
    const auto t = r.ticks();
    r.run(100);
    r.screenDown("b");   // inside the engine's press: right after its hold and gap
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    r.run(3000);
    const auto log = r.log(from);
    CHECK(log.size() >= 3);
    if (log.size() < 3) return;
    CHECK_EQ(log[0], std::to_string(t) + " 1 down");
    CHECK(tickOf(log[1]) >= t + 1000 && tickOf(log[1]) < t + 1005);
    CHECK(log[2].find("b down") != std::string::npos);
    CHECK(tickOf(log[2]) >= tickOf(log[1]) + 500 && tickOf(log[2]) < tickOf(log[1]) + 505);
}

TEST(live_drops_waiting_but_finishes_must_finish) {
    TypingRig r;
    r.run(100);
    const std::size_t mark = r.m->keyEvents().size();
    r.paste("\xC3\xA9\xC3\xA9");   // é é: kbii, a, kbii each (the closing kbii must finish)
    r.run(100);                     // the first kbii of the first é is down
    CHECK_EQ(r.log(mark).size(), std::size_t(1));
    CHECK(r.log(mark).size() == 1 && r.log(mark)[0].find("kbii down") != std::string::npos);
    r.screenDown("b");
    r.screenUp("b");
    r.run(20'000);
    const auto log = r.log(mark);
    // kbii down/up, the closing kbii down/up, then b; a and the second é are dropped.
    CHECK_EQ(log.size(), std::size_t(6));
    CHECK_EQ(count(log, "kbii down"), std::size_t(2));
    CHECK_EQ(count(log, " a "), std::size_t(0));
    if (log.size() == 6) CHECK(log[4].find("b down") != std::string::npos);
}

TEST(live_input_stops_a_key_command) {
    TypingRig r;
    r.run(100);
    const auto k = r.key({"1", "2", "3"});
    r.run(200);
    r.screenDown("b");
    r.screenUp("b");
    r.run(10'000);
    const auto* rep = r.reply(k);
    CHECK(rep && !rep->done && rep->reason == kStoppedByKey);
}

TEST(held_until_let_go) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.hostDown(letter('a', "KeyA"));
    r.run(50'000);   // far longer than the minimum
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    r.hostUp("KeyA");
    const auto t = r.ticks();
    CHECK_EQ(r.log(from).size(), std::size_t(2));
    CHECK(r.log(from).size() > 1 && r.log(from)[1] == std::to_string(t) + " a up");
}

TEST(let_go_before_start_is_one_min_press) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.key({"1"});
    r.screenDown("a");
    r.screenUp("a");   // let go before its press started
    r.run(10'000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    if (log.size() == 4) {
        CHECK(log[2].find("a down") != std::string::npos);
        CHECK(tickOf(log[3]) - tickOf(log[2]) >= 1000 && tickOf(log[3]) - tickOf(log[2]) < 1005);
    }
}

TEST(auto_repeat_ignored) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.hostDown(letter('c', "KeyC"));
    HostKeyEvent rep = letter('c', "KeyC");
    rep.autoRepeat = true;
    for (int i = 0; i < 5; ++i) {
        r.run(500);
        r.hostDown(rep);
    }
    r.hostUp("KeyC");
    r.run(2000);
    CHECK_EQ(r.log(from).size(), std::size_t(2));
}

TEST(two_keys_held_together_both_down) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.screenDown("a", 1);
    r.run(300);
    r.screenDown("b", 2);
    CHECK_EQ(r.m->keysDown().size(), std::size_t(2));
    r.run(5000);
    r.screenUp("b", 2);
    CHECK_EQ(r.m->keysDown().size(), std::size_t(1));
    r.run(500);
    r.screenUp("a", 1);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    if (log.size() == 4) {
        CHECK(log[0].find("a down") != std::string::npos && log[1].find("b down") != std::string::npos);
        CHECK(log[2].find("b up") != std::string::npos && log[3].find("a up") != std::string::npos);
    }
}

TEST(engine_never_overlaps) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.key({"1", "2", "3"});
    r.screenDown("a");   // a live key behind... drops them
    r.screenUp("a");
    r.run(100);
    r.key({"1", "2", "3"});
    r.run(20'000);
    const auto log = r.log(from);
    int down = 0;
    for (const auto& l : log) {   // never two keys down at once
        down += l.find(" down") != std::string::npos ? 1 : -1;
        CHECK(down <= 1);
    }
    CHECK_EQ(down, 0);
}

TEST(key_up_of_other_key_does_nothing) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.hostDown(letter('a', "KeyA"));
    r.run(2000);
    r.hostUp("KeyB");   // another key's up
    r.screenUp("a", 9);   // a pointer that holds nothing
    r.run(2000);
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    r.hostUp("KeyA");
    CHECK_EQ(r.log(from).size(), std::size_t(2));
}

TEST(two_holders_one_key_down_until_last) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.screenDown("a", 1);
    r.hostDown(letter('a', "KeyA"));   // the same key of the machine
    r.run(2000);
    r.screenUp("a", 1);
    r.run(2000);
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    r.hostUp("KeyA");
    CHECK_EQ(r.log(from).size(), std::size_t(2));
}

TEST(focus_lost_releases_after_min_drops_waiting) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.hostDown(letter('a', "KeyA"));
    const auto t = r.ticks();
    const auto k = r.key({"1", "2"});   // waits: a is down
    r.run(100);
    r.send("focus-lost");
    CHECK_EQ(r.log(from).size(), std::size_t(1));   // a: its minimum first
    r.run(10'000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK(log.size() == 2 && tickOf(log[1]) >= t + 1000 && tickOf(log[1]) < t + 1005);
    const auto* rep = r.reply(k);
    CHECK(rep && !rep->done && rep->reason == kFocusWasLost);
    CHECK(r.part.engine().idle());
}
