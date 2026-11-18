// Determinism: the same input at the same tick counts gives the same keys at
// the same tick counts, at regular speed, at full speed and without a wall clock.
#include "TestHarness.hpp"
#include "TypingRig.hpp"
#include <functional>

using namespace calcu::typing;
using namespace calcu::typing::test;
using calcu::machine::Ticks;

namespace {

// One script of inputs; each is applied once the machine has reached its
// tick, at the tick count the machine stands at then.
struct Step {
    Ticks at;
    std::function<void(TypingRig&)> input;
};

std::vector<Step> script() {
    return {
        {100, [](TypingRig& r) { r.screenDown("a", 1); }},
        {400, [](TypingRig& r) { r.screenUp("a", 1); }},
        {450, [](TypingRig& r) { r.key({"1", "2"}); }},
        {900, [](TypingRig& r) { r.hostDown(letter('b', "KeyB")); }},
        {6000, [](TypingRig& r) { r.hostDown(letter('c', "KeyC", true)); }},
        {6100, [](TypingRig& r) { r.hostUp("KeyB"); }},
        {9000, [](TypingRig& r) { r.paste("3A\xC3\xA9"); }},
        {9500, [](TypingRig& r) { r.screenDown("clr", 2); }},
        {9800, [](TypingRig& r) { r.screenUp("clr", 2); }},
        {12000, [](TypingRig& r) { r.type("21"); }},
    };
}

struct Run {
    std::vector<Ticks> appliedAt;
    std::vector<std::string> log;
};

// Applies the script in the rig's hosting; `atExactly`: the ticks to apply
// each input at (a replay), else wherever the hosting has got to.
Run drive(TypingRig& r, const std::vector<Ticks>* atExactly) {
    Run out;
    const auto s = script();
    for (std::size_t i = 0; i < s.size(); ++i) {
        const Ticks at = atExactly ? (*atExactly)[i] : s[i].at;
        r.runUntil(at);
        if (atExactly) CHECK_EQ(r.ticks(), at);
        out.appliedAt.push_back(r.ticks());
        s[i].input(r);
    }
    r.run(100'000);
    out.log = r.log();
    return out;
}

} // namespace

TEST(same_inputs_same_key_log_paced_full_virtual) {
    // Paced at regular speed, paced at full speed, in virtual time: each run
    // gives the inputs at the ticks it reached; replayed in virtual time at
    // those ticks, the key log is the same, tick for tick.
    for (int hosting = 0; hosting < 3; ++hosting) {
        TypingRig first(hosting == 2 ? calcu::sessions::Hosting::VirtualTime : calcu::sessions::Hosting::Paced);
        if (hosting == 1) first.send("speed", calcu::sessions::Value::object({{"full", true}}));
        const Run a = drive(first, nullptr);
        CHECK(a.log.size() >= 12);
        TypingRig replay(calcu::sessions::Hosting::VirtualTime);
        const Run b = drive(replay, &a.appliedAt);
        CHECK(a.log == b.log);
        TypingRig again(calcu::sessions::Hosting::VirtualTime);
        const Run c = drive(again, &a.appliedAt);
        CHECK(b.log == c.log);
    }
}
