#pragma once
#include <QObject>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Connector/AlpsPlotterMechanism.hpp"
#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "Tape/TapeDeck.hpp"
#include "TraceTypes.hpp"
#include "app/AudioSink.hpp"
#include "app/MachineModel.hpp"
#include "app/SharpSession.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/Listener.hpp"
#include "typing/HostKey.hpp"

class MemoryModuleManager;
class DebugController;
class FloppyDiskManager;

using shared::Model;
using shared::PC1600RomVersion;
using shared::CE1600PRomVersion;

inline QString modelSettingsKey(Model model) { return QString::fromLatin1(shared::modelKey(model)); }

using shared::DisplayFrame;

// Runs `op(std::string* error)` and copies a failure's message into the
// optional `error` -- the Shared calls' error form, for the Qt widgets.
template <class Op>
bool withQtError(QString* error, Op&& op) {
    std::string err;
    const bool ok = op(&err);
    if (!ok && error) *error = QString::fromStdString(err);
    return ok;
}

// ── MachineController: the Qt6 frontend's host of the app session ────────
//
// Hosts the library's app session on the GUI thread (LIB-3 AC-34; FE-3),
// made by shared::makeSharpSession with Calc-U-1600's part
// (app/SharpAppPart.hpp). It listens through a calcu::sessions::Listener,
// which keeps the last message of each topic; it turns them into Qt signals; it sends commands (an operation's reply comes
// later, LIB-3 AC-18); it feeds the sound to the audio sink (AC-37); and it
// calls the session from the 60 Hz frame timer (EmulationPacer). It never
// sends suspend: the Qt6 frontend keeps running in the background (AC-36).
//
// Until their steps, the widgets still reach the product's part directly
// for what has no topic yet (the bays, the debug panel, the plotters, the
// keys through SharpTypingHook; LIB-3 design §5).
class MachineController : public QObject, private shared::SharpDebugger {
    Q_OBJECT
public:
    explicit MachineController(QObject* parent = nullptr);
    ~MachineController() override;

    calcu::sessions::Listener& listener() { return m_listener; }
    shared::SharpAppPart& app() { return *m_app.part; }
    shared::SharpAppPart& session() { return *m_app.part; }
    const shared::SharpAppPart& session() const { return *m_app.part; }

    // ---- The session ----
    // One host call (the frame timer).
    void call();
    // Host calls so far and the longest gap between two (the pacing probe).
    std::uint64_t calls() const { return m_calls; }
    double longestGapMs() const { return m_longestGapMs; }
    double callMs() const { return m_callMs; }   // host time spent inside the calls
    using Reply = std::function<void(const calcu::sessions::ReplyMessage&)>;
    // Sends a command; `done` gets its reply -- at once for a command that
    // starts no operation, at the operation's end otherwise (LIB-3 D-2).
    void send(const calcu::sessions::CommandName& name, calcu::sessions::Value args = {}, Reply done = {});
    // An operation runs (the busy topic).
    bool busy() const { return m_busy; }
    // The busy topic says the running operation is at full speed now.
    bool operationAtFullSpeed() const { return m_busy && m_busyFull; }
    // Frozen (the --shots runner): no frame timer; emulated time moves only
    // through runFor() / runUntilTyped() and the operations run to their end
    // on a clock of their own, so a capture is the same run to run.
    void setFrozen(bool frozen);
    bool frozen() const { return m_frozen; }
    // Frozen only: `seconds` of host calls at 60 Hz on the frozen clock.
    void runFor(double seconds);
    // Frozen only: until the typing hook has typed everything, at most
    // `capSeconds`; false if it still types.
    bool runUntilTyped(double capSeconds);
    // Frozen only: host calls until no operation runs.
    void finishOperation();
    // The sound messages go to `sink` from now on; the platform's audio
    // backend is started once now (its first start can block).
    void attachAudio(shared::AudioSink& sink);

    // ---- Forwarded (SharpAppPart, its typing hook) ----
    Model currentModel() const { return session().currentModel(); }
    PC1500RomRevision pc1500RomRevision() const { return session().pc1500RomRevision(); }
    PC1600RomVersion pc1600RomVersion() const { return session().pc1600RomVersion(); }
    CE1600PRomVersion ce1600pRomVersion() const { return session().ce1600pRomVersion(); }

