// Harness attach (C59-5 design §6, A1 to A5): the endpoint on a port the
// system picks, a host loop on a thread of its own that calls the session
// about every 5 ms on the real clock (as an app's session thread), the test
// product's session on the test machine `test-seg` (100,000 ticks a
// second), and client sockets in the test.

#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestProduct.hpp"
#include "harness/attach/AttachEndpoint.hpp"
#include "harness/attach/AttachPort.hpp"
#include "harness/attach/AttachedHarness.hpp"
#include "harness/Run.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/Operation.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/TopicMirror.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace calcu;
using namespace calcu::harness;
using namespace std::chrono_literals;

namespace {

pacing::WallTime wallNow() {
    return std::chrono::duration_cast<pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
}

// The app: its own frontend listening, the session called on a thread of its own.
struct TestApp final : AttachInbox, sessions::MessageSink {
    test::TestProduct product;
    std::unique_ptr<sessions::AppSession> session;
    std::unique_ptr<AttachEndpoint> endpoint;
    std::unique_ptr<AttachedHarness> harness;
    std::atomic<bool> suspended{false};
    std::atomic<int> frontendMessages{0};
    std::atomic<int> asks{0};
    std::atomic<int> replies{0};
    sessions::ListenerId frontend = 0;
    std::atomic<bool> stop{false};
    std::mutex mutex;
    std::deque<AttachEvent> events;
    std::deque<std::function<void()>> jobs;
    std::thread loop;

    explicit TestApp(std::uint16_t port = 0) {
        sessions::AppSessionSetup s;
        s.clock = wallNow;
        s.hostTime = [] { return std::chrono::system_clock::now(); };
        s.parts = product.sessionParts({});
        session = std::make_unique<sessions::AppSession>(std::move(s));
        sessions::HostServices fe;
        fe.offered = {sessions::HostService::ClipboardText};
        frontend = session->listen(*this, fe);
        auto m = machine::test::buildTestMachine(machine::test::kSeg);
        pacing::PacingProfile profile;
        profile.slice = std::chrono::microseconds(20'000);
        session->machineSession().setMachine(std::move(m), profile, 0);
        endpoint = std::make_unique<AttachEndpoint>(*this, port);
        harness = std::make_unique<AttachedHarness>(*session, product, *endpoint, [this] { return suspended.load(); },
                                                    product.libraryParts());
    }
    AttachEndpoint::Opened open() { return endpoint->open(); }
    void start() {
        loop = std::thread([this] {
            while (!stop) {
                std::deque<AttachEvent> ev;
                std::deque<std::function<void()>> js;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    ev.swap(events);
                    js.swap(jobs);
                }
                for (auto& j : js) j();
                for (auto& e : ev) harness->handle(e);
                if (!suspended) {
                    session->call(wallNow());
                    harness->afterHostCall();
                }
                std::this_thread::sleep_for(5ms);
            }
        });
    }
    ~TestApp() {
        stop = true;
        if (loop.joinable()) loop.join();
        harness.reset();
        endpoint.reset();
    }
    void post(AttachEvent e) override {
        std::lock_guard<std::mutex> lock(mutex);
        events.push_back(std::move(e));
    }
    // A job on the app's thread, waited for.
    void onApp(std::function<void()> f) {
        std::mutex m;
        std::condition_variable cv;
        bool done = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            jobs.push_back([&] {
                f();
                std::lock_guard<std::mutex> l(m);
                done = true;
                cv.notify_all();
            });
        }
        std::unique_lock<std::mutex> l(m);
        cv.wait_for(l, 5s, [&] { return done; });
    }
    void deliver(const sessions::Message& m) override {
        ++frontendMessages;
        if (std::holds_alternative<sessions::ServiceAsk>(m)) ++asks;
        if (std::holds_alternative<sessions::ReplyMessage>(m)) ++replies;
    }
    machine::test::TestMachine& machine() {
        return static_cast<machine::test::TestMachine&>(*session->machineSession().machine());
    }
};

