#include "DebugController.hpp"

#include <QTimer>

#include <algorithm>

#include "AppSettings.hpp"
#include "DapServer.hpp"
#include "DapSession.hpp"
#include "Machine/LegacyCalls.hpp"
#include "debugger/RunControl.hpp"
#include "MachineController.hpp"
#include "Operations.hpp"

DebugController::DebugController(MachineController* machines, QObject* parent)
    : QObject(parent), m_machines(machines) {
    m_server = new DapServer(this);
    connect(m_server, &DapServer::clientConnected, this, &DebugController::onClientConnected);
    connect(m_server, &DapServer::clientDisconnected, this, &DebugController::onClientDisconnected);
    connect(m_server, &DapServer::messageReceived, this, &DebugController::dispatch);
    connect(m_machines, &MachineController::appBusyChanged, this, &DebugController::setAppBusy);
}

void DebugController::dispatch(const QJsonObject& message) {
    // A synchronous load pumps the event loop: messages arriving meanwhile
    // (or while a request is being handled) wait, in order.
    m_queued.push_back(message);
    drainQueue();
}

void DebugController::drainQueue() {
    if (m_busy || appBusy()) return;
    finishTeardown(); // a client that left meanwhile, before its successor's messages
    m_busy = true;
    while (!m_queued.empty() && !appBusy()) {
        const QJsonObject next = m_queued.front();
        m_queued.erase(m_queued.begin());
        if (m_session) m_session->handle(next);
    }
    m_busy = false;
    finishTeardown();
}

bool DebugController::appBusy() const { return m_machines->busy(); }

void DebugController::setAppBusy(bool busy) {
    if (!busy && (!m_queued.empty() || m_teardownPending))
        QTimer::singleShot(0, this, &DebugController::drainQueue);
}

DebugController::~DebugController() {
    m_session.reset();
    m_sessionActive = false;   // the session goes with the window: nothing to hand back
}

// ── Server ────────────────────────────────────────────────────────────────

namespace {
int s_commandLinePort = 0; // --dap <port>; 0 = use Settings

bool serverWanted() { return s_commandLinePort > 0 || AppSettings::dapEnabled(); }
int serverPort() { return s_commandLinePort > 0 ? s_commandLinePort : AppSettings::dapPort(); }
} // namespace

void DebugController::setCommandLinePort(int port) { s_commandLinePort = port >= 1024 && port <= 65535 ? port : 0; }

void DebugController::refreshServer() {
    m_serverError.clear();
    if (!serverWanted()) {
        m_server->close();
    } else if (!m_server->isListening() || m_server->port() != serverPort()) {
        QString error;
        if (!m_server->listen(quint16(serverPort()), &error)) m_serverError = error;
    }
    emit serverStatusChanged();
}

QString DebugController::serverStatus() const {
    if (!serverWanted()) return tr("Off");
    if (!m_server->isListening())
        return tr("Port %1 not available: %2").arg(serverPort()).arg(m_serverError);
    if (m_server->hasClient()) return tr("Client connected on 127.0.0.1:%1").arg(m_server->port());
    return tr("Listening on 127.0.0.1:%1").arg(m_server->port());
}

bool DebugController::hasClient() const { return m_server->hasClient(); }

void DebugController::disconnectClient() {
    if (m_session) {
        m_session->output(tr("Disconnected from the app (Settings > Debugger)."), QStringLiteral("important"));
        m_session->terminated();
    }
    m_server->disconnectClient(); // -> onClientDisconnected(): session ends, machine runs on
}

void DebugController::onClientConnected() {
    m_session = std::make_unique<DapSession>(m_server, this);
    emit serverStatusChanged();
}

void DebugController::onClientDisconnected() {
    // The socket may close while a request is being handled -- a Build &
    // Load pumps the event loop for seconds. That request's DapSession,
    // target and run control are still on the stack, so they go only once
    // it has returned (finishTeardown()); until then the session is just
    // detached from the controller.
    m_queued.clear(); // the old client's; a new one's must not see them
    if (m_session) m_retiredSessions.push_back(std::move(m_session));
    m_teardownPending = true;
    finishTeardown();
    emit serverStatusChanged();
}

void DebugController::finishTeardown() {
    if (!m_teardownPending || m_busy || appBusy()) return;
    m_teardownPending = false;
    endSession();
    m_retiredSessions.clear();
}

// ── Session ───────────────────────────────────────────────────────────────

calcu::debugger::RunControl* DebugController::libraryRunControl() const {
    return &m_machines->app().debuggerPart().runControl();
}

shared::dap::DapTargets* DebugController::target() {
    if (!m_sessionActive) return nullptr;
    calcu::machine::Machine* m = m_machines->machine();
    m_targets.emplace(m ? m->debugging() : nullptr);
    return m_targets->valid() ? &*m_targets : nullptr;
}

