// LIB-2 T4: emulated time through a peripheral (AC-12, AC-13; peripherals.md
// P-2, I-3).
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
std::vector<std::uint8_t> drain(Machine& m, const CapabilityId& id) {
    std::vector<std::uint8_t> out;
    auto* port = capabilityAs<calcu::peripherals::SerialPort>(m, id);
    if (!port) return out;
    std::uint8_t buf[256];
    for (std::size_t n; (n = port->takeSent(buf)) > 0;) out.insert(out.end(), buf, buf + n);
    return out;
}
} // namespace

TEST(ac12_same_emulated_seconds_on_two_tick_rates) {
    const Catalogue& cat = testCatalogue();
    auto seg = buildTestMachine(kSeg);
    auto dot = buildTestMachine(kDot);
    CHECK(seg && dot);
    if (!seg || !dot) return;
    // The same peripheral, plugged in on two machines whose ticks differ ten-fold.
    CHECK(!seg->attach(peripheralAt(cat, "aux", kUart), cat).refused);
    CHECK(!dot->attach(peripheralAt(cat, "a", kUart), cat).refused);
    CHECK(kSegRate != kDotRate);
    // One emulated second, in slices of different sizes.
    while (seg->tickCount() < kSegRate) seg->advance(std::min<Ticks>(997, kSegRate - seg->tickCount()));
    while (dot->tickCount() < kDotRate) dot->advance(std::min<Ticks>(16'667, kDotRate - dot->tickCount()));
    CHECK(seg->elapsed().seconds().count() < 1.001 && dot->elapsed().seconds().count() < 1.001);
    const auto fromSeg = drain(*seg, CapabilityId{"aux/serial-port"});
    const auto fromDot = drain(*dot, CapabilityId{"a/serial-port"});
    CHECK_EQ(fromSeg.size(), std::size_t{100});   // one byte every 10 ms
    CHECK(fromSeg == fromDot);
}

TEST(ac13_no_advance_no_output) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart)});
    CHECK(m != nullptr);
    if (!m) return;
    m->advance(100'000);
    const auto first = drain(*m, CapabilityId{"a/serial-port"});
    CHECK(!first.empty());
    const Ticks t = m->tickCount();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));   // wall time passes
    CHECK(drain(*m, CapabilityId{"a/serial-port"}).empty());
    CHECK_EQ(m->tickCount(), t);
}
