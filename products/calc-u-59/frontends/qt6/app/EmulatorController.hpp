#pragma once
#include "TIAppPart.hpp"
#include "debugger/RunState.hpp"
#include "media/MediaTopics.hpp"
#include "qt6/ClipboardService.hpp"
#include "typing/HostKey.hpp"
#include "sessions/Listener.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <functional>
#include <memory>
#include <vector>

// The Qt face of the library's app session (LIB-3 AC-28; the Swift app's is
// App/EmulatorViewModel.swift over cu59::SessionThread): the session lives on
// the GUI thread, called at 60 Hz by a timer; this object is its frontend,
// through a calcu::sessions::Listener. Every action is a command
// (cu59::commands), everything shown comes from the listener's topic mirror;
// the mirror's changes become the signals below. It decides
// nothing and keeps no state of the machine (CLAUDE.md § "Frontend Parity").
// Qt6 sends neither suspend nor continue (LIB-3 AC-31; ODD59-25).
// Since M-53 it states and answers the clipboard host services with the
// library's ClipboardService (LIB-8 adapter P-E6, until LIB-14's SessionHost
// in M-57), and hands the library's paper and run-control topics to the
// widgets that draw them (addMessageHandler).
class EmulatorController : public QObject {
    Q_OBJECT
public:
    using DebugLevel = cu59::DebugLevel;

    explicit EmulatorController(QObject* parent = nullptr);
    ~EmulatorController() override;
    /// false: the ROM lookup failed (the dialog was shown); nothing else may be called.
    bool started() const { return m_app.session != nullptr; }

    // ── Machine ──────────────────────────────────────────────────────────────
    /// The model switch (LIB-18 `model`); the start is makeAppSession's `start`.
    void start(cu59::Model model);
    cu59::Model model() const;
    void resetMachine();
    void cleanResetMachine();
    QString moduleID() const;
    /// slot-1's picker of the library's media topic (LIB-7 AC-8, AC-47): the
    /// modules by their `description`, in `menu-sort` order (the Module group).
    std::vector<calcu::media::PickerEntry> modulePicker() const;
    /// A module of that picker chosen (`media.choose {place: slot-1}`): a
    /// replace and the power cycle (no-op if it is the module plugged now).
    void chooseModule(const QString& entryId);

    // ── Display ──────────────────────────────────────────────────────────────
    const cu59::LedFrame& display() const;
    double calcIndicatorOpacity() const;
    void setFullSpeed(bool on);
    const std::optional<cu59::CueCardContent>& cueCard() const;

    // ── Keys: the library's typing (LIB-5) ─────────────────────────────────
    /// An on-screen key (0-based row 0–8, col 0–4) down, and up (the pointer's).
    void screenKeyDown(int row, int col);
    void screenKeyUp();
    /// A host key as Qt reports it; the product's keyboard map decides.
    void hostKeyDown(const calcu::typing::HostKeyEvent& e);
    void hostKeyUp(const std::string& physical);
    void focusLost();
    /// The key drawn pressed (row × 5 + col; the last one live input holds), -1 none.
    int keyboardHeldKey() const;

    // ── Printer (the paper: the library's paper panel, LIB-8) ──────────────────
    /// The PC-100C's place and TRACE (`c59.printer`, adapter P-E1 until M-57).
    bool printerConnected() const;
    bool printerTrace() const;
    void togglePrinterConnection();

    // ── The library's widgets ───────────────────────────────────────────────
    /// A command of a widget of the component set (the paper panel, the
    /// run-control buttons): paper.copy, paper.cut, set-control, pause, step, resume.
    void sendCommand(const calcu::sessions::CommandName& name, calcu::sessions::Value args) { send(name, std::move(args)); }
    /// Every state and stream message from now on, after the topics' current
    /// state (`papers`, `run-state`, `trace` and what the paper streams hold).
    void addMessageHandler(std::function<void(const calcu::sessions::Message&)> handler);

