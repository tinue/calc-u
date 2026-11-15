#include "SessionThread.hpp"
#include "sessions/LibraryTopics.hpp"
#ifdef CALCU_ATTACH
#include "harness/attach/AttachEndpoint.hpp"
#include "harness/attach/AttachedHarness.hpp"
#endif
#if defined(__APPLE__)
#include <pthread.h>
#endif

namespace cu59 {

// The frontend's listener: a message goes into the staged batch, which the
// thread publishes after each job and each host call.
struct SessionThread::Listener final : sessions::MessageSink {
    SessionThread& t;
    explicit Listener(SessionThread& owner) : t(owner) {}
    void deliver(const sessions::Message& m) override { t.stage(m); }
};

#ifdef CALCU_ATTACH
struct SessionThread::Attach final : calcu::harness::AttachInbox {
    SessionThread& t;
    std::unique_ptr<calcu::harness::AttachEndpoint> endpoint;
    std::unique_ptr<calcu::harness::AttachedHarness> harness;
    explicit Attach(SessionThread& owner) : t(owner) {}
    void post(calcu::harness::AttachEvent e) override {
        t.post([this, e = std::move(e)](HostedSession&) {
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

void SessionThread::post(std::function<void(HostedSession&)> job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        inbox_.push_back(std::move(job));
    }
    cv_.notify_all();
}

void SessionThread::send(sessions::Command c, ReplyHandler onReply) {
    post([this, c = std::move(c), r = std::move(onReply)](HostedSession& h) mutable {
        const sessions::CallId id = h.send(frontend_, std::move(c));
        if (r) handlers_[id] = std::move(r);
    });
}

void SessionThread::ask(sessions::Request q, ReplyHandler onReply) {
    post([this, q = std::move(q), r = std::move(onReply)](HostedSession& h) mutable {
        const sessions::CallId id = h.ask(frontend_, std::move(q));
        if (r) handlers_[id] = std::move(r);
    });
}

void SessionThread::answer(sessions::CallId id, sessions::ServiceAnswer a) {
    post([id, a = std::move(a)](HostedSession& h) mutable { h.answer(id, std::move(a)); });
}

void SessionThread::suspendCalls() {
    send(sessions::Command{sessions::kSuspendCommand, sessions::Value::object({})});
    post([this](HostedSession&) { suspended_ = true; });   // after the suspend is carried out
}

void SessionThread::continueCalls() {
    post([this](HostedSession&) { suspended_ = false; });
    send(sessions::Command{sessions::kContinueCommand, sessions::Value::object({})});
}

std::vector<Delivery> SessionThread::drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Delivery> out(std::make_move_iterator(outbox_.begin()), std::make_move_iterator(outbox_.end()));
    outbox_.clear();
    return out;
}

void SessionThread::stage(sessions::Message m) { staged_.push_back(Delivery{std::move(m), {}}); }

void SessionThread::publish() {
    if (staged_.empty()) return;
    for (auto& d : staged_)
        if (const auto* r = std::get_if<sessions::ReplyMessage>(&d.message)) {
            auto it = handlers_.find(r->call);
            if (it != handlers_.end()) {
                d.onReply = std::move(it->second);
                handlers_.erase(it);
            }
        }
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
                                      [[maybe_unused]] std::uint16_t port) {
#ifdef CALCU_ATTACH
    std::string problem = "the session is not there";
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    post([&](HostedSession& h) {
        if (h.appSession()) {
            attach_ = std::make_unique<Attach>(*this);
            attach_->endpoint = std::make_unique<calcu::harness::AttachEndpoint>(*attach_, port);
            const auto o = attach_->endpoint->open();
            if (o.ok) {
                attach_->harness = std::make_unique<calcu::harness::AttachedHarness>(
                    *h.appSession(), product, *attach_->endpoint, [this] { return suspended_.load(); });
                problem.clear();
            } else {
                problem = o.problem;
                attach_.reset();
            }
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
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);   // D-7
#endif
    std::unique_ptr<HostedSession> h = factory();   // built on this thread (AC-4)
    Listener listener(*this);
    frontend_ = h->listen(listener, services_);
    publish();
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    std::int64_t n = 0;   // signed: a duration of an unsigned count would be unsigned too
    for (;;) {
        // Every post, in order (AC-6); commands are carried out on arrival,
        // also while suspended (design §3.3).
        for (;;) {
            std::function<void(HostedSession&)> job;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (quit_) goto done;
                if (inbox_.empty()) break;
                job = std::move(inbox_.front());
                inbox_.pop_front();
            }
            job(*h);
            publish();
        }
        const auto now = Clock::now();
        auto next = t0 + (n + 1) * kPeriod;
        if (!suspended_ && now >= next) {
            h->call(clock_());   // one host call (AC-4)
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
    h.reset();   // destroyed on this thread
}

} // namespace cu59