DapRun* DebugController::runControl() { return m_sessionActive ? &m_run : nullptr; }

void DebugController::applyBreakpoints() {
    shared::dap::DapTargets* t = target();
    m_breakpoints.apply(libraryRunControl()->table(), t ? *t : shared::dap::DapTargets(nullptr), m_symbols);
}

bool DebugController::beginSession() {
    m_map.clear();
    m_breakpoints.clear();
    m_symbols = [this](const std::string& name, int64_t* value) {
        uint16_t v = 0;
        if (!m_map.symbolValue(name, &v)) return false;
        *value = v;
        return true;
    };
    m_replacedPending = false;
    m_entryPending = false;
    m_lastMachine = m_machines->machine();
    m_sessionActive = m_lastMachine != nullptr;
    if (m_sessionActive) applyBreakpoints();
    const auto* s = m_machines->listener().state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
    m_lastPaused = s && s->paused;
    return m_sessionActive;
}

void DebugController::endSession() {
    // The DAP's breakpoints and watches leave the library's table; the
    // machine runs on as without a debugger.
    if (m_sessionActive) {
        m_breakpoints.clear();
        applyBreakpoints();
        if (libraryRunControl()->paused()) libraryRunControl()->resume();
    }
    m_sessionActive = false;
    m_targets.reset();
    m_map.clear();
}

bool DebugController::attached() const { return m_sessionActive; }

// After every host call: what the run control did reaches the client
// (C1600-3 §3.6: a change to paused gives one `stopped`), the output of
// log messages too; a machine built since has its listings checked once no
// operation runs any more.
void DebugController::afterCall() {
    if (!m_sessionActive) return;
    if (m_machines->machine() != m_lastMachine) {
        m_lastMachine = m_machines->machine();
        m_replacedPending = true;
    }
    if (m_replacedPending && !appBusy() && m_lastMachine) {
        m_replacedPending = false;
        if (m_session) m_session->onMachineReplaced();
    }
    std::vector<DebugEvent> events;
    for (std::string& line : m_breakpoints.takeOutput()) {
        DebugEvent e;
        e.kind = DebugEvent::Output;
        e.text = std::move(line);
        events.push_back(std::move(e));
    }
    const auto* s = m_machines->listener().state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
    const bool paused = s && s->paused;
    if (paused && !m_lastPaused && s->reason) {
        using R = calcu::debugger::RunState::Reason;
        DebugEvent e;
        e.kind = DebugEvent::Stopped;
        e.breakpointIds = m_breakpoints.dapIds(s->hits);
        switch (*s->reason) {
        case R::Breakpoint: e.reason = m_breakpoints.entryHit(s->hits) ? DebugEvent::Entry : DebugEvent::Breakpoint; break;
        case R::Watch: e.reason = DebugEvent::DataBreakpoint; break;
        case R::Step: e.reason = DebugEvent::Step; break;
        case R::Entry: e.reason = DebugEvent::Entry; break;
        case R::Pause:
        case R::PauseOnStart: e.reason = m_entryPending ? DebugEvent::Entry : DebugEvent::Pause; break;
        }
        m_entryPending = false;
        e.thread = 1;
        if (shared::dap::DapTargets* t = target(); t && s->target)
            for (const auto& th : t->threads())
                if (t->targetId(th.id) == *s->target) e.thread = th.id;
        events.push_back(std::move(e));
    }
    m_lastPaused = paused;
    if (m_session && !events.empty()) m_session->onEvents(events);
}

std::vector<shared::dap::BreakpointStatus> DebugController::rebindListings() {
    shared::dap::DapTargets* t = target();
    if (!t) return {};
    for (const auto& b : m_map.bindings()) m_map.verify(b.id, t->bankMatch(), t->codePeek());
    auto changed = m_breakpoints.reresolve(m_map);
    applyBreakpoints();
    return changed;
}

void DebugController::setOperations(Operations* ops) {
    m_sync = ops;
}

bool DebugController::loadPreset(const QString& path, QString* error, bool armOnly) {
    if (!m_sync) {
        *error = tr("Presets can't be loaded from the debugger here");
        return false;
    }
    const bool ok = m_sync->runAndWait(tr("Load Preset"), shared::commands::kLoadPreset,
                                       calcu::sessions::Value::object({{"path", path.toStdString()}, {"armOnly", armOnly}}), error);
    // The preset rebuilt the machine; the caller re-binds the listings.
    m_lastMachine = m_machines->machine();
    m_replacedPending = false;
    return ok;
}

