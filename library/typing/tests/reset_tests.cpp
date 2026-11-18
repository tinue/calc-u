// A reset and a new machine: every key the engine holds comes up, nothing waits.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

TEST(reset_power_cycle_cold_boot_replace_all_up_nothing_waits) {
    for (int how = 0; how < 4; ++how) {   // reset, power cycle, cold boot: dropInput; then a new machine
        TypingRig r;
        r.run(100);
        const std::size_t from = r.m->keyEvents().size();
        r.screenDown("a");
        r.hostDown(letter('b', "KeyB"));
        const auto k = r.key({"1", "2"});   // waits: a and b are down
        r.paste("123");
        CHECK_EQ(r.m->keysDown().size(), std::size_t(2));
        if (how < 3) {
            r.session->machineSession().dropInput();   // what machine control and the products' flows call first
            if (how == 0) r.m->reset();
            if (how == 1) r.m->powerCycle();
            if (how == 2) r.m->coldBoot();
            r.session->advanceUnpaced(0);   // the session sends what changed
            CHECK(r.m->keysDown().empty());   // up at once, without the minimum
            CHECK_EQ(r.log(from).size(), std::size_t(4));
        } else {
            r.build();   // replaced: the old machine's keys went with it
            CHECK(r.m->keysDown().empty());
        }
        CHECK(r.part.engine().idle());
        CHECK_EQ(r.part.engine().waiting(), std::size_t(0));
        const auto* rep = r.reply(k);
        CHECK(rep && !rep->done && rep->reason == kStartedOver);
        CHECK(r.held().empty());
        const std::size_t after = r.m->keyEvents().size();
        r.screenUp("a");   // the user's late key-ups do nothing
        r.hostUp("KeyB");
        r.run(20'000);
        CHECK_EQ(r.m->keyEvents().size(), after);
    }
}