    // ── Magnetic cards: the library's media topic and commands (C59-7, LIB-7 59) ──
    /// The card reader `card-reader` (TI-59 only); nullptr without one.
    const calcu::media::PlaceState* cardReader() const;
    /// A card passes through the reader now (Eject Card instead of the Crd buttons, AC-13).
    bool cardPassing() const;
    /// The saved cards for Arm Card (AC-14): the reader's picker without the
    /// empty entry and the implicit fresh card.
    std::vector<calcu::media::PickerEntry> savedCards() const;
    /// Arm Card (`media.choose`): the chosen card fitted, not fed.
    void armCard(const QString& entryId);
    /// Arm Fresh Card (`media.start-new`): a fresh card named `name` fitted;
    /// the refusal, "asks: <question>" when a card of that name is saved
    /// (send again with `confirmed`), empty when done (AC-15).
    QString armFreshCard(const QString& name, bool confirmed);
    void ejectCard();
    bool tapCard(double nx, double ny);
    std::optional<int> litCardCorner() const;

    // ── Presets: LIB-6 load-preset {ref: path} ───────────────────────────────
    void loadPreset(const QString& path);

    // ── Dropped files ────────────────────────────────────────────────────────
    cu59::DropAction dropAction(const QString& path) const;
    void openDroppedFile(const QString& path);

    // ── Debugger ─────────────────────────────────────────────────────────────
    void setLiveDebugEnabled(bool on);
    void setCpuDebugEnabled(bool on);
    /// The run control's state (LIB-10's `run-state`); nullptr before the first.
    const calcu::debugger::RunState* runState() const;
    bool isFrozen() const;
    /// Pause-on-start of the level `cpu` or `program` armed (FREEZE ON START).
    bool startArmed(const calcu::debugger::LevelName& level) const;
    void armStart(const calcu::debugger::LevelName& level);
    void disarmStart();

    const cu59::LiveDebugSnapshot& liveSnapshot() const;
    const std::vector<cu59::StepEntry>& programListing() const;
    cu59::ProgramSource programListingSource() const;
    /// Running: the last 32 instructions; frozen: the whole ring plus the next.
    const std::vector<cu59::InspectorEntry>& cpuHistory() const;
    const std::vector<uint32_t>& heatmapCounts() const;
    void clearHeatmap() { send(cu59::commands::kClearHeatmap); }

    DebugLevel debugLevel() const;
    void cycleDebugLevel() { send(cu59::commands::kCycleDebugLevel); }
    QString logText() const;
    void clearLog() { send(cu59::commands::kClearLog); }
    void dumpVars() { send(cu59::commands::kDumpVars); }
    void dumpScom() { send(cu59::commands::kDumpScom); }
    void dumpProg() { send(cu59::commands::kDumpProg); }
    void dumpMemory() { send(cu59::commands::kDumpMemory); }

    /// The session's emulated time (the request LIB-11's `ticks` uses): ticks
    /// and seconds; std::nullopt without a machine. For `--pacing-probe`.
    std::optional<std::pair<uint64_t, double>> emulatedTime();

    /// The TRACE button's trace (LIB-10's `trace` topic and command).
    bool traceRunning() const;
    void setTrace(bool on);

    QString asmFileName() const;
    int asmWordCount() const;
    QString asmStatus() const;
    bool asmOverlayActive() const;
    void loadAsmFile(const QString& path);
    void runAsmOverlay() { send(cu59::commands::kRunAsm); }
    void clearAsmOverlay() { send(cu59::commands::kClearAsm); }

signals:
    void modelChanged();
    void displayChanged();
    void cueCardChanged();
    void printerChanged();
    void cardStateChanged();
    void runStateChanged();      ///< the run control's state, full speed, playback, held key
    void liveSnapshotChanged();
    void cpuChanged();
    void heatmapChanged();
    void logChanged();
    void asmChanged();
    void traceChanged();
    void errorOccurred(const QString& title, const QString& message);
    void debugPanelRequested(int tab);  ///< KEYSTROKES DebugPanel: (0 CALCULATOR, 1 CPU, 2 LOG)

private:
    calcu::sessions::ReplyMessage send(const calcu::sessions::CommandName& name,
                                       calcu::sessions::Value args = calcu::sessions::Value::object({}));
    bool sendFlag(const calcu::sessions::CommandName& name, calcu::sessions::Value args);
    template <class P> const P* state(const calcu::sessions::TopicName& t) const { return m_listener.state<P>(t); }
    void onMessage(const calcu::sessions::Message&);
    void topicChanged(const calcu::sessions::TopicName&);

    calcu::sessions::ProductSession<cu59::TIAppPart> m_app;
    mutable calcu::sessions::Listener m_listener;   // stops listening before the session goes
    QTimer m_frameTimer;
    calcu::qt6::ClipboardService m_clipboard;
    std::vector<std::function<void(const calcu::sessions::Message&)>> m_handlers;
};
