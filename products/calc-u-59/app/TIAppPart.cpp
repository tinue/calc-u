#include "TIAppPart.hpp"
#include "TIAppPartInternal.hpp"
#include "PacingProfiles.hpp"
#include "../core/TIAdapters.hpp"
#include "TIDebuggerSetup.hpp"
#include "TIPrinters.hpp"
#include "../core/TIDebugging.hpp"
#include "sessions/LibraryTopics.hpp"
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
    return Value::object({{"title", card->title}});
}
Value TIPrinterState::toValue() const { return Value::object({{"connected", connected}, {"trace", trace}}); }
Value TICardReaderState::toValue() const {
    return Value::object({{"swiping", state == CardState::Swiping},
                          {"file", fileName},
                          {"fitted", fitted},
                          {"lit-corner", litCorner ? Value(*litCorner) : Value()}});
}
Value TIRunState::toValue() const {
    return Value::object({{"full-speed", fullSpeed},
                          {"keystrokes-playing", keystrokesPlaying}});
}
Value TIDebugLiveState::toValue() const {
    return Value::object({{"enabled", enabled}, {"current-step", live.currentStep}});
}
Value TIDebugCpuState::toValue() const {
    return Value::object({{"enabled", enabled}, {"entries", static_cast<int64_t>(history.size())}});
}
Value TIDebugHeatmapState::toValue() const { return Value::object({{"cells", static_cast<int64_t>(counts.size())}}); }
Value TIDebugLogState::toValue() const {
    return Value::object({{"level", static_cast<int>(level)}, {"text", text}});
}
Value TIDebugAsmState::toValue() const {
    return Value::object({{"file", fileName}, {"words", wordCount}, {"status", status}, {"active", active}});
}
Value TIShowDebugPanel::toValue() const { return Value(static_cast<int>(panel)); }

NoticeParts splitNotice(const std::string& sentence) {
    const auto at = sentence.find(": ");
    if (at == std::string::npos) return {"Calc-U-59", sentence};
    return {sentence.substr(0, at), sentence.substr(at + 2)};
}

// ── The part ─────────────────────────────────────────────────────────────────


TIAppPart::TIAppPart(TIAppSetup setup)
    : m_setup(std::move(setup)),
      m_files(m_setup.saveFolder, m_setup.fileIO),
      m_hooks(std::make_unique<Hooks>(*this)),
      m_moduleID(m_setup.moduleID) {}

TIAppPart::~TIAppPart() {
    // Before the session: the TI-58C's memory is written while the machine
    // session still holds the machine.
    persistConstantMemory();
    if (m_typing) m_typing->engine().setObserver(nullptr);
    m_session.reset();
    m_queue.clear();
}

void TIAppPart::setTyping(calcu::typing::TypingPart* typing) { m_typing = typing; }

void TIAppPart::setLibraryParts(calcu::paper::PaperPart* paper, calcu::debugger::DebuggerPart* debugger,
                                TILevelRules* rules) {
    m_paperPart = paper;
    m_debugger = debugger;
    m_rules = rules;
}

calcu::debugger::RunControl* TIAppPart::runControl() const { return m_debugger ? &m_debugger->runControl() : nullptr; }

calcu::debugger::RunState TIAppPart::runState() const {
    return m_debugger ? m_debugger->runControl().state() : calcu::debugger::RunState{};
}

bool TIAppPart::isFrozen() const { return m_debugger && m_debugger->runControl().paused(); }

