#include "EmulatorController.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "KeyIds.hpp"
#include "ModuleCatalog.hpp"
#include "ProductRoms.hpp"
#include "QtResourceBundle.hpp"
#include "sessions/LibraryTopics.hpp"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

namespace {

namespace cs = calcu::sessions;
using cs::Value;

QString qs(const std::string& s) { return QString::fromStdString(s); }

std::optional<std::string> readResource(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    return f.readAll().toStdString();
}

// The file's bytes as a command argument; absent when it cannot be read.
Value withBytes(Value args, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return args;
    const QByteArray b = f.readAll();
    std::vector<Value::Member> members = *args.members();
    members.push_back({"bytes", Value(Value::Bytes(b.begin(), b.end()))});
    return Value(std::move(members));
}

const cu59::TIModelState kNoModel{};
const cu59::TILedState kNoLed{};
const cu59::TICueCardState kNoCue{};
const cu59::TIPrinterState kNoPrinter{};
const cu59::TICardReaderState kNoCard{};
const cu59::TIRunState kNoRun{};
const cu59::TIDebugLiveState kNoLive{};
const cu59::TIDebugCpuState kNoCpu{};
const cu59::TIDebugHeatmapState kNoHeatmap{};
const cu59::TIDebugLogState kNoLog{};
const cu59::TIDebugAsmState kNoAsm{};

template <class P> const P& orNone(const P* p, const P& none) { return p ? *p : none; }

} // namespace

EmulatorController::EmulatorController(QObject* parent) : QObject(parent) {
    // The ROM lookup first (LIB-12): without the full set, the dialog and no session.
    auto roms = cu59::lookUpRoms(QtResourceBundle());
    if (!roms.ok()) {
        QMessageBox::critical(nullptr, QStringLiteral("Calc-U-59: ROM files missing"),
                              qs("Calc-U-59 cannot start.\n\n" + calcu::roms::describe(roms.missing)));
        return;
    }
    cu59::TIAppSetup setup;
    setup.roms = std::move(roms.set);
    setup.cuecards = readResource(QStringLiteral(":/roms/cuecards.txt")).value_or("");
    setup.startModel = AppSettings::settings().startupModel();
    setup.saveFolder = AppPaths::toFsPath(AppPaths::cardDir()).string();   // cards, ti58c.mem (LIB-9)
    setup.trace = {AppPaths::toFsPath(AppPaths::traceDir()),
                   static_cast<uint64_t>(AppSettings::settings().traceMaxFileSizeMB()) * 1'000'000};
    m_app = cu59::makeAppSession(std::move(setup), cu59::steadyWallTime);
    m_mirror.onChange = [this](const cs::TopicName& t) { topicChanged(t); };
    m_me = m_app->session->listen(*this, cs::HostServices{});   // Qt states no host service yet (C59-8)
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    m_frameTimer.setInterval(1000 / 60);
    connect(&m_frameTimer, &QTimer::timeout, this, [this] { m_app->session->call(cu59::steadyWallTime()); });
    m_frameTimer.start();
}

EmulatorController::~EmulatorController() {
    m_frameTimer.stop();
    m_app.reset();
}

void EmulatorController::deliver(const cs::Message& m) {
    if (const auto* r = std::get_if<cs::ReplyMessage>(&m)) {
        m_lastReply = *r;
        return;
    }
    if (const auto* s = std::get_if<cs::StreamMessage>(&m)) {
        if (s->topic == cs::kNoticesTopic) {   // an error alert (LIB-3 AC-30), as today until C59-8
            for (const auto& p : s->appended)
                if (const auto* n = cs::payloadAs<cs::Notice>(p)) {
                    const auto parts = cu59::splitNotice(n->sentence);
                    emit errorOccurred(qs(parts.title), qs(parts.message));
                }
            return;
        }
        if (s->topic == cs::kLogTopic) {   // diagnostics: the developer's, not shown
            for (const auto& p : s->appended)
                if (const auto* l = cs::payloadAs<cs::LogLine>(p)) qDebug("%s", l->text.c_str());
            return;
        }
        if (s->topic == cu59::topics::kShowDebugPanel) {
            for (const auto& p : s->appended)
                if (const auto* d = cs::payloadAs<cu59::TIShowDebugPanel>(p)) emit debugPanelRequested(static_cast<int>(d->panel));
            return;
        }
    }
    m_mirror.deliver(m);
}

