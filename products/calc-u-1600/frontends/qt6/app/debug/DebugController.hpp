#pragma once
#include <QObject>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QJsonObject>

#include "Debug/ProgramLoader.hpp"
#include "Debug/SourceMap.hpp"
#include "Display/LcdText.hpp"
#include "app/dap/DapBreakpoints.hpp"
#include "app/dap/DapStepping.hpp"
#include "app/dap/DapTargets.hpp"
#include "debugger/RunState.hpp"
#include "machine/Machine.hpp"

class DapServer;
class DapSession;
class MachineController;
class DebugController;

namespace calcu::debugger {
class RunControl;
}

// What a DAP session tells its client: a pause of the run control (with
// its reason and the DAP's breakpoint IDs) or a line of output (a log
// message, a condition that cannot be worked out).
struct DebugEvent {
    enum Kind : uint8_t { Stopped, Output };
    Kind kind = Stopped;
    enum Reason : uint8_t { Breakpoint, DataBreakpoint, Step, Pause, Entry };
    Reason reason = Pause;
    int thread = 0;
    std::vector<int> breakpointIds;
    std::string text;
};

// The DAP session's view of the run control (LIB-10 §10.8): the library's,
// through the DAP's layer in app/dap/ (its steps and the source map).
class DapRun {
public:
    explicit DapRun(DebugController& c) : m_c(c) {}
    bool locate(int thread, uint16_t pc, debug::SourceLocation* out) const;
    debug::BankMatch bankMatch() const;
    debug::CodePeek codePeek() const;
    const shared::dap::DapBreakpoints::SymbolLookup& symbols() const;
    void resume();
    // Pause (reason "entry" told for an entry stop of a reset or a load).
    void pause(DebugEvent::Reason reason = DebugEvent::Pause);
    void step(int thread, shared::dap::StepKind kind, bool line);

private:
    DebugController& m_c;
};

// The app side of the debugger (adapter L-A1 of LIB-10 §10.9 until C1600-3
// moves it to app/dap/ in M-50). Owns the DAP server (enabled in Settings)
// and, while a client is attached, the session's source map and the DAP's
// breakpoints. The machine runs under the library's run control (the
// DebuggerPart of SharpAppPart) whether or not a client is attached: pause,
// step and resume are its calls; a stop reaches the client through the
// `run-state` topic, after the host call it happened in.
class DebugController : public QObject {
    Q_OBJECT
public:
    explicit DebugController(MachineController* machines, QObject* parent = nullptr);
    ~DebugController() override;
    MachineController& machines() { return *m_machines; }

    /// Starts, stops or rebinds the server from the Settings values (or
    /// the --dap command-line port, which overrides them for the run).
    void refreshServer();
    static void setCommandLinePort(int port);
    bool hasClient() const;
    /// Drops the attached client (Settings > Debugger > Disconnect); the
    /// machine runs on as without a debugger.
    void disconnectClient();
    /// "Listening on 127.0.0.1:32168", "Client connected", "Port in use: …",
    /// "Off".
    QString serverStatus() const;

    /// A debug session is attached.
    bool attached() const;
    /// After every host call (the frame): stops and output lines go to the
    /// client; a new machine has its listings checked again.
    void afterCall();

    // ── For DapSession ────────────────────────────────────────────────────
    /// Starts a session on the live machine; false if there is none.
    bool beginSession();
    void endSession();
    shared::dap::DapTargets* target();
    DapRun* runControl();
    calcu::debugger::RunControl* libraryRunControl() const;
    debug::SourceMap& sourceMap() { return m_map; }
    shared::dap::DapBreakpoints& breakpoints() { return m_breakpoints; }
    /// Hands the library's table the DAP's breakpoints (after each change).
    void applyBreakpoints();
    const shared::dap::DapBreakpoints::SymbolLookup& symbols() const { return m_symbols; }
    /// Verifies every listing binding against memory and re-resolves the
    /// breakpoints; returns the breakpoint statuses that changed.
    std::vector<shared::dap::BreakpointStatus> rebindListings();
    /// The next pause is told as "entry".
    void expectEntry() { m_entryPending = true; }
    /// The session resumed, stepped or paused the machine: its next pause
    /// is a new one (a `stopped`), also when it lands within the next host
    /// call or the machine was paused already, as the client expects.
    void noteRunning() {
        m_lastPaused = false;
        m_entryPending = false;
    }

    /// Loads a preset the way File > Load Preset does; `armOnly` stops
    /// before the boot (PresetFile::armOnly).
    bool loadPreset(const QString& path, QString* error, bool armOnly = false);
    /// A clean machine for a program load: `preset` if given, else the
    /// model's default preset (Settings), else an All Reset and boot.
    /// `how` says which it was.
    bool cleanStart(const QString& preset, QString* how, QString* error);

    /// What to do once a program is loaded: nothing, type its CALL, or type
    /// its CALL and stop at its entry.
    enum class After { None, Call, StopOnEntry };
    /// Build & Load: loads the program, binds its listing, re-resolves the
    /// breakpoints, then does `after`. A non-empty `command` is typed in
    /// place of the program's CALL (e.g. a CALL with arguments).
    debug::LoadResult loadProgram(const debug::LoadRequest& request, After after, const std::string& command = {});
    /// Types a BASIC line and ENTER into the machine as it runs.
    void typeCommand(const std::string& line);
    /// The LCD as text; false with no machine.
    bool lcdText(LcdText* out) const;
    /// Machine reset (all = RAM cleared first) without the boot run: with
    /// `stop` the machine halts before its first instruction (reason
    /// "entry"), else it runs on.
    bool resetMachine(bool allReset, bool stop, QString* error);

signals:
    void serverStatusChanged();

private:
    /// An operation of the session runs: the machine is being rebuilt or
    /// driven by it, so DAP messages wait until it is done.
    bool appBusy() const;
    /// MachineController::appBusyChanged: drains what waited once it is done.
    void setAppBusy(bool busy);
    void onClientConnected();
    void onClientDisconnected();
    /// Ends the session of a client that disconnected, once no request or
    /// synchronous operation is running (see onClientDisconnected()).
    void finishTeardown();
    void dispatch(const QJsonObject& message);
    void drainQueue();

    MachineController* m_machines; // not owned
    DapServer* m_server = nullptr;
    std::unique_ptr<DapSession> m_session;
    QString m_serverError;

    std::optional<shared::dap::DapTargets> m_targets;
    DapRun m_run{*this};
    debug::SourceMap m_map;
    shared::dap::DapBreakpoints m_breakpoints;
    shared::dap::DapBreakpoints::SymbolLookup m_symbols;
    bool m_sessionActive = false;
    bool m_lastPaused = false;
    bool m_entryPending = false;
    calcu::machine::Machine* m_lastMachine = nullptr;
    bool m_replacedPending = false;   // a new machine: its listings are checked once no operation runs
    bool m_busy = false;     // handling a message (which may itself load)
    std::vector<QJsonObject> m_queued;
    bool m_teardownPending = false; // client gone, session not yet ended
    std::vector<std::unique_ptr<DapSession>> m_retiredSessions; // detached, maybe still on the stack
    /// A command of the session, waited for in a nested event loop while the
    /// host's timer goes on calling the session (the DAP's requests answer
    /// when the load is done; an adapter of M-45's that M-50 removes with the
    /// DAP move). A failure goes to `error`.
    bool runAndWait(const calcu::sessions::CommandName& name, calcu::sessions::Value args, QString* error);
};
