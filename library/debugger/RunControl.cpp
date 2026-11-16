#include "debugger/RunControl.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Operation.hpp"
#include <algorithm>
#include <memory>

namespace calcu::debugger {

using machine::Ticks;

RunControl::RunControl(LevelRules& rules, TraceSession& trace) : rules_(rules), trace_(trace) {}

machine::Debugging* RunControl::debugging() const {
    machine::Machine* m = ctx_ ? ctx_->machine() : nullptr;
    return m ? m->debugging() : nullptr;
}

std::optional<std::string> RunControl::refusal() const {
    if (!ctx_ || !ctx_->machine()) return std::string(sessions::kNoMachine);
    if (!debugging()) return std::string(kNoTarget);
    return std::nullopt;
}

// ── State and replies ────────────────────────────────────────────────────

RunState RunControl::state() const {
    RunState s;
    s.paused = mode_ == Mode::Paused;   // D-14: stepping and a pending pause say running
    if (s.paused) {
        s.owner = level_;
        s.reason = reason_;
        if (machine::Debugging* d = debugging(); d && d->targetCount() > 0)
            s.target = d->target(d->current()).info().name;
        s.hits = hits_;
    }
    s.armed = armed_;
    s.pauseAtStart = pauseAtStart_;
    return s;
}

void RunControl::publish() {
    if (topic_) topic_->set(std::make_shared<RunState>(state()));
}

void RunControl::finishCalls() {
    if (calls_.empty() || !ctx_) return;
    const auto s = std::make_shared<RunState>(state());
    for (sessions::CallId c : std::exchange(calls_, {})) ctx_->reply(c, sessions::ReplyMessage{0, true, {}, s});
}

void RunControl::enterPaused(LevelName level, RunState::Reason reason, std::vector<RunState::Hit> hits) {
    mode_ = Mode::Paused;
    level_ = std::move(level);
    reason_ = reason;
    hits_ = std::move(hits);
    if (reason != RunState::Reason::Entry) armed_.reset();   // a pause disarms both (AC-9); entry is separate (AC-46)
    ctx_->machineSession().setPaused(true);
    publish();
    finishCalls();
}

// ── G11: a new machine ───────────────────────────────────────────────────

void RunControl::sync() {
    if (!ctx_) return;
    sessions::MachineSession& ms = ctx_->machineSession();
    if (ms.generation() == seenGeneration_) return;
    seenGeneration_ = ms.generation();
    mode_ = Mode::Running;
    hits_.clear();
    armed_.reset();
    ms.setPaused(false);
    finishCalls();
    table_.rebind(debugging());   // AC-15: by target name, the rest dropped
    trace_.end(*ctx_, TraceSession::Ending::NewMachine);
    if (ctx_->machine() && pauseAtStart_) {   // G9
        pauseAtStart_ = false;
        enterPaused(kCpu, RunState::Reason::Entry);
    }
    publish();
}

// ── G10: Reset, Cold boot ────────────────────────────────────────────────

void RunControl::restarted(RestartKind kind) {
    sync();
    if (mode_ != Mode::Running) {
        mode_ = Mode::Running;
        hits_.clear();
        ctx_->machineSession().setPaused(false);
        finishCalls();   // a step's reply goes out done, with the run state (EC-6)
    }
    if (armed_ && ctx_->machine()) rules_.startArmed(*armed_, *ctx_->machine());   // seen afresh from the restart on
    if (armed_ && rules_.startFiresAtRestart(*armed_, kind)) enterPaused(*armed_, RunState::Reason::PauseOnStart);
    if (pauseAtStart_) {
        pauseAtStart_ = false;
        enterPaused(kCpu, RunState::Reason::Entry);
    }
    publish();
}

// ── G1: the gate ─────────────────────────────────────────────────────────

machine::Checks RunControl::checks() {
    machine::Checks c;
    const bool free = !ctx_->busy();   // I-3: no breakpoint or watch while an operation runs
    c.breakpoints = free && table_.anyBreakpoints();
    c.watches = free && table_.anyWatches();
    if (mode_ == Mode::PausePending || mode_ == Mode::Stepping || armed_ || profile_.on()) c.observer = this;
    return c;
}

Ticks RunControl::advance(machine::Machine& m, Ticks atLeast, bool& stoppedEarly) {
    sync();
    stoppedEarly = false;
    if (mode_ == Mode::Paused) {   // AC-5: no emulated time passes
        stoppedEarly = true;
        return 0;
    }
    machine::Debugging* d = m.debugging();
    if (!d && !trace_.running()) return m.advance(atLeast);
    // While a trace runs the advance is cut into pieces of 1/60 s, the trace
    // pumped after each (T2).
    const Ticks slice = trace_.running() ? sessions::OperationContext::runPiece(m.descriptor().tickRate) : atLeast;
    Ticks ran = 0;
    while (ran < atLeast) {
        const Ticks piece = std::min(atLeast - ran, slice);
        if (d) d->setChecks(checks());
        landed_ = Landed::None;
        const Ticks r = m.advance(piece);
        ran += r;
        bool stopped = false;
        if (d) {
            if (auto stop = d->takeStop()) {
                stopped = true;
                onStop(*stop, *d);
            }
        }
        trace_.pump(*ctx_);
        if (mode_ == Mode::Paused) {
            stoppedEarly = true;
            break;
        }
        if (r == 0 && !stopped) break;   // a machine that does not run
    }
    if (d) d->setChecks({});   // the fast path between advances
    publish();
    return ran;
}

bool RunControl::stopAfter(const machine::InstructionEvent& ev) {
    if (profile_.on()) profile_.count(ev.target, ev.address);
    if (landed_ != Landed::None) return true;
    machine::Machine* m = ctx_->machine();
    if (armed_ && m && rules_.startFiresAfter(*armed_, ev, *m)) {   // G8, also in an operation (AC-13)
        firedLevel_ = *armed_;
        landed_ = Landed::StartFired;
        return true;
    }
    if (mode_ == Mode::PausePending && rules_.pauseLandsAfter(level_, ev)) {
        landed_ = Landed::PauseLanded;
        return true;
    }
    if (mode_ == Mode::Stepping && stepDoneAfter(ev)) {
        landed_ = Landed::StepDone;
        return true;
    }
    return false;
}

bool RunControl::stepDoneAfter(const machine::InstructionEvent& ev) {
    if (level_ == kProgram) {   // one step is what the product says
        if (!rules_.programStepDoneAfter(ev)) return false;
        return --left_ == 0;
    }
    if (ev.target != stepTarget_) return false;   // the other targets run meanwhile (AC-39)
    machine::Debugging* d = debugging();
    if (!d) return true;
    const machine::DebugTarget& t = d->target(stepTarget_);
    switch (kind_) {
    case StepKind::In:
        if (ev.nextAddress == ev.address && repeats_ < rules_.cpuStepRepeats()) {   // AC-7, AC-29
            ++repeats_;
            return false;
        }
        repeats_ = 0;
        return countedStepDone(t);
    case StepKind::Over:   // G5: a call is carried out with its routine
        if (overCall_ && t.callDepth() > depth0_) return false;
        if (countedStepDone(t)) return true;
        overCall_ = t.flowAt(ev.nextAddress) == machine::Flow::Call;
        depth0_ = t.callDepth();
        return false;
    case StepKind::Out:   // until the routine the target is in has returned
        return t.flowAt(ev.address) == machine::Flow::Return && t.callDepth() < depth0_;
    }
    return false;
}

// A counted step is over: the step ends at its count, or (D-16) when its
// test says not to go on.
bool RunControl::countedStepDone(const machine::DebugTarget& t) {
    if (--left_ == 0) return true;
    return until_ ? !until_(t) : false;
}

void RunControl::onStop(const machine::DebugStop& stop, machine::Debugging& d) {
    const machine::DebugTargetId& name = d.target(stop.target).info().name;
    switch (stop.kind) {
    case machine::DebugStop::Breakpoint: {   // G6
        std::vector<RunState::Hit> hits;
        const machine::DebugTarget& t = d.target(stop.target);
        for (const Breakpoint* b : table_.at(name, stop.address)) {
            if (!b->qualifier.empty() && !t.qualifierMatches(b->qualifier, stop.address)) continue;
            if (b->test && !b->test(t)) continue;   // AC-12: the setter's test says go on
            hits.push_back(RunState::Hit{b->id, b->setter});
        }
        if (hits.empty()) {
            d.passOnce(stop.target);   // the advance goes on within the same gate call
            return;
        }
        enterPaused(kCpu, RunState::Reason::Breakpoint, std::move(hits));
        return;
    }
    case machine::DebugStop::Watch: {   // G7: the lowest address hit is named (EC-4)
        std::vector<machine::WatchHit> sorted = stop.hits;
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const machine::WatchHit& a, const machine::WatchHit& b) { return a.address < b.address; });
        const machine::DebugTarget& t = d.target(stop.target);
        for (const auto& h : sorted) {
            std::vector<RunState::Hit> hits;
            for (const Watch* w : table_.hitBy(name, h)) {
                if (w->test && !w->test(t)) continue;
                hits.push_back(RunState::Hit{w->id, w->setter});
            }
            if (!hits.empty()) {
                enterPaused(kCpu, RunState::Reason::Watch, std::move(hits));
                return;
            }
        }
        land();   // no watch of a matching kind: what the observer said
        return;
    }
    case machine::DebugStop::Observer: land(); return;
    }
}

