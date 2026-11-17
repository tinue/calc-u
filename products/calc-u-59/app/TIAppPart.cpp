#include "TIAppPart.hpp"
#include "TIMagneticCardKind.hpp"
#include "PacingProfiles.hpp"
#include "Settings.hpp"
#include "TIDebuggerSetup.hpp"
#include "TIPrinters.hpp"
#include "TITyping.hpp"
#include "../core/TIDebugging.hpp"
#include "files/Files.hpp"
#include "media/Writer.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Listener.hpp"
#include "sessions/Operation.hpp"
#include "sessions/Args.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>
#include <filesystem>

namespace cu59 {

using namespace calcu;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::StartOperation;
using sessions::Value;

// ── Payload values (for translating hosts and the harness's `state`) ─────────
// The value forms are fixed (`calcu state c59.led|c59.cue-card|c59.printer`).

Value TILedState::toValue() const {
    return Value::object({{"calc-indicator", calcIndicatorOpacity}});
}
Value TICueCardState::toValue() const {
    if (!card) return Value();
    return Value::object({{"title", card->title},
                          {"on-show", onShow},
                          {"lit-corner", litCorner ? Value(*litCorner) : Value()}});
}
Value TIPrinterState::toValue() const { return Value::object({{"connected", connected}, {"trace", trace}}); }

// ── The part ─────────────────────────────────────────────────────────────────

namespace {


// LIB-10 §12.6: the debug panel shows the views; the trace folder and its
// limit are LibraryParts' (the host's places), none on the Web (hand-over).
TIDebuggerOptions debuggerOptions(TIDebugPart& panel) {
    TIDebuggerOptions o;
    o.handOver = true;
    o.showPanel = [&panel](sessions::SessionContext& c, const std::string& view) { panel.showView(c, view); };
    return o;
}

} // namespace

TIAppPart::TIAppPart(TIAppSetup setup)
    : m_setup(std::move(setup)),
      m_files(m_setup.paths->saveFolder(), m_setup.files),
      m_keeper(
          m_files, [this](std::string sentence) { if (m_ctx) m_ctx->notice(std::move(sentence)); },
          [this] { return m_ctx ? calcu::media::isoTime(m_ctx->hostTime()) : std::string(); }),
      m_machines([this] { return m_keeper.contentForBuild(); }),
      m_rules(TIKeywordSetup{[this](sessions::SessionContext& c, const CueCardContent& card) {
          m_ctx = &c;
          fitOwnCueCard(card);
      }}),
      m_bundle(m_setup.examplesFolder.empty() ? nullptr
                                              : std::make_unique<presets::DirectoryPresetBundle>(calcu::files::toPath(m_setup.examplesFolder))),
      m_debugPart(TIDebugSetup{m_setup.roms.set, m_setup.debugTabsShown}),
      m_fileKinds(m_debugPart),
      m_lib(librarySetup()) {
    m_debugPart.bindDebugger(&m_lib.debugger());
    m_machines.setKeptTrace([this] { return m_printerTrace; });   // C59-6 AC-6: into an attached PC-100C
}

calcu::parts::LibraryPartsSetup TIAppPart::librarySetup() {
    calcu::parts::LibraryPartsSetup s;
    s.machines = &m_machines;
    s.settings = m_setup.settings;
    s.paths = m_setup.paths;
    s.roms = m_setup.roms;
    s.traceAvailable = m_setup.traceAvailable;
    s.placesAtStart = true;   // the media's and screenshots' places as before Part E (LIB-6 E-1); traces: LIB-10 AC-53
    s.uploadFolder = m_setup.uploadFolder;
    s.typing = &typingProduct();
    s.hosting = m_setup.hosting;
    s.presetRules = &m_rules;
    s.bundle = m_bundle.get();
    s.papers = tiPrinterPapers();   // LIB-4: the PC-100C owns the paper panel
    s.debugger = tiDebuggerSetup(m_levelRules, debuggerOptions(m_debugPart));
    s.productName = "Calc-U-59";
    s.presetExtensions = tiPresetExtensions();   // LIB-13 AC-11: no size limit
    s.fileKinds = m_fileKinds.kinds();
    s.controlRules = {&m_debugPart};   // C59-4 AC-25
    // LIB-7 59 (M-55): the magnetic card in the save folder, slot-1's modules.
    s.mediaKinds = {magneticCardKind()};
    s.mediaFiles = m_files.files();   // the host's (C59-5 D-6), else the plain one
    s.moduleListing = tiModuleListing(m_setup.roms.set);
    return s;
}

TIAppPart::~TIAppPart() {
    m_session.reset();
}

std::vector<sessions::SessionPart*> TIAppPart::sessionParts(std::vector<sessions::SessionPart*> host,
                                                           std::vector<sessions::SessionPart*> after) {
    calcu::parts::SessionSlots slots;
    slots.host = std::move(host);
    slots.fileKinds = {&m_debugPart};   // the file kinds' (TIFileKinds), before the open part
    slots.product = {&m_memoryPart, this};
    slots.after = std::move(after);
    return m_lib.sessionParts(slots);
}

calcu::machine::Machine* TIAppPart::machine() const { return m_ms ? m_ms->machine() : nullptr; }

const calcu::debugger::RunControl& TIAppPart::runControl() const { return m_lib.debugger().runControl(); }

calcu::debugger::RunState TIAppPart::runState() const { return runControl().state(); }

bool TIAppPart::isFrozen() const { return runControl().paused(); }

void TIAppPart::attach(sessions::SessionSetup& s) {
    using sessions::History;
    m_ctx = &s.context();
    m_ms = &m_ctx->machineSession();
    m_led = &s.stateTopic(topics::kLed);
    m_cue = &s.stateTopic(topics::kCueCard);
    m_printer = &s.stateTopic(topics::kPrinter);
    // C59-7 D-12: a tap on the fitted card.
    add(s, commands::kTapCard, [this](const Value& a) -> CommandOutcome {
        return Done{std::make_shared<TIFlag>(tapCard(sessions::realArg(a, "x"), sessions::realArg(a, "y")))};
    });
    // The machine comes with LIB-18's `start`; the card wait is the media part's (LIB-17 AC-16).
    settle();
}

void TIAppPart::add(sessions::SessionSetup& s, const sessions::CommandName& name, Handler h) {
    s.command(name, [this, h = std::move(h)](const Value& args, sessions::SessionContext& ctx) {
        m_ctx = &ctx;
        CommandOutcome out = h(args);
        settle();
        return out;
    });
}

void TIAppPart::beforeCall(sessions::SessionContext& ctx, pacing::WallTime) {
    m_ctx = &ctx;
    settle();
}

void TIAppPart::afterCall(sessions::SessionContext& ctx, pacing::WallTime now) {
    m_ctx = &ctx;
    frame(steadyTimeOf(now));
    followPlugs(false);
    m_keeper.afterCall(machine(), m_userCueCard, now);   // C59-3 AC-5
    settle();
}

void TIAppPart::suspending(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    m_keeper.writeNow(machine(), m_userCueCard);   // C59-3 AC-6
    settle();
}

void TIAppPart::closing(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    m_keeper.writeNow(machine(), m_userCueCard);   // C59-3 AC-6
}

void TIAppPart::beforeMachineChange(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    m_keeper.writeNow(machine(), m_userCueCard);   // C59-3 AC-6: before it is replaced
}

void TIAppPart::machineChanged(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    if (!m_ms || m_ms->generation() == m_seenGeneration) return;
    m_seenGeneration = m_ms->generation();
    followMachine();
    settle();
}

void TIAppPart::operationEnded(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    // An operation run in one go (Hosting::VirtualTime, the harness) ends
    // before the session follows a machine it replaced: follow it first.
    machineChanged(ctx);
    followPlugs(false);   // an attach or a detach (LIB-18 AC-21)
    settle();
}

void TIAppPart::beforeFlush(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    // A switch set by LIB-2's set-control (TRACE, C59-6 AC-6), on the machine followed.
    if (m_ms && m_ms->generation() == m_seenGeneration) followPlugs(false);
    refreshCueCard();   // a card fitted or fed by a library command (C59-7 AC-22)
    settle();
}

namespace {
// A topic set only when the value differs from the one it holds.
template <class P> void setIfChanged(sessions::StateTopic& t, std::shared_ptr<P> p) {
    const auto* now = sessions::payloadAs<P>(t.current());
    if (!now || !now->same(*p)) t.set(std::move(p));
}
} // namespace

void TIAppPart::settle() {
    if (!m_led) return;
    auto led = std::make_shared<TILedState>();
    led->led = m_display;
    led->calcIndicatorOpacity = m_calcOpacity;
    setIfChanged(*m_led, std::move(led));
    auto cue = std::make_shared<TICueCardState>();
    cue->card = m_cueCard;
    cue->onShow = m_onShow;
    cue->litCorner = m_litCorner;
    setIfChanged(*m_cue, std::move(cue));
    auto printer = std::make_shared<TIPrinterState>();
    printer->connected = m_printerConnected;
    printer->trace = m_printerTrace;
    setIfChanged(*m_printer, std::move(printer));
}

void TIAppPart::diagnostic(const std::string& message) {
    if (m_ctx) m_ctx->log(message);
}

calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, pacing::WallClock clock,
                                                          std::optional<std::uint32_t> jitterSeed,
                                                          std::vector<sessions::SessionPart*> before,
                                                          std::vector<sessions::SessionPart*> after, bool start) {
    shell::HostSessionSetup host;
    host.clock = std::move(clock);
    host.jitterSeed = jitterSeed;
    host.before = std::move(before);
    host.after = std::move(after);
    host.start = start;
    return makeAppSession(std::move(setup), std::move(host));
}

calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, shell::HostSessionSetup host) {
    auto part = std::make_shared<TIAppPart>(std::move(setup));
    TIAppPart& p = *part;
    // The one order (parts::LibraryParts): LIB-4's ControlPart (D-13; the
    // product entries of C59-4 AC-25), the host's own and SettingsPart, the
    // library's, the product's, the host's behind them.
    host.owned = std::move(part);
    return shell::makeHostSession(p, std::move(host));
}

} // namespace cu59
