// The serial port kind and SerialPortQueues.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "peripherals/SerialPort.hpp"
#include <chrono>
#include <numeric>

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::peripherals;

TEST(bytes_out_and_in_in_order) {
    // Through the capability of a machine: what the test UART sends comes out
    // oldest first; what is put in reaches it in order.
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(testCatalogue(), "a", kUart)});
    CHECK(m != nullptr);
    if (!m) return;
    const auto entries = m->capabilities(SerialPort::kKind);
    const Owner owner = PeripheralOwner{kUart, PlacePath{{PlaceName{"a"}}}};
    CHECK(entries.size() == 1 && entries[0].id == CapabilityId{"a/serial-port"} && entries[0].owner == owner);
    auto* port = capabilityAs<SerialPort>(*m, CapabilityId{"a/serial-port"});
    CHECK(port != nullptr);
    if (!port) return;
    m->advance(kDotRate / 10);   // 100 ms: ten bytes
    std::uint8_t out[4];
    std::vector<std::uint8_t> sent;
    for (std::size_t n; (n = port->takeSent(out)) > 0;) sent.insert(sent.end(), out, out + n);   // four at a time
    std::vector<std::uint8_t> want(10);
    std::iota(want.begin(), want.end(), std::uint8_t{0});
    CHECK(sent == want);
    const std::uint8_t in[] = {'H', 'E', 'L', 'L', 'O'};
    CHECK_EQ(port->putReceived(in), std::size_t{5});
    m->advance(10);
    CHECK(m->uartAt(PlacePath{{PlaceName{"a"}}})->received == std::vector<std::uint8_t>(in, in + 5));
    CHECK(port->used());
}

TEST(line_settings_as_set) {
    SerialPortQueues q;
    CHECK(q.lineSettings() == LineSettings{});   // nothing set: nothing reported
    CHECK(!q.used());
    q.machineSetsLine(LineSettings{1200, 8, true, std::nullopt});
    const LineSettings l = q.lineSettings();
    CHECK(l.baud == std::optional<std::uint32_t>(1200) && l.bitsPerChar == std::optional<int>(8));
    CHECK(l.dtr == std::optional<bool>(true) && !l.rts.has_value());   // a line the model lacks stays empty
    q.setPeerLines(PeerLines{true, true, false, false});
    CHECK(q.peerLines() == (PeerLines{true, true, false, false}));
    q.markUsed();
    CHECK(q.used());
}

TEST(receive_limit_leaves_rest_with_caller) {
    SerialPortQueues q;
    std::vector<std::uint8_t> big(SerialPortQueues::kReceiveLimit + 100, 0x55);
    CHECK_EQ(q.putReceived(big), SerialPortQueues::kReceiveLimit);
    CHECK_EQ(q.putReceived(big), std::size_t{0});
    std::uint8_t b = 0;
    CHECK(q.machineReceives(b) && b == 0x55);
    CHECK_EQ(q.putReceived(big), std::size_t{1});
}

TEST(sending_never_waits) {
    SerialPortQueues q;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1'000'000; ++i) q.machineSends(static_cast<std::uint8_t>(i));   // nobody takes them
    const auto took = std::chrono::steady_clock::now() - t0;
    CHECK_EQ(q.sentWaiting(), std::size_t{1'000'000});
    CHECK(took < std::chrono::seconds(2));
    std::uint8_t first[3];
    CHECK(q.takeSent(first) == 3 && first[0] == 0 && first[1] == 1 && first[2] == 2);
}
