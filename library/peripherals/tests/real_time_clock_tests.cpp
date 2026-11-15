// LIB-2 T7: the real-time clock kind, hostDateTime, setClocksFromHost, and
// the session part that sets the clocks (AC-21 to AC-23). test-dot's clock
// keeps every field but the year.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "peripherals/RealTimeClock.hpp"
#include <chrono>
#include <ctime>
#include <thread>

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::peripherals;
namespace st = calcu::sessions::test;

namespace {
RealTimeClock* clockOf(Machine& m) { return capabilityAs<RealTimeClock>(m, CapabilityId{"rtc/real-time-clock"}); }

ClockFields kept(const HostDateTime& h) {   // what test-dot's chip keeps of it
    ClockFields f = fieldsOf(h);
    f.year.reset();
    return f;
}

// The instant of the session rig (TestParts.hpp): 50 * 365 days after 1970.
const auto kRigInstant = std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50));
} // namespace

TEST(ac21_read_and_set_kept_fields) {
    auto m = buildTestMachine(kDot, std::nullopt, {}, 0);
    CHECK(m != nullptr);
    if (!m) return;
    const auto entries = m->capabilities(RealTimeClock::kKind);
    const Owner rtc = ComponentOwner{ComponentId{"rtc"}};
    CHECK(entries.size() == 1 && entries[0].owner == rtc);
    RealTimeClock* c = clockOf(*m);
    CHECK(c != nullptr);
    if (!c) return;
    c->set(ClockFields{2026, 11, 15, 0, 13, 45, 30});
    const ClockFields f = c->read();
    CHECK(!f.year.has_value());   // the chip keeps no year: ignored, not reported
    CHECK(f == (ClockFields{std::nullopt, 11, 15, 0, 13, 45, 30}));
    // A field left empty is left as it is.
    c->set(ClockFields{std::nullopt, std::nullopt, std::nullopt, std::nullopt, 9, std::nullopt, std::nullopt});
    CHECK(c->read() == (ClockFields{std::nullopt, 11, 15, 0, 9, 45, 30}));
    // test-seg has no clock.
    auto seg = buildTestMachine(kSeg);
    CHECK(seg && seg->capabilities(RealTimeClock::kKind).empty());
}

TEST(ac21_host_date_time_is_local_time) {
    const auto t = kRigInstant + std::chrono::milliseconds(1234);
    const HostDateTime h = hostDateTime(t);
    const std::time_t secs = std::chrono::system_clock::to_time_t(kRigInstant) + 1;
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &secs);
#else
    localtime_r(&secs, &local);
#endif
    CHECK(h == (HostDateTime{local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_wday, local.tm_hour,
                             local.tm_min, local.tm_sec, 234}));
}

TEST(ac22_clock_moves_by_emulated_time_only) {
    auto m = buildTestMachine(kDot, std::nullopt, {}, 0);
    CHECK(m != nullptr);
    if (!m) return;
    RealTimeClock* c = clockOf(*m);
    CHECK(c != nullptr);
    if (!c) return;
    c->set(ClockFields{std::nullopt, 11, 15, 0, 23, 59, 30}, 500);   // half a second preloaded
    std::this_thread::sleep_for(std::chrono::milliseconds(50));   // wall time without an advance
    CHECK(c->read() == (ClockFields{std::nullopt, 11, 15, 0, 23, 59, 30}));
    m->advance(kDotRate / 2);   // the half second
    CHECK(c->read().second == std::optional<int>(31));
    m->advance(Ticks{kDotRate} * 29);   // 29 s more: midnight, Monday the 16th
    CHECK(c->read() == (ClockFields{std::nullopt, 11, 16, 1, 0, 0, 0}));
}

TEST(ac23_set_from_host_at_build_and_after_operation) {
    PeripheralsPart part;
    st::RigOptions o;
    o.extraParts = {&part};
    st::Rig r(o);
    auto mm = buildTestMachine(kDot, std::nullopt, {}, 0);
    TestMachine* m = mm.get();
    r.m = m;
    r.session->machineSession().setMachine(std::move(mm), {}, 0);
    r.call();   // the session sees the new machine
    RealTimeClock* c = clockOf(*m);
    CHECK(c != nullptr);
    if (!c) return;
    const ClockFields host = kept(hostDateTime(kRigInstant));
    CHECK(c->read() == host);
    CHECK_EQ(m->clock()->setCalls, 1);
    // Two emulated seconds later the clock is ahead of the (fixed) host time.
    while (m->tickCount() < 2 * Ticks{kDotRate}) r.call();
    CHECK(!(c->read() == host));
    // At the end of an operation, the host's time again.
    r.part.operation = st::runningFor(3 * Ticks{kDotRate});
    r.send("tp.op");
    r.callWhileBusy();
    CHECK(!r.session->busy());
    CHECK(c->read() == host);
    CHECK_EQ(m->clock()->setCalls, 2);
    // A stopped operation ends too.
    r.part.operation = st::runningFor(100 * Ticks{kDotRate});
    r.send("tp.op");
    r.call();
    r.send("tp.stop");
    // (`tp.stop` starts an operation that ends at once, LIB-18 D-6: both ends set it.)
    CHECK(!r.session->busy() && c->read() == host && m->clock()->setCalls == 4);
}