bool DebugController::cleanStart(const QString& preset, QString* how, QString* error) {
    if (!m_sync) {
        *error = tr("The machine can't be set up from the debugger here");
        return false;
    }
    bool ok = false;
    if (!preset.isEmpty()) {
        *how = tr("preset %1").arg(preset);
        ok = m_sync->runAndWait(tr("Load Preset"), shared::commands::kLoadPreset,
                                calcu::sessions::Value::object({{"path", preset.toStdString()}}), error);
    } else if (const QString path = AppSettings::defaultPresetPath(m_machines->currentModel());
               !path.isEmpty()) {
        // As the app applies it: refused if it's for another model.
        *how = tr("default preset %1").arg(path);
        ok = m_sync->runAndWait(
            tr("Default Preset"), shared::commands::kLoadDefaultPreset,
            calcu::sessions::Value::object({{"path", path.toStdString()},
                                            {"model", shared::settingsModelName(m_machines->currentModel())}}),
            error);
    } else {
        // No preset: the machine as Reset All leaves it, booted to the prompt.
        *how = tr("All Reset (no default preset for this model)");
        ok = m_sync->runAndWait(tr("Reset All"), shared::commands::kReset, calcu::sessions::Value::object({{"all", true}}),
                                error);
    }
    // The caller re-binds the listings.
    m_lastMachine = m_machines->machine();
    m_replacedPending = false;
    return ok;
}

debug::LoadResult DebugController::loadProgram(const debug::LoadRequest& request, After after,
                                               const std::string& command) {
    debug::LoadResult r;
    if (!target() || !m_sync) {
        r.error = "no machine";
        return r;
    }
    // An instant load into the running machine, between two host calls.
    calcu::machine::Machine* machine = m_machines->machine();
    if (!machine || appBusy()) {
        r.error = machine ? "an operation is running" : "no machine";
        return r;
    }
    r = sharp::legacy::loadProgram(*machine, request, m_map);
    if (!r.ok) return r;
    if (after == After::None) return r;
    if (!command.empty()) r.callCommand = command;
    if (r.callCommand.empty()) {
        r.warnings.push_back("no BASIC command starts this code; load it, then call it from your own code");
        return r;
    }
    if (after == After::StopOnEntry) m_breakpoints.setEntry(request.thread, r.entry);
    applyBreakpoints();
    m_machines->typeCommand(r.callCommand); // typed and entered as the machine runs
    m_run.resume();
    return r;
}

void DebugController::typeCommand(const std::string& line) { m_machines->typeCommand(line); }

bool DebugController::lcdText(LcdText* out) const { return m_machines->lcdText(out); }

bool DebugController::resetMachine(bool allReset, bool stop, QString* error) {
    calcu::machine::Machine* machine = m_machines->machine();
    if (!machine || !m_sync) {
        *error = tr("No machine is running");
        return false;
    }
    // Without the boot run (ROM research starts at the vector), but like
    // any reset: typing dropped, clock re-seeded after (C1600-3 D-2).
    if (appBusy()) {
        *error = tr("Another operation is running.");
        return false;
    }
    m_machines->cancelPaste();
    if (allReset) machine->coldBoot();
    else machine->reset();
    m_machines->app().seedClockFromHost();
    // L-A2: the restart is told, so the run control runs again.
    libraryRunControl()->restarted(allReset ? calcu::debugger::RestartKind::ColdBoot : calcu::debugger::RestartKind::Reset);
    if (stop) m_run.pause(DebugEvent::Entry);
    else m_run.resume();
    return true;
}

// ── DapRun ────────────────────────────────────────────────────────────────

bool DapRun::locate(int thread, uint16_t pc, debug::SourceLocation* out) const {
    shared::dap::DapTargets* t = m_c.target();
    return t && m_c.sourceMap().lookup(thread, pc, t->bankMatch(), out, t->codePeek());
}

debug::BankMatch DapRun::bankMatch() const {
    shared::dap::DapTargets* t = m_c.target();
    return t ? t->bankMatch() : debug::BankMatch([](int, const debug::BankKey&, uint16_t) { return false; });
}

debug::CodePeek DapRun::codePeek() const {
    shared::dap::DapTargets* t = m_c.target();
    return t ? t->codePeek() : debug::CodePeek([](int, uint16_t, uint8_t*) { return false; });
}

const shared::dap::DapBreakpoints::SymbolLookup& DapRun::symbols() const { return m_c.symbols(); }

void DapRun::resume() {
    m_c.noteRunning();
    m_c.libraryRunControl()->resume();
}

void DapRun::pause(DebugEvent::Reason reason) {
    m_c.noteRunning();
    if (reason == DebugEvent::Entry) m_c.expectEntry();
    m_c.libraryRunControl()->pause(calcu::debugger::kCpu);
}

void DapRun::step(int thread, shared::dap::StepKind kind, bool line) {
    shared::dap::DapTargets* t = m_c.target();
    if (!t) return;
    if (shared::dap::step(*m_c.libraryRunControl(), *t, m_c.sourceMap(), thread, kind, line).ok) m_c.noteRunning();
}
