#pragma once
#include "FrameSampler.hpp"
#include "TIKeystrokePlayback.hpp"
#include "MachineModelInfo.hpp"
#include "ProductRoms.hpp"
#include "StateFile.hpp"
#include "machine/Machine.hpp"
#include "typing/TypingEngine.hpp"
#include "peripherals/Controls.hpp"
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// One running calculator: the machine plus the runtime around it that every
// frontend shares — timed key input and KEYSTROKES playback and the 60 Hz
// frame sampler. One per machine (a model switch builds a new one);
// cu59::TIAppPart owns it. The library's machine session runs and owns the
// machine (releaseMachine()); this keeps using it on the session's one
// thread (ADR 0009): sliceEnded() moves KEYSTROKES playback on
// (TIKeystrokePlayback, until M-54). Keys go through the library's typing
// engine (LIB-5). Since M-53 the pause, the steps and the traces are the
// library's run control and trace session (LIB-10; TIAppPart wires the
// KEYSTROKES directives to them, adapter L-E1). See
// reference/SharedArchitecture.md.

namespace cu59 {

/// A card queued by KEYSTROKES ArmCard: / InsertCard:, waiting to be fed.
struct QueuedCard {
    std::string name;                                 ///< mem:… or a card file name
    std::shared_ptr<const CardDefinition> definition; ///< the file's CARD: block, if any
    bool insertNow = false;                           ///< InsertCard: fed at idle, playback waits for it
    int side = 0;                                     ///< side of a card file (0 or 1)
};

class EmulatorSession : private TIKeystrokePlayback::Host {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void fullSpeedChanged(bool) {}
        virtual void playbackStarted() {}
        virtual void playbackFinished() {}
        virtual void logLine(const std::string&) {}
        /// KEYSTROKES `Trace:` (L-E1): start a trace of that file, or end it (nullopt).
        virtual void traceRequested(const std::optional<std::string>&) {}
        /// KEYSTROKES reached a reload; call hostDone() once it is applied.
        virtual void reloadRequested(const std::shared_ptr<const ReloadStage>&) {}
        /// KEYSTROKES reached a GUI control; call hostDone() once it is done.
        virtual void controlRequested(const KeystrokeEvent&) {}
        /// The machine is paused by the run control (playback and queued cards wait).
        virtual bool paused() const { return false; }
    };

    /// Build the machine through the machine interface (calcu::machine::build,
    /// tiCatalogue()) from the looked-up ROM set, with `peripherals` in its
    /// places (C59-6). Reads no file. A module ID the set has no file for
    /// leaves the slot empty.
    EmulatorSession(Model model, const calcu::roms::RomSet& roms, MachinePeripherals peripherals,
                    Listener& listener);
    ~EmulatorSession();
    EmulatorSession(const EmulatorSession&) = delete;
    EmulatorSession& operator=(const EmulatorSession&) = delete;

    Model model() const { return m_model; }
    /// The machine and its adapters (C59-1's TI adapters).
    class Access {
    public:
        explicit Access(calcu::machine::Machine& m) : m_m(m) {}
        calcu::machine::Machine& machine() { return m_m; }
        TICardReaderAdapter& cardReader() { return *cardReaderOf(m_m); }
        TIMemoryAdapter& memory() { return *memoryOf(m_m); }
        TIDebugPanelAdapter& debugPanel() { return *debugPanelOf(m_m); }

    private:
        calcu::machine::Machine& m_m;
    };
    Access access() const { return Access(m_machineRef); }
    calcu::machine::Machine& machine() { return m_machineRef; }
    /// The machine goes to the library's machine session, which owns it from
    /// now on; this session keeps using it until it is destroyed.
    std::unique_ptr<calcu::machine::Machine> releaseMachine() { return std::move(m_machine); }
    bool ownsMachine() const { return m_machine != nullptr; }
    /// The bytes of the module plugged into `slot-1` (empty if none), kept for
    /// the debugger (adapter until M-56, C59-6 design §5).
    const std::vector<uint8_t>& moduleImage() const { return m_moduleImage; }