void EmulatorController::topicChanged(const cs::TopicName& t) {
    namespace T = cu59::topics;
    if (t == T::kModel) {
        // The last used model is a setting the frontend writes (LIB-9; C59-5 D-3).
        const cu59::Model m = model();
        if (m_savedModel != m) AppSettings::settings().setLastUsedModel(m);
        m_savedModel = m;
        emit modelChanged();
        emit cardStateChanged();
    } else if (t == T::kLed) {
        emit displayChanged();
    } else if (t == T::kCueCard) {
        emit cueCardChanged();
    } else if (t == T::kPrinter) {
        m_printerLines.clear();
        for (const auto& l : orNone(state<cu59::TIPrinterState>(T::kPrinter), kNoPrinter).lines) m_printerLines << qs(l);
        emit printerChanged();
    } else if (t == T::kCardReader) {
        emit cardStateChanged();
    } else if (t == T::kRunState) {
        emit runStateChanged();
    } else if (t == T::kDebugLive) {
        emit liveSnapshotChanged();
    } else if (t == T::kDebugCpu) {
        emit cpuChanged();
    } else if (t == T::kDebugHeatmap) {
        emit heatmapChanged();
    } else if (t == T::kDebugLog) {
        emit logChanged();
    } else if (t == T::kDebugAsm) {
        emit asmChanged();
    } else if (t == T::kTrace) {
        emit traceChanged();
    }
}

cs::ReplyMessage EmulatorController::send(const cs::CommandName& name, Value args) {
    m_lastReply.reset();
    const cs::CallId id = m_app->session->send(m_me, cs::Command{name, std::move(args)});
    if (m_lastReply && m_lastReply->call == id) return *m_lastReply;
    return cs::ReplyMessage{id, true, "", nullptr};   // an operation: its reply comes when it ends
}

bool EmulatorController::sendFlag(const cs::CommandName& name, Value args) {
    const cs::ReplyMessage r = send(name, std::move(args));
    const auto* f = cs::payloadAs<cu59::TIFlag>(r.result);
    return r.done && f && f->value;
}

// ── Machine ──────────────────────────────────────────────────────────────────

void EmulatorController::start(cu59::Model m) {
    if (!m_savedModel || *m_savedModel != m || !state<cu59::TIModelState>(cu59::topics::kModel))
        send(cu59::commands::kStartMachine, Value::object({{"model", static_cast<int>(m)}}));
}
cu59::Model EmulatorController::model() const { return orNone(state<cu59::TIModelState>(cu59::topics::kModel), kNoModel).model; }
QString EmulatorController::moduleID() const { return qs(orNone(state<cu59::TIModelState>(cu59::topics::kModel), kNoModel).moduleID); }
void EmulatorController::changeModule(const QString& id) {
    send(cu59::commands::kChangeModule, Value::object({{"id", id.toStdString()}}));
}
QVector<cu59::ModuleMetadata> EmulatorController::allModules() const {
    const auto& all = orNone(state<cu59::TIModelState>(cu59::topics::kModel), kNoModel).modules;
    return QVector<cu59::ModuleMetadata>(all.begin(), all.end());
}

// ── Display ──────────────────────────────────────────────────────────────────

const cu59::LedFrame& EmulatorController::display() const { return orNone(state<cu59::TILedState>(cu59::topics::kLed), kNoLed).led; }
double EmulatorController::calcIndicatorOpacity() const {
    return orNone(state<cu59::TILedState>(cu59::topics::kLed), kNoLed).calcIndicatorOpacity;
}
void EmulatorController::setFullSpeed(bool on) { send(cu59::commands::kFullSpeed, Value::object({{"on", on}})); }
const std::optional<cu59::CueCardContent>& EmulatorController::cueCard() const {
    return orNone(state<cu59::TICueCardState>(cu59::topics::kCueCard), kNoCue).card;
}

