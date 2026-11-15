#include "TIAppPart.hpp"
#include "PacingProfiles.hpp"
#include "../core/TIAdapters.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Operation.hpp"

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
Value TIPrinterState::toValue() const {
    Value::Array l;
    for (const auto& s : lines) l.push_back(Value(s));
    return Value::object({{"connected", connected}, {"trace", trace}, {"lines", Value(std::move(l))}});
}
Value TICardReaderState::toValue() const {
    return Value::object({{"swiping", state == SessionController::CardState::Swiping},
                          {"file", fileName},
                          {"fitted", fitted},
                          {"lit-corner", litCorner ? Value(*litCorner) : Value()}});
}
Value TIRunState::toValue() const {
    return Value::object({{"frozen", debug.frozen},
                          {"full-speed", fullSpeed},
                          {"keystrokes-playing", keystrokesPlaying},
                          {"held-key", heldKey}});
}
Value TITraceState::toValue() const { return Value::object({{"manual-trace", manualTrace}}); }
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

// ── Wall time ────────────────────────────────────────────────────────────────

pacing::WallTime steadyWallTime() {
    return std::chrono::duration_cast<pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
}

std::chrono::steady_clock::time_point steadyTimeOf(pacing::WallTime w) {
    return std::chrono::steady_clock::time_point(std::chrono::duration_cast<std::chrono::steady_clock::duration>(w));
}

// ── The machine session's adapters (LIB-3 59 design §5) ──────────────────────

struct TIAppPart::Hooks final : sessions::SliceHook, sessions::AdvanceGate, sessions::WaitProbe {
    TIAppPart& part;
    machine::Ticks last = 0;
    explicit Hooks(TIAppPart& p) : part(p) {}

    // TIKeySequencerHook: the KeySequencer at each slice end, with the wall
    // time the slice was due at (LIB-17 adapter SliceRequest::due).
    machine::Ticks nextDue(machine::Ticks now) const override { return now > last ? now : sessions::kNever; }
    void act(machine::Machine&, machine::Ticks now, pacing::WallTime due) override {
        last = now;
        if (EmulatorSession* s = part.m_c->session()) s->sliceEnded(steadyTimeOf(due));
    }
    // TIFreezeGate: the freeze and breakpoint checks inside the advance.
    machine::Ticks advance(machine::Machine&, machine::Ticks atLeast, bool& stoppedEarly) override {
        EmulatorSession* s = part.m_c->session();
        if (!s) {
            stoppedEarly = true;
            return 0;
        }
        return s->advanceHosted(atLeast, stoppedEarly);
    }
    // TICardWaitAdapter: the TI-59 needs a card now (LIB-17 AC-16).
    bool waitsForMedium(const machine::Machine& m) const override {
        const auto* r = cardReaderOf(const_cast<machine::Machine&>(m));
        return r && r->isInCardWait() && r->isWaitingForCard();   // the idle loop polls the switch too
    }
};

// ── The part ─────────────────────────────────────────────────────────────────

TIAppPart::TIAppPart(TIAppSetup setup)
    : m_setup(std::move(setup)), m_files(m_setup.saveFolder, m_setup.fileIO), m_hooks(std::make_unique<Hooks>(*this)) {
    m_c = std::make_unique<SessionController>(static_cast<SessionController::Platform&>(*this), m_setup.roms,
                                              m_setup.cuecards, m_setup.moduleID, Threading::Hosted);
}

TIAppPart::~TIAppPart() {
    m_c.reset();   // persists the TI-58C's memory, while the machine session still holds the machine
}

TIAppSession::~TIAppSession() {
    part.reset();
    session.reset();
}

void TIAppPart::start(sessions::AppSession& s) {
    m_ctx = &s.context();
    m_ms = &s.machineSession();
    m_ms->setGate(m_hooks.get());
    m_ms->addHook(*m_hooks);
    m_ms->addWaitProbe(*m_hooks);
    m_c->startMachine(m_setup.startModel);
    m_changes = ~0u;
    settle();
    s.advanceUnpaced(0);   // the machine joins: the display read, everything flushed
}

