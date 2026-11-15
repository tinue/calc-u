#pragma once
#include "sessions/AppSession.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include "pacing/WallClock.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// The Swift frontend's session thread (C59-5 AC-4 to AC-6, AC-8; design
// §3.1, D-7, D-11): one thread of its own builds, calls and destroys the app
// session, about 60 times a second; the frontend's thread only posts and
// drains. A host adapter at the ADR 0009 boundary (LIB-1 design §2a): the one
// place in the app layer with a thread and locks. Never built by the Web
// (as it never started 2.3.0's threaded runner).
//
// The two queues (the Mailbox): posts in, one FIFO; messages out to the
// frontend, one FIFO, filled only by this thread. The wake runs once per
// batch (FS 3: "one hop per batch"); the frontend drains the whole batch.

namespace calcu::harness {
class AttachEndpoint;
class AttachedHarness;
class HarnessProduct;
struct AttachEvent;
} // namespace calcu::harness

namespace cu59 {

namespace sessions = calcu::sessions;

/// What the thread drives: the library's app session, or one made for a test.
class HostedSession {
public:
    virtual ~HostedSession() = default;
    virtual sessions::ListenerId listen(sessions::MessageSink&, sessions::HostServices) = 0;
    virtual sessions::CallId send(sessions::ListenerId, sessions::Command) = 0;
    virtual sessions::CallId ask(sessions::ListenerId, sessions::Request) = 0;
    virtual void answer(sessions::CallId, sessions::ServiceAnswer) = 0;
    virtual void call(calcu::pacing::WallTime now) = 0;
    virtual sessions::AppSession* appSession() = 0;   // for attach; nullptr in a test session
};

/// HostedSession over a library app session the host owns through `owner`.
class AppSessionHost final : public HostedSession {
public:
    AppSessionHost(sessions::AppSession& s, std::shared_ptr<void> owner) : s_(s), owner_(std::move(owner)) {}
    sessions::ListenerId listen(sessions::MessageSink& m, sessions::HostServices h) override { return s_.listen(m, std::move(h)); }
    sessions::CallId send(sessions::ListenerId l, sessions::Command c) override { return s_.send(l, std::move(c)); }
    sessions::CallId ask(sessions::ListenerId l, sessions::Request r) override { return s_.ask(l, std::move(r)); }
    void answer(sessions::CallId id, sessions::ServiceAnswer a) override { s_.answer(id, std::move(a)); }
    void call(calcu::pacing::WallTime now) override { s_.call(now); }
    sessions::AppSession* appSession() override { return &s_; }

private:
    sessions::AppSession& s_;
    std::shared_ptr<void> owner_;
};

/// One item the frontend drains: a message, and for the reply to a command
/// it sent with a handler, that handler (run by the frontend, on its thread).
struct Delivery {
    sessions::Message message;
    std::function<void(const sessions::ReplyMessage&)> onReply;
};

class SessionThread {
public:
    using Factory = std::function<std::unique_ptr<HostedSession>()>;   // runs on the thread
    using ReplyHandler = std::function<void(const sessions::ReplyMessage&)>;
    static constexpr std::chrono::microseconds kPeriod{16'667};          // AC-4
    static constexpr std::chrono::milliseconds kMaxBehind{100};

    /// `wake`: called on this thread when the frontend's queue goes from empty
    /// to not empty (the Swift side: one dispatch_async to the main queue).
    SessionThread(Factory, sessions::HostServices frontendServices, calcu::pacing::WallClock clock,
                  std::function<void()> wake);
    ~SessionThread();   // stops calling, destroys the session on the thread, joins
    SessionThread(const SessionThread&) = delete;
    SessionThread& operator=(const SessionThread&) = delete;

    // ── Any thread; each returns at once (AC-5) and keeps its order (AC-6).
    void send(sessions::Command, ReplyHandler onReply = {});
    void ask(sessions::Request, ReplyHandler onReply = {});
    void answer(sessions::CallId, sessions::ServiceAnswer);
    /// A job on the session thread (tests; the host's own adapters).
    void post(std::function<void(HostedSession&)> job);
    /// AC-8: the suspend command is carried out, then the thread stops calling.
    void suspendCalls();
    /// Calling again, then the continue command.
    void continueCalls();
    bool callsSuspended() const { return suspended_.load(); }

    /// The frontend's thread: everything that arrived, in order.
    std::vector<Delivery> drain();

    /// Debug builds (C59-5 AC-10, EC-1): harness attach on 127.0.0.1:port for
    /// `product`. Waits for the thread. Returns the problem, empty when open.
    std::string openAttach(calcu::harness::HarnessProduct& product, std::uint16_t port);

    /// For tests: the number of host calls made, and the thread's id.
    std::uint64_t calls() const { return calls_.load(); }
    std::thread::id threadId() const { return thread_.get_id(); }

private:
    struct Listener;
    struct Attach;
    void run(Factory);
    void stage(sessions::Message);
    void publish();

    sessions::HostServices services_;
    calcu::pacing::WallClock clock_;
    std::function<void()> wake_;
    std::atomic<bool> suspended_{false};
    std::atomic<std::uint64_t> calls_{0};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void(HostedSession&)>> inbox_;
    std::deque<Delivery> outbox_;
    bool quit_ = false;

    // The session thread's own (no lock).
    std::vector<Delivery> staged_;
    std::map<sessions::CallId, ReplyHandler> handlers_;
    std::unique_ptr<Attach> attach_;
    sessions::ListenerId frontend_ = 0;

    std::thread thread_;
};

} // namespace cu59
