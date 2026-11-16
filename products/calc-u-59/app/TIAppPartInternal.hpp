#pragma once
#include "TIAppPart.hpp"
#include "../core/TIAdapters.hpp"

// The two nested helpers of cu59::TIAppPart, shared by TIAppPart.cpp (the
// session plumbing) and TIAppPartFlows.cpp (the product's flows). Not for
// other users.

namespace cu59 {

// The machine session's adapters (LIB-3 59 design §5).

struct TIAppPart::Hooks final : calcu::sessions::SliceHook, calcu::sessions::WaitProbe {
    TIAppPart& part;
    calcu::machine::Ticks last = 0;
    explicit Hooks(TIAppPart& p) : part(p) {}

    // TIKeystrokePlayback at each slice end, with the wall time the slice
    // was due at (LIB-17 adapter SliceRequest::due, until M-54).
    calcu::machine::Ticks nextDue(calcu::machine::Ticks now) const override { return now > last ? now : calcu::sessions::kNever; }
    void act(calcu::machine::Machine&, calcu::machine::Ticks now, calcu::pacing::WallTime due) override {
        last = now;
        if (EmulatorSession* s = part.m_session.get()) s->sliceEnded(steadyTimeOf(due));
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
    void traceRequested(const std::optional<std::string>& fileName) override {
        c.later(generation, [&c = c, fileName] { c.scriptedTrace(fileName); });
    }
    bool paused() const override { return c.isFrozen(); }

private:
    void setPlaying(bool on) {
        c.later(generation, [&c = c, on] {
            c.m_keystrokesPlaying = on;
            c.emitChanged(RunStateChanged);
        });
    }
};

} // namespace cu59
