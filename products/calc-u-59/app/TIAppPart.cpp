#include "TIAppPart.hpp"
#include "TIMagneticCardKind.hpp"
#include "PacingProfiles.hpp"
#include "Settings.hpp"
#include "../core/TIAdapters.hpp"
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

Value TIModelState::toValue() const {
    return Value::object({{"model", static_cast<int>(model)}, {"module", moduleID}});
}
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
Value TIRunState::toValue() const {
    return Value::object({{"full-speed", fullSpeed},
                          {"keystrokes-playing", keystrokesPlaying}});
}
NoticeParts splitNotice(const std::string& sentence) {
    const auto at = sentence.find(": ");
    if (at == std::string::npos) return {"Calc-U-59", sentence};
    return {sentence.substr(0, at), sentence.substr(at + 2)};
}

// ── The part ─────────────────────────────────────────────────────────────────

namespace {

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// LIB-10 §12.6: the trace folder of the host, none on the Web (hand-over).
TIDebuggerOptions debuggerOptions(const TIAppSetup& s, TIDebugPart& panel) {
    TIDebuggerOptions o;
    const TraceTarget t = s.trace;
    const std::function<bool()> available = s.traceAvailable;
    if (!t.dir.empty()) {
        o.traceFolder = [t, available]() -> std::optional<calcu::settings::Paths::Ensured> {
            if (available && !available()) return calcu::settings::Paths::Ensured{false, t.dir.string(), "The trace location is not available."};
            std::error_code ec;
            std::filesystem::create_directories(t.dir, ec);
            if (ec) return calcu::settings::Paths::Ensured{false, t.dir.string(), ec.message()};
            const std::u8string u = t.dir.u8string();
            return calcu::settings::Paths::Ensured{true, std::string(u.begin(), u.end()), {}};
        };
    }
    if (t.maxBytes > 0) o.traceLimitMb = [mb = static_cast<long>(std::max<uint64_t>(1, t.maxBytes / 1'000'000))] { return mb; };
    o.handOver = t.dir.empty();
    o.showPanel = [&panel](sessions::SessionContext& c, const std::string& view) { panel.showView(c, view); };
    return o;
}

// C59-4 AC-12: TRACE can be used where the trace folder can be written, or
// where the host keeps the traces (the Web).
std::function<bool()> traceAvailable(const TIAppSetup& s) {
    const TraceTarget t = s.trace;
    const std::function<bool()> available = s.traceAvailable;
    return [t, available] {
        if (t.dir.empty()) return true;
        if (available && !available()) return false;
        std::error_code ec;
        std::filesystem::create_directories(t.dir, ec);
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
      m_files(m_setup.saveFolder, m_setup.fileIO),
      m_mediaFiles(std::make_shared<FileIOAccess>(m_setup.fileIO ? m_setup.fileIO : plainFileIO())),
      m_keeper(m_files, [this](std::string sentence) { if (m_ctx) m_ctx->notice(std::move(sentence)); }),
      m_machines([this] { return m_keeper.contentForBuild(); }),
      m_rules(TIKeywordSetup{[this](sessions::SessionContext& c, const CueCardContent& card) {
          m_ctx = &c;
          fitOwnCueCard(card);
      }}),
      m_bundle(m_setup.examplesFolder.empty() ? nullptr
                                              : std::make_unique<presets::DirectoryPresetBundle>(pathOf(m_setup.examplesFolder))),
      m_debugPart(TIDebugSetup{m_setup.roms, traceAvailable(m_setup), false}),
      m_fileKinds(m_debugPart),
      m_lib(librarySetup()) {
    m_debugPart.bindDebugger(m_lib.debugger());
}

calcu::parts::LibraryPartsSetup TIAppPart::librarySetup() {
    if (!m_setup.settings) {
        m_ownSettings.emplace(calcu::settings::SettingsFile::defaultsOnly(settings::settingsDecl()));
        m_setup.settings = &*m_ownSettings;
    }
    calcu::settings::HostDirs dirs;
    dirs.frontend = calcu::settings::FrontendKind::Swift;   // the save folder as the host passed it (LIB-9 AC-14)
    dirs.saveFolder = m_setup.saveFolder;
    m_paths = std::make_unique<calcu::settings::Paths>(*m_setup.settings, dirs);
    calcu::parts::LibraryPartsSetup s;
    s.catalogue = &tiCatalogue();
    s.machines = &m_machines;
    s.settings = m_setup.settings;
    s.roms = m_setup.roms;
    if (!s.roms) s.romsMissing = m_setup.romsMissing.empty() ? std::string("The ROM files are missing.") : m_setup.romsMissing;
    s.typing = &typingProduct();
    s.hosting = m_setup.hosting;
    s.presetRules = &m_rules;
    s.bundle = m_bundle.get();
    s.home = m_setup.home;
    s.screenshotFolder = [this]() -> std::optional<std::string> {
        // LIB-6 AC-25: the save folder, made when a step needs it.
        std::optional<std::string> folder = m_setup.screenshotFolder ? m_setup.screenshotFolder()
                                                                     : std::optional<std::string>(m_setup.saveFolder);
        if (!folder || folder->empty()) return std::nullopt;
        std::error_code ec;
        std::filesystem::create_directories(pathOf(*folder), ec);
        return folder;
    };
    s.papers = tiPrinterPapers();
    s.debugger = tiDebuggerSetup(m_levelRules, debuggerOptions(m_setup, m_debugPart));
    s.productName = "Calc-U-59";
    s.presetExtensions = tiPresetExtensions();   // LIB-13 AC-11: no size limit
    s.fileKinds = m_fileKinds.kinds();
    s.controlRules = {&m_debugPart};   // C59-4 AC-25
    s.printers = {kPC100C};
    // LIB-7 59 (M-55): the magnetic card in the save folder, slot-1's modules.
    s.mediaKinds = {magneticCardKind()};
    s.mediaPaths = m_paths.get();
    s.mediaFiles = m_mediaFiles;
    s.moduleListing = tiModuleListing(m_setup.roms);
    return s;
}

TIAppPart::~TIAppPart() {
    if (auto* t = m_lib.typing()) t->engine().setObserver(nullptr);
    m_session.reset();
}

std::vector<sessions::SessionPart*> TIAppPart::sessionParts() {
    std::vector<sessions::SessionPart*> parts = m_lib.sessionParts();
    parts.push_back(&m_memoryPart);
    parts.push_back(&m_debugPart);
    parts.push_back(this);
    return parts;
}

calcu::machine::Machine* TIAppPart::machine() const { return m_ms ? m_ms->machine() : nullptr; }

calcu::debugger::RunControl* TIAppPart::runControl() const {
    auto* d = const_cast<TIAppPart*>(this)->m_lib.debugger();
    return d ? &d->runControl() : nullptr;
}

calcu::debugger::RunState TIAppPart::runState() const {
    auto* rc = runControl();
    return rc ? rc->state() : calcu::debugger::RunState{};
}

bool TIAppPart::isFrozen() const {
    auto* rc = runControl();
    return rc && rc->paused();
}

void TIAppPart::attach(sessions::SessionSetup& s) {
    using sessions::History;
    m_ctx = &s.context();
    m_ms = &m_ctx->machineSession();
    m_modelTopic = &s.stateTopic(topics::kModel);
    m_led = &s.stateTopic(topics::kLed);
    m_cue = &s.stateTopic(topics::kCueCard);
    m_printer = &s.stateTopic(topics::kPrinter);
    m_run = &s.stateTopic(topics::kRunState);
    // LIB-12 AC-5, LIB-14 AC-19: without the full set no machine is built;
    // the frontends show the files.
    if (!m_setup.roms)
        s.stateTopic(sessions::TopicName{"roms-missing"})
            .set(sessions::valuePayload("roms-missing",
                                        Value::object({{"text", m_setup.romsMissing.empty() ? std::string("The ROM files are missing.")
                                                                                            : m_setup.romsMissing}})));

    auto done = [](auto&& f) {
        return [f](const Value& a) -> CommandOutcome {
            f(a);
            return Done{};
        };
    };
    auto flag = [](bool v) -> CommandOutcome { return Done{std::make_shared<TIFlag>(v)}; };
    TIAppPart& c = *this;
    using namespace commands;
    using sessions::bytesArg;
    using sessions::realArg;
    using sessions::textArg;

    add(s, kFullSpeed, done([&c](const Value& a) { c.setFullSpeed(sessions::flagArg(a, "on")); }));
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

void TIAppPart::setFullSpeed(bool on) {
    if (m_ctx) m_ctx->setCommandedSpeed(on ? Speed::Full : Speed::Regular);
    syncSpeed();
}

void TIAppPart::publish(uint32_t c) {
    if (!c || !m_modelTopic) return;
    if (m_observer) m_observer(c);
    using S = TIAppPart;
    const TIAppPart& k = *this;
    auto make = [this](auto p) {
        p->serial = ++m_serial;
        return p;
    };
    if (c & S::ModelChanged) {
        auto p = make(std::make_shared<TIModelState>());
        p->model = k.model();
        p->moduleID = k.moduleID();
        p->modules = k.allModules();
        m_modelTopic->set(std::move(p));
    }
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
    if (c & S::RunStateChanged) {
        auto p = make(std::make_shared<TIRunState>());
        p->fullSpeed = k.fullSpeed();
        p->keystrokesPlaying = k.keystrokesPlaying();
        m_run->set(std::move(p));
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
    auto part = std::make_shared<TIAppPart>(std::move(setup));
    sessions::AppSessionSetup s;
    s.clock = std::move(clock);
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    std::vector<sessions::SessionPart*> lib = part->sessionParts();
    // LIB-5's TypingPart first (LIB-3 AC-12: before the product's), LIB-4's
    // ControlPart (D-13; the product entries of C59-4 AC-25), then the host's own.
    s.parts.push_back(lib.front());
    s.parts.push_back(&part->libraryParts().controlPart());
    for (sessions::SessionPart* p : before) s.parts.push_back(p);
    s.parts.insert(s.parts.end(), lib.begin() + 1, lib.end());
    for (sessions::SessionPart* p : after) s.parts.push_back(p);
    s.hosting = sessions::Hosting::Paced;
    s.jitterSeed = jitterSeed;
    calcu::sessions::ProductSession<TIAppPart> out;
    out.part = part.get();
    s.owned = std::move(part);
    out.session = std::make_unique<sessions::AppSession>(std::move(s));
    if (start) {
        // LIB-18: the start, once (the startup model with its startup preset
        // or its built-in default), sent by the factory for the native hosts.
        sessions::Listener l;
        l.listen(*out.session);
        l.send(sessions::kStartCommand, Value::object({}));
    }
    return out;
}

} // namespace cu59
