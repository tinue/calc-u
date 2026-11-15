#pragma once
#include "CooperativePacing.hpp"
#include "FrameSampler.hpp"
#include "FreezeController.hpp"
#include "KeySequencer.hpp"
#include "MachineGuard.hpp"
#include "MachineModelInfo.hpp"
#include "ProductRoms.hpp"
#include "StateFile.hpp"
#include "TraceWriter.hpp"
#include "peripherals/Controls.hpp"
#include <atomic>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// One running calculator: the machine plus the runtime around it that both
// frontends share — the paced run loop and its thread, the debugger's freeze
// state (frozen, owner, armed freezes, stepping), the trace flags, timed key
// input and KEYSTROKES playback, the 60 Hz frame sampler and the two binary
// trace writers (manual and scripted). One session per machine (a model
// switch builds a new one); SessionController owns it. Everything runs on
// the host's one thread: with Threading::Cooperative pump() runs the machine
// (the Web); with Threading::Hosted the library's machine session does, and
// owns the machine (releaseMachine(); LIB-3 59's TIAppPart). See
// reference/SharedArchitecture.md.

namespace cu59 {

/// Which consumers currently need the core's trace ring (TRACE_* flags).
struct TraceConsumers {
    bool cpuPanelVisible = false;
    bool frozen = false;
    bool manualTrace = false;      ///< the "C" indicator / trace toggle
    bool freezeArmed = false;      ///< FREEZE ON START or next-PC-change armed
    bool scriptedTrace = false;
};

/// The debugger's freeze state. `generation` changes on every freeze, resume
/// and clear, so a view built for an earlier freeze can be recognised.
struct DebugState {
    bool frozen = false;
    std::optional<FreezeOwner> owner;   ///< the panel whose RESUME / STEP apply
    bool pcChangeArmed = false;         ///< CALCULATOR: FREEZE ON START (next step boundary)
    bool scanLoopExitArmed = false;     ///< CPU: FREEZE ON START (scan-loop exit)
    uint64_t generation = 0;
    bool operator==(const DebugState& o) const {
        return frozen == o.frozen && owner == o.owner && pcChangeArmed == o.pcChangeArmed &&
               scanLoopExitArmed == o.scanLoopExitArmed && generation == o.generation;
    }
};

/// Why a frozen machine is ready to be shown.
enum class FrozenViewReason {
    Freeze,           ///< FREEZE: at the next keycode boundary
    FreezeFired,      ///< an armed freeze fired
    StepKeycode,      ///< STEP in the CALCULATOR panel
    StepInstruction,  ///< STEP in the CPU panel
};

/// The trace flags for those consumers; with none active the core runs on its
/// TRACE_NONE fast path.
uint32_t traceFlagsFor(const TraceConsumers& c);

/// A card queued by KEYSTROKES ArmCard: / InsertCard:, waiting to be fed.
struct QueuedCard {
    std::string name;                                 ///< mem:… or a card file name
    std::shared_ptr<const CardDefinition> definition; ///< the file's CARD: block, if any
    bool insertNow = false;                           ///< InsertCard: fed at idle, playback waits for it
    int side = 0;                                     ///< side of a card file (0 or 1)
};

class EmulatorSession : private KeySequencer::Host {
public:
    struct Listener {
        virtual ~Listener() = default;
        /// debugState() changed off the caller's thread (an armed freeze fired).
        virtual void debugStateChanged() {}
        /// The machine is stopped at a point to show (after beginNextStep); on
        /// the emulation thread. Ignore it if debugState().generation moved on.
        virtual void frozenViewReady(uint64_t /*generation*/, FrozenViewReason) {}
        virtual void heldKeyChanged(int /*highlightID*/) {}
        virtual void fullSpeedChanged(bool) {}
        virtual void playbackStarted() {}
        virtual void playbackFinished() {}
        virtual void logLine(const std::string&) {}
        /// A scripted "Trace:" file could not be created.
        virtual void scriptedTraceFailed(const std::string& /*fileName*/) {}
        /// KEYSTROKES reached a reload; call hostDone() once it is applied.
        virtual void reloadRequested(const std::shared_ptr<const ReloadStage>&) {}
        /// KEYSTROKES reached a GUI control; call hostDone() once it is done.
        virtual void controlRequested(const KeystrokeEvent&) {}
    };

    /// Build the machine through the machine interface (calcu::machine::build,
    /// tiCatalogue()) from the looked-up ROM set, with `peripherals` in its
    /// places (C59-6). Reads no file. A module ID the set has no file for
    /// leaves the slot empty.
    EmulatorSession(Model model, const calcu::roms::RomSet& roms, MachinePeripherals peripherals,
                    Listener& listener, Threading threading = Threading::Cooperative);
    ~EmulatorSession();
    EmulatorSession(const EmulatorSession&) = delete;
    EmulatorSession& operator=(const EmulatorSession&) = delete;