void TIAppPart::attach(sessions::SessionSetup& s) {
    using sessions::History;
    m_ctx = &s.context();
    m_ms = &m_ctx->machineSession();
    m_modelTopic = &s.stateTopic(topics::kModel);
    m_led = &s.stateTopic(topics::kLed);
    m_cue = &s.stateTopic(topics::kCueCard);
    m_printer = &s.stateTopic(topics::kPrinter);
    m_card = &s.stateTopic(topics::kCardReader);
    m_run = &s.stateTopic(topics::kRunState);
    m_live = &s.stateTopic(topics::kDebugLive);
    m_cpu = &s.stateTopic(topics::kDebugCpu);
    m_heatmap = &s.stateTopic(topics::kDebugHeatmap);
    m_log = &s.stateTopic(topics::kDebugLog);
    m_asm = &s.stateTopic(topics::kDebugAsm);
    m_panel = &s.streamTopic(topics::kShowDebugPanel, History::None);

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

    add(s, kStartMachine, [&c](const Value& a) -> CommandOutcome {
        const int64_t m = sessions::integerArg(a, "model", -1);
        if (m < 0 || m > 2) return Refused{"The model is 0 (TI-59), 1 (TI-58) or 2 (TI-58C)."};
        c.startMachine(static_cast<Model>(m));
        return Done{};
    });
    add(s, kReset, done([&c](const Value&) { c.resetMachine(); }));
    add(s, kCleanReset, done([&c](const Value&) { c.cleanResetMachine(); }));
    add(s, kPersistConstantMemory, done([&c](const Value&) { c.persistConstantMemory(); }));
    // AC-32: a power cycle runs as an operation, until the ROM is ready for keys.
    add(s, kChangeModule, [this, &c](const Value& a) -> CommandOutcome {
        const std::string before = c.moduleID();
        c.changeModule(textArg(a, "id"));
        if (c.moduleID() == before) return Done{};
        return StartOperation{untilIdle(false)};
    });
    add(s, kFullSpeed, done([&c](const Value& a) { c.setFullSpeed(sessions::flagArg(a, "on")); }));
    add(s, kPrinterConnected, [this, &c](const Value& a) -> CommandOutcome {
        const bool on = sessions::flagArg(a, "on");
        if (on == c.printerConnected()) return Done{};
        c.setPrinterConnected(on);
        return StartOperation{untilIdle(false)};
    });
    add(s, kTogglePrinterConnection, [this, &c](const Value&) -> CommandOutcome {
        c.togglePrinterConnection();
        return StartOperation{untilIdle(false)};
    });
    // P-E2: the paper panel's controls of the PC-100C.
    add(s, kSetControl, [&c](const Value& a) -> CommandOutcome {
        const std::string peripheral = textArg(a, "peripheral");
        if (!peripheral.empty() && peripheral != kPC100C.value) return Refused{"There is no peripheral " + peripheral + "."};
        if (!c.setPrinterControl(textArg(a, "control"), sessions::flagArg(a, "on")))
            return Refused{"The PC-100C has no control " + textArg(a, "control") + "."};
        return Done{};
    });
    add(s, kFitCard, [&c, flag](const Value& a) {
        return flag(c.fitCard(bytesArg(a, "bytes"), textArg(a, "path"), textArg(a, "name")));
    });
    add(s, kFitBlankCard, [&c, flag](const Value& a) {
        return flag(c.fitBlankCard(textArg(a, "path"), textArg(a, "name")));
    });
    add(s, kFeedSide, [&c, flag](const Value& a) { return flag(c.feedSide(static_cast<int>(sessions::integerArg(a, "side")))); });
    add(s, kTapCard, [&c, flag](const Value& a) { return flag(c.tapCard(realArg(a, "x"), realArg(a, "y"))); });
    add(s, kEjectCard, done([&c](const Value&) { c.ejectCard(); }));
    // AC-32: a preset runs as an operation until its KEYSTROKES have played.
    add(s, kLoadState, [this, &c](const Value& a) -> CommandOutcome {
        c.loadStateFile(bytesArg(a, "bytes"));
        adoptMachine();
        return StartOperation{untilIdle(true)};
    });
    add(s, kOpenDropped, [this, &c](const Value& a) -> CommandOutcome {
        const std::string name = textArg(a, "name");
        const bool preset = c.dropAction(name) == DropAction::LoadPreset;
        const bool ok = c.openDroppedFile(bytesArg(a, "bytes"), textArg(a, "path"), name);
        if (preset && ok) {
            adoptMachine();
            return StartOperation{untilIdle(true)};
        }
        return Done{std::make_shared<TIFlag>(ok)};
    });
    add(s, kLiveDebug, done([&c](const Value& a) { c.setLiveDebugEnabled(sessions::flagArg(a, "on")); }));
    add(s, kCpuDebug, done([&c](const Value& a) { c.setCpuDebugEnabled(sessions::flagArg(a, "on")); }));
    add(s, kClearHeatmap, done([&c](const Value&) { c.clearHeatmap(); }));
    add(s, kCycleDebugLevel, done([&c](const Value&) { c.cycleDebugLevel(); }));
    add(s, kClearLog, done([&c](const Value&) { c.clearLog(); }));
    add(s, kDumpVars, done([&c](const Value&) { c.dumpVars(); }));
    add(s, kDumpScom, done([&c](const Value&) { c.dumpScom(); }));
    add(s, kDumpProg, done([&c](const Value&) { c.dumpProg(); }));
    add(s, kDumpMemory, done([&c](const Value&) { c.dumpMemory(); }));
    add(s, kLoadAsm, done([&c](const Value& a) {
        const Value* t = a.find("text");
        std::optional<std::string> text;
        if (t && t->string()) text = *t->string();
        c.loadAsm(text, textArg(a, "name"));
    }));
    add(s, kRunAsm, done([&c](const Value&) { c.runAsm(); }));
    add(s, kClearAsm, done([&c](const Value&) { c.clearAsm(); }));
    s.request(kDropAction, [&c](const Value& a, sessions::SessionContext&) {
        sessions::ReplyMessage r;
        r.done = true;
        r.result = std::make_shared<TIDropAction>(c.dropAction(textArg(a, "name")));
        return r;
    });

    // The adapters on the machine session, then the first machine (the
    // session reads its display before it is first called). The gate is the
    // run control's (LIB-10; without a DebuggerPart the session's own).
    m_ms->addHook(*m_hooks);
    m_ms->addWaitProbe(*m_hooks);
    startMachine(m_setup.startModel);
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

sessions::Operation TIAppPart::presetOperation() {
    settle();
    return untilIdle(true);
}

sessions::Operation TIAppPart::untilIdle(bool keystrokes) {
    return [this, keystrokes](sessions::OperationContext& op) { return runUntilIdle(op, keystrokes); };
}

// A power cycle waits for the ROM's start-up, at most 10 s of emulated time;
// a preset for its KEYSTROKES, at the speed they ask for (regular unless
// FullSpeed:, as in 2.3.0; LIB-3 59 design §4).
sessions::Task<sessions::OperationResult> TIAppPart::runUntilIdle(sessions::OperationContext& op, bool keystrokes) {
    sessions::MachineSession& ms = op.session().machineSession();
    for (int pieces = 0;; ++pieces) {
        EmulatorSession* s = m_session.get();
        const machine::Machine* m = ms.machine();
        if (!s || !m) break;
        if (keystrokes ? !s->keystrokesPending() : (s->access().machine().isKeyReady() || pieces >= 600)) break;
        op.setSpeed(s->isFullSpeed() ? Speed::Full : Speed::Regular);
        const machine::Ticks piece = sessions::OperationContext::runPiece(m->descriptor().tickRate);
        sessions::MachineSession* msp = &ms;
        co_await sessions::request([msp, piece] {
            bool stopped = false;
            return msp->advance(piece, stopped);
        });
    }
    co_return sessions::OperationResult{};
}

void TIAppPart::beforeCall(sessions::SessionContext& ctx, pacing::WallTime) {
    m_ctx = &ctx;
    settle();
}

void TIAppPart::afterCall(sessions::SessionContext& ctx, pacing::WallTime now) {
    m_ctx = &ctx;
    frame(steadyTimeOf(now));
    settle();
}

void TIAppPart::suspending(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    persistConstantMemory();   // as 2.3.0's suspend (C59-5 AC-8)
    settle();
}

void TIAppPart::settle() {
    for (int i = 0; i < 10'000 && !m_queue.empty(); ++i) {
        auto job = std::move(m_queue.front());
        m_queue.pop_front();
        job();
    }
    adoptMachine();
    syncSpeed();
    followRunState();
    publish(std::exchange(m_changes, 0u));
}

void TIAppPart::adoptMachine() {
    EmulatorSession* s = m_session.get();
    if (!m_ms || !s || !s->ownsMachine()) return;
    m_hooks->last = 0;
    // LIB-10 §12.11: a pause or a step waiting to land runs on in the host
    // call that asked for it (2.3.0's freeze and STEP ran there, at most
    // 50000 weights), instead of in paced emulated time.
    if (auto* d = s->machine().debugging(); d && m_debugger) {
        calcu::debugger::RunControl* rc = &m_debugger->runControl();
        static_cast<TIDebugging*>(d)->setRunOn([rc] { return !rc->running() && !rc->paused(); });
    }
    m_ms->setMachine(s->releaseMachine(), pacingProfile(s->model()), 0);   // AC-33: no sound
}

void TIAppPart::syncSpeed() {
    EmulatorSession* s = m_session.get();
    if (!m_ctx || !s) return;
    const Speed commanded = m_ctx->commandedSpeed();
    const bool full = s->isFullSpeed();
    if (commanded != m_lastCommanded) {   // the library's `speed` command
        m_lastCommanded = commanded;
        m_lastFull = commanded == Speed::Full;
        if (full != m_lastFull) setFullSpeed(m_lastFull);
    } else if (full != m_lastFull) {      // the controller's own (c59.full-speed, KEYSTROKES)
        m_lastFull = full;
        m_ctx->setCommandedSpeed(full ? Speed::Full : Speed::Regular);
        m_lastCommanded = m_ctx->commandedSpeed();
    }
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
        m_cue->set(std::move(p));
    }
    if (c & S::PrinterChanged) {
        auto p = make(std::make_shared<TIPrinterState>());
        p->connected = k.printerConnected();
        p->trace = k.printerTrace();
        m_printer->set(std::move(p));
    }
    if (c & (S::CardChanged | S::ModelChanged | S::CueCardChanged)) {
        auto p = make(std::make_shared<TICardReaderState>());
        p->state = k.cardState();
        p->fileName = k.cardFileName();
        p->fitted = k.cardFitted();
        p->onShow = k.cardOnShow();
        p->litCorner = k.litCardCorner();
        m_card->set(std::move(p));
    }
    if (c & S::RunStateChanged) {
        auto p = make(std::make_shared<TIRunState>());
        p->fullSpeed = k.fullSpeed();
        p->keystrokesPlaying = k.keystrokesPlaying();
        m_run->set(std::move(p));
    }
    if (c & (S::LiveChanged | S::ListingChanged)) {
        auto p = make(std::make_shared<TIDebugLiveState>());
        p->enabled = k.liveDebugEnabled();
        p->live = k.debug().live();
        p->listing = k.debug().listing();
        p->listingSource = k.debug().listingSource();
        p->listingCurrentIndex = k.debug().listingCurrentIndex();
        m_live->set(std::move(p));
    }
    if (c & S::CpuChanged) {
        auto p = make(std::make_shared<TIDebugCpuState>());
        p->enabled = k.cpuDebugEnabled();
        p->history = k.debug().cpuHistory();
        m_cpu->set(std::move(p));
    }
    if (c & S::HeatmapChanged) {
        auto p = make(std::make_shared<TIDebugHeatmapState>());
        p->counts = k.debug().heatmapCounts();
        m_heatmap->set(std::move(p));
    }
    if (c & S::LogChanged) {
        auto p = make(std::make_shared<TIDebugLogState>());
        p->level = k.debugLevel();
        p->text = k.logText();
        m_log->set(std::move(p));
    }
    if (c & S::AsmChanged) {
        auto p = make(std::make_shared<TIDebugAsmState>());
        p->fileName = k.asmFileName();
        p->status = k.asmStatus();
        p->wordCount = k.asmWordCount();
        p->active = k.asmActive();
        m_asm->set(std::move(p));
    }
}

