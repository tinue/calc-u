// The Swift frontend's session thread (C59-5 design §6, H1 to H3): a session
// made for the test, whose calls take as long as it wants, on the real clock.

#include "TestHarness.hpp"
#include "AppSessionRig.hpp"
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

struct FakeSession final : HostedSession {
    Shared& s;
    calcu::sessions::MessageSink* sink = nullptr;
    calcu::sessions::CallId next = 0;
    explicit FakeSession(Shared& sh) : s(sh) { note(); }
    ~FakeSession() override { note(); }
    void note() {
        std::lock_guard<std::mutex> l(s.m);
        s.threads.insert(std::this_thread::get_id());
    }
    calcu::sessions::ListenerId listen(calcu::sessions::MessageSink& m, calcu::sessions::HostServices) override {
        sink = &m;
        return 1;
    }
    calcu::sessions::CallId send(calcu::sessions::ListenerId, Command c) override {
        note();
        {
            std::lock_guard<std::mutex> l(s.m);
            s.commands.push_back(c.name.value);
            s.commandAtCall.emplace_back(c.name.value, s.calls.load());
        }
        const auto id = ++next;
        sink->deliver(ReplyMessage{id, true, "", nullptr});
        return id;
    }
    calcu::sessions::CallId ask(calcu::sessions::ListenerId, calcu::sessions::Request) override { return ++next; }
    void answer(calcu::sessions::CallId, calcu::sessions::ServiceAnswer) override {}
    void call(calcu::pacing::WallTime) override {
        note();
        ++s.calls;
        if (s.slowCalls > 0) {
            --s.slowCalls;
            std::this_thread::sleep_for(200ms);
        }
    }
    calcu::sessions::AppSession* appSession() override { return nullptr; }
};

std::unique_ptr<SessionThread> thread(Shared& s, std::atomic<int>* wakes = nullptr) {
    return std::make_unique<SessionThread>([&s] { return std::make_unique<FakeSession>(s); },
                                           calcu::sessions::HostServices{}, steadyWallTime,
                                           [wakes] {
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
    CHECK(s.calls.load() - c0 >= 8);
    std::lock_guard<std::mutex> l(s.m);
    CHECK(s.commands.back() == "continue");
}

// The real session on the thread: Calc-U-59 runs, and a key reaches it.
TEST(c59_5_ac4_ti59_on_the_session_thread) {
    TIAppSetup setup;
    setup.roms = testroms::romSet();
    setup.cuecards = testroms::catalog();
    testroms::TempFolder folder;
    setup.saveFolder = folder.path.string();
    std::atomic<std::uint64_t> ticks{0};
    auto t = std::make_unique<SessionThread>(
        [setup] {
            auto app = makeAppSession(setup, steadyWallTime);
            auto* s = app->session.get();
            return std::make_unique<AppSessionHost>(*s, std::shared_ptr<TIAppSession>(std::move(app)));
        },
        calcu::sessions::HostServices{}, steadyWallTime, [] {});
    std::this_thread::sleep_for(500ms);
    t->post([&](HostedSession& h) { ticks = h.appSession()->machineSession().machine()->tickCount(); });
    CHECK(waitFor([&] { return ticks.load() > 0; }));
    std::vector<Delivery> got = t->drain();
    bool led = false;
    for (auto& d : got)
        if (const auto* m = std::get_if<calcu::sessions::StateMessage>(&d.message)) led |= m->topic == topics::kLed;
    CHECK(led);
}
