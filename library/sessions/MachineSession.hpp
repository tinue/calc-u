#pragma once
#include "machine/Machine.hpp"
#include "pacing/Pacer.hpp"
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

// The machine session (glossary): a machine, its pacing and its input
// queue. It owns the pacer (library/pacing) and implements its
// SliceRunner. Single-threaded: used on the thread the host chose.

namespace calcu::sessions {

inline constexpr machine::Ticks kNever = std::numeric_limits<machine::Ticks>::max();

// The input queue: the typing engine (library/typing).
class SliceHook {
public:
    virtual machine::Ticks nextDue(machine::Ticks now) const = 0;   // kNever when idle
    virtual void act(machine::Machine&, machine::Ticks now, pacing::WallTime due) = 0;
    // Drop the text still waiting and release the keys
    // held (before a build, Reset, Cold boot and power cycle; the
    // typing engine implements it). Nothing by default.
    virtual void drop() {}

protected:
    ~SliceHook() = default;
};

// The debugger's run control (pause, breakpoints): how far a slice runs.
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
    MachineSession(pacing::WallClock, std::optional<std::uint32_t> jitterSeed);   // starts afresh

    // A new machine with its pacing profile; starts afresh with the new
    // profile and is not paused.
    void setMachine(std::unique_ptr<machine::Machine>, const pacing::PacingProfile&, int soundRate);
    std::unique_ptr<machine::Machine> takeMachine();
    machine::Machine* machine() { return machine_.get(); }
    const machine::Machine* machine() const { return machine_.get(); }
    int soundRate() const { return soundRate_; }
    std::uint64_t generation() const { return generation_; }   // grows with every setMachine/takeMachine
    // Called before the machine or a peripheral of
    // it changes, while the old one is still there: by setMachine and
    // takeMachine when a machine runs, and by the startup's MachineControl before
    // an attach, detach or replace. The app session calls its parts'
    // beforeMachineChange from it.
    void setAboutToChange(std::function<void()> f) { aboutToChange_ = std::move(f); }
    void aboutToChange() {
        if (aboutToChange_ && machine_) aboutToChange_();
    }

    void addHook(SliceHook& h) { hooks_.push_back(&h); }
    // SliceHook::drop() on every hook.
    void dropInput();
    void setGate(AdvanceGate* g) { gate_ = g; }

    void setSuspended(bool);   // false starts afresh
    bool suspended() const { return suspended_; }
    void setPaused(bool);      // the debugger's pause; false starts afresh
    bool paused() const { return paused_; }
    bool runs() const { return machine_ && !suspended_ && !paused_; }

    pacing::PaceResult pace(pacing::Speed);            // one host call: nothing when !runs()
    machine::Ticks advanceUnpaced(machine::Ticks);     // virtual-time hosts: nothing when !runs()
    // The one advance: in pieces cut at each hook's nextDue, through the gate
    // when one is set; at least `atLeast` ticks unless the gate stops.
    machine::Ticks advance(machine::Ticks atLeast, bool& stoppedEarly);
    // The earliest tick a hook is due at from now (kNever: none): a host
    // that waits for a hook's work ends its advances there.
    machine::Ticks nextDue() const;
    // The machine waits for the user (machine/Idle.hpp).
    bool waitsForInput() const;

    void setSliceSource(SliceSource* s) { source_ = s; }
    pacing::SliceResult runSlice(const pacing::SliceRequest&) override;

    void startAfresh();                                   // Pacer::startAfresh
    std::uint64_t freshStarts() const { return fresh_; }  // how often the pacer was told (tests)

private:
    void actOnDueHooks();

    pacing::Pacer pacer_;
    std::unique_ptr<machine::Machine> machine_;
    int soundRate_ = 0;
    std::uint64_t generation_ = 0;
    std::vector<SliceHook*> hooks_;
    AdvanceGate* gate_ = nullptr;
    std::function<void()> aboutToChange_;
    SliceSource* source_ = nullptr;
    bool suspended_ = false, paused_ = false;
    pacing::WallTime due_{0};
    std::uint64_t fresh_ = 0;
};

} // namespace calcu::sessions