// ── Keys ─────────────────────────────────────────────────────────────────────

void EmulatorController::pressKey(int row, int col) {
    send(cu59::commands::kPressKey, Value::object({{"code", static_cast<int>(cu59::matrixCode(row, col))}}));
}
void EmulatorController::releaseKey(int row, int col) {
    send(cu59::commands::kReleaseKey, Value::object({{"code", static_cast<int>(cu59::matrixCode(row, col))}}));
}
void EmulatorController::keyboardDown(const std::vector<uint8_t>& codes) {
    send(cu59::commands::kKeyboardDown, Value::object({{"codes", Value(Value::Bytes(codes))}}));
}
int EmulatorController::keyboardHeldKey() const { return orNone(state<cu59::TIRunState>(cu59::topics::kRunState), kNoRun).heldKey; }

// ── Printer ──────────────────────────────────────────────────────────────────

const std::vector<PrinterCodeLine>& EmulatorController::printerCodeLines() const {
    return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).codeLines;
}
double EmulatorController::printerStripHeightUnits() const {
    return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).stripHeightUnits;
}
bool EmulatorController::printerConnected() const {
    return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).connected;
}
bool EmulatorController::printerTrace() const { return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).trace; }
void EmulatorController::pressPrinterPrint(bool pressed) { send(cu59::commands::kPrinterPrint, Value::object({{"pressed", pressed}})); }
void EmulatorController::pressPrinterAdv(bool pressed) { send(cu59::commands::kPrinterAdv, Value::object({{"pressed", pressed}})); }

// ── Card reader ──────────────────────────────────────────────────────────────

EmulatorController::CardState EmulatorController::cardState() const {
    return orNone(state<cu59::TICardReaderState>(cu59::topics::kCardReader), kNoCard).state;
}
QString EmulatorController::cardFileName() const {
    return qs(orNone(state<cu59::TICardReaderState>(cu59::topics::kCardReader), kNoCard).fileName);
}
void EmulatorController::fitCard(const QString& path) {
    send(cu59::commands::kFitCard,
         withBytes(Value::object({{"path", path.toStdString()}, {"name", QFileInfo(path).fileName().toStdString()}}), path));
}
void EmulatorController::fitBlankCard(const QString& savePath) {
    send(cu59::commands::kFitBlankCard,
         Value::object({{"path", savePath.toStdString()}, {"name", QFileInfo(savePath).fileName().toStdString()}}));
}
bool EmulatorController::tapCard(double nx, double ny) {
    return sendFlag(cu59::commands::kTapCard, Value::object({{"x", nx}, {"y", ny}}));
}
std::optional<int> EmulatorController::litCardCorner() const {
    return orNone(state<cu59::TICardReaderState>(cu59::topics::kCardReader), kNoCard).litCorner;
}

// ── State files ──────────────────────────────────────────────────────────────

void EmulatorController::loadStateFile(const QString& path) {
    send(cu59::commands::kLoadState, withBytes(Value::object({}), path));
}

cu59::SessionController::DropAction EmulatorController::dropAction(const QString& path) const {
    const cs::CallId id = m_app->session->ask(
        m_me, cs::Request{cu59::commands::kDropAction, Value::object({{"name", QFileInfo(path).fileName().toStdString()}})});
    if (m_lastReply && m_lastReply->call == id)
        if (const auto* a = cs::payloadAs<cu59::TIDropAction>(m_lastReply->result)) return a->action;
    return cu59::SessionController::DropAction::NotAccepted;
}

void EmulatorController::openDroppedFile(const QString& path) {
    send(cu59::commands::kOpenDropped,
         withBytes(Value::object({{"path", path.toStdString()}, {"name", QFileInfo(path).fileName().toStdString()}}), path));
}

// ── Debugger ─────────────────────────────────────────────────────────────────

