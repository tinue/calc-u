#include "EmulatorController.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "KeyIds.hpp"
#include "ModuleCatalog.hpp"
#include "TIMagneticCardKind.hpp"
#include "ProductRoms.hpp"
#include "QtResourceBundle.hpp"
#include "sessions/LibraryTopics.hpp"
#include "MachineModelInfo.hpp"
#include "presets/PresetsPart.hpp"
#include "startup/StartupPart.hpp"
#include <QCoreApplication>
#include <QDir>
#include "debugger/RunState.hpp"
#include "paper/PaperTopics.hpp"
#include "typing/TypingTopics.hpp"
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
    setup.saveFolder = AppPaths::toFsPath(AppPaths::saveDir()).string();   // cards, ti58c.memory.yaml (LIB-9)
    // LIB-18: the startup model, `last-model` and the startup presets are the
    // settings file's; makeAppSession sends `start` (the startup model with its
    // startup preset, or its built-in default: ML, no printer).
    setup.settings = &AppSettings::settings().file();
    // LIB-6 AC-31, AC-33: the bundled examples beside the program (bundle.yaml).
    setup.examplesFolder = AppPaths::toFsPath(AppPaths::bundledExamplesDir()).string();
    setup.home = AppPaths::toFsPath(QDir::homePath()).string();
    setup.trace = {AppPaths::toFsPath(AppPaths::traceDir()),
                   static_cast<uint64_t>(AppSettings::settings().traceMaxFileSizeMB()) * 1'000'000};
    m_app = cu59::makeAppSession(std::move(setup), cu59::steadyWallTime);
    m_listener.mirror().onChange = [this](const cs::TopicName& t) { topicChanged(t); };
    m_listener.onMessage = [this](const cs::Message& m) { onMessage(m); };
    // LIB-8 P-E6: the clipboard for the paper panel's Copy (until M-57).
    cs::HostServices services;
    services.offered = {cs::HostService::ClipboardImage, cs::HostService::ClipboardText};
    m_listener.listen(*m_app.session, services);
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    m_frameTimer.setInterval(1000 / 60);
    connect(&m_frameTimer, &QTimer::timeout, this, [this] { m_app.session->call(cu59::steadyWallTime()); });
    m_frameTimer.start();
}

EmulatorController::~EmulatorController() {
    m_frameTimer.stop();
    // The parts' last duties (the TI-58C's constant memory file, C59-3 AC-6).
    if (m_app.session) m_app.session->close();
    m_listener.stop();
    m_app = {};
}

