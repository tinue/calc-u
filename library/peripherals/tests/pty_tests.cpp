// LIB-2 T8: host links on real pseudo-terminals (AC-18, AC-19, AC-20, EC-7).
// Built and run only where CALCU_HAS_PTY is set (macOS, Linux), as the CTest
// entry calcu_library_pty_tests. The test plays the program on the user's
// computer: it opens the serial port file in a temporary folder for serial
// links.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "peripherals/HostLinks.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <string>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

using namespace calcu::machine;
using namespace calcu::machine::test;
using namespace calcu::peripherals;
namespace st = calcu::sessions::test;
namespace fs = std::filesystem;

namespace {
const PlacePath kA{{PlaceName{"a"}}};
const PlacePath kB{{PlaceName{"b"}}};
const char* const kFile = "test-uart.serial";

HostLinkRules uartRule(bool openAtOnce = true) {
    return [openAtOnce](const CapabilityEntry& e) -> std::optional<HostLinkRule> {
        const auto* o = std::get_if<PeripheralOwner>(&e.owner);
        if (o && o->peripheral == kUart) return HostLinkRule{kFile, openAtOnce};
        return std::nullopt;
    };
}

// A folder for serial links of its own, removed at the end.
struct TempFolder {
    std::string path;
    TempFolder() {
        std::string t = (fs::temp_directory_path() / "calcu-pty-XXXXXX").string();
        if (::mkdtemp(t.data())) path = t;
    }
    ~TempFolder() {
        std::error_code ec;
        if (!path.empty()) fs::remove_all(path, ec);
    }
};

// The program on the user's computer: opens the file at 9600 baud, raw, while
// the machine set 1200 (the host link ignores the line settings, AC-18).
int openPeer(const std::string& path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return fd;
    termios t;
    if (::tcgetattr(fd, &t) == 0) {
        ::cfmakeraw(&t);
        ::cfsetispeed(&t, B9600);
        ::cfsetospeed(&t, B9600);
        ::tcsetattr(fd, TCSANOW, &t);
    }
    return fd;
}

// Reads until `n` bytes came or `timeout` passed.
std::vector<std::uint8_t> readPeer(int fd, std::size_t n, std::chrono::milliseconds timeout = std::chrono::seconds(3)) {
    std::vector<std::uint8_t> out;
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (out.size() < n && std::chrono::steady_clock::now() < end) {
        pollfd p{fd, POLLIN, 0};
        if (::poll(&p, 1, 20) <= 0) continue;
        std::uint8_t buf[256];
        const ssize_t got = ::read(fd, buf, sizeof buf);
        if (got > 0) out.insert(out.end(), buf, buf + got);
    }
    return out;
}

bool writePeer(int fd, const std::string& s) { return ::write(fd, s.data(), s.size()) == static_cast<ssize_t>(s.size()); }

std::vector<std::uint8_t> counting(std::size_t from, std::size_t n) {
    std::vector<std::uint8_t> v;
    for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<std::uint8_t>(from + i));
    return v;
}

// Host calls until `done` or about three seconds of wall time.
template <class F> bool callUntil(st::Rig& r, F done) {
    for (int i = 0; i < 300 && !done(); ++i) {
        r.call();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return done();
}

bool isLink(const std::string& p) {
    struct stat s;
    return ::lstat(p.c_str(), &s) == 0 && S_ISLNK(s.st_mode);
}
} // namespace