void EmulatorController::setLiveDebugEnabled(bool on) { send(cu59::commands::kLiveDebug, Value::object({{"on", on}})); }
void EmulatorController::setCpuDebugEnabled(bool on) { send(cu59::commands::kCpuDebug, Value::object({{"on", on}})); }
bool EmulatorController::isFrozen() const { return orNone(state<cu59::TIRunState>(cu59::topics::kRunState), kNoRun).debug.frozen; }
std::optional<cu59::FreezeOwner> EmulatorController::freezeOwner() const {
    return orNone(state<cu59::TIRunState>(cu59::topics::kRunState), kNoRun).debug.owner;
}
bool EmulatorController::pendingFreezeOnPCChange() const {
    return orNone(state<cu59::TIRunState>(cu59::topics::kRunState), kNoRun).debug.pcChangeArmed;
}
bool EmulatorController::pendingCpuScanLoopFreeze() const {
    return orNone(state<cu59::TIRunState>(cu59::topics::kRunState), kNoRun).debug.scanLoopExitArmed;
}
void EmulatorController::freeze(cu59::FreezeOwner owner) {
    send(cu59::commands::kFreeze, Value::object({{"owner", owner == cu59::FreezeOwner::Calculator ? 1 : 0}}));
}
const cu59::LiveDebugSnapshot& EmulatorController::liveSnapshot() const {
    return orNone(state<cu59::TIDebugLiveState>(cu59::topics::kDebugLive), kNoLive).live;
}
const std::vector<cu59::StepEntry>& EmulatorController::programListing() const {
    return orNone(state<cu59::TIDebugLiveState>(cu59::topics::kDebugLive), kNoLive).listing;
}
cu59::ProgramSource EmulatorController::programListingSource() const {
    return orNone(state<cu59::TIDebugLiveState>(cu59::topics::kDebugLive), kNoLive).listingSource;
}
const std::vector<cu59::InspectorEntry>& EmulatorController::cpuHistory() const {
    return orNone(state<cu59::TIDebugCpuState>(cu59::topics::kDebugCpu), kNoCpu).history;
}
const std::vector<uint32_t>& EmulatorController::heatmapCounts() const {
    return orNone(state<cu59::TIDebugHeatmapState>(cu59::topics::kDebugHeatmap), kNoHeatmap).counts;
}
EmulatorController::DebugLevel EmulatorController::debugLevel() const {
    return orNone(state<cu59::TIDebugLogState>(cu59::topics::kDebugLog), kNoLog).level;
}
QString EmulatorController::logText() const { return qs(orNone(state<cu59::TIDebugLogState>(cu59::topics::kDebugLog), kNoLog).text); }
bool EmulatorController::manualTrace() const {
    const auto* t = state<cu59::TITraceState>(cu59::topics::kTrace);
    return t && t->manualTrace;
}
void EmulatorController::setManualTrace(bool on) { send(cu59::commands::kManualTrace, Value::object({{"on", on}})); }
QString EmulatorController::asmFileName() const { return qs(orNone(state<cu59::TIDebugAsmState>(cu59::topics::kDebugAsm), kNoAsm).fileName); }
int EmulatorController::asmWordCount() const { return orNone(state<cu59::TIDebugAsmState>(cu59::topics::kDebugAsm), kNoAsm).wordCount; }
QString EmulatorController::asmStatus() const { return qs(orNone(state<cu59::TIDebugAsmState>(cu59::topics::kDebugAsm), kNoAsm).status); }
bool EmulatorController::asmOverlayActive() const { return orNone(state<cu59::TIDebugAsmState>(cu59::topics::kDebugAsm), kNoAsm).active; }
void EmulatorController::loadAsmFile(const QString& path) {
    Value args = Value::object({{"name", QFileInfo(path).fileName().toStdString()}});
    if (auto text = readResource(path)) {
        std::vector<Value::Member> members = *args.members();
        members.push_back({"text", Value(*text)});
        args = Value(std::move(members));
    }
    send(cu59::commands::kLoadAsm, std::move(args));
}
