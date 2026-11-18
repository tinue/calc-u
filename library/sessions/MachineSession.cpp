#include "sessions/MachineSession.hpp"
#include <algorithm>

namespace calcu::sessions {

using machine::Ticks;

MachineSession::MachineSession(pacing::WallClock clock, std::optional<std::uint32_t> seed)
    : pacer_(std::move(clock), seed) {
    startAfresh();
}

void MachineSession::setMachine(std::unique_ptr<machine::Machine> m, const pacing::PacingProfile& profile,
                                int soundRate) {
    aboutToChange();   // the old machine is still there
    machine_ = std::move(m);
    soundRate_ = machine_ ? soundRate : 0;
    ++generation_;
    paused_ = false;   // a new machine is not paused
    if (machine_) {
        pacer_.setMachine(profile, machine_->descriptor().tickRate);   // starts afresh
        ++fresh_;
    }
}

std::unique_ptr<machine::Machine> MachineSession::takeMachine() {
    aboutToChange();
    ++generation_;
    soundRate_ = 0;
    return std::move(machine_);
}

void MachineSession::dropInput() {
    for (SliceHook* h : hooks_) h->drop();
}

void MachineSession::setSuspended(bool s) {
    const bool was = suspended_;
    suspended_ = s;
    if (was && !s) startAfresh();
}

void MachineSession::setPaused(bool p) {
    const bool was = paused_;
    paused_ = p;
    if (was && !p) startAfresh();
}

void MachineSession::startAfresh() {
    pacer_.startAfresh();
    ++fresh_;
}

pacing::PaceResult MachineSession::pace(pacing::Speed speed) {
    if (!runs()) return {};
    return pacer_.pace(speed, *this);
}

Ticks MachineSession::advanceUnpaced(Ticks ticks) {
    if (!runs()) return 0;
    bool stopped = false;
    due_ = pacing::WallTime{0};
    return advance(ticks, stopped);
}

void MachineSession::actOnDueHooks() {
    const Ticks now = machine_->tickCount();
    for (SliceHook* h : hooks_)
        if (h->nextDue(now) <= now) h->act(*machine_, now, due_);
}

Ticks MachineSession::advance(Ticks atLeast, bool& stoppedEarly) {
    stoppedEarly = false;
    if (!machine_) return 0;
    Ticks ran = 0;
    while (ran < atLeast) {
        actOnDueHooks();
        const Ticks now = machine_->tickCount();
        Ticks piece = atLeast - ran;
        for (SliceHook* h : hooks_) {
            const Ticks due = h->nextDue(now);
            if (due != kNever && due > now) piece = std::min<Ticks>(piece, due - now);
        }
        const Ticks r = gate_ ? gate_->advance(*machine_, piece, stoppedEarly) : machine_->advance(piece);
        ran += r;
        if (stoppedEarly || r == 0) break;
    }
    actOnDueHooks();
    return ran;
}

Ticks MachineSession::nextDue() const {
    if (!machine_) return kNever;
    const Ticks now = machine_->tickCount();
    Ticks due = kNever;
    for (const SliceHook* h : hooks_) due = std::min(due, h->nextDue(now));
    return due;
}

bool MachineSession::waitsForInput() const {
    if (!reportsWaiting_ || !machine_) return false;
    if (machine_->isKeyReady()) return true;
    return std::any_of(probes_.begin(), probes_.end(), [&](WaitProbe* p) { return p->waitsForMedium(*machine_); });
}

pacing::SliceResult MachineSession::runSlice(const pacing::SliceRequest& req) {
    due_ = req.due;
    if (source_) return source_->runSlice(*this, req);
    pacing::SliceResult res;
    if (!machine_) return res;
    res.ticksRun = advance(req.ticks, res.stoppedEarly);
    res.waitsForInput = waitsForInput();
    return res;
}

} // namespace calcu::sessions
