#pragma once
#include "FreezeController.hpp"
#include "MachineGuard.hpp"
#include "Pacing.hpp"
#include "Threading.hpp"
#include "machine/EmulatedTime.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <random>

// What remains of 2.3.0's EmulationRunner (LIB-17 design §5, adapter
// CooperativePacing; removed in M-34 with SessionController): a serial job
// queue whose long-running job is the run loop, on the one thread of the
// session's host. Everything that steps the machine while it isn't running
// — freeze-to-keycode, single steps, ASM overlay entry — is posted here, so
// it runs after the loop has stopped and never inside it.
//
// - Threading::Cooperative (the Web frontend and SessionController's tests):
//   pump(now), called at 60 Hz, runs 2.3.0's batches due by `now` (Pacing.hpp)
//   and the jobs queued after the loop stopped.
// - Threading::Hosted (cu59::TIAppPart on the library's app session): the
//   machine session runs the machine with LIB-17's pacer; the run flag only
//   says whether it may (MachineSession::setPaused), and advanceHosted() is
//   the one advance of its slices (TIFreezeGate).

namespace cu59 {

class CooperativePacing {
public:
    using Clock = std::chrono::steady_clock;

    struct Callbacks {
        std::function<void(FreezeOwner)> freezeFired;     ///< loop stopped by an armed freeze
        std::function<void()> batchDone;                  ///< after every batch / slice (TI-58C persist debounce)
        /// Cooperative: the clock that ends a full-speed slice (tests inject one).
        std::function<Clock::time_point()> wallClock = [] { return Clock::now(); };
        /// Seeds the batch-end jitter (BatchJitter; tests fix it).
        uint32_t jitterSeed = std::random_device{}();
    };

    /// Cooperative: how long one pump() may run batches back to back at full speed.
    static constexpr std::chrono::milliseconds kFullSpeedSlice{10};

    /// Sets `freeze` as the machine's run-control hook for the object's life.
    CooperativePacing(MachineGuard& guard, Model model, FreezeController& freeze, Callbacks callbacks,
                      Threading threading = Threading::Cooperative);
    ~CooperativePacing();
    CooperativePacing(const CooperativePacing&) = delete;
    CooperativePacing& operator=(const CooperativePacing&) = delete;

    /// Queue the run loop (no-op if already running).
    void start();
    /// The loop stops after the current batch.
    void stop() { m_running = false; }
    bool isRunning() const { return m_running.load(); }

    void setFullSpeed(bool on) { m_fullSpeed = on; }
    bool fullSpeed() const { return m_fullSpeed.load(); }

    /// Queue a job (after the run loop has stopped, if it is running).
    void post(std::function<void()> job);
    /// Runs the queued jobs, then `job`, inline.
    void sync(const std::function<void()>& job);

    /// Cooperative: run the batches due by `now` (regular speed), or batches
    /// for up to kFullSpeedSlice (full speed), then the jobs queued behind a
    /// stopped loop. `afterBatch` gets the time each batch ends at.
    /// Hosted: only the jobs.
    void pump(Clock::time_point now, const std::function<void(Clock::time_point)>& afterBatch);

    /// Hosted: one advance of at least `atLeast` ticks with the freeze and
    /// breakpoint checks inside it (C59-1 §4); a stop ends the loop and sets
    /// `stopped`.
    calcu::machine::Ticks advanceHosted(calcu::machine::Ticks atLeast, bool& stopped);

private:
    /// Stopped: the batch stopped the loop (breakpoint, armed freeze).
    enum class BatchEnd { Stopped, Paced, Unpaced };
    BatchEnd runOneBatch(int32_t& carry);
    /// Run queued jobs up to a running loop (an empty job marks it).
    void drainJobs();

    MachineGuard& m_guard;
    FreezeController& m_freeze;
    Callbacks m_callbacks;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_fullSpeed{false};
    const Threading m_threading;
    BatchJitter m_jitter;

    // Cooperative run loop state.
    int32_t m_carry = 0;
    Clock::time_point m_deadline{};
    bool m_draining = false;

    std::mutex m_mutex;
    std::deque<std::function<void()>> m_jobs;
};

} // namespace cu59
