#pragma once
#include "sessions/AppSession.hpp"
#include "sessions/Listener.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include "pacing/WallClock.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// The Swift frontend's session thread (C59-5 AC-4 to AC-6, AC-8; design
// §3.1, D-7, D-11): one thread of its own builds, calls and destroys the app
// session, about 60 times a second; the frontend's thread only posts and
// drains. A host adapter at the ADR 0009 boundary (LIB-1 design §2a): the one
// place in the app layer with a thread and locks. Never built by the Web.
//
// The two queues (the Mailbox): posts in, one FIFO; messages out to the
// frontend, one FIFO, filled only by this thread. The wake runs once per
// batch (FS 3: "one hop per batch"); the frontend drains the whole batch.

namespace calcu::parts {
class LibraryParts;
}
namespace calcu::harness {
class AttachEndpoint;
class AttachedHarness;
class HarnessProduct;
struct AttachEvent;
} // namespace calcu::harness

namespace cu59 {

namespace sessions = calcu::sessions;

/// One item the frontend drains: a message, and for the reply to a command
/// it sent with a handler, that handler (run by the frontend, on its thread).
struct Delivery {
    sessions::Message message;
    std::function<void(const sessions::ReplyMessage&)> onReply;
};

class SessionThread {
public:
    /// The session the thread hosts (makeAppSession(...).session; a test's
    /// own); runs on the thread.
    using Factory = std::function<std::unique_ptr<sessions::AppSession>()>;
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
    void post(std::function<void(sessions::AppSession&)> job);
    /// AC-8: the suspend command is carried out, then the thread stops calling.
    void suspendCalls();
    /// Calling again, then the continue command.
    void continueCalls();
    bool callsSuspended() const { return suspended_.load(); }

    /// The frontend's thread: everything that arrived, in order.
    std::vector<Delivery> drain();

    /// Debug builds (C59-5 AC-10, EC-1): harness attach on 127.0.0.1:port for
    /// `product`, its commands on the session's library parts (`parts`, asked
    /// on the session thread; LIB-14 D-27). Waits for the thread. Returns the
    /// problem, empty when open.
    std::string openAttach(calcu::harness::HarnessProduct& product, std::uint16_t port,
                           std::function<calcu::parts::LibraryParts&()> parts);

    /// For tests: the number of host calls made, and the thread's id.
    std::uint64_t calls() const { return calls_.load(); }
    std::thread::id threadId() const { return thread_.get_id(); }

private:
    struct Attach;
    using Job = std::function<void(sessions::Listener&)>;
    void enqueue(Job);
    void run(Factory);
    sessions::Listener::ReplyHandler stageReply(ReplyHandler onReply);
    void publish();

    sessions::HostServices services_;
    calcu::pacing::WallClock clock_;
    std::function<void()> wake_;
    std::atomic<bool> suspended_{false};
    std::atomic<std::uint64_t> calls_{0};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Job> inbox_;
    std::deque<Delivery> outbox_;
    bool quit_ = false;

    // The session thread's own (no lock).
    std::vector<Delivery> staged_;
    std::unique_ptr<Attach> attach_;

    std::thread thread_;
};

} // namespace cu59
