#pragma once
#include "DebugLog.hpp"
#include "LedGeometry.hpp"
#include "TIAppPart.hpp"
#include "debugger/RunState.hpp"
#include "media/MediaTopics.hpp"
#include "pacing/WallClock.hpp"
#include "qt6/ProductHost.hpp"
#include "qt6/SessionHost.hpp"
#include "typing/HostKey.hpp"
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The product's glue around the session host (LIB-14 design §10.4, M-57; as
// Calc-U-1600's MachineController): makes Calc-U-59's app session
// (cu59::makeAppSession with LIB-14's SettingsPart in front, over
// AppPaths::folders()) and the Qt6 component set's SessionHost over it
// (calcu::qt6::ProductHost), which hosts it on the GUI thread (the
// 60 Hz timer, the listener, the host services, harness attach). The shell
// draws the menus, the bar, the settings, About, the notices, "Loading…" and
// "ROMs missing" from the host; this class keeps what only the product
// draws: the display, the cue card, the card controls' reads, the PC-100C's
// TRACE position, the debug panel's topics and commands (C59-4), the LOG
// tab's view of the log. It decides nothing and keeps no state of the
// machine (CLAUDE.md § "Frontend Parity"). Qt6 sends neither suspend nor
// continue (LIB-3 AC-31; ODD59-25).
class EmulatorController : public QObject {
    Q_OBJECT
public:
    using DebugLevel = cu59::DebugLevel;

    /// `clock`: the host's wall clock (the tests' scripted one).
    explicit EmulatorController(QObject* parent = nullptr, calcu::pacing::WallClock clock = cu59::steadyWallTime);
    ~EmulatorController() override;

    calcu::qt6::SessionHost& host() { return m_session->host(); }
    cu59::TIAppPart& app() { return m_session->part(); }
    calcu::sessions::AppSession& session() { return m_session->session(); }
    /// The ROM lookup found the full set; else the topic `roms-missing` says
    /// which files are missing and the shell shows it (LIB-14 AC-19).
    bool romsFound() const { return m_romsFound; }
    /// The host's timer, then LIB-18's `start` (the startup model with its
    /// startup preset, or its built-in default: ML, no printer).
    void start();

    // ── Machine (drawn from the control state; the shell sends its commands) ──
    /// The running model ("TI-59", …); empty before the first control state.
    std::string modelId() const;
    bool isTI59() const { return modelId() == "TI-59"; }
    /// The PC-100C in `printer` (the control state), and TRACE (`c59.printer`).
    bool printerConnected() const;
    bool printerTrace() const;
    /// `[display] led-style` as the `settings` topic has it.
    cu59::LedStyle ledStyle() const;

    // ── Display ──────────────────────────────────────────────────────────────
    const cu59::LedFrame& display() const;
    double calcIndicatorOpacity() const;
    const std::optional<cu59::CueCardContent>& cueCard() const;

    // ── Keys: the library's typing (LIB-5); the pointer is the shell's ─────
    void hostKeyDown(const calcu::typing::HostKeyEvent& e);
    void hostKeyUp(const std::string& physical);
    void focusLost();
    /// The key drawn pressed (row × 5 + col; the last one live input holds), -1 none.
    int keyboardHeldKey() const;

    // ── The library's widgets ───────────────────────────────────────────────
    /// A command of a widget of the component set (the paper panel, the
    /// run-control buttons): paper.copy, paper.cut, set-control, pause, step, resume.
    void sendCommand(const calcu::sessions::CommandName& name, calcu::sessions::Value args) { send(name, std::move(args)); }

    // ── Magnetic cards: the library's media topic and commands (C59-7, LIB-7 59) ──
    /// A place of the media topic (`slot-1`'s picker, LIB-7 AC-47); nullptr without one.
    const calcu::media::PlaceState* mediaPlace(const std::string& place) const;
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
    /// A tap on the card window, in canvas fractions: a top corner of a fitted
    /// card on show feeds that side (`c59.tap-card`). False: nothing fed.
    bool tapCard(double nx, double ny);
    /// The fitted magnetic card is on show (no module cue card covers it).
    bool cardOnShow() const;
    std::optional<int> litCardCorner() const;

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

