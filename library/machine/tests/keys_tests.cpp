#include "TestHarness.hpp"
#include "TestMachines.hpp"

// Machine keys: down and up at tick counts, unknown keys refused.

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
std::unique_ptr<TestMachine> readyMachine() {
    auto m = buildTestMachine(kSeg);
    if (m) runUntilKeyReady(*m);
    return m;
}
} // namespace

TEST(key_down_up_at_tick_counts) {
    auto m = readyMachine();
    CHECK(m != nullptr);
    if (!m) return;
    m->advance(100);
    const Ticks down = m->tickCount();
    CHECK(m->keyDown(KeyId{"1"}) == KeyCall::Done);
    m->advance(300);
    const Ticks up = m->tickCount();
    CHECK(m->keyUp(KeyId{"1"}) == KeyCall::Done);
    CHECK(m->keyEvents() == (std::vector<KeyEvent>{{KeyId{"1"}, true, down}, {KeyId{"1"}, false, up}}));
    CHECK(m->keysDown().empty());
}

TEST(two_keys_down_together) {
    auto m = readyMachine();
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(m->keyDown(KeyId{"1"}) == KeyCall::Done);
    m->advance(20);
    CHECK(m->keyDown(KeyId{"2"}) == KeyCall::Done);
    CHECK(m->keysDown() == (std::vector<KeyId>{KeyId{"1"}, KeyId{"2"}}));
    m->advance(20);
    CHECK(m->keysDown().size() == 2);   // both stay down until each gets its key up
    m->keyUp(KeyId{"1"});
    CHECK(m->keysDown() == std::vector<KeyId>{KeyId{"2"}});
    m->keyUp(KeyId{"2"});
    CHECK(m->keysDown().empty());
}

TEST(unknown_key_is_refused_state_unchanged) {
    auto m = readyMachine();
    CHECK(m != nullptr);
    if (!m) return;
    m->keyDown(KeyId{"1"});
    const Ticks t = m->tickCount();
    const auto events = m->keyEvents();
    const auto down = m->keysDown();
    const DisplayOutput d = m->readDisplay();
    CHECK(m->keyDown(KeyId{"no-such-key"}) == KeyCall::UnknownKey);
    CHECK(m->keyUp(KeyId{"no-such-key"}) == KeyCall::UnknownKey);
    CHECK(m->keyDown(KeyId{"a"}) == KeyCall::UnknownKey);   // a key of the other model
    CHECK(m->keyEvents() == events);
    CHECK(m->keysDown() == down);
    CHECK_EQ(m->tickCount(), t);
    CHECK(sameDisplay(m->readDisplay(), d));
}

TEST(repeated_down_or_up_changes_nothing) {
    auto m = readyMachine();
    CHECK(m != nullptr);
    if (!m) return;
    m->keyDown(KeyId{"3"});
    const auto events = m->keyEvents();
    const DisplayOutput d = m->readDisplay();
    CHECK(m->keyDown(KeyId{"3"}) == KeyCall::Done);   // already down
    CHECK(m->keyUp(KeyId{"2"}) == KeyCall::Done);     // not down
    CHECK(m->keyEvents() == events);
    CHECK(m->keysDown() == std::vector<KeyId>{KeyId{"3"}});
    CHECK(sameDisplay(m->readDisplay(), d));
}
