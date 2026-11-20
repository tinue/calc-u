#pragma once
#include "machine/EmulatedTime.hpp"
#include "pacing/PacingProfile.hpp"
#include "pacing/SliceJitter.hpp"
#include "pacing/WallClock.hpp"
#include <chrono>
#include <cstdint>
#include <optional>

// The pacer: it runs a machine in slices so that its emulated time keeps to
// the wall clock the caller supplies, at regular speed and at full speed. One
// pacer serves every model, each with its pacing profile; the long-run rate
// is exact (errors of single calls do not add up).
//
// Ownership and threads: the machine session owns one Pacer and makes every
// call, the clock's included, on its own thread. The pacer has no lock and
// no thread, never reads a machine and never changes a tick count: it
// knows emulated time only from the ticks each slice reports.

namespace calcu::pacing {

struct SliceRequest {
    machine::Ticks ticks;   // at least this many
    WallTime due;           // regular speed: the wall time the slice's end is due; full speed: the time it runs
};

struct SliceResult {
    machine::Ticks ticksRun = 0;   // as the machine reports
    bool stoppedEarly = false;     // a debugger pause or a debug target stopped it; the call ends
    bool waitsForInput = false;    // the machine waits for input in this slice
};

// Implemented by the machine session: runs one slice on its machine.
class SliceRunner {
public:
    virtual SliceResult runSlice(const SliceRequest&) = 0;

protected:
    ~SliceRunner() = default;
};

struct PaceResult {
    machine::Ticks ticksRun = 0;
    int slices = 0;
    bool ranUnpaced = false;   // a full-speed slice ran: the session throws its sound away
    bool lagDropped = false;   // the lag was dropped; for tests and the log
};

class Pacer {
public:
    static constexpr std::chrono::milliseconds kLagLimit{100};           // a larger lag is dropped, not caught up
    static constexpr std::chrono::milliseconds kFullSpeedAllowance{14};  // per call; the host keeps the rest of ~16 ms

    explicit Pacer(WallClock clock, std::optional<std::uint32_t> seed = std::nullopt);   // a seed makes the jitter reproducible

    void setMachine(const PacingProfile&, machine::TickRate);   // starts afresh
    void startAfresh();                                         // no lag, nothing to wait out
    PaceResult pace(Speed, SliceRunner&);                       // one call of the host

private:
    enum class Outcome { Done, Stopped, LeftWaiting };

    void rebase(WallTime at);
    machine::Ticks nextEnd();
    Outcome paced(WallTime now, SliceRunner&, PaceResult&, bool atFull);
    void unpaced(WallTime callStart, SliceRunner&, PaceResult&);
    SliceResult run(SliceRunner&, machine::Ticks, WallTime due, PaceResult&);

    WallClock clock_;
    SliceJitter jitter_;
    PacingProfile profile_;
    machine::TickRate rate_ = 0;
    machine::Ticks slice_ = 1;       // the slice in ticks
    machine::Ticks lagLimit_ = 0;

    // The origin: wall time origin_ and the ticks run since, ran_; the index
    // n_ of the last slice end on the grid, and the next end once drawn.
    WallTime origin_{0};
    machine::Ticks ran_ = 0;
    std::uint64_t n_ = 0;
    std::optional<machine::Ticks> next_;

    Speed lastSpeed_ = Speed::Regular;
    bool waiting_ = false;           // at full speed, paced while the machine waits
    WallTime lastUnpacedEnd_{0};
};

} // namespace calcu::pacing
