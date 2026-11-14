#include "TestHarness.hpp"
#include "TestMachines.hpp"

// LIB-1 AC-11, AC-42 (design §6, T7).

using namespace calcu::machine;
using namespace calcu::machine::test;

TEST(ac11_operations_return_at_once_and_end_at_key_readiness) {
    for (int op = 0; op < 3; ++op) {
        auto m = buildTestMachine(kDot, VariantId{"V2"});
        CHECK(m != nullptr);
        if (!m) return;
        runUntilKeyReady(*m);
        m->advance(1'000);
        CHECK(m->isKeyReady());
        const Ticks t = m->tickCount();
        if (op == 0) m->reset();
        else if (op == 1) m->powerCycle();
        else m->coldBoot();
        // It returned at once: nothing ran inside the call, the start-up is ahead.
        CHECK_EQ(m->tickCount(), t);
        CHECK(!m->isKeyReady());
        // The start-up runs with the advances that follow; the operation is
        // over at key readiness.
        const Ticks ran = runUntilKeyReady(*m);
        CHECK(ran >= kStartUpTicks && ran < kStartUpTicks + 4);
        CHECK(m->isKeyReady());
        CHECK(m->model() == kDot && m->variant() == VariantId{"V2"});
    }
}

TEST(ac42_key_readiness_only_after_start_up) {
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->isKeyReady());
    m->advance(10);
    CHECK(!m->isKeyReady());
    m->advance(kStartUpTicks - 10 - 6);   // 48 ticks in all, still short of 50
    CHECK(m->tickCount() < kStartUpTicks);
    CHECK(!m->isKeyReady());
    runUntilKeyReady(*m);
    CHECK(m->tickCount() >= kStartUpTicks);
    CHECK(m->isKeyReady());
}
