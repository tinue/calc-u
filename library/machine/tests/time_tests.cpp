#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <chrono>
#include <thread>

// Emulated time on a machine: advance() runs whole instructions.

using namespace calcu::machine;
using namespace calcu::machine::test;

TEST(advance_runs_whole_instructions_and_reports_them) {
    auto m = buildTestMachine(kSeg);   // instructions of 3 and 5 ticks in turn
    CHECK(m != nullptr);
    if (!m) return;
    CHECK_EQ(m->advance(1), Ticks{3});
    CHECK_EQ(m->advance(1), Ticks{5});
    CHECK_EQ(m->advance(7), Ticks{8});     // 3 + 5
    CHECK_EQ(m->advance(4), Ticks{8});     // 3 is not enough: one more whole instruction
    CHECK_EQ(m->tickCount(), Ticks{24});
    int wrong = 0;
    for (Ticks ask = 1; ask < 200; ask += 7) {
        const Ticks before = m->tickCount();
        const std::uint64_t instr = m->instructionsRun();
        const Ticks ran = m->advance(ask);
        if (ran < ask || ran >= ask + 5) ++wrong;                 // at least asked, whole instructions
        if (m->tickCount() - before != ran) ++wrong;              // the count grew by what it reported
        const std::uint64_t n = m->instructionsRun() - instr;    // n whole instructions of 3 and 5
        if (ran < 3 * n || ran > 5 * n) ++wrong;
    }
    CHECK_EQ(wrong, 0);
}

TEST(tick_count_never_goes_back) {
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK_EQ(m->tickCount(), Ticks{0});
    Ticks last = 0;
    int back = 0;
    auto read = [&] { if (m->tickCount() < last) ++back; last = m->tickCount(); };
    m->advance(100); read();
    m->keyDown(KeyId{"a"}); read();
    m->advance(40); read();
    m->reset(); read();
    m->advance(400); read();
    m->keyDown(KeyId{"b"}); read();
    m->powerCycle(); read();
    m->advance(4); read();
    m->coldBoot(); read();
    m->advance(1'000); read();
    m->keyUp(KeyId{"a"}); read();
    CHECK_EQ(back, 0);
    CHECK_EQ(last, Ticks{1'544});
    CHECK_EQ(buildTestMachine(kDot)->tickCount(), Ticks{0});
}

TEST(no_advance_no_change) {
    auto m = buildTestMachine(kSeg, std::nullopt, {peripheralAt(testCatalogue(), "printer", kPrinter)});
    CHECK(m != nullptr);
    if (!m) return;
    runUntilKeyReady(*m);
    m->advance(1'234);
    const Ticks t = m->tickCount();
    const DisplayOutput d = m->readDisplay();
    auto* counter = capabilityAs<TestCounter>(*m, CapabilityId{"printer/test.counter"});
    CHECK(counter != nullptr);
    const std::uint64_t c = counter ? counter->count : 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK_EQ(m->tickCount(), t);
    CHECK(sameDisplay(m->readDisplay(), d));
    CHECK(counter && counter->count == c);
}

TEST(one_tick_whatever_clocks_inside) {
    // test-dot runs an inner block at 3/8 of its tick, converted inside.
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK_EQ(m->advance(800), Ticks{800});
    CHECK_EQ(m->tickCount(), Ticks{800});
    CHECK_EQ(m->innerCycles(), std::uint64_t{300});
    CHECK(m->elapsed() == EmulatedTime(800, kDotRate));
}

TEST(advance_by_zero_runs_nothing) {
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    m->advance(10);
    const Ticks t = m->tickCount();
    const std::uint64_t n = m->instructionsRun();
    CHECK_EQ(m->advance(0), Ticks{0});
    CHECK_EQ(m->tickCount(), t);
    CHECK_EQ(m->instructionsRun(), n);
}