void RunControl::land() {
    switch (landed_) {
    case Landed::None: return;
    case Landed::StartFired: enterPaused(firedLevel_, RunState::Reason::PauseOnStart); break;
    case Landed::PauseLanded: enterPaused(level_, RunState::Reason::Pause); break;
    case Landed::StepDone: enterPaused(level_, RunState::Reason::Step); break;   // AC-45: the step's level owns it
    }
    landed_ = Landed::None;
}

// ── G2 to G9 ─────────────────────────────────────────────────────────────

RunControl::Result RunControl::pause(const LevelName& level, sessions::CallId call) {
    sync();
    if (auto r = refusal()) return Result{false, *r};
    if (!rules_.hasLevel(level)) return Result{false, "The machine has no level " + level.value + "."};   // AC-6
    armed_.reset();   // a pause disarms pause-on-start (AC-9)
    if (mode_ == Mode::Paused) {   // EC-1
        publish();
        return Result{};
    }
    if (mode_ == Mode::Stepping) {   // G2: the step ends there; its reply goes out first
        mode_ = Mode::Running;
        finishCalls();
    }
    if (mode_ == Mode::PausePending) {
        if (call) calls_.push_back(call);
        publish();
        return Result{true, {}, true};
    }
    if (rules_.pauseLandsAtOnce(level)) {
        enterPaused(level, RunState::Reason::Pause);
        return Result{};
    }
    rules_.pauseAsked(level, *ctx_->machine());
    mode_ = Mode::PausePending;
    level_ = level;
    if (call) calls_.push_back(call);
    publish();
    return Result{true, {}, true};
}

