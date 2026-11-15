#pragma once
#include "machine/Machine.hpp"
#include "pacing/Pacer.hpp"
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

// The machine session (LIB-3 design §3.7; glossary): a machine, its pacing
// and its input queue. It owns the pacer (LIB-17) and implements its
// SliceRunner. Single-threaded: used on the thread the host chose.

namespace calcu::sessions {

inline constexpr machine::Ticks kNever = std::numeric_limits<machine::Ticks>::max();

// The input queue: LIB-5's typing engine; today's typing behind adapters.
class SliceHook {
public:
    virtual machine::Ticks nextDue(machine::Ticks now) const = 0;   // kNever when idle
    virtual void act(machine::Machine&, machine::Ticks now, pacing::WallTime due) = 0;
    // LIB-18 design §3.4, D-12: drop the text still waiting and release the
    // keys held (before a build, Reset, Cold boot, power cycle).
    virtual void drop() {}

protected:
    ~SliceHook() = default;
};

// AC-22: a media place that waits for a medium (LIB-7).
class WaitProbe {
public:
    virtual bool waitsForMedium(const machine::Machine&) const = 0;

protected:
    ~WaitProbe() = default;
};

// Reserved for LIB-10's run control (pause, breakpoints).
class AdvanceGate {
public:
    virtual machine::Ticks advance(machine::Machine&, machine::Ticks atLeast, bool& stoppedEarly) = 0;

protected:
    ~AdvanceGate() = default;
};

class MachineSession;

// Where the ticks of a slice come from while an operation runs (the app
// session's running operation).
class SliceSource {
public:
    virtual pacing::SliceResult runSlice(MachineSession&, const pacing::SliceRequest&) = 0;

protected:
    ~SliceSource() = default;
};

class MachineSession final : public pacing::SliceRunner {
public:
    MachineSession(pacing::WallClock, std::optional<std::uint32_t> jitterSeed);   // starts afresh (AC-23)

    // A new machine with its pacing profile; starts afresh (AC-23; LIB-17 AC-14).
    void setMachine(std::unique_ptr<machine::Machine>, const pacing::PacingProfile&, int soundRate);
    std::unique_ptr<machine::Machine> takeMachine();
    machine::Machine* machine() { return machine_.get(); }
    const machine::Machine* machine() const { return machine_.get(); }
    int soundRate() const { return soundRate_; }
    std::uint64_t generation() const { return generation_; }   // grows with every setMachine/takeMachine

    void addHook(SliceHook& h) { hooks_.push_back(&h); }
    void addWaitProbe(WaitProbe& p) { probes_.push_back(&p); }
    void setGate(AdvanceGate* g) { gate_ = g; }
    void dropInput();   // LIB-18 §3.4: drop() on every hook

    void setSuspended(bool);   // AC-24; false starts afresh
    bool suspended() const { return suspended_; }
    void setPaused(bool);      // reserved for LIB-10 (AC-25); false starts afresh (AC-23)
    bool paused() const { return paused_; }
    bool runs() const { return machine_ && !suspended_ && !paused_; }

    pacing::PaceResult pace(pacing::Speed);            // one host call: nothing when !runs() (AC-3, AC-24, AC-25)
    machine::Ticks advanceUnpaced(machine::Ticks);     // virtual-time hosts: nothing when !runs()
    // The one advance: in pieces cut at each hook's nextDue, through the gate
    // when one is set; at least `atLeast` ticks unless the gate stops.
    machine::Ticks advance(machine::Ticks atLeast, bool& stoppedEarly);
    // The earliest tick a hook is due at from now (kNever: none): a host
    // that waits for a hook's work ends its advances there (LIB-5 D-5).
    machine::Ticks nextDue() const;
    // AC-22: key readiness or a waiting media place, unless switched off.
    bool waitsForInput() const;
    void setReportsWaiting(bool r) { reportsWaiting_ = r; }

    void setSliceSource(SliceSource* s) { source_ = s; }
    pacing::SliceResult runSlice(const pacing::SliceRequest&) override;

    void startAfresh();                                   // Pacer::startAfresh (AC-23)
    std::uint64_t freshStarts() const { return fresh_; }  // how often the pacer was told (tests, AC-23)

private:
    void actOnDueHooks();

    pacing::Pacer pacer_;
    std::unique_ptr<machine::Machine> machine_;
    int soundRate_ = 0;
    std::uint64_t generation_ = 0;
    std::vector<SliceHook*> hooks_;
    std::vector<WaitProbe*> probes_;
    AdvanceGate* gate_ = nullptr;
    SliceSource* source_ = nullptr;
    bool suspended_ = false, paused_ = false, reportsWaiting_ = true;
    pacing::WallTime due_{0};
    std::uint64_t fresh_ = 0;
};

} // namespace calcu::sessions