// What the mirror does not keep for the views: the notices, the log and the
// panel requests, taken from it as they come. An operation's reply is not
// waited for.
void EmulatorController::onMessage(const cs::Message& m) {
    if (const auto* a = std::get_if<cs::ServiceAsk>(&m)) {   // P-E6
        cs::ServiceAnswer answer;
        if (calcu::qt6::ClipboardService::handles(a->service)) answer = m_clipboard.answer(*a);
        else answer.reason = "This frontend does not offer it.";
        m_app.session->answer(a->ask, std::move(answer));
        return;
    }
    if (std::holds_alternative<cs::StateMessage>(m) || std::holds_alternative<cs::StreamMessage>(m))
        for (const auto& h : m_handlers) h(m);
    if (const auto* s = std::get_if<cs::StreamMessage>(&m)) {
        // The paper streams are the panel's (it keeps the paper); the mirror keeps none once it listens.
        if (s->topic.value.rfind("paper", 0) == 0) {
            if (m_handlers.empty()) return;   // kept until the panel takes them (addMessageHandler)
            (void)m_listener.mirror().takeAppended(s->topic);
            (void)m_listener.mirror().takeReset(s->topic);
            return;
        }
        const bool handled = s->topic == cs::kNoticesTopic || s->topic == cs::kLogTopic ||
                             s->topic == cu59::topics::kShowDebugPanel;
        if (handled) (void)m_listener.mirror().takeAppended(s->topic);
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
}

void EmulatorController::topicChanged(const cs::TopicName& t) {
    namespace T = cu59::topics;
    if (t == T::kModel) {
        // The last used model is LIB-18's StartupPart's ([startup] last-model).
        emit modelChanged();
        emit cardStateChanged();
    } else if (t == T::kLed) {
        emit displayChanged();
    } else if (t == T::kCueCard) {
        emit cueCardChanged();
    } else if (t == T::kPrinter) {
        emit printerChanged();
    } else if (t == calcu::media::kMediaTopic) {   // the card reader and slot-1 (LIB-7 59)
        emit cardStateChanged();
    } else if (t == T::kRunState || t == calcu::typing::kHeldKeysTopic || t == calcu::debugger::kRunStateTopic) {
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
    } else if (t == calcu::debugger::kTraceTopic) {
        emit traceChanged();
    }
}

cs::ReplyMessage EmulatorController::send(const cs::CommandName& name, Value args) {
    const auto sent = m_listener.send(name, std::move(args));
    if (sent.reply) return *sent.reply;
    return cs::ReplyMessage{sent.call, true, "", nullptr};   // an operation: its reply comes when it ends
}

std::optional<std::pair<uint64_t, double>> EmulatorController::emulatedTime() {
    const cs::ReplyMessage r = m_listener.ask(cs::kTicksRequest, Value::object({}));
    const auto* t = cs::payloadAs<cs::EmulatedTimeReply>(r.result);
    if (!t) return std::nullopt;
    return std::make_pair(static_cast<uint64_t>(t->ticks), t->seconds);
}

bool EmulatorController::sendFlag(const cs::CommandName& name, Value args) {
    const cs::ReplyMessage r = send(name, std::move(args));
    const auto* f = cs::payloadAs<cu59::TIFlag>(r.result);
    return r.done && f && f->value;
}

// ── Machine ──────────────────────────────────────────────────────────────────

// LIB-18: the model switch (`model {name}`), the module (`attach`/`detach`
// slot-1) and the printer (`attach`/`detach` printer), each an operation.
void EmulatorController::start(cu59::Model m) {
    send(calcu::startup::kModelCommand, Value::object({{"name", cu59::modelId(m).value}}));
}
void EmulatorController::resetMachine() { send(calcu::startup::kResetCommand); }
void EmulatorController::cleanResetMachine() { send(calcu::startup::kColdBootCommand); }
void EmulatorController::togglePrinterConnection() {
    if (printerConnected()) send(calcu::startup::kDetachCommand, Value::object({{"place", "printer"}}));
    else send(calcu::startup::kAttachCommand, Value::object({{"place", "printer"}, {"peripheral", "PC-100C"}}));
}
cu59::Model EmulatorController::model() const { return orNone(state<cu59::TIModelState>(cu59::topics::kModel), kNoModel).model; }
QString EmulatorController::moduleID() const { return qs(orNone(state<cu59::TIModelState>(cu59::topics::kModel), kNoModel).moduleID); }
std::vector<calcu::media::PickerEntry> EmulatorController::modulePicker() const {
    std::vector<calcu::media::PickerEntry> out;
    const auto* media = state<calcu::media::MediaState>(calcu::media::kMediaTopic);
    if (const auto* slot = media ? media->place("slot-1") : nullptr)
        for (const auto& e : slot->picker)
            if (e.group == calcu::media::PickerEntry::Group::Module) out.push_back(e);
    return out;
}
void EmulatorController::chooseModule(const QString& entryId) {
    if (entryId.isEmpty() || entryId == QStringLiteral("module:") + moduleID()) return;
    send(calcu::media::kChooseCommand, Value::object({{"place", "slot-1"}, {"entry", entryId.toStdString()}}));
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

void EmulatorController::screenKeyDown(int row, int col) {
    const auto id = cu59::keyIdAt(cu59::matrixCode(row, col));
    if (!id) return;
    send(calcu::typing::kScreenKeyDown, Value::object({{"key", id->value}, {"pointer", 1}}));
}
void EmulatorController::screenKeyUp() {
    send(calcu::typing::kScreenKeyUp, Value::object({{"key", ""}, {"pointer", 1}}));
}
void EmulatorController::hostKeyDown(const calcu::typing::HostKeyEvent& e) {
    send(calcu::typing::kHostKeyDown, calcu::typing::toValue(e));
}
void EmulatorController::hostKeyUp(const std::string& physical) {
    send(calcu::typing::kHostKeyUp, Value::object({{"physical", physical}}));
}
void EmulatorController::focusLost() { send(calcu::typing::kFocusLost); }
int EmulatorController::keyboardHeldKey() const {
    const auto* h = state<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic);
    if (!h || h->live.empty()) return -1;
    const auto code = cu59::matrixCodeOf(h->live.back());
    return code ? (*code / 10 - 1) * 5 + (*code % 10 - 1) : -1;
}

// ── Printer ──────────────────────────────────────────────────────────────────

bool EmulatorController::printerConnected() const {
    return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).connected;
}
bool EmulatorController::printerTrace() const { return orNone(state<cu59::TIPrinterState>(cu59::topics::kPrinter), kNoPrinter).trace; }

