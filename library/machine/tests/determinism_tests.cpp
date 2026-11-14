#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <array>
#include <chrono>
#include <thread>

// LIB-1 AC-6 (design §6, T11).

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
// One snapshot of everything a caller can see: tick count, display, sound
// and the capability outputs.
struct Seen {
    Ticks ticks;
    DisplayOutput display;
    std::vector<std::int16_t> sound;
    std::vector<std::uint64_t> capabilityValues;
};

Seen look(TestMachine& m) {
    Seen s{m.tickCount(), m.readDisplay(), {}, {}};
    std::array<std::int16_t, 256> buf{};
    for (std::size_t n; (n = m.takeSound(buf)) > 0;) s.sound.insert(s.sound.end(), buf.begin(), buf.begin() + n);
    for (const auto& e : m.capabilities()) {
        if (auto* c = capabilityAs<TestCounter>(m, e.id)) s.capabilityValues.push_back(c->count);
        else if (auto* g = capabilityAs<TestGeneric>(m, e.id)) s.capabilityValues.push_back(g->instructions);
    }
    return s;
}

// The same calls at the same tick counts; `gapMs` of wall time between them.
std::vector<Seen> script(TestMachine& m, const std::vector<std::string>& keys, int gapMs) {
    std::vector<Seen> seen;
    auto gap = [&] { if (gapMs) std::this_thread::sleep_for(std::chrono::milliseconds(gapMs)); };
    for (std::size_t i = 0; i < keys.size(); ++i) {
        m.advance(97 + 31 * i);
        gap();
        m.keyDown(KeyId{keys[i]});
        seen.push_back(look(m));
        m.advance(53);
        gap();
        m.keyUp(KeyId{keys[i]});
        if (i == 2) m.reset();
        seen.push_back(look(m));
    }
    return seen;
}

bool same(const std::vector<Seen>& a, const std::vector<Seen>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].ticks != b[i].ticks || !sameDisplay(a[i].display, b[i].display) || a[i].sound != b[i].sound ||
            a[i].capabilityValues != b[i].capabilityValues)
            return false;
    return true;
}
} // namespace

TEST(ac6_two_machines_built_alike_agree) {
    const Catalogue& cat = testCatalogue();
    {
        auto a = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "printer", kPrinter)});
        auto b = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "printer", kPrinter)});
        CHECK(a && b);
        if (!a || !b) return;
        const std::vector<std::string> keys{"1", "2", "clr", "3", "0"};
        CHECK(same(script(*a, keys, 0), script(*b, keys, 7)));
    }
    {
        auto a = buildTestMachine(kDot, VariantId{"V2"});
        auto b = buildTestMachine(kDot, VariantId{"V2"});
        CHECK(a && b);
        if (!a || !b) return;
        const std::vector<std::string> keys{"a", "off", "on", "b", "c"};
        const auto sa = script(*a, keys, 3);
        const auto sb = script(*b, keys, 0);
        CHECK(same(sa, sb));
        std::size_t samples = 0;
        for (const auto& s : sa) samples += s.sound.size();
        CHECK(samples > 0);
    }
}
