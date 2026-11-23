#include "EmulatorController.hpp"
#include "AppPaths.hpp"
#include "KeyIds.hpp"
#include "ProductRoms.hpp"
#include "QtResourceBundle.hpp"
#include "TICatalogue.hpp"
#include "TIMagneticCardKind.hpp"
#include "control/ControlState.hpp"
#include "debugger/RunState.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/Inputs.hpp"
#include "typing/TypingTopics.hpp"
#include <QFile>
#include <QFileInfo>
#include <memory>
#include <utility>

namespace {

namespace cs = calcu::sessions;
using cs::Value;

QString qs(const std::string& s) { return QString::fromStdString(s); }

std::optional<std::string> readResource(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    return f.readAll().toStdString();
}

const cu59::TICalcIndicatorState kNoLed{};
const cu59::TICueCardState kNoCue{};
const cu59::TICalculatorDebug kNoLive{};
const cu59::TIProgramListing kNoListing{};
const cu59::TICpuDebug kNoCpu{};
const cu59::TIHeatmap kNoHeatmap{};
const cu59::TIDebugView kNoView{};
const cu59::TIAsmOverlayState kNoAsm{};

template <class P> const P& orNone(const P* p, const P& none) { return p ? *p : none; }

} // namespace

EmulatorController::EmulatorController(QObject* parent, calcu::pacing::WallClock clock) : QObject(parent) {
    calcu::shell::HostSettings& settings = AppPaths::folders().settings();   // opened at the first use
    // The ROM lookup first: without the full set the session has no
    // machine and the topic roms-missing names the files.
    cu59::TIAppSetup setup;
    setup.roms = cu59::lookUpRoms(QtResourceBundle());
    m_romsFound = setup.roms.ok();
    AppPaths::saveDir();    // the save folder made at the start; the trace folder at each trace
    // The startup model, `last-model` and the startup presets are the
    // settings file's; the save folder (cards, ti58c.memory.yaml), the trace
    // folder, its limit and `~/` its places'.
    setup.settings = &settings.file();
    setup.paths = &settings.paths();
    // The bundled examples beside the program (bundle.yaml).
    setup.examplesFolder = calcu::qt6::toFsPath(AppPaths::bundledExamplesDir()).string();
    // The settings dialog reads and writes the product's file through
    // the session; a change is applied at once (the LED style through the
    // `settings` topic). A new save folder is used from the next start
    // (the TI-58C's constant memory file is read at its build);
    // a new trace folder or limit from the next trace.
    calcu::shell::HostSessionSetup host;
    host.settings = &settings;
    host.clock = clock;
    cs::HostServices services;
    services.offered = {cs::HostService::ClipboardImage, cs::HostService::ClipboardText, cs::HostService::OpenUrl};
    m_session = std::make_unique<calcu::qt6::ProductHost<cu59::TIAppPart>>(
        cu59::makeAppSession(std::move(setup), std::move(host)), std::move(services), std::move(clock));
    this->host().addMessageHandler([this](const cs::Message& m) { onMessage(m); });
    connect(&this->host(), &calcu::qt6::SessionHost::topicChanged, this, &EmulatorController::topicChanged);
    // The LOG tab's text at 1 Hz (the paging of cu59::RingLog).
    m_logTimer.setInterval(1000);
    connect(&m_logTimer, &QTimer::timeout, this, [this] {
        if (!std::exchange(m_logDirty, false)) return;
        m_logText = QString::fromStdString(m_log.displayText());
        emit logChanged();
    });
    m_logTimer.start();
}

EmulatorController::~EmulatorController() {
    // The parts' last duties (the TI-58C's constant memory file; what a
    // card still has to write). No message goes out after it.
    // Ended first, while the LOG tab's view is still there.
    m_session.reset();
}

void EmulatorController::start() { m_session->start(); }

// The LOG tab keeps its own view of the log; the notices are the shell's
// dialog, the paper the panel area's.
void EmulatorController::onMessage(const cs::Message& m) {
    const auto* s = std::get_if<cs::StreamMessage>(&m);
    if (!s || s->topic != cs::kLogTopic) return;
    for (const auto& p : s->appended)
        if (const auto* l = cs::payloadAs<cs::LogLine>(p)) {
            std::size_t at = 0;
            for (;;) {   // a message may hold several lines (the machine's of one call)
                const std::size_t nl = l->text.find('\n', at);
                m_log.write(l->text.substr(at, nl == std::string::npos ? std::string::npos : nl - at));
                if (nl == std::string::npos) break;
                at = nl + 1;
            }
            m_logDirty = true;
        }
}

