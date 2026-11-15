#pragma once
#include "machine/EmulatedTime.hpp"
#include "pacing/PacingProfile.hpp"
#include "pacing/WallClock.hpp"
#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <utility>

// Operations (LIB-3 design §3.5; spec AC-14 to AC-19, EC-3, EC-4, EC-6): a
// long piece of work (a preset, a load, a power cycle) written as a C++20
// coroutine that the app session runs one part per host call.
//
// Task<T>, Job<T> and the run request are taken from Calc-U-1600's
// `core/Job.hpp` (resumable machine jobs), which becomes a forwarding header
// onto these in M-31 (adapter JobForward).
//
// Equivalence by construction: a run request is never split. A Job hands the
// machine exactly the calls the coroutine asked for, in the same order,
// whatever budget step() is given; only where the job pauses between them
// changes. So an operation run in parts leaves the machine in the same state,
// tick for tick, as one run in one go (AC-19).
//
// Rules for writing one:
//  - Every function that (transitively) runs the machine is a coroutine
//    returning Task<T>, and awaits the ones it calls (AC-17: a nested
//    operation is awaited directly).
//  - Coroutine parameters are taken by value, except the context and
//    out-pointers: a Task starts lazily, after the full expression that
//    created it -- a reference to a temporary argument would dangle.
//  - Single-threaded: one job runs at a time per thread.

namespace calcu::sessions {

// What a suspended job is waiting for: one machine call.
struct Pending {
    std::function<std::uint64_t()> run;   // the machine call
    std::uint64_t* result = nullptr;      // where its return value goes
    std::coroutine_handle<> resume;       // the coroutine to continue afterwards
    bool host = false;                    // a wait for the host's next call (yieldToHost), no machine call
    explicit operator bool() const { return static_cast<bool>(resume); }
};

namespace detail {

// The Pending of the job being resumed on this thread.
inline thread_local Pending* tl_pending = nullptr;

struct PromiseBase {
    std::coroutine_handle<> continuation;
    std::exception_ptr exception;

    std::suspend_always initial_suspend() noexcept { return {}; }
    struct FinalAwaiter {
        bool await_ready() noexcept { return false; }
        template <class P> std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) noexcept {
            const std::coroutine_handle<> next = h.promise().continuation;
            return next ? next : std::noop_coroutine();
        }
        void await_resume() noexcept {}
    };
    FinalAwaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception = std::current_exception(); }
};

template <class T> struct Promise;

} // namespace detail

// A lazily started coroutine producing a T. Await it from another
// coroutine, or hand it to a Job.
template <class T = void> class [[nodiscard]] Task {
public:
    using promise_type = detail::Promise<T>;
    using Handle = std::coroutine_handle<promise_type>;

    Task() = default;
    explicit Task(Handle h) : h_(h) {}
    Task(Task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() {
        if (h_) h_.destroy();
    }

    bool done() const { return !h_ || h_.done(); }
    Handle handle() const { return h_; }

    // co_await: run this task, then continue the awaiting coroutine.
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() { return h_.promise().take(); }

private:
    Handle h_;
};

namespace detail {

template <class T> struct Promise : PromiseBase {
    std::optional<T> value;
    Task<T> get_return_object() { return Task<T>(std::coroutine_handle<Promise>::from_promise(*this)); }
    template <class U> void return_value(U&& v) { value.emplace(std::forward<U>(v)); }
    T take() {
        if (exception) std::rethrow_exception(exception);
        return std::move(*value);
    }
};

template <> struct Promise<void> : PromiseBase {
    Task<void> get_return_object() { return Task<void>(std::coroutine_handle<Promise>::from_promise(*this)); }
    void return_void() {}
    void take() {
        if (exception) std::rethrow_exception(exception);
    }
};

// co_await request(call): suspends the whole job until the driver has made
// the call; yields its return value.
struct RunAwaiter {
    std::function<std::uint64_t()> call;
    std::uint64_t result = 0;

    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        Pending* p = tl_pending;
        p->run = std::move(call);
        p->result = &result;
        p->resume = h;
    }
    std::uint64_t await_resume() noexcept { return result; }
};

// co_await yieldToHost(): suspends the whole job until the host's next call;
// Job::step resumes it once and then returns.
struct HostAwaiter {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        Pending* p = tl_pending;
        p->run = [] { return std::uint64_t{0}; };
        p->result = nullptr;
        p->resume = h;
        p->host = true;
    }
    void await_resume() noexcept {}
};

} // namespace detail

