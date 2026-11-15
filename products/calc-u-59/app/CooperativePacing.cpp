#include "CooperativePacing.hpp"

namespace cu59 {

namespace {

const auto kBatch = std::chrono::duration_cast<CooperativePacing::Clock::duration>(std::chrono::duration<double>(kBatchSeconds));
// A stall longer than this (debugger stop, a hidden browser tab) restarts
// the schedule instead of bursting to catch up.
constexpr auto kMaxLag = std::chrono::milliseconds(100);

/// The batch due at `deadline` is late by more than kMaxLag: start over at `now`.
bool lagging(CooperativePacing::Clock::time_point deadline, CooperativePacing::Clock::time_point now) {
    return now - deadline > kMaxLag;
}

} // namespace

CooperativePacing::CooperativePacing(MachineGuard& guard, Model model, FreezeController& freeze, Callbacks callbacks,
                                     Threading threading)
    : m_guard(guard), m_freeze(freeze), m_callbacks(std::move(callbacks)), m_threading(threading),
      m_jitter(batchTargetCycles(model), m_callbacks.jitterSeed) {
    m_guard.access().debugTarget().setRunControl(&m_freeze);
}

CooperativePacing::~CooperativePacing() {
    m_running = false;
    m_guard.access().debugTarget().setRunControl(nullptr);
}

void CooperativePacing::start() {
    if (m_running.exchange(true)) return;
    m_carry = 0;
    m_jitter.reset();
    m_deadline = {};  // lagging: pump() starts the schedule at its `now`
    post(nullptr);
}

void CooperativePacing::post(std::function<void()> job) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.push_back(std::move(job));
}

void CooperativePacing::sync(const std::function<void()>& job) {
    if (!m_draining) drainJobs();
    job();
}

CooperativePacing::BatchEnd CooperativePacing::runOneBatch(int32_t& carry) {
    BatchResult r;
    bool idle;
    {
        auto a = m_guard.access();
        r = runBatch(a.machine(), carry, kTicksPerCycle * m_jitter.next(), m_freeze);
        idle = a.pacingHints().inIdleAndKeyDetect();
    }
    if (r.stop == BatchResult::Stop::Breakpoint) {  // Core breakpoints are set by headless tools only
        m_running = false;
        return BatchEnd::Stopped;
    }
    if (r.stop == BatchResult::Stop::Freeze) {
        m_running = false;
        if (m_callbacks.freezeFired) m_callbacks.freezeFired(r.freezeOwner);
        return BatchEnd::Stopped;
    }
    return shouldSleep(m_fullSpeed, idle, r.polledHardware) ? BatchEnd::Paced : BatchEnd::Unpaced;
}

calcu::machine::Ticks CooperativePacing::advanceHosted(calcu::machine::Ticks atLeast, bool& stopped) {
    stopped = false;
    if (!m_running) {
        stopped = true;
        return 0;
    }
    calcu::machine::Ticks ran = 0;
    std::optional<FreezeOwner> fired;
    bool breakpoint = false;
    {
        auto a = m_guard.access();
        ran = a.machine().advance(atLeast);
        switch (a.debugTarget().takeStop()) {
        case TIStop::Breakpoint: breakpoint = true; break;
        case TIStop::Observer: fired = m_freeze.takeFired().value_or(FreezeOwner::Cpu); break;
        case TIStop::None: break;
        }
    }
    if (m_callbacks.batchDone) m_callbacks.batchDone();
    if (breakpoint || fired) {
        m_running = false;
        stopped = true;
        if (fired && m_callbacks.freezeFired) m_callbacks.freezeFired(*fired);
    }
    return ran;
}

void CooperativePacing::drainJobs() {
    m_draining = true;
    for (;;) {
        std::function<void()> job;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_jobs.empty()) break;
            if (!m_jobs.front()) {  // the run loop: jobs behind it wait until it stops
                if (m_running) break;
                m_jobs.pop_front();
                continue;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }
        job();
    }
    m_draining = false;
}

// 2.3.0's threaded schedule with the sleeps replaced by returning: a batch
// is due once its start time has passed.
void CooperativePacing::pump(Clock::time_point now, const std::function<void(Clock::time_point)>& afterBatch) {
    drainJobs();
    if (m_threading == Threading::Hosted) return;
    bool loopActive;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        loopActive = m_running && !m_jobs.empty() && !m_jobs.front();
    }
    if (loopActive) {
        if (lagging(m_deadline, now)) m_deadline = now;
        const auto sliceEnd = m_callbacks.wallClock() + kFullSpeedSlice;
        while (m_running && m_deadline <= now) {
            const BatchEnd end = runOneBatch(m_carry);
            if (end == BatchEnd::Stopped) break;
            const bool paced = end == BatchEnd::Paced;
            if (paced) m_deadline += kBatch;
            if (m_callbacks.batchDone) m_callbacks.batchDone();
            if (afterBatch) afterBatch(paced ? m_deadline : now);
            if (!paced && m_callbacks.wallClock() >= sliceEnd) {
                m_deadline = now;  // full speed: go on at the next pump
                break;
            }
        }
    }
    drainJobs();
}

} // namespace cu59