    // ---- Keys and typing: the library's typing part (LIB-5) ----
    // A faceplate key by key ID (`on` too: the ON/BREAK line), one pointer.
    void pressKey(const std::string& name);
    void releaseKey(const std::string& name);
    void hostKeyDown(const calcu::typing::HostKeyEvent& e);
    void hostKeyUp(const std::string& physical);
    void hostText(const std::string& utf8);   // the input method's commit (EC-4)
    void focusLost();                         // AC-10
    void pasteText(const std::string& text);  // Edit > Paste Text (AC-12, AC-34)
    // The debugger's auto-start: `line`, then ENTER, through the engine (not
    // a user key: not dropped while busy; a character without a key is left out).
    void typeCommand(const std::string& line);
    // What is still waiting goes (a reset from the debugger).
    void cancelPaste();

    // The display as the `display` topic last sent it.
    GrayImage currentScreenImage() const;
    DisplayFrame currentDisplay() const { return m_frame; }
    bool lcdText(LcdText* out) const;

    calcu::machine::Machine* machine() const { return session().machine(); }
    bool hasLiveMachine() const { return session().hasLiveMachine(); }

    std::vector<std::string> inspectorView(inspect::View view) const { return session().inspectorView(view); }
    std::vector<inspect::MenuEntry> inspectorMenu(bool dumps) const { return session().inspectorMenu(dumps); }
    void endTrace() { app().endTrace(); }
    std::uint64_t traceBytes() const { return session().traceBytes(); }
    bool traceActive() const { return session().traceActive(); }

    std::vector<std::uint8_t> drainCE158PrinterOutput() { return app().drainCE158PrinterOutput(); }
    bool hostDriveAttached() const { return session().hostDriveAttached(); }
    std::vector<AlpsPlotterMechanism::FlatPoint> plotPoints(shared::PaperKind kind) const {
        return session().plotPoints(kind);
    }
    std::uint64_t plotRevision(shared::PaperKind kind) const { return session().plotRevision(kind); }
    void clearPaper(shared::PaperKind kind) { app().clearPaper(kind); }
    shared::ControlState controlState() const;
    TapeDeck::Status tapeStatus() const;

    // ---- QString translations ----
    bool beginTrace(const QString& path) { return app().beginTrace(path.toStdString()); }
    bool tapeEject(QString* error = nullptr);
    void refreshSerialLinkDirectory() { app().refreshSerialLinkDirectory(); }
    QString serialLinkStatus() const { return QString::fromStdString(session().serialLinkStatus()); }
    QString ce158SerialLinkStatus() const { return QString::fromStdString(session().ce158SerialLinkStatus()); }

    // ---- Qt side ----
    DebugController* debugController() const { return m_debug.get(); }
    void refreshDebugServer();

signals:
    void modelChanged(Model model);
    // The serial port files moved (c1600.serial-links).
    void serialLinksMoved();
    // A bay changed a slot outside the picker (c1600.machine).
    void bayModuleChanged(int slot);
    // A recording was saved: the tape list changed (c1600.tapes).
    void tapeListChanged();
    // The control state changed while an operation runs (a preset armed
    // its machine): the window shows it now (LIB-3 AC-15).
    void presetWasArmed();
    // An operation started / ended (the busy topic).
    void appBusyChanged(bool busy);
    // The busy topic's speed changed while an operation runs.
    void operationSpeedChanged();
    // A notice (LIB-3 AC-10): shown as it is.
    void noticeShown(const QString& sentence);
    // The display topic changed.
    void displayChanged();

private:
    // shared::SharpDebugger (SharpDebuggerGate, until M-45)
    void machineAboutToChange() override;
    void machineReplaced() override;
    bool runDebuggerSlice(std::uint64_t ticks) override;

    void onMessage(const calcu::sessions::Message& message);
    void onState(const calcu::sessions::TopicName& topic);
    calcu::pacing::WallTime now();

    calcu::sessions::ProductSession<shared::SharpAppPart> m_app;
    calcu::sessions::Listener m_listener;   // stops listening before the session goes
    // Declared after the session so it goes first on destruction: its debug
    // target still refers to the live machine.
    std::unique_ptr<DebugController> m_debug;
    std::unique_ptr<shared::SoundFeed> m_sound;
    std::string m_lastNotice;   // a failed operation's notice; its reply is not shown again

    DisplayFrame m_frame;
    bool m_busy = false;
    bool m_busyFull = false;
    shared::MachinePayload m_machine;
    std::uint64_t m_tapeListChanges = 0;
    std::uint64_t m_serialMoves = 0;
    bool m_frozen = false;
    calcu::pacing::WallTime m_frozenNow{0};
    std::uint64_t m_calls = 0;
    double m_longestGapMs = 0;
    double m_callMs = 0;
    calcu::pacing::WallTime m_lastCall{0};
};