// KEYSTROKES DebugPanel: and the library's `debug-panel:` step (L-E4, until
// M-56): the overlay keeps the CPU tab (C59-4 AC-2, LIB-10 D-26).
void TIAppPart::showDebugPanel(DebugPanel panel) {
    if (m_asmActive && panel == DebugPanel::Calculator) return;
    if (m_panel) m_panel->append(std::make_shared<TIShowDebugPanel>(panel));
}

void TIAppPart::showDebugPanelNamed(const std::string& view) {
    if (view == "calculator") showDebugPanel(DebugPanel::Calculator);
    else if (view == "cpu") showDebugPanel(DebugPanel::Cpu);
    else if (view == "log") showDebugPanel(DebugPanel::Log);
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
                                                          std::vector<sessions::SessionPart*> after) {
    struct Parts {
        calcu::typing::TypingPart typing{typingProduct()};
        TILevelRules rules;
        std::unique_ptr<TIAppPart> app;
        calcu::paper::PaperPart paper{tiCatalogue(), tiPrinterPapers()};
        std::unique_ptr<calcu::debugger::DebuggerPart> debugger;
        explicit Parts(TIAppSetup s) {
            debugger = std::make_unique<calcu::debugger::DebuggerPart>(tiDebuggerSetup(rules, debuggerOptions(s)));
            app = std::make_unique<TIAppPart>(std::move(s));
            app->setTyping(&typing);
            app->setLibraryParts(&paper, debugger.get(), &rules);
        }
        // LIB-10 §12.6: the trace folder of the host, none on the Web (hand-over).
        TIDebuggerOptions debuggerOptions(const TIAppSetup& s) {
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
            o.showPanel = [this](sessions::SessionContext&, const std::string& view) { app->showDebugPanelNamed(view); };
            return o;
        }
    };
    auto parts = std::make_shared<Parts>(std::move(setup));
    TIAppPart* part = parts->app.get();
    sessions::AppSessionSetup s;
    s.clock = std::move(clock);
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    s.parts = std::move(before);
    s.parts.insert(s.parts.begin(), &parts->typing);   // LIB-5: before the product's (LIB-3 AC-12)
    s.parts.push_back(part);
    s.parts.push_back(&parts->paper);           // LIB-8 D-23
    s.parts.push_back(parts->debugger.get());   // LIB-10: after the paper
    for (sessions::SessionPart* p : after) s.parts.push_back(p);
    s.hosting = sessions::Hosting::Paced;
    s.jitterSeed = jitterSeed;
    calcu::sessions::ProductSession<TIAppPart> out;
    out.part = part;
    s.owned = std::move(parts);
    out.session = std::make_unique<sessions::AppSession>(std::move(s));
    return out;
}

} // namespace cu59