TEST(ac18_bytes_both_ways_moved_between_advances) {
    TempFolder dir;
    SKIP_UNLESS(!dir.path.empty(), "no temporary folder");
    PeripheralsPart part(uartRule(), dir.path);
    st::RigOptions o;
    o.extraParts = {&part};
    st::Rig r(o);
    auto mm = buildTestMachine(kDot, std::nullopt, {peripheralAt(testCatalogue(), "a", kUart)}, 0);
    TestMachine* m = mm.get();
    part.hostLinks().machineChanged(*m);   // the file exists before the machine sends
    const auto s = part.hostLinks().status();
    CHECK(s.size() == 1 && s[0].open);
    if (s.size() != 1 || !s[0].open) return;
    const int fd = openPeer(s[0].path);
    CHECK(fd >= 0);
    if (fd < 0) return;
    r.m = m;
    r.session->machineSession().setMachine(std::move(mm), {}, 0);
    // Machine → program: every byte, in order, whatever the program's baud rate.
    callUntil(r, [&] { return m->uartAt(kA)->sent >= 30; });
    const auto got = readPeer(fd, 30);
    CHECK(got == counting(0, 30));
    CHECK(m->uartAt(kA)->port.lineSettings().baud == std::optional<std::uint32_t>(1200));
    // Program → machine, moved only with a host call (between two advances).
    CHECK(writePeer(fd, "HELLO"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(m->uartAt(kA)->received.empty());   // no call, nothing moved
    callUntil(r, [&] { return m->uartAt(kA)->received.size() >= 5; });
    CHECK(m->uartAt(kA)->received == std::vector<std::uint8_t>({'H', 'E', 'L', 'L', 'O'}));
    CHECK(m->uartAt(kA)->port.peerLines().dcd);   // the program has written: it is there
    ::close(fd);
}

TEST(ac19_fixed_name_in_the_folder) {
    TempFolder dir;
    SKIP_UNLESS(!dir.path.empty(), "no temporary folder");
    const Catalogue& cat = testCatalogue();
    HostLinks links(uartRule(), dir.path);
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart)}, 0);
    links.machineChanged(*m);
    const std::string want = dir.path + "/" + kFile;
    CHECK(links.status().size() == 1 && links.status()[0].path == want);
    CHECK(isLink(want));
    char target[1024] = {};
    CHECK(::readlink(want.c_str(), target, sizeof target - 1) > 0 && std::strncmp(target, "/dev/", 5) == 0);
    // The same name after a detach, an attach elsewhere, and another machine.
    CHECK(!m->detach(kA, kUart, cat).refused);
    links.machineChanged(*m);
    CHECK(!m->attach(peripheralAt(cat, "b", kUart), cat).refused);
    links.pump(*m);
    auto other = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "b", kUart)}, 0);
    links.machineChanged(*other);
    links.pump(*other);
    CHECK(links.status().size() == 1 && links.status()[0].path == want && isLink(want));
    // Without a folder: the raw device.
    HostLinks bare(uartRule(), std::nullopt);
    bare.machineChanged(*other);
    CHECK(bare.status().size() == 1 && bare.status()[0].path.rfind("/dev/", 0) == 0);
}

TEST(ac20_file_outlives_port_and_reconnects) {
    TempFolder dir;
    SKIP_UNLESS(!dir.path.empty(), "no temporary folder");
    const Catalogue& cat = testCatalogue();
    HostLinks links(uartRule(), dir.path);
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart)}, 0);
    links.machineChanged(*m);
    const std::string file = dir.path + "/" + kFile;
    const int fd = openPeer(file);
    CHECK(fd >= 0);
    if (fd < 0) return;
    m->advance(kDotRate / 10);
    links.pump(*m);
    CHECK(readPeer(fd, 10) == counting(0, 10));
    // The port goes away (a detach): the file stays, the program is not cut off.
    CHECK(!m->detach(kA, kUart, cat).refused);
    links.machineChanged(*m);
    CHECK(links.status().size() == 1 && !links.status()[0].connected && links.status()[0].open);
    CHECK(isLink(file));
    pollfd p{fd, POLLOUT, 0};
    CHECK(::poll(&p, 1, 100) >= 0 && (p.revents & POLLHUP) == 0);
    CHECK(writePeer(fd, "AB"));   // still a terminal that takes bytes
    // A port of the same kind again (another machine): connected again.
    auto next = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "b", kUart)}, 0);
    links.machineChanged(*next);
    CHECK(links.status()[0].connected);
    next->advance(kDotRate / 20);
    links.pump(*next);
    CHECK(readPeer(fd, 5) == counting(0, 5));
    // What the program sent meanwhile reaches the new port.
    for (int i = 0; i < 100 && next->uartAt(kB)->received.size() < 2; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        links.pump(*next);
        next->advance(10);
    }
    CHECK(next->uartAt(kB)->received == std::vector<std::uint8_t>({'A', 'B'}));
    ::close(fd);
}

TEST(ec7_no_reader_machine_not_held_up) {
    TempFolder dir;
    SKIP_UNLESS(!dir.path.empty(), "no temporary folder");
    HostLinks links(uartRule(), dir.path);
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(testCatalogue(), "a", kUart)}, 0);
    links.machineChanged(*m);
    CHECK(links.status().size() == 1 && links.status()[0].open);   // nobody opens the file
    TestUart* u = m->uartAt(kA);
    const auto t0 = std::chrono::steady_clock::now();
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 100'000; ++i) u->port.machineSends(static_cast<std::uint8_t>(i));
        m->advance(kDotRate / 10);
        links.pump(*m);
    }
    const auto took = std::chrono::steady_clock::now() - t0;
    CHECK(took < std::chrono::seconds(2));
    CHECK(m->tickCount() >= kDotRate);
    CHECK_EQ(u->port.sentWaiting(), std::size_t{0});   // taken out every time
}
