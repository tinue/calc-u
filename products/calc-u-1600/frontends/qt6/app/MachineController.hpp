#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Debug/Inspect/Inspector.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Display/LcdText.hpp"
#include "app/AudioSink.hpp"
#include "app/SharpSession.hpp"
#include "app/SharpTopics.hpp"
#include "qt6/ProductHost.hpp"
#include "qt6/SessionHost.hpp"
#include "sessions/AppSession.hpp"
#include "typing/HostKey.hpp"

class DapSocket;

// ── MachineController: the product's glue around the session host ─────────
//
// Makes Calc-U-1600's app session (makeSharpSession,
// with the library's SettingsPart over AppPaths::folders()) and the Qt6 component
// set's SessionHost over it (calcu::qt6::ProductHost), which hosts it on the GUI thread (the 60 Hz timer, the listener, the
// host services, harness attach). The shell draws from the host; this class
// keeps what only the product needs: the sound feed (a message handler of
// the host), the DAP server's socket, the typed keys of the
// window's host keyboard and the SHIFT tap, and the product's reads (the
// screen image, the INSPECTOR, the CE-158's printer). It never sends
// suspend: the Qt6 frontend keeps running in the background.
class MachineController : public QObject {
    Q_OBJECT
public:
    explicit MachineController(QObject* parent = nullptr);
    ~MachineController() override;

    calcu::qt6::SessionHost& host() { return m_session->host(); }
    cu1600::SharpAppPart& app() { return m_session->part(); }
    const cu1600::SharpAppPart& app() const { return m_session->part(); }
    calcu::sessions::AppSession& session() { return m_session->session(); }

    // The start: the host's timer, then `start`. A missing ROM file
    // gives the topic roms-missing; the shell shows it.
    void start();

    using Reply = calcu::qt6::SessionHost::Reply;
    void send(const calcu::sessions::CommandName& name, calcu::sessions::Value args = {}, Reply done = {});
    // A request, answered at once (requests do not wait).
    calcu::sessions::ReplyMessage ask(const calcu::sessions::RequestName& name, calcu::sessions::Value args = {});
    template <class P> const P* state(const calcu::sessions::TopicName& t) const {
        const calcu::sessions::PayloadPtr* p = m_session->host().topics().state(t);
        return p ? calcu::sessions::payloadAs<P>(*p) : nullptr;
    }

    bool busy() const;   // the busy topic: an operation runs
    // The sound messages go to `sink` from now on; the platform's audio
    // backend is started once now (its first start can block).
    void attachAudio(cu1600::AudioSink& sink);

    // ---- Keys and typing: the library's typing part ----
    void pressKey(const std::string& name);
    void releaseKey(const std::string& name);
    void hostKeyDown(const calcu::typing::HostKeyEvent& e);
    void hostKeyUp(const std::string& physical);
    void hostText(const std::string& utf8);   // the input method's commit
    void focusLost();                         // every held key comes up
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
    bool machinePaused() const;   // the library's `run-state` topic
    std::vector<std::uint8_t> drainCE158PrinterOutput() { return app().drainCE158PrinterOutput(); }
    cu1600::LampsPayload lamps() const;   // the tape counter, the floppy lamp
    void refreshSerialLinkDirectory() { app().refreshSerialLinkDirectory(); }
    QString serialLinkStatus() const { return QString::fromStdString(app().serialLinkStatus()); }
    QString ce158SerialLinkStatus() const { return QString::fromStdString(app().ce158SerialLinkStatus()); }

    // ---- The DAP server (app/dap/DapPart) ----
    // `--dap <port>` for this run; the refusal, also a notice.
    std::optional<std::string> setDapRunPort(long port);
    // The status line of Settings ▸ Debugger (the topic c1600.dap).
    QString dapStatus() const;
    bool dapClient() const;
    void disconnectDap();   // Settings ▸ Debugger ▸ Disconnect

    // The pacing probe: host calls so far and the longest gap.
    std::uint64_t calls() const { return m_calls; }
    double longestGapMs() const { return m_longestGapMs; }

signals:
    // A program file was opened (a drop, Open With, the start
    // argument): the window runs its program flow on it. `kind`:
    // basic-program or machine-code-program.
    void programOpened(const QString& name, const QByteArray& bytes, const QString& kind);
    void appBusyChanged(bool busy);       // the busy topic, after the host call
    void runStateChanged();               // the library's `run-state`
    void traceStateChanged();             // the library's `trace`
    void serialLinksMoved();              // c1600.serial-links

private:
    void onMessage(const calcu::sessions::Message& message);

    // The session (with the library's SettingsPart) and its host; ended first in
    // the destructor, while the sound feed is still there.
    std::unique_ptr<calcu::qt6::ProductHost<cu1600::SharpAppPart>> m_session;
    // Declared after the session so it goes first on destruction.
    std::unique_ptr<DapSocket> m_dapSocket;
    std::unique_ptr<cu1600::SoundFeed> m_sound;
    bool m_busy = false;
    std::uint64_t m_serialMoves = 0;
    std::uint64_t m_calls = 0;
    double m_longestGapMs = 0;
    calcu::pacing::WallTime m_lastCall{0};
};