// A line client on 127.0.0.1.
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
    ~Client() { close(); }
    void close() {
        if (fd >= 0) ::close(fd);
        fd = -1;
    }
    void send(const std::string& line) {
        const std::string d = line + "\n";
        [[maybe_unused]] auto n = ::send(fd, d.data(), d.size(), 0);
    }
    // The next line, or nullopt after `timeout` or at the end.
    std::optional<std::string> line(std::chrono::milliseconds timeout = 3000ms) {
        const auto end = std::chrono::steady_clock::now() + timeout;
        for (;;) {
            const auto nl = buf.find('\n');
            if (nl != std::string::npos) {
                std::string l = buf.substr(0, nl);
                buf.erase(0, nl + 1);
                return l;
            }
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::steady_clock::now());
            if (left.count() <= 0 || fd < 0) return std::nullopt;
            pollfd p{fd, POLLIN, 0};
            if (::poll(&p, 1, static_cast<int>(left.count())) <= 0) return std::nullopt;
            char c[4096];
            const ssize_t n = ::recv(fd, c, sizeof c, 0);
            if (n <= 0) return std::nullopt;
            buf.append(c, static_cast<std::size_t>(n));
        }
    }
    // The next reply (message lines skipped), its result lines included.
    std::vector<std::string> reply(std::vector<std::string>* messages = nullptr, std::chrono::milliseconds t = 3000ms) {
        for (;;) {
            auto l = line(t);
            if (!l) return {};
            if (l->rfind("message ", 0) == 0) {
                if (messages) messages->push_back(*l);
                continue;
            }
            std::vector<std::string> out{*l};
            if (l->rfind("done ", 0) == 0) {
                const int n = std::atoi(l->c_str() + 5);
                for (int i = 0; i < n; ++i)
                    if (auto x = line(t)) out.push_back(*x);
            }
            return out;
        }
    }
    bool closedByPeer(std::chrono::milliseconds t = 2000ms) {
        const auto end = std::chrono::steady_clock::now() + t;
        while (std::chrono::steady_clock::now() < end) {
            pollfd p{fd, POLLIN, 0};
            if (::poll(&p, 1, 50) > 0) {
                char c[256];
                const ssize_t n = ::recv(fd, c, sizeof c, 0);
                if (n == 0) return true;
                if (n > 0) buf.append(c, static_cast<std::size_t>(n));
            }
        }
        return false;
    }
};

} // namespace

TEST(c59_5_ac10_port_from_argument_else_59059) {
    CHECK_EQ(attachPort(std::vector<std::string>{}), std::uint16_t{59059});
    CHECK_EQ(attachPort(std::vector<std::string>{"app", "--attach-port", "59160"}), std::uint16_t{59160});
    CHECK_EQ(attachPort(std::vector<std::string>{"--attach-port=59161"}), std::uint16_t{59161});
    CHECK_EQ(attachPort(std::vector<std::string>{"--attach-port", "nope"}), std::uint16_t{59059});
    CHECK_EQ(attachPort(std::vector<std::string>{"--attach-port", "70000"}), std::uint16_t{59059});
}

TEST(c59_5_ac10_bound_to_loopback_only) {
    TestApp app;
    CHECK(app.open().ok);
    CHECK(app.endpoint->port() != 0);
    // Reached on 127.0.0.1, not on another address of the host: the socket's
    // own address is the loopback one.
    Client c(app.endpoint->port());
    CHECK(c.fd >= 0);
    sockaddr_in peer{};
    socklen_t len = sizeof peer;
    CHECK(::getpeername(c.fd, reinterpret_cast<sockaddr*>(&peer), &len) == 0);
    CHECK_EQ(ntohl(peer.sin_addr.s_addr), static_cast<std::uint32_t>(INADDR_LOOPBACK));
}

TEST(c59_5_ec1_port_in_use_host_runs_and_log_says_so) {
    TestApp first;
    CHECK(first.open().ok);
    TestApp second(first.endpoint->port());
    const auto o = second.open();
    CHECK(!o.ok);
    CHECK(!o.problem.empty());
    second.start();   // the host runs all the same
    const auto t0 = second.machine().tickCount();
    std::this_thread::sleep_for(100ms);
    std::uint64_t t1 = 0;
    second.onApp([&] { t1 = second.machine().tickCount(); });
    CHECK(t1 > t0);
}

TEST(c59_5_ac11_quit_ends_attach_not_host) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    {
        Client c(app.endpoint->port());
        c.send("ticks");
        CHECK(c.reply().at(0) == "done 2");
        c.send("quit");
        CHECK(c.closedByPeer());
    }
    std::this_thread::sleep_for(20ms);   // the app's next turn
    bool attached = true;
    app.onApp([&] { attached = app.harness->attached(); });
    CHECK(!attached);
    Client again(app.endpoint->port());   // the app goes on, and takes the next harness
    again.send("ticks");
    CHECK(again.reply().at(0) == "done 2");
}

