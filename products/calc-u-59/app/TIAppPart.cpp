#include "TIAppPart.hpp"
#include "TIMagneticCardKind.hpp"
#include "PacingProfiles.hpp"
#include "Settings.hpp"
#include "TIDebuggerSetup.hpp"
#include "TIPrinters.hpp"
#include "TITyping.hpp"
#include "../core/TIDebugging.hpp"
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
using pacing::Speed;

// ── Payload values (for translating hosts and the harness's `state`) ─────────

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

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// LIB-10 §12.6: the debug panel shows the views; the trace folder and its
// limit are LibraryParts' (the host's places), none on the Web (hand-over).
TIDebuggerOptions debuggerOptions(TIDebugPart& panel) {
    TIDebuggerOptions o;
    o.handOver = true;
    o.showPanel = [&panel](sessions::SessionContext& c, const std::string& view) { panel.showView(c, view); };
    return o;
}

// C59-4 AC-12: TRACE can be used where the trace folder (the one of the
// start) can be made, or where the host keeps the traces (the Web: no trace
// folder).
std::function<bool()> traceAvailable(const TIAppSetup& s) {
    const std::optional<std::string> folder = s.paths->traceFolder();
    const std::function<bool()> available = s.traceAvailable;
    return [folder, available] {
        if (!folder || folder->empty()) return true;
        if (available && !available()) return false;
        std::error_code ec;
        std::filesystem::create_directories(pathOf(*folder), ec);
        return !ec;
    };
}

// The host's file access of the save folder as the media part's (LIB-7 D-30;
// C59-5 D-6): Swift's coordinated iCloud access, else the plain one.
class FileIOAccess final : public calcu::media::FileAccess {
public:
    explicit FileIOAccess(std::shared_ptr<FileIO> io) : io_(std::move(io)) {}
    Read read(const std::string& path) const override {
        FileIO::Read r = io_->read(path);
        if (r.unreadable) return {std::nullopt, true};
        if (!r.raw) return {};
        return {std::string(r.raw->begin(), r.raw->end()), false};
    }
    Written write(const std::string& path, const std::string& bytes) const override {
        if (io_->write(path, bytes)) return {true, {}};
        return {false, "the file cannot be written."};
    }

private:
    std::shared_ptr<FileIO> io_;
};

} // namespace

TIAppPart::TIAppPart(TIAppSetup setup)
    : m_setup(std::move(setup)),
      m_files(m_setup.paths->saveFolder(), m_setup.fileIO),
      m_mediaFiles(std::make_shared<FileIOAccess>(m_setup.fileIO ? m_setup.fileIO : plainFileIO())),
      m_keeper(m_files, [this](std::string sentence) { if (m_ctx) m_ctx->notice(std::move(sentence)); }),
      m_machines([this] { return m_keeper.contentForBuild(); }),
      m_rules(TIKeywordSetup{[this](sessions::SessionContext& c, const CueCardContent& card) {
          m_ctx = &c;
          fitOwnCueCard(card);
      }}),
      m_bundle(m_setup.examplesFolder.empty() ? nullptr
                                              : std::make_unique<presets::DirectoryPresetBundle>(pathOf(m_setup.examplesFolder))),
      m_debugPart(TIDebugSetup{m_setup.roms.set, traceAvailable(m_setup), m_setup.debugTabsShown}),
      m_fileKinds(m_debugPart),
      m_lib(librarySetup()) {
    m_debugPart.bindDebugger(&m_lib.debugger());
}

calcu::parts::LibraryPartsSetup TIAppPart::librarySetup() {
    calcu::parts::LibraryPartsSetup s;
    s.machines = &m_machines;
    s.settings = m_setup.settings;
    s.paths = m_setup.paths;
    s.roms = m_setup.roms;
    s.traceAvailable = m_setup.traceAvailable;
    s.placesAtStart = true;   // as before Part E (LIB-10, LIB-6 E-1)
    s.screenshotFolder = m_setup.screenshotFolder;
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
    s.mediaFiles = m_mediaFiles;
    s.moduleListing = tiModuleListing(m_setup.roms.set);
    return s;
}

TIAppPart::~TIAppPart() {
    m_lib.typing().engine().setObserver(nullptr);
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
    auto flag = [](bool v) -> CommandOutcome { return Done{std::make_shared<TIFlag>(v)}; };
    TIAppPart& c = *this;
    using namespace commands;
    using sessions::bytesArg;
    using sessions::realArg;
    using sessions::textArg;

    // P-E2: the paper panel's controls of the PC-100C.
    add(s, kSetControl, [&c](const Value& a) -> CommandOutcome {
        const std::string peripheral = textArg(a, "peripheral");
        if (!peripheral.empty() && peripheral != kPC100C.value) return Refused{"There is no peripheral " + peripheral + "."};
        if (!c.setPrinterControl(textArg(a, "control"), sessions::flagArg(a, "on")))
            return Refused{"The PC-100C has no control " + textArg(a, "control") + "."};
        return Done{};
    });
    add(s, kTapCard, [&c, flag](const Value& a) { return flag(c.tapCard(realArg(a, "x"), realArg(a, "y"))); });
    // The machine comes with LIB-18's `start`; the card wait is the media part's (LIB-17 AC-16).
    m_changes = ~0u;
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
    refreshCueCard();   // a card fitted or fed by a library command (C59-7 AC-22)
    settle();
}

void TIAppPart::settle() {
    syncSpeed();
    followRunState();
    publish(std::exchange(m_changes, 0u));
}

void TIAppPart::syncSpeed() {
    if (!m_ctx) return;
    const bool full = m_ctx->commandedSpeed() == Speed::Full;
    const bool runs = m_ctx->busy();
    if (full != m_fullSpeed || runs != m_operationRuns) {
        m_fullSpeed = full;
        m_operationRuns = runs;
        emitChanged(RunStateChanged);
    }
}

void TIAppPart::publish(uint32_t c) {
    if (!c || !m_led) return;
    if (m_observer) m_observer(c);
    using S = TIAppPart;
    const TIAppPart& k = *this;
    auto make = [this](auto p) {
        p->serial = ++m_serial;
        return p;
    };
    if (c & S::DisplayChanged) {
        auto p = make(std::make_shared<TILedState>());
        p->led = k.display();
        p->calcIndicatorOpacity = k.calcIndicatorOpacity();
        m_led->set(std::move(p));
    }
    if (c & (S::CueCardChanged | S::CardChanged | S::ModelChanged)) {
        auto p = make(std::make_shared<TICueCardState>());
        p->card = k.cueCard();
        p->onShow = k.cardOnShow();
        p->litCorner = k.litCardCorner();
        m_cue->set(std::move(p));
    }
    if (c & S::PrinterChanged) {
        auto p = make(std::make_shared<TIPrinterState>());
        p->connected = k.printerConnected();
        p->trace = k.printerTrace();
        m_printer->set(std::move(p));
    }
}

void TIAppPart::error(const std::string& title, const std::string& message) {
    if (m_ctx) m_ctx->notice(title + ": " + message);
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
