// HostLinks with an injected link factory, on every platform: the rules
// the real host links build on. The real
// pseudo-terminals: pty_tests.cpp.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "peripherals/HostLinks.hpp"
#include "peripherals/SerialPort.hpp"
#include <deque>

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::peripherals;

namespace {
const PlacePath kA{{PlaceName{"a"}}};

// A link that keeps what it is given and hands out what the test queued.
struct FakeLink final : HostLink {
    std::vector<std::uint8_t>* written;
    std::deque<std::uint8_t>* toMachine;
    FakeLink(std::vector<std::uint8_t>* w, std::deque<std::uint8_t>* t) : written(w), toMachine(t) {}
    std::size_t read(std::span<std::uint8_t> out) override {
        std::size_t n = 0;
        for (; n < out.size() && !toMachine->empty(); ++n) {
            out[n] = toMachine->front();
            toMachine->pop_front();
        }
        return n;
    }
    void write(std::span<const std::uint8_t> in) override { written->insert(written->end(), in.begin(), in.end()); }
    bool peerOpen() const override { return true; }
    std::string path() const override { return "/fake/" + std::string("test-uart.serial"); }
    bool moveTo(const std::optional<std::string>&) override { return true; }
};

HostLinkRules uartRule(bool openAtOnce) {
    return [openAtOnce](const CapabilityEntry& e) -> std::optional<HostLinkRule> {
        const auto* o = std::get_if<PeripheralOwner>(&e.owner);
        if (o && o->peripheral == kUart) return HostLinkRule{"test-uart.serial", openAtOnce};
        return std::nullopt;
    };
}
} // namespace

TEST(without_pty_machine_runs_and_bytes_go_nowhere) {
    int made = 0;
    HostLinks links(uartRule(true), std::string("/nowhere"), [&](const std::optional<std::string>&, const std::string&) {
        ++made;
        return std::unique_ptr<HostLink>();   // a platform without pseudo-terminals
    });
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(testCatalogue(), "a", kUart)}, 0);
    CHECK(m != nullptr);
    if (!m) return;
    links.machineChanged(*m);
    auto* port = capabilityAs<SerialPort>(*m, CapabilityId{"a/serial-port"});
    for (int slice = 0; slice < 60; ++slice) {   // one emulated second in slices
        m->advance(kDotRate / 60);
        links.pump(*m);
        std::uint8_t b;
        CHECK(port && port->takeSent(std::span<std::uint8_t>(&b, 1)) == 0);   // drained, dropped
    }
    CHECK(m->tickCount() >= kDotRate);
    CHECK(m->uartAt(kA)->sent >= 100);
    CHECK(m->uartAt(kA)->received.empty());
    CHECK_EQ(made, 1);
    const auto s = links.status();
    CHECK(s.size() == 1 && s[0].fileName == "test-uart.serial" && !s[0].open && s[0].connected && s[0].path.empty());
    // And with no rules at all, nothing happens to a port.
    HostLinks none({}, std::nullopt);
    none.pump(*m);
    CHECK(none.status().empty());
}

TEST(host_links_move_bytes_by_rule_and_rebind) {
    std::vector<std::uint8_t> written;
    std::deque<std::uint8_t> toMachine{'O', 'K'};
    int made = 0;
    HostLinks links(uartRule(false), std::nullopt, [&](const std::optional<std::string>&, const std::string&) {
        ++made;
        return std::make_unique<FakeLink>(&written, &toMachine);
    });
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {}, 0);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->attach(peripheralAt(cat, "a", kUart), cat).refused);
    links.machineChanged(*m);
    CHECK_EQ(made, 0);   // not at once: not used yet (the CE-158's rule)
    m->advance(kDotRate / 20);   // 50 ms: the UART sends, so it is used
    links.pump(*m);
    CHECK_EQ(made, 1);
    CHECK_EQ(written.size(), std::size_t{5});
    m->advance(10);
    CHECK(m->uartAt(kA)->received == std::vector<std::uint8_t>({'O', 'K'}));
    // The port goes away: the link stays, unbound; then it is bound again.
    CHECK(!m->detach(kA, kUart, cat).refused);
    links.machineChanged(*m);
    CHECK(links.status().size() == 1 && !links.status()[0].connected);
    CHECK(!m->attach(peripheralAt(cat, "b", kUart), cat).refused);
    links.pump(*m);
    CHECK(links.status().size() == 1 && links.status()[0].connected);
    CHECK_EQ(made, 1);
}