TEST(c59_5_ec2_gone_without_quit_as_never_attached) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    {
        Client c(app.endpoint->port());
        c.send("key-down 1");
        CHECK(c.reply().at(0) == "done");
        bool down = false;
        app.onApp([&] { down = !app.machine().keysDown().empty(); });
        CHECK(down);
    }   // closed without quit
    std::this_thread::sleep_for(100ms);
    bool attached = true, down = true;
    app.onApp([&] {
        attached = app.harness->attached();
        down = !app.machine().keysDown().empty();
    });
    CHECK(!attached);
    CHECK(!down);   // the key it held came up
    Client next(app.endpoint->port());
    next.send("ticks");
    CHECK(next.reply().at(0) == "done 2");
}

TEST(c59_5_ec4_second_harness_one_line_and_closed) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client first(app.endpoint->port());
    first.send("ticks");
    CHECK(first.reply().at(0) == "done 2");
    Client second(app.endpoint->port());
    CHECK(second.line() == std::optional<std::string>(AttachEndpoint::kSecondHarness));
    CHECK(second.closedByPeer());
    first.send("ticks");
    CHECK(first.reply().at(0) == "done 2");   // the first stays
}

TEST(c59_5_ac11_command_script_as_in_serve) {
    // The same script through serve and through attach: the same replies,
    // but for the time `wait` reports.
    // (The display and the capabilities differ by setup: the app's machine
    // has run, and serve's session also has the machine-state part.)
    const std::vector<std::string> script = {"key-down 1", "key-up 1", "state test.count", "test count",
                                             "nonsense", "state nope", "wait 2", "key-down"};
    test::TestRun serve({}, machine::test::kSeg);
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    for (const auto& l : script) {
        std::string want = serve.line(l);
        while (!want.empty() && want.back() == '\n') want.pop_back();
        std::string got;
        c.send(l);
        for (const auto& x : c.reply()) got += (got.empty() ? "" : "\n") + x;
        if (got != want) std::printf("  %s: serve [%s] attach [%s]\n", l.c_str(), want.c_str(), got.c_str());
        CHECK(got == want);
    }
}

TEST(c59_5_ac12_every_state_topic_once_on_attach) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    std::this_thread::sleep_for(50ms);
    Client c(app.endpoint->port());
    c.send("ticks");
    std::vector<std::string> messages;
    CHECK(c.reply(&messages).at(0) == "done 2");
    int display = 0, busy = 0, count = 0;
    for (const auto& m : messages) {
        display += m.rfind("message display ", 0) == 0;
        busy += m.rfind("message busy ", 0) == 0;
        count += m.rfind("message test.count ", 0) == 0;
    }
    CHECK_EQ(busy, 1);
    CHECK_EQ(count, 1);
    CHECK(display >= 1);
}

TEST(c59_5_ac12_reply_only_to_sender) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    std::this_thread::sleep_for(50ms);
    Client c(app.endpoint->port());
    c.send("ticks");
    CHECK(c.reply().at(0) == "done 2");
    c.send("key-down 1");
    CHECK(c.reply().at(0) == "done");
    app.onApp([&] {});
    CHECK_EQ(app.replies.load(), 0);   // the app's frontend got none of the harness's replies
}

TEST(c59_5_ac12_host_service_never_asked_of_harness) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    c.send("ticks");
    CHECK(c.reply().at(0) == "done 2");
    bool harnessOffers = true, frontendOffers = false;
    app.onApp([&] {
        harnessOffers = false;
        frontendOffers = app.session->context().hostOffers(sessions::HostService::ClipboardText);
        // The harness states none: only the frontend offers the clipboard.
        harnessOffers = app.session->context().hostOffers(sessions::HostService::FilePicker);
    });
    CHECK(frontendOffers);
    CHECK(!harnessOffers);
}

TEST(c59_5_ac12_frontend_keeps_receiving) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    c.send("key-down 1");
    CHECK(c.reply().at(0) == "done");
    const int before = app.frontendMessages.load();
    c.send("key-up 1");
    CHECK(c.reply().at(0) == "done");
    std::this_thread::sleep_for(200ms);
    CHECK(app.frontendMessages.load() > before);   // the display it changed reaches the app's frontend too
}

TEST(c59_5_ac14_wait_passes_on_the_app_clock) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    const auto w0 = std::chrono::steady_clock::now();
    c.send("wait 300ms");
    const auto r = c.reply(nullptr, 3000ms);
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - w0).count();
    CHECK(r.size() == 3 && r[0] == "done 2");
    CHECK(wall >= 0.29 && wall < 1.0);
    if (r.size() == 3) {
        const long long t = std::atoll(r[1].c_str() + 7);   // "ticks: N"
        CHECK(t >= 30'000 && t < 30'000 + 4'000);           // 300 ms at 100,000 a second, within a slice and a call
    }
}

