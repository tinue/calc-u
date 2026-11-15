// LIB-14 L4 (design §6): LoadingRule and NoticeQueue, rules L1, N1 to N4,
// D-7 (spec AC-15, AC-17, EC-1, EC-2, EC-7), on library/testing/'s scripted
// wall clock. Mirrored in shell_model.test.js.
#include "ScriptedClock.hpp"
#include "TestHarness.hpp"
#include "shell/Loading.hpp"
#include "shell/Notices.hpp"
#include <chrono>
#include <string>

using namespace calcu;
using namespace calcu::shell;
using namespace std::chrono_literals;
using sessions::Value;

TEST(ac15_loading_after_500_ms_at_full_speed) {
    testing::ScriptedClock clock;
    LoadingRule rule;
    rule.busy(true, true, clock.now);
    CHECK(!rule.shown(clock.now));
    CHECK(rule.due() && *rule.due() == clock.now + 500ms);
    clock.now += 499ms;
    rule.busy(true, true, clock.now);   // unchanged input keeps the stretch
    CHECK(!rule.shown(clock.now));
    clock.now += 1ms;
    CHECK(rule.shown(clock.now));
    // Hidden at once when it ends, or goes on at regular speed.
    rule.busy(true, false, clock.now);
    CHECK(!rule.shown(clock.now));
    CHECK(!rule.due());
    rule.busy(true, true, clock.now);
    clock.now += 600ms;
    CHECK(rule.shown(clock.now));
    rule.busy(false, false, clock.now);
    CHECK(!rule.shown(clock.now));
    // Full speed outside an operation (the display held, AC-43) is no operation.
    rule.busy(false, true, clock.now);
    clock.now += 1s;
    CHECK(!rule.shown(clock.now));
}

TEST(ec1_short_operation_shows_nothing) {
    testing::ScriptedClock clock;
    LoadingRule rule;
    rule.busy(true, true, clock.now);
    clock.now += 450ms;
    CHECK(!rule.shown(clock.now));
    rule.busy(false, false, clock.now);
    clock.now += 100ms;
    CHECK(!rule.shown(clock.now));
}

TEST(ec7_back_to_full_speed_counts_again) {
    testing::ScriptedClock clock;
    LoadingRule rule;
    rule.busy(true, true, clock.now);
    clock.now += 400ms;
    rule.busy(true, false, clock.now);   // a stretch at regular speed
    clock.now += 50ms;
    rule.busy(true, true, clock.now);    // full again: counts from here
    const auto start = clock.now;
    clock.now += 400ms;
    CHECK(!rule.shown(clock.now));
    clock.now = start + 500ms;
    CHECK(rule.shown(clock.now));
}

TEST(ac17_notices_one_after_the_other) {
    NoticeQueue q;
    CHECK(q.current() == nullptr);
    q.push("The first went wrong.");
    q.push("The second went wrong.");
    q.push("The first went wrong.");   // a notice is shown as it is, also again
    CHECK_EQ(q.current()->text, std::string("The first went wrong."));
    CHECK(!q.current()->question);
    q.close();
    CHECK_EQ(q.current()->text, std::string("The second went wrong."));
    q.close();
    CHECK_EQ(q.current()->text, std::string("The first went wrong."));
    q.close();
    CHECK(q.current() == nullptr);
    q.close();   // nothing to close: nothing happens
    CHECK(q.current() == nullptr);
}

TEST(ec2_notice_over_loading) {
    testing::ScriptedClock clock;
    LoadingRule rule;
    NoticeQueue q;
    rule.busy(true, true, clock.now);
    clock.now += 600ms;
    CHECK(rule.shown(clock.now));
    q.push("A file could not be read.");
    CHECK(q.current() != nullptr);
    CHECK(rule.shown(clock.now));   // both kept
    q.close();                      // OK closes only the notice
    CHECK(rule.shown(clock.now));
    CHECK(q.current() == nullptr);
}

TEST(refusal_equal_to_last_notice_not_repeated) {
    NoticeQueue q;
    q.push("The disk is full.");
    q.refusal("The disk is full.");   // the reply repeats the notice (D-7)
    CHECK_EQ(q.waiting(), std::size_t{1});
    q.refusal("Another operation is running.");
    CHECK_EQ(q.waiting(), std::size_t{2});
    q.refusal("");   // no reason: nothing
    CHECK_EQ(q.waiting(), std::size_t{2});
}

TEST(asks_refusal_is_a_question_yes_resends_confirmed) {
    NoticeQueue q;
    const sessions::Command sent{sessions::CommandName{"media.delete"}, Value::object({{"kind", "card"}, {"name", "A"}})};
    q.refusal("asks: Delete the card A?", sent);
    CHECK(q.current() && q.current()->question);
    CHECK_EQ(q.current()->text, std::string("Delete the card A?"));
    auto again = q.answer(true);
    CHECK(again.has_value());
    CHECK_EQ(again->name.value, std::string("media.delete"));
    CHECK(again->args == Value::object({{"kind", "card"}, {"name", "A"}, {"confirmed", true}}));
    CHECK(q.current() == nullptr);
    q.refusal("asks: Replace it?", sent);
    CHECK(!q.answer(false).has_value());   // No sends nothing
    CHECK(q.current() == nullptr);
}
