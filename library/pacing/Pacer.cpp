#include "pacing/Pacer.hpp"
#include <algorithm>

// Emulated time is compared with an absolute deadline from the origin, never
// with the length of the last slice, so no lag builds up: due ticks
// D = ticksIn(now − origin), against the ticks run since the origin, which
// carry every overshoot.

namespace calcu::pacing {

using machine::Ticks;

Pacer::Pacer(WallClock clock, std::optional<std::uint32_t> seed) : clock_(std::move(clock)), jitter_(seed) {}

void Pacer::setMachine(const PacingProfile& profile, machine::TickRate rate) {
    profile_ = profile;
    rate_ = rate;
    slice_ = std::max<Ticks>(1, ticksIn(profile.slice, rate));
    lagLimit_ = ticksIn(kLagLimit, rate);
    startAfresh();
}

void Pacer::startAfresh() {   // no lag, nothing to wait out; the jitter is not reseeded
    rebase(clock_());
    waiting_ = false;
    lastUnpacedEnd_ = origin_;
}

void Pacer::rebase(WallTime at) {
    origin_ = at;
    ran_ = 0;
    n_ = 0;
    // The first slice after a rebase ends on its grid point, so it runs at
    // once; the jitter goes on from the next one. A jittered first end would
    // wait out its offset, and a caller that drops the lag at every call (a
    // throttled browser tab) would almost never run the machine.
    next_ = slice_;
}

Ticks Pacer::nextEnd() {   // Tₙ = n·S + rₙ (rₙ the jitter offset, else 0)
    if (!next_) next_ = (n_ + 1) * slice_ + (profile_.jitter ? jitter_.offset(slice_) : 0);
    return *next_;
}

SliceResult Pacer::run(SliceRunner& runner, Ticks ticks, WallTime due, PaceResult& res) {
    const SliceResult s = runner.runSlice(SliceRequest{ticks, due});
    ran_ += s.ticksRun;
    res.ticksRun += s.ticksRun;
    ++res.slices;
    return s;
}

PaceResult Pacer::pace(Speed speed, SliceRunner& runner) {
    PaceResult res;
    if (rate_ == 0) return res;   // no machine yet
    const WallTime now = clock_();
    if (speed == Speed::Regular) {
        waiting_ = false;   // the origin of the waiting stretch is a regular one: go on from it
        lastSpeed_ = speed;
        paced(now, runner, res, false);
        return res;
    }
    if (lastSpeed_ != Speed::Full) {
        lastUnpacedEnd_ = now;
        waiting_ = false;
    }
    lastSpeed_ = speed;
    if (waiting_) {   // paced while the machine waits
        if (paced(now, runner, res, true) != Outcome::LeftWaiting) return res;
        waiting_ = false;
    }
    unpaced(now, runner, res);
    return res;
}

// One call at the pace of regular speed. At full speed, while the machine waits,
// a slice that does not report "waits for input" returns to full speed.
Pacer::Outcome Pacer::paced(WallTime now, SliceRunner& runner, PaceResult& res, bool atFull) {
    Ticks due = ticksIn(now - origin_, rate_);
    if (due > ran_ && due - ran_ > lagLimit_) {   // dropped whole, nothing caught up
        rebase(now);
        due = 0;
        res.lagDropped = true;
    }
    auto after = [&](const SliceResult& s) {
        if (s.stoppedEarly) return Outcome::Stopped;
        if (atFull && !s.waitsForInput) return Outcome::LeftWaiting;
        return Outcome::Done;
    };
    for (;;) {   // every slice whose end is due within one slice
        const Ticks end = nextEnd();
        if (end > due + slice_) return Outcome::Done;
        ++n_;
        next_.reset();
        if (end <= ran_) continue;   // overshot past this end already
        const Outcome o = after(run(runner, end - ran_, origin_ + wallTimeOf(end, rate_), res));
        if (o != Outcome::Done) return o;
    }
}

// Full speed: slice after slice until the allowance of the call is used up
// on the supplied clock, at least one slice; then rebase on the clock.
void Pacer::unpaced(WallTime callStart, SliceRunner& runner, PaceResult& res) {
    const WallTime until = callStart + kFullSpeedAllowance;
    bool ranOne = false;
    while (!ranOne || clock_() < until) {
        const Ticks end = nextEnd();
        ++n_;
        next_.reset();
        if (end <= ran_) continue;
        const SliceResult s = run(runner, end - ran_, clock_(), res);
        ranOne = true;
        if (s.stoppedEarly) return;   // a debugger stop ends the call
        if (s.waitsForInput) {
            // This slice is slice 1 of a paced schedule that starts where the
            // last unpaced slice ended; the final rebase below is skipped.
            origin_ = lastUnpacedEnd_;
            ran_ = s.ticksRun;
            n_ = 1;
            next_.reset();
            waiting_ = true;
            if (paced(clock_(), runner, res, true) != Outcome::LeftWaiting) return;
            waiting_ = false;
        }
        lastUnpacedEnd_ = clock_();
    }
    rebase(clock_());   // from here on as if afresh
}

} // namespace calcu::pacing