void TIAppPart::attach(sessions::SessionSetup& s) {
    using sessions::History;
    m_model = &s.stateTopic(topics::kModel);
    m_led = &s.stateTopic(topics::kLed);
    m_cue = &s.stateTopic(topics::kCueCard);
    m_printer = &s.stateTopic(topics::kPrinter);
    m_card = &s.stateTopic(topics::kCardReader);
    m_run = &s.stateTopic(topics::kRunState);
    m_trace = &s.stateTopic(topics::kTrace);
    m_live = &s.stateTopic(topics::kDebugLive);
    m_cpu = &s.stateTopic(topics::kDebugCpu);
    m_heatmap = &s.stateTopic(topics::kDebugHeatmap);
    m_log = &s.stateTopic(topics::kDebugLog);
    m_asm = &s.stateTopic(topics::kDebugAsm);
    m_panel = &s.streamTopic(topics::kShowDebugPanel, History::None);

    auto boolArg = [](const Value& a, const char* key) {
        const Value* v = a.find(key);
        return v && v->boolean() && *v->boolean();
    };
    auto intArg = [](const Value& a, const char* key, int64_t fallback = 0) {
        const Value* v = a.find(key);
        return v && v->integer() ? *v->integer() : fallback;
    };
    auto realArg = [](const Value& a, const char* key) {
        const Value* v = a.find(key);
        if (v && v->real()) return *v->real();
        if (v && v->integer()) return static_cast<double>(*v->integer());
        return 0.0;
    };
    auto strArg = [](const Value& a, const char* key) {
        const Value* v = a.find(key);
        return v && v->string() ? *v->string() : std::string();
    };
    auto bytesArg = [](const Value& a, const char* key) -> std::optional<std::vector<uint8_t>> {
        const Value* v = a.find(key);
        if (v && v->bytes()) return *v->bytes();
        return std::nullopt;
    };
    auto done = [](auto&& f) {
        return [f](const Value& a) -> CommandOutcome {
            f(a);
            return Done{};
        };
    };
    auto flag = [](bool v) -> CommandOutcome { return Done{std::make_shared<TIFlag>(v)}; };
    SessionController& c = *m_c;
    using namespace commands;

    add(s, kStartMachine, [&c, intArg](const Value& a) -> CommandOutcome {
        const int64_t m = intArg(a, "model", -1);
        if (m < 0 || m > 2) return Refused{"The model is 0 (TI-59), 1 (TI-58) or 2 (TI-58C)."};
        c.startMachine(static_cast<Model>(m));
        return Done{};
    });
    add(s, kReset, done([&c](const Value&) { c.resetMachine(); }));
    add(s, kCleanReset, done([&c](const Value&) { c.cleanResetMachine(); }));
    add(s, kPersistConstantMemory, done([&c](const Value&) { c.persistConstantMemory(); }));
    // AC-32: a power cycle runs as an operation, until the ROM is ready for keys.
    add(s, kChangeModule, [this, &c, strArg](const Value& a) -> CommandOutcome {
        const std::string before = c.moduleID();
        c.changeModule(strArg(a, "id"));
        if (c.moduleID() == before) return Done{};
        return StartOperation{untilIdle(false)};
    });
    add(s, kPressKey, done([&c, intArg](const Value& a) { c.pressKey(static_cast<uint8_t>(intArg(a, "code"))); }));
    add(s, kReleaseKey, done([&c, intArg](const Value& a) { c.releaseKey(static_cast<uint8_t>(intArg(a, "code"))); }));
    add(s, kKeyboardDown, done([&c, bytesArg](const Value& a) { c.keyboardDown(bytesArg(a, "codes").value_or(std::vector<uint8_t>{})); }));
    add(s, kKeyboardUp, done([&c](const Value&) { c.keyboardUp(); }));
    add(s, kKeyboardRelease, done([&c](const Value&) { c.keyboardRelease(); }));
    add(s, kFullSpeed, done([&c, boolArg](const Value& a) { c.setFullSpeed(boolArg(a, "on")); }));
    add(s, kPrinterConnected, [this, &c, boolArg](const Value& a) -> CommandOutcome {
        const bool on = boolArg(a, "on");
        if (on == c.printerConnected()) return Done{};
        c.setPrinterConnected(on);
        return StartOperation{untilIdle(false)};
    });
    add(s, kTogglePrinterConnection, [this, &c](const Value&) -> CommandOutcome {
        c.togglePrinterConnection();
        return StartOperation{untilIdle(false)};
    });
    add(s, kTogglePrinterTrace, done([&c](const Value&) { c.togglePrinterTrace(); }));
    add(s, kPrinterPrint, done([&c, boolArg](const Value& a) { c.pressPrinterPrint(boolArg(a, "pressed")); }));
    add(s, kPrinterAdv, done([&c, boolArg](const Value& a) { c.pressPrinterAdv(boolArg(a, "pressed")); }));
    add(s, kCutPaper, done([&c](const Value&) { c.cutPaper(); }));
    add(s, kFitCard, [&c, bytesArg, strArg, flag](const Value& a) {
        return flag(c.fitCard(bytesArg(a, "bytes"), strArg(a, "path"), strArg(a, "name")));
    });
    add(s, kFitBlankCard, [&c, strArg, flag](const Value& a) {
        return flag(c.fitBlankCard(strArg(a, "path"), strArg(a, "name")));
    });
    add(s, kFeedSide, [&c, intArg, flag](const Value& a) { return flag(c.feedSide(static_cast<int>(intArg(a, "side")))); });
    add(s, kTapCard, [&c, realArg, flag](const Value& a) { return flag(c.tapCard(realArg(a, "x"), realArg(a, "y"))); });
    add(s, kEjectCard, done([&c](const Value&) { c.ejectCard(); }));
    // AC-32: a preset runs as an operation until its KEYSTROKES have played.
    add(s, kLoadState, [this, &c, bytesArg](const Value& a) -> CommandOutcome {
        c.loadStateFile(bytesArg(a, "bytes"));
        adoptMachine();
        return StartOperation{untilIdle(true)};
    });
    add(s, kOpenDropped, [this, &c, bytesArg, strArg](const Value& a) -> CommandOutcome {
        const std::string name = strArg(a, "name");
        const bool preset = c.dropAction(name) == SessionController::DropAction::LoadPreset;
        const bool ok = c.openDroppedFile(bytesArg(a, "bytes"), strArg(a, "path"), name);
        if (preset && ok) {
            adoptMachine();
            return StartOperation{untilIdle(true)};
        }
        return Done{std::make_shared<TIFlag>(ok)};
    });
    add(s, kLiveDebug, done([&c, boolArg](const Value& a) { c.setLiveDebugEnabled(boolArg(a, "on")); }));
    add(s, kCpuDebug, done([&c, boolArg](const Value& a) { c.setCpuDebugEnabled(boolArg(a, "on")); }));
    add(s, kFreeze, done([&c, intArg](const Value& a) {
        c.freeze(intArg(a, "owner") == 1 ? FreezeOwner::Calculator : FreezeOwner::Cpu);
    }));
    add(s, kResume, done([&c](const Value&) { c.resume(); }));
    add(s, kArmPcChange, done([&c](const Value&) { c.armPcChange(); }));
    add(s, kArmScanLoopExit, done([&c](const Value&) { c.armScanLoopExit(); }));
    add(s, kDisarmPcChange, done([&c](const Value&) { c.disarmPcChange(); }));
    add(s, kDisarmScanLoopExit, done([&c](const Value&) { c.disarmScanLoopExit(); }));
    add(s, kStepKeycode, done([&c](const Value&) { c.stepKeycode(); }));
    add(s, kStepInstruction, done([&c](const Value&) { c.stepInstruction(); }));
    add(s, kClearHeatmap, done([&c](const Value&) { c.clearHeatmap(); }));
    add(s, kCycleDebugLevel, done([&c](const Value&) { c.cycleDebugLevel(); }));
    add(s, kClearLog, done([&c](const Value&) { c.clearLog(); }));
    add(s, kDumpVars, done([&c](const Value&) { c.dumpVars(); }));
    add(s, kDumpScom, done([&c](const Value&) { c.dumpScom(); }));
    add(s, kDumpProg, done([&c](const Value&) { c.dumpProg(); }));
    add(s, kDumpMemory, done([&c](const Value&) { c.dumpMemory(); }));
    add(s, kManualTrace, done([&c, boolArg](const Value& a) { c.setManualTrace(boolArg(a, "on")); }));
    add(s, kLoadAsm, done([&c, strArg](const Value& a) {
        const Value* t = a.find("text");
        std::optional<std::string> text;
        if (t && t->string()) text = *t->string();
        c.loadAsm(text, strArg(a, "name"));
    }));
    add(s, kRunAsm, done([&c](const Value&) { c.runAsm(); }));
    add(s, kClearAsm, done([&c](const Value&) { c.clearAsm(); }));
    s.request(kDropAction, [&c, strArg](const Value& a, sessions::SessionContext&) {
        sessions::ReplyMessage r;
        r.done = true;
        r.result = std::make_shared<TIDropAction>(c.dropAction(strArg(a, "name")));
        return r;
    });
}

