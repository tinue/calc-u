#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "app/AudioSink.hpp"
#include "app/SharpSession.hpp"
#include "app/SharpTopics.hpp"
#include "qt6/SessionHost.hpp"
#include "sessions/AppSession.hpp"
#include "typing/HostKey.hpp"

namespace calcu::media {
struct MediaState;
} // namespace calcu::media
namespace calcu::shell {
class SettingsPart;
} // namespace calcu::shell

class DebugController;

// ── MachineController: the product's glue around the session host ─────────
//
// LIB-14 Part D (D.3): makes Calc-U-1600's app session (makeSharpSession,
// with LIB-14's SettingsPart) and the Qt6 component set's SessionHost over
// it, which hosts it on the GUI thread (the 60 Hz timer, the listener, the
// host services, harness attach). The shell draws from the host; this class
// keeps what only the product needs: the sound feed (a message handler of
// the host), the debugger's controller (until M-50), the typed keys of the
// window's host keyboard and the SHIFT tap, and the product's reads (the
// screen image, the INSPECTOR, the CE-158's printer). It never sends
// suspend: the Qt6 frontend keeps running in the background (LIB-3 AC-36).
class MachineController : public QObject {
    Q_OBJECT
public:
    explicit MachineController(QObject* parent = nullptr);
    ~MachineController() override;

    calcu::qt6::SessionHost& host() { return *m_host; }
    shared::SharpAppPart& app() { return *m_app.part; }
    const shared::SharpAppPart& app() const { return *m_app.part; }
    calcu::sessions::AppSession& session() { return *m_app.session; }

    // The start (LIB-18): the host's timer, then `start`. A missing ROM file
    // gives the topic roms-missing; the shell shows it (LIB-14 AC-19).
    void start();

    using Reply = calcu::qt6::SessionHost::Reply;
    void send(const calcu::sessions::CommandName& name, calcu::sessions::Value args = {}, Reply done = {});
    // A command waited for in a nested event loop while the host's timer
    // goes on calling the session (a dialog that answers with the outcome:
    // Name & Save, Save Tape).
    calcu::sessions::ReplyMessage sendAndWait(const calcu::sessions::CommandName& name, calcu::sessions::Value args = {});
    // A request, answered at once (requests do not wait, LIB-3).
    calcu::sessions::ReplyMessage ask(const calcu::sessions::RequestName& name, calcu::sessions::Value args = {});
    template <class P> const P* state(const calcu::sessions::TopicName& t) const {
        const calcu::sessions::PayloadPtr* p = m_host->topics().state(t);
        return p ? calcu::sessions::payloadAs<P>(*p) : nullptr;
    }

    bool busy() const;   // the busy topic: an operation runs
    // The sound messages go to `sink` from now on; the platform's audio
    // backend is started once now (its first start can block).
    void attachAudio(shared::AudioSink& sink);

    // ---- Keys and typing: the library's typing part (LIB-5) ----
    void pressKey(const std::string& name);
    void releaseKey(const std::string& name);
    void hostKeyDown(const calcu::typing::HostKeyEvent& e);
    void hostKeyUp(const std::string& physical);
    void hostText(const std::string& utf8);   // the input method's commit (EC-4)
    void focusLost();                         // AC-10
    void pasteText(const std::string& text);
    // The debugger's auto-start: `line`, then ENTER, through the engine (not
    // a user key: not dropped while busy).
    void typeCommand(const std::string& line);
    void cancelPaste();

    // ---- Reads ----
    const calcu::machine::DisplayOutput* display() const;
    GrayImage currentScreenImage() const;
    bool lcdText(LcdText* out) const;
    calcu::machine::Machine* machine() const { return app().machine(); }
    bool hasLiveMachine() const { return app().hasLiveMachine(); }
    std::vector<std::string> inspectorView(inspect::View view) const { return app().inspectorView(view); }
    std::vector<inspect::MenuEntry> inspectorMenu(bool dumps) const { return app().inspectorMenu(dumps); }
    bool traceActive() const;     // LIB-10's `trace` topic
    bool machinePaused() const;   // LIB-10's `run-state` topic
    std::vector<std::uint8_t> drainCE158PrinterOutput() { return app().drainCE158PrinterOutput(); }
    shared::LampsPayload lamps() const;   // the tape counter, the floppy lamp
    const calcu::media::MediaState* media() const;
    void refreshSerialLinkDirectory() { app().refreshSerialLinkDirectory(); }
    QString serialLinkStatus() const { return QString::fromStdString(app().serialLinkStatus()); }
    QString ce158SerialLinkStatus() const { return QString::fromStdString(app().ce158SerialLinkStatus()); }

    // ---- The debugger (until M-50) ----
    DebugController* debugController() const { return m_debug.get(); }
    void refreshDebugServer();

    // The pacing probe (LIB-17 §5): host calls so far and the longest gap.
    std::uint64_t calls() const { return m_calls; }
    double longestGapMs() const { return m_longestGapMs; }

signals:
    // A program file was opened (a drop, Open With, the start argument;
    // LIB-13 D4.2): the window runs its program flow on it. `kind`:
    // basic-program or machine-code-program.
    void programOpened(const QString& name, const QByteArray& bytes, const QString& kind);
    void appBusyChanged(bool busy);       // the busy topic, after the host call
    void runStateChanged();               // LIB-10 `run-state`
    void traceStateChanged();             // LIB-10 `trace`
    void serialLinksMoved();              // c1600.serial-links

private:
    void onMessage(const calcu::sessions::Message& message);

    std::unique_ptr<calcu::shell::SettingsPart> m_settingsPart;
    calcu::sessions::ProductSession<shared::SharpAppPart> m_app;
    std::unique_ptr<calcu::qt6::SessionHost> m_host;   // stops listening before the session goes
    // Declared after the session so it goes first on destruction: its debug
    // target still refers to the live machine.
    std::unique_ptr<DebugController> m_debug;
    std::unique_ptr<shared::SoundFeed> m_sound;
    bool m_busy = false;
    std::uint64_t m_serialMoves = 0;
    std::uint64_t m_calls = 0;
    double m_longestGapMs = 0;
    calcu::pacing::WallTime m_lastCall{0};
};
