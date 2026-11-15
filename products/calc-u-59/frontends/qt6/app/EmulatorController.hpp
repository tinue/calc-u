#pragma once
#include "TIAppPart.hpp"
#include "typing/HostKey.hpp"
#include "sessions/Listener.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <memory>

// The Qt face of the library's app session (LIB-3 AC-28; the Swift app's is
// App/EmulatorViewModel.swift over cu59::SessionThread): the session lives on
// the GUI thread, called at 60 Hz by a timer; this object is its frontend,
// through a calcu::sessions::Listener. Every action is a command
// (cu59::commands), everything shown comes from the listener's topic mirror;
// the mirror's changes become the signals below. It decides
// nothing and keeps no state of the machine (CLAUDE.md § "Frontend Parity").
// Qt6 sends neither suspend nor continue (LIB-3 AC-31; ODD59-25).
class EmulatorController : public QObject {
    Q_OBJECT
public:
    using CardState = cu59::CardState;
    using DebugLevel = cu59::DebugLevel;

    explicit EmulatorController(QObject* parent = nullptr);
    ~EmulatorController() override;
    /// false: the ROM lookup failed (the dialog was shown); nothing else may be called.
    bool started() const { return m_app.session != nullptr; }

    // ── Machine ──────────────────────────────────────────────────────────────
    void start(cu59::Model model);
    cu59::Model model() const;
    void resetMachine() { send(cu59::commands::kReset); }
    void cleanResetMachine() { send(cu59::commands::kCleanReset); }
    void persistConstantMemory() { send(cu59::commands::kPersistConstantMemory); }
    QString moduleID() const;
    /// Swap to a different module and power-cycle (no-op if unchanged).
    void changeModule(const QString& moduleID);
    QVector<cu59::ModuleMetadata> allModules() const;

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

    // ── Printer ──────────────────────────────────────────────────────────────
    const QStringList& printerLines() const { return m_printerLines; }
    const std::vector<PrinterCodeLine>& printerCodeLines() const;
    double printerStripHeightUnits() const;
    bool printerConnected() const;
    bool printerTrace() const;
    void togglePrinterConnection() { send(cu59::commands::kTogglePrinterConnection); }
    void togglePrinterTrace() { send(cu59::commands::kTogglePrinterTrace); }
    void pressPrinterPrint(bool pressed);
    void pressPrinterAdv(bool pressed);
    void cutPaper() { send(cu59::commands::kCutPaper); }

    // ── Card reader ──────────────────────────────────────────────────────────
    CardState cardState() const;
    QString cardFileName() const;
    void fitCard(const QString& path);
    void fitBlankCard(const QString& savePath);
    bool tapCard(double nx, double ny);
    std::optional<int> litCardCorner() const;
    void ejectCard() { send(cu59::commands::kEjectCard); }

    // ── State files ──────────────────────────────────────────────────────────
    void loadStateFile(const QString& path);

    // ── Dropped files ────────────────────────────────────────────────────────
    cu59::DropAction dropAction(const QString& path) const;
    void openDroppedFile(const QString& path);

    // ── Debugger ─────────────────────────────────────────────────────────────
    void setLiveDebugEnabled(bool on);
    void setCpuDebugEnabled(bool on);
    bool isFrozen() const;
    /// The panel that owns the freeze (none while running).
    std::optional<cu59::FreezeOwner> freezeOwner() const;
    bool pendingFreezeOnPCChange() const;
    bool pendingCpuScanLoopFreeze() const;
    void freeze(cu59::FreezeOwner owner);
    void unfreeze() { send(cu59::commands::kResume); }
    void freezeOnNextPCChange() { send(cu59::commands::kArmPcChange); }
    void freezeOnScanLoopExit() { send(cu59::commands::kArmScanLoopExit); }
    void disarmPcChangeFreeze() { send(cu59::commands::kDisarmPcChange); }
    void disarmScanLoopFreeze() { send(cu59::commands::kDisarmScanLoopExit); }
    void stepKeycode() { send(cu59::commands::kStepKeycode); }
    void stepFrozen() { send(cu59::commands::kStepInstruction); }

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

    bool manualTrace() const;
    void setManualTrace(bool on);

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
    void runStateChanged();      ///< freeze, armed freezes, full speed, playback, held key
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
    QStringList m_printerLines;  ///< the paper's text lines, as Qt types
    std::optional<cu59::Model> m_savedModel;
};
