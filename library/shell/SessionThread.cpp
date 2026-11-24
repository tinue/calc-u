#include "shell/SessionThread.hpp"
#include "sessions/LibraryTopics.hpp"
#include <exception>
#include <string>
#ifdef CALCU_ATTACH
#include "harness/attach/AttachEndpoint.hpp"
#include "harness/attach/AttachedHarness.hpp"
#endif
#if defined(__APPLE__)
#include <pthread.h>
#endif

namespace calcu::shell {

#ifdef CALCU_ATTACH
struct SessionThread::Attach final : calcu::harness::AttachInbox {
    SessionThread& t;
    std::unique_ptr<calcu::harness::AttachEndpoint> endpoint;
    std::unique_ptr<calcu::harness::AttachedHarness> harness;
    explicit Attach(SessionThread& owner) : t(owner) {}
    void post(calcu::harness::AttachEvent e) override {
        t.post([this, e = std::move(e)](sessions::AppSession&) {
            if (harness) harness->handle(e);
        });
    }
};
#else
struct SessionThread::Attach {};
#endif

SessionThread::SessionThread(Factory factory, sessions::HostServices services, calcu::pacing::WallClock clock,
                             std::function<void()> wake)
    : services_(std::move(services)), clock_(std::move(clock)), wake_(std::move(wake)) {
    thread_ = std::thread([this, f = std::move(factory)]() mutable { run(std::move(f)); });
}

SessionThread::~SessionThread() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        quit_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void SessionThread::enqueue(Job job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        inbox_.push_back(std::move(job));
    }
    cv_.notify_all();
}

void SessionThread::post(std::function<void(sessions::AppSession&)> job) {
    enqueue([job = std::move(job)](sessions::Listener& l) { job(l.session()); });
}

// A reply goes into the batch where it comes, with the frontend's handler.
sessions::Listener::ReplyHandler SessionThread::stageReply(ReplyHandler onReply) {
    return [this, r = std::move(onReply)](const sessions::ReplyMessage& m) { staged_.push_back(Delivery{m, r}); };
}

void SessionThread::send(sessions::Command c, ReplyHandler onReply) {
    enqueue([this, c = std::move(c), r = std::move(onReply)](sessions::Listener& l) mutable {
        l.send(c.name, std::move(c.args), stageReply(std::move(r)));
    });
}

void SessionThread::ask(sessions::Request q, ReplyHandler onReply) {
    enqueue([this, q = std::move(q), r = std::move(onReply)](sessions::Listener& l) mutable {
        l.ask(q.name, std::move(q.args), stageReply(std::move(r)));
    });
}

void SessionThread::answer(sessions::CallId id, sessions::ServiceAnswer a) {
    post([id, a = std::move(a)](sessions::AppSession& s) mutable { s.answer(id, std::move(a)); });
}

void SessionThread::suspendCalls() {
    send(sessions::Command{sessions::kSuspendCommand, sessions::Value::object({})});
    post([this](sessions::AppSession&) { suspended_ = true; });   // after the suspend is carried out
}

void SessionThread::continueCalls() {
    post([this](sessions::AppSession&) { suspended_ = false; });
    send(sessions::Command{sessions::kContinueCommand, sessions::Value::object({})});
}

std::vector<Delivery> SessionThread::drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Delivery> out(std::make_move_iterator(outbox_.begin()), std::make_move_iterator(outbox_.end()));
    outbox_.clear();
    return out;
}

void SessionThread::publish() {
    if (staged_.empty()) return;
    bool wasEmpty;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wasEmpty = outbox_.empty();
        for (auto& d : staged_) outbox_.push_back(std::move(d));
    }
    staged_.clear();
    if (wasEmpty && wake_) wake_();
}

std::string SessionThread::openAttach([[maybe_unused]] calcu::harness::HarnessProduct& product,
                                      [[maybe_unused]] std::uint16_t port,
                                      [[maybe_unused]] std::function<calcu::parts::LibraryParts&()> parts) {
#ifdef CALCU_ATTACH
    std::string problem = "the session is not there";
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    post([&](sessions::AppSession& session) {
        attach_ = std::make_unique<Attach>(*this);
        attach_->endpoint = std::make_unique<calcu::harness::AttachEndpoint>(*attach_, port);
        const auto o = attach_->endpoint->open();
        if (o.ok) {
            attach_->harness = std::make_unique<calcu::harness::AttachedHarness>(
                session, product, *attach_->endpoint, [this] { return suspended_.load(); }, parts());
            problem.clear();
        } else {
            problem = o.problem;
            attach_.reset();
        }
        std::lock_guard<std::mutex> lock(m);
        done = true;
        cv.notify_all();
    });
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return done; });
    return problem;
#else
    return "this build has no harness attach";
#endif
}

void SessionThread::run(Factory factory) {
#if defined(__APPLE__)
    // The user waits for these calls: without the QoS class the system may
    // put the thread on an efficiency core and the 60 Hz pace slips.
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
    // A throw out of the factory has no session whose app log could take
    // it: it ends the app through std::terminate, whose handler prints what().
    std::unique_ptr<sessions::AppSession> session = factory();   // built on this thread
    // A throw out of the session ends the app as well (std::terminate); its
    // cause goes to the session's app log first, so the log file keeps it.
    auto ends = [&session](const char* where, const std::exception& e) {
        session->context().appLog().error(std::string("The session ends: ") + where + " threw: " + e.what());
    };
    // The frontend's listener: a message goes into the staged batch, which
    // the thread publishes after each job and each host call.
    sessions::Listener listener;
    listener.onMessage = [this](const sessions::Message& m) { staged_.push_back(Delivery{m, {}}); };
    listener.listen(*session, services_);
    publish();
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    std::int64_t n = 0;   // signed: a duration of an unsigned count would be unsigned too
    for (;;) {
        // Every post, in order; commands are carried out on arrival,
        // also while suspended.
        for (;;) {
            Job job;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (quit_) goto done;
                if (inbox_.empty()) break;
                job = std::move(inbox_.front());
                inbox_.pop_front();
            }
            try {
                job(listener);
            } catch (const std::exception& e) {
                ends("a call from the frontend", e);
                throw;
            }
            publish();
        }
        const auto now = Clock::now();
        auto next = t0 + (n + 1) * kPeriod;
        if (!suspended_ && now >= next) {
            try {
                session->call(clock_());   // one host call
            } catch (const std::exception& e) {
                ends("a host call", e);
                throw;
            }
            ++calls_;
#ifdef CALCU_ATTACH
            if (attach_ && attach_->harness) attach_->harness->afterHostCall();
#endif
            publish();
            ++n;
            const auto after = Clock::now();
            next = t0 + (n + 1) * kPeriod;
            if (after - next > kMaxBehind) {   // more than 100 ms behind: rebase, no burst
                t0 = after;
                n = 0;
                next = t0 + kPeriod;
            }
        } else if (suspended_) {
            t0 = now;   // calls start afresh at continue
            n = 0;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        if (suspended_) cv_.wait(lock, [&] { return quit_ || !inbox_.empty(); });
        else cv_.wait_until(lock, next, [&] { return quit_ || !inbox_.empty(); });
    }
done:
#ifdef CALCU_ATTACH
    attach_.reset();   // the endpoint joins its I/O thread; the harness stops listening
#endif
    listener.stop();
    session.reset();   // destroyed on this thread
}

} // namespace calcu::shell