// The awaitable form of one machine call that returns the ticks it ran.
inline detail::RunAwaiter request(std::function<std::uint64_t()> call) { return detail::RunAwaiter{std::move(call)}; }
// The awaitable that gives the job back to the host until its next call
// (OperationContext::hostWait, LIB-6 D-17).
inline detail::HostAwaiter yieldToHost() { return {}; }

// Drives a Task. step() makes run requests until `budget` is spent (a
// request is never split, so it may overshoot) or the task is done.
template <class T = void> class Job {
public:
    Job() = default;
    explicit Job(Task<T> task) : task_(std::move(task)) {}

    bool valid() const { return task_.handle() != nullptr; }
    bool done() const { return task_.done(); }
    // The job waits for the host's next call (yieldToHost), not for the machine.
    bool waitsOnHost() const { return pending_ && pending_.host; }

    // Returns done(). Exceptions from the task surface in result(); an
    // exception of a machine call itself leaves step().
    bool step(std::uint64_t budget) {
        if (done()) return true;
        Pending* const outer = detail::tl_pending;
        detail::tl_pending = &pending_;
        struct Restore {
            Pending* outer;
            ~Restore() { detail::tl_pending = outer; }
        } restore{outer};

        if (!started_) {
            started_ = true;
            task_.handle().resume();
        }
        std::uint64_t spent = 0;
        while (pending_ && spent < budget) {
            if (pending_.host) {   // the host's call has come: one resume, then back to the host
                const std::coroutine_handle<> next = pending_.resume;
                pending_ = Pending{};
                next.resume();
                break;
            }
            const std::uint64_t ran = pending_.run();
            *pending_.result = ran;
            spent += ran == 0 ? 1 : ran;   // a stalled machine still makes progress here
            const std::coroutine_handle<> next = pending_.resume;
            pending_ = Pending{};
            next.resume();
        }
        return done();
    }

    // The task's result (rethrows what it threw). Only once, after done().
    T result() { return task_.handle().promise().take(); }

private:
    Task<T> task_;
    Pending pending_;
    bool started_ = false;
};

// Runs `task` straight through: the blocking form.
template <class T> T runToCompletion(Task<T> task) {
    Job<T> j(std::move(task));
    while (!j.step(std::numeric_limits<std::uint64_t>::max())) {
    }
    return j.result();
}

// ── Operations on the session ────────────────────────────────────────────

struct OperationResult {
    bool ok = true;
    std::string message;   // an English sentence; the reason when !ok
};

class SessionContext;

// What a running operation may do (design §3.5).
class OperationContext {
public:
    explicit OperationContext(SessionContext& s) : session_(&s) {}

    // co_await: the machine runs at least `ticks`, in run requests of at most
    // runPiece() each, at the operation's speed. Throws (inside the
    // operation) when no machine is running.
    Task<machine::Ticks> run(machine::Ticks ticks);
    void setSpeed(pacing::Speed s) { speed_ = s; }   // AC-14: a stretch at regular speed, paced as usual
    pacing::Speed speed() const { return speed_; }
    void setReportsWaiting(bool);                    // LIB-17 AC-16: false reports "not waiting"
    // LIB-6 D-17: co_await: ends when the session's wall clock has passed
    // `d`, looked at once per host call, also while the machine is paused (a
    // preset's `wait:` on a paused machine, LIB-6 AC-19). True when it
    // waited; false at once in Hosting::VirtualTime, which has no wall clock.
    Task<bool> hostWait(pacing::WallTime d);
    SessionContext& session() { return *session_; }

    // kRunPiece: 1/60 s of emulated time at `rate`, at least one tick.
    static machine::Ticks runPiece(machine::TickRate rate);

private:
    SessionContext* session_;
    pacing::Speed speed_ = pacing::Speed::Full;   // AC-14: full speed unless asked otherwise
};

using Operation = std::function<Task<OperationResult>(OperationContext&)>;

} // namespace calcu::sessions
