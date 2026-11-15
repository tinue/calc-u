#pragma once
#include "TIAppPart.hpp"
#include "../core/TIAdapters.hpp"

// The two nested helpers of cu59::TIAppPart, shared by TIAppPart.cpp (the
// session plumbing) and TIAppPartFlows.cpp (the product's flows). Not for
// other users.

namespace cu59 {

// The machine session's adapters (LIB-3 59 design §5).

struct TIAppPart::Hooks final : calcu::sessions::SliceHook, calcu::sessions::AdvanceGate, calcu::sessions::WaitProbe {
    TIAppPart& part;
    calcu::machine::Ticks last = 0;
    explicit Hooks(TIAppPart& p) : part(p) {}

    // TIKeySequencerHook: the KeySequencer at each slice end, with the wall
    // time the slice was due at (LIB-17 adapter SliceRequest::due).
    calcu::machine::Ticks nextDue(calcu::machine::Ticks now) const override { return now > last ? now : calcu::sessions::kNever; }
    void act(calcu::machine::Machine&, calcu::machine::Ticks now, calcu::pacing::WallTime due) override {
        last = now;
        if (EmulatorSession* s = part.m_session.get()) s->sliceEnded(steadyTimeOf(due));
    }
    // TIFreezeGate: the freeze and breakpoint checks inside the advance.
    calcu::machine::Ticks advance(calcu::machine::Machine&, calcu::machine::Ticks atLeast, bool& stoppedEarly) override {
        EmulatorSession* s = part.m_session.get();
        if (!s) {
            stoppedEarly = true;
            return 0;
        }
        return s->advanceHosted(atLeast, stoppedEarly);
    }
    // TICardWaitAdapter: the TI-59 needs a card now (LIB-17 AC-16).
    bool waitsForMedium(const calcu::machine::Machine& m) const override {
        const auto* r = cardReaderOf(const_cast<calcu::machine::Machine&>(m));
        return r && r->isInCardWait() && r->isWaitingForCard();   // the idle loop polls the switch too
    }
};

// EmulatorSession events, inside an advance or a job; each waits for the end
// of the call and is dropped if its EmulatorSession has been replaced meanwhile.
struct TIAppPart::Events : EmulatorSession::Listener {
    TIAppPart& c;
    const uint64_t generation;
    Events(TIAppPart& part, uint64_t gen) : c(part), generation(gen) {}

    void debugStateChanged() override {
        c.later(generation, [&c = c] { c.pullDebugState(); });
    }
    void frozenViewReady(uint64_t freezeGeneration, FrozenViewReason reason) override {
        c.later(generation, [&c = c, freezeGeneration, reason] { c.onFrozenView(freezeGeneration, reason); });
    }
    void heldKeyChanged(int id) override {
        c.later(generation, [&c = c, id] {
            c.m_heldKey = id;
            c.emitChanged(RunStateChanged);
        });
    }
    void fullSpeedChanged(bool on) override {
        c.later(generation, [&c = c, on] {
            if (c.m_fullSpeed == on) return;
            c.m_fullSpeed = on;
            c.emitChanged(RunStateChanged);
        });
    }
    void playbackStarted() override { setPlaying(true); }
    void playbackFinished() override { setPlaying(false); }
    void reloadRequested(const std::shared_ptr<const ReloadStage>& stage) override {
        c.later(generation, [&c = c, stage] { c.applyReloadStage(*stage); });
    }
    void controlRequested(const KeystrokeEvent& event) override {
        c.later(generation, [&c = c, event] { c.applyControl(event); });
    }
    void logLine(const std::string& line) override {
        c.later(generation, [&c = c, line] { c.log({line}); });
    }
    void scriptedTraceFailed(const std::string& fileName) override {
        c.later(generation, [&c = c, fileName] {
            c.error("Trace error", "KEYSTROKES \"Trace: " + fileName + "\" could not create its file in " +
                                       c.m_setup.trace.dir.string());
        });
    }

private:
    void setPlaying(bool on) {
        c.later(generation, [&c = c, on] {
            c.m_keystrokesPlaying = on;
            c.emitChanged(RunStateChanged);
        });
    }
};

} // namespace cu59
