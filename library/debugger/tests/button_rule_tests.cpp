// The rule of the run-control buttons from the table both component sets
// read.
#include "ButtonCases.hpp"
#include "TestHarness.hpp"
#include "debugger/ButtonRule.hpp"

using namespace calcu::debugger;
using namespace calcu::debugger::test;
using std::chrono::milliseconds;

TEST(rule_cases_from_table) {
    const ButtonCases t = readButtonCases();
    CHECK(t.cases.size() >= 7);
    for (const auto& c : t.cases) {
        const ButtonUse got = buttonUse(c.level, c.state ? &*c.state : nullptr);
        CHECK(got == c.use);
    }
}

TEST(step_repeats_four_a_second_while_held) {
    const ButtonCases t = readButtonCases();
    CHECK_EQ(t.repeatMs, 250);
    CHECK_EQ(StepRepeat::kInterval.count(), static_cast<long long>(t.repeatMs));
    StepRepeat s;
    CHECK(s.press(milliseconds(1000)));   // fires on press
    int fired = 1;
    for (int ms = 1001; ms <= 2000; ++ms)
        if (s.tick(milliseconds(ms), true)) ++fired;
    CHECK_EQ(fired, 5);   // at 1250, 1500, 1750, 2000: four a second
    s.release();
    CHECK(!s.tick(milliseconds(2250), true));
    CHECK(!s.held());
    s.press(milliseconds(3000));
    CHECK(!s.tick(milliseconds(3250), false));   // step cannot be used: the repeat stops
    CHECK(!s.held());
}
