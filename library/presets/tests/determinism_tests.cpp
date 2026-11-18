// LIB-6 L11: determinism and the elapsed time of a quick user (AC-11, AC-64).
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
using calcu::sessions::Hosting;

namespace {
struct State {
    std::vector<std::uint8_t> memory, display;
    Ticks ticks = 0;
    std::uint64_t presses = 0;
    std::vector<std::string> log;
};
State stateOf(Rig& r) {
    State s;
    if (auto* m = r.machine()->memory()) s.memory = m->bytes;
    s.display = displayBytes(r.machine()->readDisplay());
    s.ticks = r.ticks();
    s.presses = r.machine()->presses();
    s.log = r.logLines();
    return s;
}
std::string elapsedLine(const std::vector<std::string>& log) {
    for (const auto& l : log)
        if (l.rfind("Elapsed: ", 0) == 0) return l;
    return {};
}
// "Elapsed: m:ss.mmm" in milliseconds.
long long millisOf(const std::string& l) {
    const std::size_t colon = l.find(':', 9);
    return std::stoll(l.substr(9, colon - 9)) * 60'000 + std::stoll(l.substr(colon + 1, 2)) * 1000 +
           std::stoll(l.substr(colon + 4, 3));
}
} // namespace

TEST(ac11_same_preset_twice_same_memory_display_ticks_log) {
    RigOptions o;
    o.program = 25'000;
    Rig r(o);
    const std::string text = header("test-dot") + "a: test-hub\nkeys:\n  - type: abc\n  - sync-clock:\n  - wait: 50ms\n"
                                                  "  - key: b\n  - elapsed-time: report\n";
    r.logLines();
    CHECK(r.done(r.load(text)));
    const State first = stateOf(r);
    r.send("speed", Value::object({{"full", true}}));   // something in between
    CHECK(r.done(r.load(text)));
    const State second = stateOf(r);
    CHECK(first.display == second.display);
    CHECK_EQ(first.ticks, second.ticks);
    CHECK_EQ(first.presses, second.presses);
    CHECK(first.log == second.log);
    CHECK(r.machine()->clock()->read() == r.machine()->clock()->read());
    // test-seg with its memory.
    const std::string seg = header() + "test-memory: { at: 3, bytes: 0102 }\nkeys:\n  - key: 1\n  - key: 2 3\n";
    CHECK(r.done(r.load(seg)));
    const State a = stateOf(r);
    CHECK(r.done(r.load(seg)));
    const State b = stateOf(r);
    CHECK(a.memory == b.memory && a.display == b.display && a.ticks == b.ticks && a.log == b.log);
}

TEST(ac64_elapsed_is_compute_holds_gaps_waits) {
    // test-seg: a program of P ticks after key 1; holds of 1000 ticks and gaps
    // of 500 (the typing profile); idle after 200 ticks of readiness (D-9).
    constexpr Ticks kP = 30'000, kHold = 1'000, kGap = 500, kStretch = 200, kWait = 1'000;
    const std::string text = header() + "keys:\n  - elapsed-time: start\n  - key: 1\n  - wait: 10ms\n  - key: 2\n"
                                        "  - elapsed-time: report\n";
    auto elapsed = [&](Hosting h) {
        RigOptions o;
        o.program = kP;
        o.hosting = h;
        Rig r(o);
        r.logLines();
        const CallId c = r.load(text);
        r.callWhileBusy();
        CHECK(r.done(c));
        return elapsedLine(r.logLines());
    };
    const std::string oneGo = elapsed(Hosting::VirtualTime);
    const std::string paced = elapsed(Hosting::Paced);
    CHECK_EQ(oneGo, paced);   // the host's speed does not change it
    const long long ms = millisOf(oneGo);
    // compute (the program) + the holds + the wait; no more than a gap per
    // press and one idle stretch per waited item (D-9) on top.
    const long long low = static_cast<long long>((kP + kHold + kWait + kHold) / 100);
    const long long high = static_cast<long long>((kP + 2 * (kHold + kGap) + kWait + 2 * kStretch + 100) / 100);
    CHECK(ms >= low);
    CHECK(ms <= high);
}