void TIAppPart::add(sessions::SessionSetup& s, const sessions::CommandName& name, Handler h) {
    s.command(name, [this, h = std::move(h)](const Value& args, sessions::SessionContext& ctx) {
        m_ctx = &ctx;
        CommandOutcome out = h(args);
        settle();
        return out;
    });
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
        EmulatorSession* s = m_c->session();
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
    m_c->tick(steadyTimeOf(now));
    settle();
}

void TIAppPart::suspending(sessions::SessionContext& ctx) {
    m_ctx = &ctx;
    m_c->persistConstantMemory();   // as SessionController::suspend (C59-5 AC-8)
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
    publish(std::exchange(m_changes, 0u));
    if (m_ms && m_c->session()) m_ms->setPaused(!m_c->session()->isRunning());
}

void TIAppPart::adoptMachine() {
    EmulatorSession* s = m_c->session();
    if (!m_ms || !s || !s->ownsMachine()) return;
    m_hooks->last = 0;
    m_ms->setMachine(s->releaseMachine(), pacingProfile(s->model()), 0);   // AC-33: no sound
}

void TIAppPart::syncSpeed() {
    EmulatorSession* s = m_c->session();
    if (!m_ctx || !s) return;
    const Speed commanded = m_ctx->commandedSpeed();
    const bool full = s->isFullSpeed();
    if (commanded != m_lastCommanded) {   // the library's `speed` command
        m_lastCommanded = commanded;
        m_lastFull = commanded == Speed::Full;
        if (full != m_lastFull) m_c->setFullSpeed(m_lastFull);
    } else if (full != m_lastFull) {      // the controller's own (c59.full-speed, KEYSTROKES)
        m_lastFull = full;
        m_ctx->setCommandedSpeed(full ? Speed::Full : Speed::Regular);
        m_lastCommanded = m_ctx->commandedSpeed();
    }
}