    Model model() const { return m_model; }
    /// The machine and its adapters under MachineGuard (C59-1 design D-1):
    /// hold it per call, never across sync() or a listener call.
    MachineGuard::Access access() const { return m_guard.access(); }
    /// The machine without the guard: for code on the one thread that runs
    /// it (a test, a sync() job that takes access() itself).
    calcu::machine::Machine& machine() { return m_machineRef; }
    /// Hosted: the machine goes to the library's machine session, which owns
    /// it from now on; this session keeps using it until it is destroyed.
    std::unique_ptr<calcu::machine::Machine> releaseMachine() { return std::move(m_machine); }
    Threading threading() const { return m_threading; }
    bool ownsMachine() const { return m_machine != nullptr; }
    /// The bytes of the module plugged into `slot-1` (empty if none), kept for
    /// the debugger (adapter until M-53, C59-6 design §5).
    const std::vector<uint8_t>& moduleImage() const { return m_moduleImage; }

    // ── Places (C59-6; adapters until M-54, design §5) ───────────────────────
    // Each changes the place under the guard, between two advances, and
    // returns what follows (the power cycle of the pin-out, which the caller
    // runs, LIB-2 AC-24) or the refusal. Nothing changes on a refusal.
    /// The module of that ID into `slot-1` (a replace if one is there), or
    /// the slot emptied (std::nullopt).
    calcu::machine::ChangeResult setModule(const std::optional<std::string>& moduleID, const calcu::roms::RomSet& roms);
    /// The PC-100C attached with TRACE at `trace`, or detached. Before a
    /// detach the line being printed ends where the head is, and what is on
    /// the paper and not yet taken is handed back (EC-2).
    struct PrinterChange {
        calcu::machine::ChangeResult result;
        std::vector<std::string> lines;
        std::vector<PrinterCodeLine> codeLines;
    };
    PrinterChange setPrinter(bool connected, bool trace);
    /// A control of the PC-100C under the guard: a key down or up, the switch
    /// on or off (C59-6 AC-5). False without a PC-100C.
    bool printerControl(const calcu::machine::ControlId& id, bool on);

    // ── Run loop ─────────────────────────────────────────────────────────────
    void start() { m_runner->start(); }
    void stop() { m_runner->stop(); }
    bool isRunning() const { return m_runner->isRunning(); }
    void post(std::function<void()> job) { m_runner->post(std::move(job)); }
    /// Stop first, or this waits for the run loop.
    void sync(const std::function<void()>& job) { m_runner->sync(job); }
    void setFullSpeed(bool on) override;
    bool isFullSpeed() const { return m_runner->fullSpeed(); }
    /// Cooperative: run the batches, key input and jobs due by `now`.
    /// Hosted: the jobs, and the key input when no slice ran since the last pump.
    void pump(std::chrono::steady_clock::time_point now);

    // ── Hosted (LIB-3 59: TIFreezeGate, TIKeySequencerHook) ──────────────────
    /// One advance of a machine-session slice, with the freeze checks.
    calcu::machine::Ticks advanceHosted(calcu::machine::Ticks atLeast, bool& stopped) {
        return m_runner->advanceHosted(atLeast, stopped);
    }
    /// A slice ended at wall time `due`: key input advances to it.
    void sliceEnded(std::chrono::steady_clock::time_point due) {
        m_sliceRan = true;
        m_keys->advance(due);
    }

    /// Since the last call: a batch ran (TI-58C RAM may have changed).
    bool takeRamDirty() { return m_ramDirty.exchange(false); }

    // ── Debugger: freeze and step (UI thread) ────────────────────────────────
    DebugState debugState() const;
    bool frozen() const { return m_frozen.load(); }
    /// Stop, freeze for `owner`, disarm both armed freezes; the machine runs on
    /// to the next keycode boundary, then frozenViewReady(Freeze).
    void freeze(FreezeOwner owner);
    /// Freeze where the machine stands (stopped already), no step.
    void freezeInPlace(FreezeOwner owner);
    /// Unfreeze and start the run loop.
    void resume();
    /// Unfreeze without starting (the caller has stopped the loop to change state).
    void clearFreeze();
    /// Freeze before the next program byte executes (CALCULATOR) or when the
    /// keyboard scan loop exits (CPU); arming one disarms the other.
    void armPcChange();
    void armScanLoopExit();
    void disarmPcChange();
    void disarmScanLoopExit();
    /// While frozen: one keycode (→ StepKeycode) or one instruction, stepping
    /// through a HOLD (WAIT Dn) of up to 32 instructions (→ StepInstruction).
    void stepKeycode();
    void stepInstruction();
    /// The CPU panel is visible (it needs the trace ring).
    void setCpuPanelVisible(bool visible);