RunControl::Result RunControl::resume() {
    sync();
    if (auto r = refusal()) return Result{false, *r};
    switch (mode_) {
    case Mode::Running: return Result{};   // EC-2
    case Mode::PausePending:
    case Mode::Stepping:
        mode_ = Mode::Running;
        finishCalls();
        publish();
        return Result{};
    case Mode::Paused: break;
    }
    machine::Debugging* d = debugging();
    d->passOnce(d->current());   // AC-10, AC-39: only the target about to run passes
    mode_ = Mode::Running;
    hits_.clear();
    ctx_->machineSession().setPaused(false);   // pacing starts afresh (AC-16; LIB-3 AC-23)
    peripherals::setClocksFromHost(*ctx_->machine(), peripherals::hostDateTime(ctx_->hostTime()));   // AC-16
    publish();
    return Result{};
}

RunControl::Result RunControl::step(const LevelName& level, std::uint64_t count,
                                    std::optional<machine::DebugTargetId> target, StepKind kind, sessions::CallId call,
                                    StepTest until) {
    sync();
    if (auto r = refusal()) return Result{false, *r};
    if (!rules_.hasLevel(level)) return Result{false, "The machine has no level " + level.value + "."};
    if (mode_ != Mode::Paused) return Result{false, kNotPaused};   // AC-7
    if (count == 0) return Result{false, "A step needs a count of at least 1."};
    machine::Debugging& d = *debugging();
    std::size_t t = d.current();
    if (target) {
        const auto i = targetIndex(d, *target);
        if (!i) return Result{false, "The machine has no debug target " + target->value + "."};
        t = *i;
    }
    if (kind != StepKind::In && !d.target(t).info().offers.stepOverOut)
        return Result{false, "The target " + d.target(t).info().name.value + " offers no stepping over and out."};
    d.passOnce(d.current());   // EC-5: the instruction at a breakpoint is carried out
    mode_ = Mode::Stepping;
    level_ = level;
    kind_ = kind;
    stepTarget_ = t;
    until_ = kind == StepKind::Out ? StepTest{} : std::move(until);
    left_ = kind == StepKind::In || until_ ? count : 1;
    repeats_ = 0;
    hits_.clear();
    const machine::DebugTarget& dt = d.target(t);
    overCall_ = kind == StepKind::Over && dt.flowAt(dt.nextAddress()) == machine::Flow::Call;
    depth0_ = dt.callDepth();
    if (call) calls_.push_back(call);
    ctx_->machineSession().setPaused(false);   // the run state says running meanwhile (D-14)
    publish();
    return Result{true, {}, true};
}

RunControl::Result RunControl::pauseOnStart(std::optional<LevelName> level) {
    sync();
    if (auto r = refusal()) return Result{false, *r};
    if (!level) {
        armed_.reset();
        publish();
        return Result{};
    }
    if (!rules_.hasLevel(*level)) return Result{false, "The machine has no level " + level->value + "."};
    armed_ = *level;   // arming one level disarms the other
    rules_.startArmed(*level, *ctx_->machine());
    publish();
    return Result{};
}

void RunControl::pauseAtStart() {
    sync();
    pauseAtStart_ = true;
    publish();
}

} // namespace calcu::debugger