void TIAppPart::publish(uint32_t c) {
    if (!c || !m_model) return;
    using S = SessionController;
    const SessionController& k = *m_c;
    auto make = [this](auto p) {
        p->serial = ++m_serial;
        return p;
    };
    if (c & S::ModelChanged) {
        auto p = make(std::make_shared<TIModelState>());
        p->model = k.model();
        p->moduleID = k.moduleID();
        p->modules = k.allModules();
        m_model->set(std::move(p));
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
    if (c & (S::PrinterChanged | S::PaperCut)) {
        auto p = make(std::make_shared<TIPrinterState>());
        p->lines = k.printerLines();
        p->codeLines = k.printerCodeLines();
        p->stripHeightUnits = k.printerStripHeightUnits();
        p->connected = k.printerConnected();
        p->trace = k.printerTrace();
        m_printer->set(std::move(p));
    }
    if (c & (S::CardChanged | S::ModelChanged | S::CueCardChanged)) {
        auto p = make(std::make_shared<TICardReaderState>());
        p->state = k.cardState();
        p->fileName = k.cardFileName();
        p->fitted = k.cardFitted();
        p->litCorner = k.litCardCorner();
        m_card->set(std::move(p));
    }
    if (c & S::RunStateChanged) {
        auto p = make(std::make_shared<TIRunState>());
        p->debug = k.debugState();
        p->fullSpeed = k.fullSpeed();
        p->keystrokesPlaying = k.keystrokesPlaying();
        p->heldKey = k.heldKey();
        m_run->set(std::move(p));
    }
    if (c & S::TraceChanged) {
        auto p = make(std::make_shared<TITraceState>());
        p->manualTrace = k.manualTrace();
        m_trace->set(std::move(p));
    }
    if (c & (S::LiveChanged | S::ListingChanged)) {
        auto p = make(std::make_shared<TIDebugLiveState>());
        p->enabled = k.liveDebugEnabled();
        p->live = k.debug().live();
        p->listing = k.debug().listing();
        p->listingSource = k.debug().listingSource();
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

SessionController::Platform::CardRead TIAppPart::readCard(const std::string& path) {
    FileIO::Read r = m_files.readCard(path);
    return {std::move(r.raw), r.unreadable};
}

void TIAppPart::showDebugPanel(DebugPanel panel) {
    if (m_panel) m_panel->append(std::make_shared<TIShowDebugPanel>(panel));
}

void TIAppPart::error(const std::string& title, const std::string& message) {
    if (m_ctx) m_ctx->notice(title + ": " + message);
}

void TIAppPart::diagnostic(const std::string& message) {
    if (m_ctx) m_ctx->log(message);
}

std::unique_ptr<TIAppSession> makeAppSession(TIAppSetup setup, pacing::WallClock clock,
                                             std::optional<std::uint32_t> jitterSeed,
                                             std::vector<sessions::SessionPart*> before) {
    auto out = std::make_unique<TIAppSession>();
    out->part = std::make_unique<TIAppPart>(std::move(setup));
    sessions::AppSessionSetup s;
    s.clock = std::move(clock);
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    s.parts = std::move(before);
    s.parts.push_back(out->part.get());
    s.hosting = sessions::Hosting::Paced;
    s.jitterSeed = jitterSeed;
    out->session = std::make_unique<sessions::AppSession>(std::move(s));
    out->part->start(*out->session);
    return out;
}

} // namespace cu59
