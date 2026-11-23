// A paused machine: the rest of a hold passes in host time, a waiting press
// waits for the machine to run.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

TEST(held_key_on_paused_machine_comes_up_after_rest_in_host_time) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.screenDown("a");
    const auto t0 = r.ticks();
    r.run(200);   // 2 ms of the 10 ms minimum
    const auto ran = r.ticks() - t0;
    r.session->machineSession().setPaused(true);   // a freeze inside the hold
    r.call(1ms);
    r.screenUp("a");
    const auto frozenAt = r.ticks();
    r.call(5ms);   // less than the rest (10 ms - ran)
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    const auto rest = std::chrono::microseconds((1000 - ran) * 10);
    r.call(std::chrono::duration_cast<calcu::pacing::WallTime>(rest));
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK(log.size() == 2 && tickOf(log[1]) == frozenAt);   // up on the paused machine
    CHECK_EQ(r.ticks(), frozenAt);
}

TEST(at_once_in_virtual_time) {
    TypingRig r(calcu::sessions::Hosting::VirtualTime);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.key({"1"});   // a typed press: its release is asked for already
    r.run(100);
    r.session->machineSession().setPaused(true);
    r.screenDown("b");   // the next command: the part sees the pause; a live key still goes down
    CHECK_EQ(r.log(from).size(), std::size_t(3));   // 1 down, 1 up at once (in virtual time), b down
    r.screenUp("b");
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    CHECK(log.size() == 4 && log[1].find("1 up") != std::string::npos && log[3].find("b up") != std::string::npos);
}

TEST(waiting_press_waits_for_run) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    r.session->machineSession().setPaused(true);
    const std::size_t from = r.m->keyEvents().size();
    const auto k = r.key({"1"});
    for (int i = 0; i < 20; ++i) r.call(16ms);
    CHECK(r.log(from).empty());
    r.session->machineSession().setPaused(false);
    for (int i = 0; i < 20; ++i) r.call(16ms);
    CHECK_EQ(r.log(from).size(), std::size_t(2));
    CHECK(r.reply(k) && r.reply(k)->done);
}

// Paused and typed in one go, with no host call or command of the part
// between (the DAP's set-up): the press waits for the run, it does not go
// down on the paused machine.
TEST(press_typed_right_after_a_pause_waits_for_run) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.session->machineSession().setPaused(true);
    r.part.engine().typeKeys({Press{calcu::machine::KeyId{"1"}}}, Origin::Typed, false, std::nullopt);
    CHECK(r.log(from).empty());
    for (int i = 0; i < 20; ++i) r.call(16ms);
    CHECK(r.log(from).empty());
    r.session->machineSession().setPaused(false);
    for (int i = 0; i < 20; ++i) r.call(16ms);
    CHECK_EQ(r.log(from).size(), std::size_t(2));
}