    // ── Keys ─────────────────────────────────────────────────────────────────
    /// Press / release a matrix code (logged to the manual trace).
    void pressKey(uint8_t matrixCode) override;
    void releaseKey(uint8_t matrixCode) override;
    void keyboardDown(const std::vector<uint8_t>& codes) { m_keys->keyDown(codes); }
    void keyboardUp() { m_keys->keyUp(); }
    void keyboardRelease() { m_keys->releaseAll(); }
    void playKeystrokes(std::vector<KeystrokeEvent> events) { m_keys->play(std::move(events)); }
    void cancelKeystrokes() { m_keys->cancel(); }
    bool keystrokesPlaying() const { return m_keys->isPlaying(); }
    /// A script plays or waits to play (LIB-3 59: the preset operation's end).
    bool keystrokesPending() const { return m_keys->scriptPending(); }
    /// The next queued card the machine takes where it stands: the first
    /// InsertCard card in the idle scan loop, the first ArmCard card in the
    /// card wait of Write / INV Write (TICardReaderAdapter::isInCardWait). UI thread.
    std::optional<QueuedCard> takeQueuedCard();
    /// A reload has been applied, or an InsertCard card has passed (or failed).
    void hostDone();
    /// Drop the queued cards (a new preset, a reset); returns their names.
    std::vector<std::string> clearQueuedCards();

    // ── Frames ───────────────────────────────────────────────────────────────
    FrameUpdate tick(int debugLevel, bool cardSwiping) {
        auto a = access();
        return m_sampler.tick(a.machine(), debugLevel, cardSwiping);
    }

    // ── Traces ───────────────────────────────────────────────────────────────
    TraceWriter& manualTrace() { return m_manualTrace; }
    TraceWriter& scriptedTrace() { return m_scriptedTrace; }
    /// The manual ("C" indicator) trace: a fresh file at `path`.
    bool openManualTrace(const std::filesystem::path& path, uint64_t maxBytes);
    /// Drain what is left and close it.
    void closeManualTrace();
    /// A trace writer is open: drainTrace() at 60 Hz.
    bool needsDrain() const { return m_manualTrace.isOpen() || m_scriptedTrace.isOpen(); }
    /// Where scripted "Trace:" files go, and the session size limit.
    void setTraceLocation(const std::filesystem::path& dir, uint64_t maxBytes);
    /// Move the core's trace ring into the open writers (both share the one
    /// ring cursor). Returns which writers hit their size limit and closed.
    struct DrainResult { bool manualLimit = false; bool scriptedLimit = false; };
    DrainResult drainTrace();

    /// "Elapsed: m:ss.mmm" since the last zeroElapseTime (or reset).
    std::string elapsedReport() const;

private:
    // KeySequencer::Host (pressKey, releaseKey and setFullSpeed are public above)
    bool isKeyReady() override { return access().machine().isKeyReady(); }
    bool isFrozen() override { return m_frozen.load(); }
    void heldKeyChanged(int id) override { m_listener.heldKeyChanged(id); }
    void scriptedTrace(const std::optional<std::string>& fileName) override;
    void zeroElapseTime() override { m_elapseBaseline = access().machine().tickCount(); }
    void onFreezeFired(FreezeOwner owner);       // emulation thread
    /// Set the frozen state (and owner, generation); disarms both armed freezes when freezing.
    uint64_t setFrozenState(bool frozen, std::optional<FreezeOwner> owner);
    void applyTraceFlags();
    void reportElapseTime() override { m_listener.logLine(elapsedReport()); }
    void playbackStarted() override { m_listener.playbackStarted(); }
    void playbackFinished() override;
    void applyReload(const std::shared_ptr<const ReloadStage>& stage) override;
    void queueCard(const KeystrokeEvent& event) override;
    void control(const KeystrokeEvent& event) override {
        ++m_hostPending;
        m_listener.controlRequested(event);
    }
    bool hostPending() override { return m_hostPending.load() > 0; }

    const Model m_model;
    const Threading m_threading;
    Listener& m_listener;
    std::unique_ptr<calcu::machine::Machine> m_machine;
    calcu::machine::Machine& m_machineRef{*m_machine};
    mutable MachineGuard m_guard{m_machineRef};
    bool m_sliceRan = false;   ///< hosted: a slice ended since the last pump
    std::vector<uint8_t> m_moduleImage;
    FreezeController m_freeze;
    FrameSampler m_sampler;
    TraceWriter m_manualTrace;
    TraceWriter m_scriptedTrace;
    std::mutex m_traceMutex;           ///< serialises drainTrace with scripted open/close
    std::filesystem::path m_traceDir;
    uint64_t m_traceMaxBytes = 50'000'000;
    std::vector<CpuFrame> m_drainBuf = std::vector<CpuFrame>(1024);  ///< drainTrace scratch, under m_traceMutex
    mutable std::mutex m_stateMutex;   ///< m_state, m_cpuPanelVisible, trace-flag updates
    DebugState m_state;
    bool m_cpuPanelVisible = false;
    std::atomic<bool> m_frozen{false};  ///< m_state.frozen, for the key sequencer
    std::atomic<bool> m_ramDirty{false};
    std::atomic<uint64_t> m_elapseBaseline{0};
    std::mutex m_cardMutex;            ///< m_queuedCards
    std::deque<QueuedCard> m_queuedCards;
    std::atomic<int> m_hostPending{0}; ///< reloads and InsertCard cards not done yet
    std::unique_ptr<CooperativePacing> m_runner;
    std::unique_ptr<KeySequencer> m_keys;   ///< last: its threads stop first
};

} // namespace cu59