    /// FREEZE and FREEZE ON START of a tab (C59-4 AC-5, AC-7).
    cu59::FreezeButtons freezeButtons(const calcu::debugger::LevelName& level) const {
        return cu59::freezeButtons(level, runState());
    }
    /// A product entry of the control state (C59-4 AC-25): c59.asm-run,
    /// c59.trace, c59.calculator-tab; true while there is none yet.
    bool entryAvailable(const char* name) const;
    QString entryReason(const char* name) const;

    // The debug panel's product topics (C59-4 AC-25).
    const cu59::LiveDebugSnapshot& liveSnapshot() const;
    const std::vector<cu59::StepEntry>& programListing() const;
    cu59::ProgramSource programListingSource() const;
    /// Running: the last 32 instructions; frozen: the whole ring plus the next.
    const std::vector<cu59::InspectorEntry>& cpuHistory() const;
    const std::vector<uint32_t>& heatmapCounts() const;
    void clearHeatmap() { send(cu59::commands::kHeatmapReset); }

    DebugLevel debugLevel() const;
    /// The LOG button: off, info, debug, off (`c59.log-level`, AC-11).
    void cycleDebugLevel();
    /// The LOG tab's view of LIB-3's `log`: the last 2000 lines (AC-9).
    QString logText() const { return m_logText; }
    /// Empties the tab's view, not the log.
    void clearLog();
    void dumpVars() { dump("vars"); }
    void dumpScom() { dump("scom"); }
    void dumpProg() { dump("prog"); }
    void dumpMemory() { dump("memory"); }

    /// The TRACE button's trace (LIB-10's `trace` topic and command).
    bool traceRunning() const;
    void setTrace(bool on);

    QString asmFileName() const;
    int asmWordCount() const;
    QString asmStatus() const;
    bool asmOverlayActive() const;
    void loadAsmFile(const QString& path);
    void runAsmOverlay() { send(cu59::commands::kAsmRun); }
    void clearAsmOverlay() { send(cu59::commands::kAsmClear); }

signals:
    void modelChanged();         ///< the running model (the control state)
    void displayChanged();
    void cueCardChanged();
    void printerChanged();       ///< the PC-100C plugged in or out, TRACE
    void cardStateChanged();
    void settingsChanged();      ///< the `settings` topic (the LED style)
    void runStateChanged();      ///< the run control's state, full speed, playback, held key
    void liveSnapshotChanged();
    void cpuChanged();
    void heatmapChanged();
    void logChanged();
    void asmChanged();
    void traceChanged();
    void debugPanelRequested(int tab);  ///< the topic debug-view asked for a tab (0 CALCULATOR, 1 CPU, 2 LOG)
    void entriesChanged();              ///< the control state's product entries

private:
    calcu::sessions::ReplyMessage send(const calcu::sessions::CommandName& name,
                                       calcu::sessions::Value args = calcu::sessions::Value::object({}));
    calcu::sessions::ReplyMessage ask(const calcu::sessions::RequestName& name, calcu::sessions::Value args);
    template <class P> const P* state(const calcu::sessions::TopicName& t) const { return m_session->host().topics().stateOf<P>(t); }
    void onMessage(const calcu::sessions::Message&);
    void topicChanged(const QString&);
    void dump(const char* what) { send(cu59::commands::kDump, calcu::sessions::Value::object({{"what", what}})); }

    bool m_romsFound = false;
    // The session (with LIB-14's SettingsPart) and its host; ended first in
    // the destructor.
    std::unique_ptr<calcu::qt6::ProductHost<cu59::TIAppPart>> m_session;
    std::string m_model;               ///< the model last seen in the control state
    bool m_printerSeen = false;
    cu59::RingLog m_log;               ///< the LOG tab's view (AC-9)
    QString m_logText;
    bool m_logDirty = false;
    QTimer m_logTimer;                 ///< the view's text at 1 Hz, as today
    std::uint64_t m_viewRequests = 0;  ///< debug-view's count seen
};