void EmulatorController::topicChanged(const QString& name) {
    namespace T = cu59::topics;
    const cs::TopicName t{name.toStdString()};
    if (t == T::kCalcIndicator) {
        emit displayChanged();
    } else if (t == T::kCueCard) {
        emit cueCardChanged();
        emit cardStateChanged();
    } else if (t == calcu::media::kMediaTopic) {   // the card reader
        emit cardStateChanged();
    } else if (t == calcu::typing::kHeldKeysTopic || t == calcu::debugger::kRunStateTopic) {
        emit runStateChanged();
    } else if (t == T::kCalculatorDebug || t == T::kProgramListing) {
        emit liveSnapshotChanged();
    } else if (t == T::kCpuDebug) {
        emit cpuChanged();
    } else if (t == T::kHeatmap) {
        emit heatmapChanged();
    } else if (t == T::kDebugView) {
        const auto& v = orNone(state<cu59::TIDebugView>(T::kDebugView), kNoView);
        if (v.requests > m_viewRequests) {   // a `debug-panel:` step, or Run
            m_viewRequests = v.requests;
            emit debugPanelRequested(static_cast<int>(v.view));
        }
    } else if (t == cs::kLogLevelTopic) {
        emit logChanged();   // the level shown
    } else if (t == T::kAsmOverlay) {
        emit asmChanged();
    } else if (t == calcu::control::kControlStateTopic) {
        const std::string model = modelId();
        if (model != m_model) {
            m_model = model;
            emit modelChanged();
            emit cardStateChanged();
        }
        // The PC-100C plugged in or out, a control of it moved.
        auto printer = std::make_pair(printerConnected(), printerControls());
        if (printer != m_printerSeen) {
            m_printerSeen = std::move(printer);
            emit printerChanged();
        }
        emit entriesChanged();
    } else if (t == calcu::debugger::kTraceTopic) {
        emit traceChanged();
    } else if (t == calcu::shell::kSettingsTopic) {
        emit settingsChanged();
    }
}

cs::ReplyMessage EmulatorController::send(const cs::CommandName& name, Value args) {
    // An operation replies when it ends, after this returns: the handler
    // keeps what it writes into alive.
    auto got = std::make_shared<std::optional<cs::ReplyMessage>>();
    const cs::CallId id = host().send(name, std::move(args), [got](const cs::ReplyMessage& r) { *got = r; });
    if (*got) return **got;
    return cs::ReplyMessage{id, true, "", nullptr};   // an operation: its reply comes when it ends
}

cs::ReplyMessage EmulatorController::ask(const cs::RequestName& name, Value args) {
    return m_session->ask(name, std::move(args));   // requests do not wait
}

// ── Machine: drawn from the control state ───────────────────────────────────

std::string EmulatorController::modelId() const {
    const auto* c = state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return c ? c->model.value : std::string();
}
namespace {
const calcu::machine::PlacePath kPrinterPath{{::kTIPrinterPlace}};
} // namespace

bool EmulatorController::printerConnected() const {
    const auto* c = state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const auto* p = c ? c->place(kPrinterPath) : nullptr;
    if (!p) return false;
    for (const auto& f : p->fitting)
        if (f.plugged) return true;
    return false;
}
std::vector<calcu::peripherals::ControlInfo> EmulatorController::printerControls() const {
    const auto* c = state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    return c ? c->controlsAt(kPrinterPath) : std::vector<calcu::peripherals::ControlInfo>{};
}
cu59::LedStyle EmulatorController::ledStyle() const {
    const Value v = m_session->host().value(calcu::shell::kSettingsTopic);
    for (const auto& r : calcu::shell::read::elements(v, "rows"))
        if (calcu::shell::read::text(r, "section") == "display" && calcu::shell::read::text(r, "key") == "led-style")
            return calcu::shell::read::text(r, "value") == "classic" ? cu59::LedStyle::Classic : cu59::LedStyle::Modernized;
    return cu59::LedStyle::Modernized;
}

// ── Display ──────────────────────────────────────────────────────────────────

const cu59::LedFrame& EmulatorController::display() const { return orNone(state<cu59::TICalcIndicatorState>(cu59::topics::kCalcIndicator), kNoLed).led; }
double EmulatorController::calcIndicatorOpacity() const {
    return orNone(state<cu59::TICalcIndicatorState>(cu59::topics::kCalcIndicator), kNoLed).calcIndicatorOpacity;
}
const std::optional<cu59::CueCardContent>& EmulatorController::cueCard() const {
    return orNone(state<cu59::TICueCardState>(cu59::topics::kCueCard), kNoCue).card;
}

// ── Keys ─────────────────────────────────────────────────────────────────────

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

// ── Card reader ──────────────────────────────────────────────────────────────

