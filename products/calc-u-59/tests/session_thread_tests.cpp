// The Swift frontend's session thread (C59-5 design §6, H1 to H3): the
// library's session with a part made for the test, whose calls take as long
// as it wants, on the real clock.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "../app/SessionThread.hpp"
#include "sessions/LibraryTopics.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::sessions::Command;
using calcu::sessions::CommandName;
using calcu::sessions::ReplyMessage;
using calcu::sessions::Value;
using Clock = std::chrono::steady_clock;

namespace {

struct Shared {
    std::mutex m;
    std::vector<std::string> commands;   // in the order carried out
    std::atomic<int> calls{0};
    std::atomic<int> slowCalls{0};       // the next calls take 200 ms each
    std::set<std::thread::id> threads;   // built, called, destroyed on
    std::vector<std::pair<std::string, int>> commandAtCall;   // command, calls made when it arrived
};

// The commands the tests send (each a command of the test part).
std::vector<std::string> commandNames() {
    std::vector<std::string> out = {"key", "x", "while-suspended"};
    for (int i = 0; i < 500; ++i) {
        out.push_back("a" + std::to_string(i));
        out.push_back("b" + std::to_string(i));
    }
    for (int i = 0; i < 100; ++i) out.push_back("k" + std::to_string(i));
    return out;
}

// The library's app session with a part that notes what reaches it: its
// commands, the suspend, the host calls (which take as long as the test
// wants) and the threads it is built, called and destroyed on.
struct RecordingPart final : calcu::sessions::SessionPart {
    Shared& s;
    explicit RecordingPart(Shared& sh) : s(sh) { note(); }
    ~RecordingPart() { note(); }
    void note() {
        std::lock_guard<std::mutex> l(s.m);
        s.threads.insert(std::this_thread::get_id());
    }
    void record(const std::string& name) {
        note();
        std::lock_guard<std::mutex> l(s.m);
        s.commands.push_back(name);
        s.commandAtCall.emplace_back(name, s.calls.load());
    }
    void attach(calcu::sessions::SessionSetup& setup) override {
        for (const std::string& n : commandNames())
            setup.command(CommandName{n}, [this, n](const Value&, calcu::sessions::SessionContext&) {
                record(n);
                return calcu::sessions::CommandOutcome{calcu::sessions::Done{}};
            });
    }
    void beforeCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override {
        note();
        ++s.calls;
        if (s.slowCalls > 0) {
            --s.slowCalls;
            std::this_thread::sleep_for(200ms);
        }
    }
    void suspending(calcu::sessions::SessionContext&) override { record("suspend"); }
};

std::unique_ptr<calcu::sessions::AppSession> recordingSession(Shared& s) {
    auto part = std::make_shared<RecordingPart>(s);
    calcu::sessions::AppSessionSetup setup;
    setup.clock = steadyWallTime;
    setup.parts = {part.get()};
    setup.owned = std::move(part);
    return std::make_unique<calcu::sessions::AppSession>(std::move(setup));
}

std::unique_ptr<SessionThread> thread(Shared& s, std::atomic<int>* wakes = nullptr) {
    return std::make_unique<SessionThread>([&s] { return recordingSession(s); }, calcu::sessions::HostServices{},
                                           steadyWallTime, [wakes] {
                                               if (wakes) ++*wakes;
                                           });
}

Command cmd(const std::string& n) { return Command{CommandName{n}, Value::object({})}; }

template <class F> bool waitFor(F f, std::chrono::milliseconds t = 3000ms) {
    const auto end = Clock::now() + t;
    while (Clock::now() < end) {
        if (f()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return f();
}

} // namespace

// H1 (AC-6): posts from two threads reach the session in each poster's order.
TEST(c59_5_ac6_posts_reach_the_session_in_order) {
    Shared s;
    auto t = thread(s);
    auto poster = [&](const char* who) {
        for (int i = 0; i < 500; ++i) t->send(cmd(std::string(who) + std::to_string(i)));
    };
    std::thread a(poster, "a"), b(poster, "b");
    a.join();
    b.join();
    CHECK(waitFor([&] {
        std::lock_guard<std::mutex> l(s.m);
        return s.commands.size() == 1000;
    }));
    std::lock_guard<std::mutex> l(s.m);
    int na = 0, nb = 0;
    bool ordered = true;
    for (const auto& c : s.commands) {
        if (c[0] == 'a') ordered &= c == "a" + std::to_string(na++);
        else ordered &= c == "b" + std::to_string(nb++);
    }
    CHECK(ordered);
    CHECK(na == 500 && nb == 500);
}

// H1 (AC-6): the frontend drains the messages in order, with one wake a batch.
TEST(c59_5_ac6_messages_drained_in_order_one_wake_per_batch) {
    Shared s;
    std::atomic<int> wakes{0};
    auto t = thread(s, &wakes);
    std::vector<int> order;
    for (int i = 0; i < 100; ++i)
        t->send(cmd("k" + std::to_string(i)), [&order, i](const ReplyMessage&) { order.push_back(i); });
    std::vector<Delivery> got;
    CHECK(waitFor([&] {
        for (auto& d : t->drain()) got.push_back(std::move(d));
        return got.size() >= 100;
    }));
    for (auto& d : got)
        if (d.onReply) d.onReply(std::get<ReplyMessage>(d.message));   // the frontend runs them, in order
    bool ordered = order.size() == 100;
    for (int i = 0; i < static_cast<int>(order.size()); ++i) ordered &= order[i] == i;
    CHECK(ordered);
    CHECK(wakes.load() >= 1);
    CHECK(wakes.load() <= 100);   // a wake only when the queue was empty
}

// H1 (AC-5): a post returns at once while a call runs.
TEST(c59_5_ac5_post_returns_while_a_call_runs) {
    Shared s;
    auto t = thread(s);
    s.slowCalls = 3;
    std::this_thread::sleep_for(50ms);   // a 200 ms call is running
    const auto t0 = Clock::now();
    t->send(cmd("key"));
    CHECK(Clock::now() - t0 < 1ms);
}

// H2 (AC-4): about 60 calls a second.
TEST(c59_5_ac4_about_60_calls_a_second) {
    Shared s;
    auto t = thread(s);
    std::this_thread::sleep_for(100ms);
    const int c0 = s.calls.load();
    std::this_thread::sleep_for(1000ms);
    const int n = s.calls.load() - c0;
    std::printf("  %d calls in 1 s\n", n);
    CHECK(n >= 57 && n <= 63);
}

// H2 (AC-4): built, called and destroyed on one thread that is not the test's.
TEST(c59_5_ac4_session_built_called_destroyed_on_one_thread) {
    Shared s;
    {
        auto t = thread(s);
        t->send(cmd("x"));
        std::this_thread::sleep_for(100ms);
    }
    std::lock_guard<std::mutex> l(s.m);
    CHECK_EQ(s.threads.size(), size_t{1});
    CHECK(s.threads.count(std::this_thread::get_id()) == 0);
}

// H2 (AC-5): a command sent during a slow call is carried out when it is over (FS 3).
TEST(c59_5_ac5_command_during_slow_call_applied_after_it) {
    Shared s;
    auto t = thread(s);
    std::this_thread::sleep_for(50ms);
    s.slowCalls = 15;
    std::this_thread::sleep_for(100ms);   // inside a 200 ms call
    const int calls = s.calls.load();
    t->send(cmd("key"));
    CHECK(waitFor([&] {
        std::lock_guard<std::mutex> l(s.m);
        return !s.commands.empty();
    }));
    std::lock_guard<std::mutex> l(s.m);
    CHECK(s.commandAtCall[0].second >= calls);   // after the call that was running
    s.slowCalls = 0;
}

// H2: more than 100 ms behind, the thread rebases and does not run calls back to back.
TEST(c59_5_ac4_behind_more_than_100_ms_rebases) {
    Shared s;
    auto t = thread(s);
    std::this_thread::sleep_for(50ms);
    s.slowCalls = 2;   // 400 ms behind
    std::this_thread::sleep_for(450ms);
    const int c0 = s.calls.load();
    std::this_thread::sleep_for(100ms);
    const int n = s.calls.load() - c0;
    CHECK(n <= 8);   // about 6, not a burst of 24
}

// H3 (AC-8): suspended, no calls, commands still carried out.
TEST(c59_5_ac8_suspended_no_calls_commands_still_carried_out) {
    Shared s;
    auto t = thread(s);
    t->suspendCalls();
    CHECK(waitFor([&] { return t->callsSuspended(); }));
    const int c0 = s.calls.load();
    t->send(cmd("while-suspended"));
    std::this_thread::sleep_for(200ms);
    CHECK(s.calls.load() - c0 <= 1);
    std::lock_guard<std::mutex> l(s.m);
    CHECK(s.commands.size() >= 2 && s.commands[0] == "suspend" && s.commands.back() == "while-suspended");
}

// H3 (AC-8): continue calls again, and the session got continue.
TEST(c59_5_ac8_continue_calls_again_and_session_got_continue) {
    Shared s;
    auto t = thread(s);
    t->suspendCalls();
    CHECK(waitFor([&] { return t->callsSuspended(); }));
    t->continueCalls();
    CHECK(waitFor([&] { return !t->callsSuspended(); }));
    const int c0 = s.calls.load();
    std::this_thread::sleep_for(200ms);
    CHECK(s.calls.load() - c0 >= 8);   // the session calls the part again: it got continue
    std::atomic<int> suspended{-1};
    t->post([&](calcu::sessions::AppSession& a) { suspended = a.machineSession().suspended() ? 1 : 0; });
    CHECK(waitFor([&] { return suspended.load() >= 0; }));
    CHECK_EQ(suspended.load(), 0);
}

// The real session on the thread: Calc-U-59 runs, and a key reaches it.
TEST(c59_5_ac4_ti59_on_the_session_thread) {
    calcu::testing::TempFolder folder;
    static calcu::settings::SettingsFile settings = calcu::settings::SettingsFile::defaultsOnly(settings::settingsDecl());
    const TIAppSetup setup = testroms::appSetup(settings, Model::TI59, folder.path());
    std::atomic<std::uint64_t> ticks{0};
    auto t = std::make_unique<SessionThread>(
        [setup] { return makeAppSession(setup, steadyWallTime).session; },
        calcu::sessions::HostServices{}, steadyWallTime, [] {});
    std::this_thread::sleep_for(500ms);
    t->post([&](calcu::sessions::AppSession& a) { ticks = a.machineSession().machine()->tickCount(); });
    CHECK(waitFor([&] { return ticks.load() > 0; }));
    std::vector<Delivery> got = t->drain();
    bool led = false;
    for (auto& d : got)
        if (const auto* m = std::get_if<calcu::sessions::StateMessage>(&d.message)) led |= m->topic == topics::kLed;
    CHECK(led);
}
