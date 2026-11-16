// LIB-14 L8 (design §6, §3.9; AC-44, built here and used in M-49): harness
// attach hosted by the Qt6 SessionHost on the GUI thread, with LIB-11's
// test product (library/harness/tests/TestProduct.*) on test-seg (100,000
// ticks a second) and a line client on 127.0.0.1. Built only with
// CALCU_ATTACH (POSIX trees with the tests; qt6/CMakeLists.txt).
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestProduct.hpp"
#include "harness/attach/AttachEndpoint.hpp"
#include "qt6/SessionHost.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "shell/Inputs.hpp"
#include <QCoreApplication>
#include <QStringList>
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace calcu;
using namespace std::chrono_literals;

namespace {

void pump() {
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

// The app: the test product's session hosted by a SessionHost, its timer on.
struct QtApp {
    harness::test::TestProduct product;
    std::unique_ptr<sessions::AppSession> session;
    std::unique_ptr<qt6::SessionHost> host;
    std::vector<std::string> log;
    explicit QtApp(bool start = true) {
        sessions::AppSessionSetup s;
        s.clock = pacing::steadyWallTime;
        s.hostTime = [] { return std::chrono::system_clock::now(); };
        for (auto* p : product.sessionParts()) s.parts.push_back(p);
        session = std::make_unique<sessions::AppSession>(std::move(s));
        pacing::PacingProfile profile;
        profile.slice = std::chrono::microseconds(20'000);
        session->machineSession().setMachine(machine::test::buildTestMachine(machine::test::kSeg), profile, 0);
        host = std::make_unique<qt6::SessionHost>(*session, sessions::HostServices{});
        host->addMessageHandler([this](const sessions::Message& m) {
            if (const auto* s = std::get_if<sessions::StreamMessage>(&m); s && s->topic == sessions::kLogTopic)
                for (const auto& p : s->appended) log.push_back(shell::read::text(p->toValue(), "text"));
        });
        if (start) host->start();
    }
    ~QtApp() {
        host.reset();
        session.reset();
    }
    machine::Ticks ticks() { return session->machineSession().machine()->tickCount(); }
};

// A line client; while it waits, the GUI thread's event loop runs (`pump`).
struct Client {
    int fd = -1;
    std::string buf;
    explicit Client(std::uint16_t port) {
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
            ::close(fd);
            fd = -1;
        }
    }
    ~Client() {
        if (fd >= 0) ::close(fd);
    }
    void send(const std::string& line) {
        const std::string d = line + "\n";
        [[maybe_unused]] auto n = ::send(fd, d.data(), d.size(), 0);
    }
    std::optional<std::string> line(std::chrono::milliseconds timeout, bool runLoop = true) {
        const auto end = std::chrono::steady_clock::now() + timeout;
        for (;;) {
            const auto nl = buf.find('\n');
            if (nl != std::string::npos) {
                std::string l = buf.substr(0, nl);
                buf.erase(0, nl + 1);
                return l;
            }
            if (std::chrono::steady_clock::now() >= end || fd < 0) return std::nullopt;
            if (runLoop) pump();
            pollfd p{fd, POLLIN, 0};
            if (::poll(&p, 1, 5) > 0) {
                char c[4096];
                const ssize_t n = ::recv(fd, c, sizeof c, 0);
                if (n <= 0) return std::nullopt;
                buf.append(c, static_cast<std::size_t>(n));
            }
        }
    }
    std::vector<std::string> reply(std::chrono::milliseconds t = 3000ms) {
        for (;;) {
            auto l = line(t);
            if (!l) return {};
            if (l->rfind("message ", 0) == 0) continue;
            std::vector<std::string> out{*l};
            if (l->rfind("done ", 0) == 0) {
                const int n = std::atoi(l->c_str() + 5);
                for (int i = 0; i < n; ++i)
                    if (auto x = line(t)) out.push_back(*x);
            }
            return out;
        }
    }
};

} // namespace

TEST(qt_attach_host_line_runs_on_gui_thread) {
    QtApp app;
    CHECK(app.host->openAttach(app.product, 0).empty());
    CHECK(app.host->attachOpen());
    Client c(app.host->attachPort());
    CHECK(c.fd >= 0);
    c.send("ticks");
    // Without the GUI thread's event loop nothing is answered: the line runs there.
    std::this_thread::sleep_for(150ms);
    CHECK(!c.line(50ms, false).has_value());
    const auto r = c.reply();
    CHECK(!r.empty() && r[0] == "done 2");
}

TEST(qt_attach_host_wait_on_app_clock) {
    QtApp app;
    CHECK(app.host->openAttach(app.product, 0).empty());
    Client c(app.host->attachPort());
    const auto w0 = std::chrono::steady_clock::now();
    const auto t0 = app.ticks();
    c.send("wait 200ms");
    const auto r = c.reply();
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - w0).count();
    CHECK(r.size() == 3 && r[0] == "done 2");
    CHECK(wall >= 0.1);   // the app's own host calls let the time pass (a late timer catches up); the harness never advances it
    CHECK(app.ticks() - t0 >= 20'000);
}

TEST(qt_attach_host_port_zero) {
    CHECK_EQ(qt6::SessionHost::attachPortFrom(QStringList{}), std::uint16_t{59059});
    CHECK_EQ(qt6::SessionHost::attachPortFrom(QStringList{"app", "--attach-port", "59160"}), std::uint16_t{59160});
    CHECK_EQ(qt6::SessionHost::attachPortFrom(QStringList{"app", "--attach-port", "0"}), std::uint16_t{0});
    CHECK_EQ(qt6::SessionHost::attachPortFrom(QStringList{"app", "--attach-port=0"}), std::uint16_t{0});
    QtApp app;
    CHECK(app.host->openAttach(app.product, 0).empty());
    CHECK(app.host->attachPort() != 0);   // the port the system picked, for an in-process client
}

TEST(qt_attach_host_bind_fails_logs_and_runs) {
    QtApp first;
    CHECK(first.host->openAttach(first.product, 0).empty());
    const auto port = first.host->attachPort();
    QtApp second;
    const std::string problem = second.host->openAttach(second.product, port);
    CHECK(!problem.empty());
    CHECK(!second.host->attachOpen());
    const auto t0 = second.ticks();
    const auto end = std::chrono::steady_clock::now() + 150ms;
    while (std::chrono::steady_clock::now() < end) {
        pump();
        std::this_thread::sleep_for(5ms);
    }
    CHECK(second.ticks() > t0);   // the app runs on
    bool logged = false;
    for (const auto& l : second.log)
        logged = logged || l.rfind("Attach is not available on port " + std::to_string(port) + ": ", 0) == 0;
    CHECK(logged);
}

// LIB-14 D-27 (M-49): the serve form's commands through the Qt6 host: `key`
// and `type` reply when their keys are up on the app's clock, the startup
// commands and the preset steps are there (a session without LIB-6's part
// refuses them with its reason; Calc-U-1600's scenarios load presets, its
// CalcU1600QtTests lib14_ac44_scenario_through_attach).
TEST(qt_attach_host_preset_key_type) {
    QtApp app;
    CHECK(app.host->openAttach(app.product, 0).empty());
    Client c(app.host->attachPort());
    c.send("wait 50ms");   // the app's first host calls: the typing part has its machine
    auto r = c.reply();
    for (const char* line : {"type 12", "key 1 2"}) {
        c.send(line);
        r = c.reply();
        if (r.empty() || r[0] != "done") std::fprintf(stderr, "  %s: %s\n", line, r.empty() ? "-" : r[0].c_str());
        CHECK(!r.empty() && r[0] == "done");
    }
    // The attach host names no preset vocabulary (no LIB-6 part): the preset
    // commands are refused by the harness; LIB-18's are the session's (the
    // test product's parts outside a run, as a frontend's).
    c.send("preset examples:none.test");
    r = c.reply();
    CHECK(!r.empty() && r[0] == "refused: This build reads no presets of format 2 yet.");
    c.send("cold-boot");
    r = c.reply();
    CHECK(!r.empty() && r[0] == "done");
}