void EmulatorController::addMessageHandler(std::function<void(const cs::Message&)> handler) {
    // What the topics hold now, then every message as it comes.
    for (const cs::TopicName& t : {calcu::paper::kPapersTopic, calcu::debugger::kRunStateTopic, calcu::debugger::kTraceTopic})
        if (const cs::PayloadPtr* p = m_listener.mirror().state(t)) handler(cs::StateMessage{t, *p});
    if (const auto* papers = state<calcu::paper::PaperList>(calcu::paper::kPapersTopic))
        for (const auto& e : papers->papers) {
            const cs::TopicName t = calcu::paper::paperTopic(e.owner);
            cs::StreamMessage s{t, m_listener.mirror().takeReset(t), m_listener.mirror().takeAppended(t)};
            if (s.reset || !s.appended.empty()) handler(s);
        }
    m_handlers.push_back(std::move(handler));
}

// ── Card reader ──────────────────────────────────────────────────────────────

const calcu::media::PlaceState* EmulatorController::cardReader() const {
    const auto* media = state<calcu::media::MediaState>(calcu::media::kMediaTopic);
    return media ? media->place("card-reader") : nullptr;
}
bool EmulatorController::cardPassing() const {
    const auto* p = cardReader();
    return p && p->passing.has_value();
}
std::vector<calcu::media::PickerEntry> EmulatorController::savedCards() const {
    std::vector<calcu::media::PickerEntry> out;
    if (const auto* p = cardReader())
        for (const auto& e : p->picker)
            if (e.group != calcu::media::PickerEntry::Group::Empty && e.id.rfind("implicit:", 0) != 0) out.push_back(e);
    return out;
}
void EmulatorController::armCard(const QString& entryId) {
    send(calcu::media::kChooseCommand, Value::object({{"place", "card-reader"}, {"entry", entryId.toStdString()}}));
}
QString EmulatorController::armFreshCard(const QString& name, bool confirmed) {
    const cs::ReplyMessage r = send(calcu::media::kStartNewCommand,
                                    Value::object({{"place", "card-reader"},
                                                   {"template", std::string("implicit:") + cu59::kFreshCardTemplate},
                                                   {"name", name.toStdString()},
                                                   {"confirmed", confirmed}}));
    return r.done ? QString() : qs(r.reason);
}
void EmulatorController::ejectCard() { send(calcu::media::kEjectCommand, Value::object({{"place", "card-reader"}})); }
bool EmulatorController::tapCard(double nx, double ny) {
    return sendFlag(cu59::commands::kTapCard, Value::object({{"x", nx}, {"y", ny}}));
}
std::optional<int> EmulatorController::litCardCorner() const {
    return orNone(state<cu59::TICueCardState>(cu59::topics::kCueCard), kNoCue).litCorner;
}

// ── Presets (LIB-6 AC-42: a file in its folder) ──────────────────────────────

void EmulatorController::loadPreset(const QString& path) {
    send(calcu::presets::kLoadPresetCommand, Value::object({{"ref", path.toStdString()}}));   // UTF-8
}

cu59::DropAction EmulatorController::dropAction(const QString& path) const {
    const cs::ReplyMessage r =
        m_listener.ask(cu59::commands::kDropAction, Value::object({{"name", QFileInfo(path).fileName().toStdString()}}));
    if (const auto* a = cs::payloadAs<cu59::TIDropAction>(r.result)) return a->action;
    return cu59::DropAction::NotAccepted;
}

void EmulatorController::openDroppedFile(const QString& path) {
    if (dropAction(path) == cu59::DropAction::LoadPreset) {   // LIB-6 AC-42: by its path, in its folder
        loadPreset(path);
        return;
    }
    send(cu59::commands::kOpenDropped,
         withBytes(Value::object({{"path", path.toStdString()}, {"name", QFileInfo(path).fileName().toStdString()}}), path));
}

// ── Debugger ─────────────────────────────────────────────────────────────────

void EmulatorController::setLiveDebugEnabled(bool on) { send(cu59::commands::kLiveDebug, Value::object({{"on", on}})); }
void EmulatorController::setCpuDebugEnabled(bool on) { send(cu59::commands::kCpuDebug, Value::object({{"on", on}})); }
const calcu::debugger::RunState* EmulatorController::runState() const {
    return state<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
}
bool EmulatorController::isFrozen() const {
    const auto* r = runState();
    return r && r->paused;
}
bool EmulatorController::startArmed(const calcu::debugger::LevelName& level) const {
    const auto* r = runState();
    return r && r->armed == level;
}
void EmulatorController::armStart(const calcu::debugger::LevelName& level) {
    send(calcu::debugger::kPauseOnStartCommand, Value::object({{"level", level.value}}));
}
void EmulatorController::disarmStart() { send(calcu::debugger::kPauseOnStartCommand, Value::object({{"off", true}})); }
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
bool EmulatorController::traceRunning() const {
    const auto* t = state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic);
    return t && t->running;
}
void EmulatorController::setTrace(bool on) {
    send(calcu::debugger::kTraceCommand, on ? Value::object({{"button", true}}) : Value::object({{"off", true}}));
}
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