const calcu::media::PlaceState* EmulatorController::mediaPlace(const std::string& place) const {
    const auto* media = state<calcu::media::MediaState>(calcu::media::kMediaTopic);
    return media ? media->place(place) : nullptr;
}
const calcu::media::PlaceState* EmulatorController::cardReader() const { return mediaPlace("card-reader"); }
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
    const cs::ReplyMessage r = send(cu59::commands::kTapCard, Value::object({{"x", nx}, {"y", ny}}));
    const auto* f = cs::payloadAs<cu59::TIFlag>(r.result);
    return r.done && f && f->value;
}
bool EmulatorController::cardOnShow() const { return orNone(state<cu59::TICueCardState>(cu59::topics::kCueCard), kNoCue).onShow; }
std::optional<int> EmulatorController::litCardCorner() const {
    return orNone(state<cu59::TICueCardState>(cu59::topics::kCueCard), kNoCue).litCorner;
}

// ── Debugger ─────────────────────────────────────────────────────────────────

void EmulatorController::setLiveDebugEnabled(bool on) { send(cu59::commands::kShowCalculatorDebug, Value::object({{"on", on}})); }
void EmulatorController::setCpuDebugEnabled(bool on) { send(cu59::commands::kShowCpuDebug, Value::object({{"on", on}})); }
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
bool EmulatorController::entryAvailable(const char* name) const {
    const auto* c = state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const auto* e = c ? c->productEntry(name) : nullptr;
    if (!e || !e->value) return true;
    const Value v = e->value->toValue();
    const Value* a = v.find("available");
    return !a || !a->boolean() || *a->boolean();
}
QString EmulatorController::entryReason(const char* name) const {
    const auto* c = state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    const auto* e = c ? c->productEntry(name) : nullptr;
    if (!e || !e->value) return {};
    const Value v = e->value->toValue();
    const Value* r = v.find("reason");
    return r && r->string() ? qs(*r->string()) : QString();
}
const cu59::LiveDebugSnapshot& EmulatorController::liveSnapshot() const {
    return orNone(state<cu59::TICalculatorDebug>(cu59::topics::kCalculatorDebug), kNoLive).live;
}
const std::vector<cu59::StepEntry>& EmulatorController::programListing() const {
    return orNone(state<cu59::TIProgramListing>(cu59::topics::kProgramListing), kNoListing).steps;
}
cu59::ProgramSource EmulatorController::programListingSource() const {
    return orNone(state<cu59::TIProgramListing>(cu59::topics::kProgramListing), kNoListing).source;
}
const std::vector<cu59::InspectorEntry>& EmulatorController::cpuHistory() const {
    return orNone(state<cu59::TICpuDebug>(cu59::topics::kCpuDebug), kNoCpu).entries;
}
const std::vector<uint32_t>& EmulatorController::heatmapCounts() const {
    return orNone(state<cu59::TIHeatmap>(cu59::topics::kHeatmap), kNoHeatmap).counts;
}
EmulatorController::DebugLevel EmulatorController::debugLevel() const {
    const auto* l = state<cs::LogLevelState>(cs::kLogLevelTopic);
    return l ? l->level : DebugLevel::Off;
}
void EmulatorController::cycleDebugLevel() {
    const auto next = static_cast<DebugLevel>((static_cast<int>(debugLevel()) + 1) % 3);
    send(cs::kLogLevelCommand, Value::object({{"level", std::string(cs::logLevelName(next))}}));
}
void EmulatorController::clearLog() {
    m_log.clear();
    m_logText.clear();
    m_logDirty = false;
    emit logChanged();
}
bool EmulatorController::traceRunning() const {
    const auto* t = state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic);
    return t && t->running;
}
void EmulatorController::setTrace(bool on) {
    send(calcu::debugger::kTraceCommand, on ? Value::object({{"button", true}}) : Value::object({{"off", true}}));
}
QString EmulatorController::asmFileName() const { return qs(orNone(state<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay), kNoAsm).fileName); }
int EmulatorController::asmWordCount() const { return orNone(state<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay), kNoAsm).instructions; }
QString EmulatorController::asmStatus() const { return qs(orNone(state<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay), kNoAsm).status); }
bool EmulatorController::asmOverlayActive() const { return orNone(state<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay), kNoAsm).running; }
void EmulatorController::loadAsmFile(const QString& path) {
    Value args = Value::object({{"name", QFileInfo(path).fileName().toStdString()}});
    if (auto text = readResource(path)) {
        std::vector<Value::Member> members = *args.members();
        members.push_back({"text", Value(*text)});
        args = Value(std::move(members));
    }
    send(cu59::commands::kAsmLoad, std::move(args));   // Select File; a refusal is a notice
}