TEST(c59_5_ac14_wait_at_full_speed_sooner) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    app.onApp([&] { app.session->send(app.frontend, sessions::Command{sessions::kSpeedCommand, sessions::Value::object({{"full", true}})}); });
    const auto w0 = std::chrono::steady_clock::now();
    c.send("wait 2s");
    const auto r = c.reply(nullptr, 5000ms);
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - w0).count();
    CHECK(!r.empty() && r[0] == "done 2");
    CHECK(wall < 1.0);
}

TEST(c59_5_ac14_harness_never_advances_the_machine) {
    TestApp app;   // not started: nobody calls the session
    CHECK(app.open().ok);
    Client c(app.endpoint->port());
    std::this_thread::sleep_for(50ms);
    // Hand the events to the harness without host calls.
    auto drain = [&] {
        std::deque<AttachEvent> ev;
        {
            std::lock_guard<std::mutex> lock(app.mutex);
            ev.swap(app.events);
        }
        for (auto& e : ev) app.harness->handle(e);
    };
    drain();
    while (c.line(100ms)) {
    }   // the state topics of the attach
    const auto t0 = app.machine().tickCount();
    c.send("wait 100ms");
    std::this_thread::sleep_for(100ms);
    drain();
    app.harness->afterHostCall();
    CHECK_EQ(app.machine().tickCount(), t0);
    CHECK(!c.line(200ms));   // no reply, no message: no time passed
}

TEST(c59_5_ec3_command_refused_while_suspended) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    app.suspended = true;
    c.send("ticks");
    CHECK(c.reply().at(0) == std::string("refused: ") + AttachedHarness::kSuspended);
    app.suspended = false;
    c.send("ticks");
    CHECK(c.reply().at(0) == "done 2");
}

TEST(c59_5_ec3_quit_still_works) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    app.suspended = true;
    c.send("quit");
    CHECK(c.closedByPeer());
}

TEST(c59_5_operation_reply_when_it_ends) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    const auto w0 = std::chrono::steady_clock::now();
    c.send("test operation 10000");   // 0.1 s of the test machine, at full speed
    const auto r = c.reply(nullptr, 5000ms);
    CHECK(!r.empty() && r[0] == "done");
    CHECK(std::chrono::steady_clock::now() - w0 < 3s);
}

// LIB-14 D-27: the attached harness has the commands of `calcu serve`, in
// the same order (the attach's own `json` aside).
TEST(attach_registry_matches_serve) {
    TestApp app;
    test::TestProduct serveProduct;
    std::string error;
    auto server = Server::start(serveProduct, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    // By name. The preset steps as commands come from the session's
    // vocabulary (SessionParts); this app has none, so serve's steps that
    // are only steps are left out.
    auto name = [](const std::string& u) { return u.substr(0, u.find(' ')); };
    std::vector<std::string> attached, served;
    for (const auto& u : app.harness->commands().usage()) attached.push_back(name(u));
    for (const auto& u : server->commands().usage()) {
        served.push_back(name(u));
        if (u.find("the preset step") != std::string::npos) continue;
        const bool in = std::find(attached.begin(), attached.end(), name(u)) != attached.end();
        if (!in) std::fprintf(stderr, "  serve only: %s\n", u.c_str());
        CHECK(in);
    }
    for (const auto& a : attached) {
        const bool in = std::find(served.begin(), served.end(), a) != served.end();
        if (!in) std::fprintf(stderr, "  attach only: %s\n", a.c_str());
        CHECK(in);
    }
    for (const char* c : {"preset", "key", "type", "reset", "cold-boot", "wait", "attach", "paper", "media", "open"}) {
        bool found = false;
        for (const auto& u : app.harness->commands().usage()) found = found || u.rfind(c, 0) == 0;
        if (!found) std::fprintf(stderr, "  no %s\n", c);
        CHECK(found);
    }
    server->finish();
}

// LIB-6 AC-34 through attach (D-27): the bare `wait` is the session's
// `wait-idle` on the app's clock (LIB-6's PresetsPart of the test product;
// the test's own second `wait-idle` went in Part E simplification E-3, a
// command has one owner); its reply is the time it took.
TEST(attach_bare_wait_ends_when_idle) {
    TestApp app;
    CHECK(app.open().ok);
    app.start();
    Client c(app.endpoint->port());
    c.send("wait");
    const auto r = c.reply(nullptr, 5000ms);
    CHECK(r.size() == 4 && r[0] == "done 3");
    if (r.size() == 4) {
        const long long t = std::atoll(r[1].c_str() + 7);   // "ticks: N"
        CHECK(t >= 0 && t < 1'000'000);   // the test machine is idle almost at once
        CHECK(r[3] == "hour reached: no");
    }
}