    // ── Places (C59-6; adapters until M-54, design §5) ───────────────────────
    // Each changes the place between two advances, and
    // returns what follows (the power cycle of the pin-out, which the caller
    // runs, LIB-2 AC-24) or the refusal. Nothing changes on a refusal.
    /// The module of that ID into `slot-1` (a replace if one is there), or
    /// the slot emptied (std::nullopt).
    calcu::machine::ChangeResult setModule(const std::optional<std::string>& moduleID, const calcu::roms::RomSet& roms);
    /// The PC-100C attached with TRACE at `trace`, or detached. A line being
    /// printed at a detach ends where the head is and goes onto its paper
    /// (LIB-8 EC-7, TIMachine::unplug).
    calcu::machine::ChangeResult setPrinter(bool connected, bool trace);
    /// A control of the PC-100C: a key down or up, the switch
    /// on or off (C59-6 AC-5). False without a PC-100C.
    bool printerControl(const calcu::machine::ControlId& id, bool on);

    void setFullSpeed(bool on) override;
    bool isFullSpeed() const { return m_fullSpeed; }
    /// After a host call: the playback at `now` when no slice ended since the last pump.
    void pump(std::chrono::steady_clock::time_point now);

    /// A slice of the machine session ended at wall time `due`: playback advances to it.
    void sliceEnded(std::chrono::steady_clock::time_point due) {
        m_sliceRan = true;
        m_ramDirty = true;   // TI-58C persist debounce
        m_playback.advance(due);
    }

    /// Since the last call: a slice ran (TI-58C RAM may have changed).
    bool takeRamDirty() { return std::exchange(m_ramDirty, false); }

    // ── Keys ─────────────────────────────────────────────────────────────────
    /// The typing engine KEYSTROKES' keys go to (LIB-5); null: none.
    void setTyping(calcu::typing::TypingEngine* engine) { m_typing = engine; }
    void playKeystrokes(std::vector<KeystrokeEvent> events) { m_playback.play(std::move(events)); }
    void cancelKeystrokes() { m_playback.cancel(); }
    bool keystrokesPlaying() const { return m_playback.isPlaying(); }
    /// A script plays or waits to play (LIB-3 59: the preset operation's end).
    bool keystrokesPending() const { return m_playback.pending(); }
    /// The next queued card the machine takes where it stands: the first
    /// InsertCard card in the idle scan loop, the first ArmCard card in the
    /// card wait of Write / INV Write (TICardReaderAdapter::isInCardWait).
    std::optional<QueuedCard> takeQueuedCard();
    /// A reload has been applied, a control is done, or an InsertCard card
    /// has passed (or failed).
    void hostDone();
    /// Drop the queued cards (a new preset, a reset); returns their names.
    std::vector<std::string> clearQueuedCards();

    // ── Frames ───────────────────────────────────────────────────────────────
    FrameUpdate tick(int debugLevel, bool cardSwiping) { return m_sampler.tick(m_machineRef, debugLevel, cardSwiping); }

    /// "Elapsed: m:ss.mmm" since the last zeroElapseTime (or reset).
    std::string elapsedReport() const;

private:
    // TIKeystrokePlayback::Host (setFullSpeed is public above)
    bool typeKey(uint8_t matrixCode) override;
    bool typingIdle() override { return !m_typing || m_typing->idle(); }
    bool isKeyReady() override { return access().machine().isKeyReady(); }
    bool isFrozen() override { return m_listener.paused(); }
    void scriptedTrace(const std::optional<std::string>& fileName) override { m_listener.traceRequested(fileName); }
    void zeroElapseTime() override { m_elapseBaseline = access().machine().tickCount(); }
    void reportElapseTime() override { m_listener.logLine(elapsedReport()); }
    void playbackStarted() override { m_listener.playbackStarted(); }
    void playbackFinished() override;
    void applyReload(const std::shared_ptr<const ReloadStage>& stage) override;
    void queueCard(const KeystrokeEvent& event) override;
    void control(const KeystrokeEvent& event) override {
        ++m_hostPending;
        m_listener.controlRequested(event);
    }
    bool hostPending() override { return m_hostPending > 0; }

    const Model m_model;
    Listener& m_listener;
    std::unique_ptr<calcu::machine::Machine> m_machine;
    calcu::machine::Machine& m_machineRef{*m_machine};
    bool m_sliceRan = false;   ///< a slice ended since the last pump
    bool m_fullSpeed = false;
    std::vector<uint8_t> m_moduleImage;
    FrameSampler m_sampler;
    bool m_ramDirty = false;
    uint64_t m_elapseBaseline = 0;
    std::deque<QueuedCard> m_queuedCards;
    int m_hostPending = 0;   ///< reloads, controls and InsertCard cards not done yet
    calcu::typing::TypingEngine* m_typing = nullptr;
    TIKeystrokePlayback m_playback{*this};
};

} // namespace cu59
