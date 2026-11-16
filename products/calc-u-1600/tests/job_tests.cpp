// Core/Job.hpp: the coroutine job driver. The equivalence of a preset run
// in parts and in one go is LIB-6's from M-46 (sharp_presets_tests.cpp,
// lib6_ac50_in_parts_against_in_one_go).

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core/Job.hpp"
#include "../core/PC1500/PC1500LcdText.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600LcdText.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"

namespace {

// A fake machine: records every runCycles() call.
struct FakeMachine {
    std::vector<std::uint64_t> calls;
    std::uint64_t runCycles(std::uint64_t n) {
        calls.push_back(n);
        return n + 1;  // overshoot by one, like a real instruction boundary
    }
};

job::Task<std::uint64_t> inner(FakeMachine& m, int n) {
    std::uint64_t total = 0;
    for (int i = 0; i < n; ++i) total += co_await job::run(m, 10 + i);
    co_return total;
}

job::Task<std::uint64_t> outer(FakeMachine& m) {
    std::uint64_t a = co_await inner(m, 3);
    co_await job::run(m, 100);
    std::uint64_t b = co_await inner(m, 2);
    co_return a + b;
}

job::Task<void> throws(FakeMachine& m) {
    co_await job::run(m, 1);
    throw std::runtime_error("boom");
}

TEST(test_driver) {
    FakeMachine straight;
    const std::uint64_t r1 = job::runToCompletion(outer(straight));
    CHECK(r1 == (11 + 12 + 13) + (11 + 12));
    CHECK((straight.calls == std::vector<std::uint64_t>{10, 11, 12, 100, 10, 11}));

    // Sliced: a request is never split, whatever the budget.
    FakeMachine sliced;
    job::Job<std::uint64_t> j(outer(sliced));
    int steps = 0;
    while (!j.step(5)) ++steps;
    CHECK(steps == 5);  // one request per step at budget 5 (each overshoots it)
    CHECK(sliced.calls == straight.calls);
    CHECK(j.result() == r1);

    // A task that never runs the machine completes on the first step.
    job::Job<int> none([]() -> job::Task<int> { co_return 7; }());
    CHECK(none.step(0));
    CHECK(none.result() == 7);

    // Exceptions come out of result().
    FakeMachine m;
    job::Job<void> bad(throws(m));
    while (!bad.step(1000)) {
    }
    bool caught = false;
    try {
        bad.result();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    CHECK(caught);

    // A blocking wrapper called inside a job runs to completion inline.
    FakeMachine nested;
    job::Job<std::uint64_t> wrap([](FakeMachine& fm) -> job::Task<std::uint64_t> {
        const std::uint64_t inside = job::runToCompletion(inner(fm, 2));
        co_await job::run(fm, 50);
        co_return inside;
    }(nested));
    while (!wrap.step(1)) {
    }
    CHECK(wrap.result() == 11 + 12);
    CHECK((nested.calls == std::vector<std::uint64_t>{10, 11, 50}));
}

} // namespace

