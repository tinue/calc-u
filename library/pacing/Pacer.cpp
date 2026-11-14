#include "pacing/Pacer.hpp"
#include <algorithm>

// The rules R1 to R7 of LIB-17 design §3. Emulated time is compared with an
// absolute deadline from the origin, never with the length of the last
// slice (frontend.md I-6): due ticks D = ticksIn(now − origin), against the
// ticks run since the origin, which carry every overshoot (AC-3).

namespace calcu::pacing {

using machine::Ticks;

Pacer::Pacer(WallClock clock, std::optional<std::uint32_t> seed) : clock_(std::move(clock)), jitter_(seed) {}

void Pacer::setMachine(const PacingProfile& profile, machine::TickRate rate) {   // R7
    profile_ = profile;
    rate_ = rate;
    slice_ = profile.slice ? std::max<Ticks>(1, ticksIn(*profile.slice, rate)) : 0;
    lagLimit_ = ticksIn(kLagLimit, rate);
    callSlice_ = std::max<Ticks>(1, rate / kCallSlicesPerSecond);
    startAfresh();
}

void Pacer::startAfresh() {   // R7: no lag, nothing to wait out
    rebase(clock_());
    waiting_ = false;
    lastUnpacedEnd_ = origin_;
}

void Pacer::rebase(WallTime at) {
    origin_ = at;
    ran_ = 0;
    n_ = 0;
    // The first slice after a rebase ends on its grid point, so it runs at
    // once (qa-report, deviations); the jitter goes on from the next one.
    next_ = slice_;
}

Ticks Pacer::nextEnd() {   // R3: Tₙ = n·S + rₙ
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
    if (waiting_) {   // R5: paced while the machine waits
        if (paced(now, runner, res, true) != Outcome::LeftWaiting) return res;
        waiting_ = false;
    }
    unpaced(now, runner, res);
    return res;
}

// R1 to R3, R6: one call at the pace of regular speed. At full speed (R5) a
// slice that does not report "waits for input" returns to full speed.
Pacer::Outcome Pacer::paced(WallTime now, SliceRunner& runner, PaceResult& res, bool atFull) {
    Ticks due = ticksIn(now - origin_, rate_);
    bool dropped = false;
    if (due > ran_ && due - ran_ > lagLimit_) {   // R1: dropped whole, nothing caught up
        rebase(now);
        due = 0;
        dropped = true;
        res.lagDropped = true;
    }
    auto after = [&](const SliceResult& s) {
        if (s.stoppedEarly) return Outcome::Stopped;
        if (atFull && !s.waitsForInput) return Outcome::LeftWaiting;
        return Outcome::Done;
    };
    if (!fixed()) {   // R2: the call
        const Ticks want = due > ran_ ? due - ran_ : dropped ? callSlice_ : 0;
        if (want == 0) return Outcome::Done;
        return after(run(runner, want, now, res));
    }
    for (;;) {   // R3: every slice whose end is due within one slice
        const Ticks end = nextEnd();
        if (end > due + slice_) return Outcome::Done;
        ++n_;
        next_.reset();
        if (end <= ran_) continue;   // overshot past this end already
        const Outcome o = after(run(runner, end - ran_, origin_ + wallTimeOf(end, rate_), res));
        if (o != Outcome::Done) return o;
    }
}

// R4, R5, R6: full speed, slice after slice until the allowance of the call
// is used up on the supplied clock, at least one slice.
void Pacer::unpaced(WallTime callStart, SliceRunner& runner, PaceResult& res) {
    const WallTime until = callStart + kFullSpeedAllowance;
    bool ranOne = false;
    while (!ranOne || clock_() < until) {
        Ticks want = callSlice_;
        if (fixed()) {
            const Ticks end = nextEnd();
            ++n_;
            next_.reset();
            if (end <= ran_) continue;
            want = end - ran_;
        }
        const SliceResult s = run(runner, want, clock_(), res);
        ranOne = true;
        res.ranUnpaced = true;
        if (s.stoppedEarly) return;   // R6
        if (profile_.fullSpeedPacedWhileWaiting && s.waitsForInput) {
            // R5: this slice is slice 1 of a paced schedule that starts where
            // the last unpaced slice ended; R4's final rebase is skipped.
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
    rebase(clock_());   // R4: from here on as if afresh
}

} // namespace calcu::pacing
