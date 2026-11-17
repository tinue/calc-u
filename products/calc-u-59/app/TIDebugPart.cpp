#include "TIDebugPart.hpp"
#include "AsmOverlay.hpp"
#include "DebugLog.hpp"
#include "DebugText.hpp"
#include "ModuleCatalog.hpp"
#include "ProductRoms.hpp"
#include "RomHeatmap.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TIDebugKinds.hpp"
#include "debugger/Levels.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include <chrono>
#include <cstdio>
#include <utility>

namespace cu59 {

using namespace calcu;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Refused;
using sessions::Value;

namespace {

constexpr const char* kNoFile = "No file selected";
constexpr const char* kFirstStatus = "Load a hex opcode file and press Run.";

std::string hexColor(uint32_t rgb) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%06x", rgb & 0xFFFFFFu);
    return buf;
}

Value linesValue(const std::vector<std::string>& lines) {
    Value::Array a;
    for (const auto& l : lines) a.emplace_back(l);
    return Value(std::move(a));
}

std::string joined(const std::vector<std::string>& lines) {
    std::string t;
    for (const auto& l : lines) t += (t.empty() ? "" : "\n") + l;
    return t;
}

const machine::PlacePath kSlot{{kTIModulePlace}};

} // namespace

std::string_view debugPanelName(DebugPanel p) {
    switch (p) {
    case DebugPanel::Calculator: return "calculator";
    case DebugPanel::Cpu: return "cpu";
    case DebugPanel::Log: return "log";
    }
    return "calculator";
}
std::optional<DebugPanel> debugPanelNamed(std::string_view n) {
    if (n == "calculator") return DebugPanel::Calculator;
    if (n == "cpu") return DebugPanel::Cpu;
    if (n == "log") return DebugPanel::Log;
    return std::nullopt;
}
std::string_view debugLevelName(DebugLevel l) {
    switch (l) {
    case DebugLevel::Off: return "off";
    case DebugLevel::Info: return "info";
    case DebugLevel::Debug: return "debug";
    }
    return "off";
}
std::optional<DebugLevel> debugLevelNamed(std::string_view n) {
    if (n == "off") return DebugLevel::Off;
    if (n == "info") return DebugLevel::Info;
    if (n == "debug") return DebugLevel::Debug;
    return std::nullopt;
}

FreezeButtons freezeButtons(const debugger::LevelName& level, const debugger::RunState* s) {
    FreezeButtons b;
    if (!s) return b;
    b.armed = s->armed == level;
    b.freeze = !s->paused && !b.armed;
    b.freezeOnStart = !s->paused && !b.armed;
    return b;
}

// ── The payloads' values: the forms the Web page draws (D-7) ────────────────

Value TICalculatorDebug::toValue() const {
    const LiveDebugSnapshot& s = live;
    char buf[64];
    Value::Object o;
    std::snprintf(buf, sizeof buf, "FA:%04X", s.fA);
    o.push_back({"fA", Value(std::string(buf))});
    std::snprintf(buf, sizeof buf, "FB:%04X", s.fB);
    o.push_back({"fB", Value(std::string(buf))});
    o.push_back({"hir0", Value("HIR0:" + s.statusRegister)});
    o.push_back({"partition", Value(partitionLine(s.programRegCount, s.dataRegCount))});
    o.push_back({"source", Value(programSourceLabel(s.programSource, s.prSourceFlag))});
    Value::Array steps;
    for (const auto& e : s.programWindow) steps.push_back(Value::object({{"text", programStepLine(e, false)}, {"current", e.isCurrent}}));
    o.push_back({"steps", Value(std::move(steps))});
    o.push_back({"stepHighlight", Value(hexColor(stepHighlightColor(s.programSource)))});
    o.push_back({"currentStep", Value(s.currentStep)});
    o.push_back({"fix", Value("FIX:" + s.fixIndicator)});
    o.push_back({"angle", Value(angleModeLabel(s.angleMode))});
    o.push_back({"angleKnown", Value(s.angleMode.has_value())});
    o.push_back({"eng", Value(!s.engIndicator.empty())});
    o.push_back({"second", Value(!s.secondIndicator.empty())});
    o.push_back({"inv", Value(!s.invIndicator.empty())});
    Value::Array returns;
    for (int i = 0; i < 6; ++i) {
        const int a = s.returnAddresses[static_cast<size_t>(i)];
        returns.push_back(Value::object({{"text", returnLevelLine(i + 1, a)},
                                         {"color", hexColor(returnLevelColor(s.returnAddressSourceFlags[static_cast<size_t>(i)], a == 0))}}));
    }
    o.push_back({"returns", Value(std::move(returns))});
    Value::Array flags;
    for (int i = 0; i < 10; ++i) flags.emplace_back(static_cast<bool>(s.calcFlags[static_cast<size_t>(i)]));
    o.push_back({"flags", Value(std::move(flags))});
    Value::Array hir;
    for (int i = 0; i < 8; ++i)
        hir.push_back(Value::object({{"text", hirLine(i + 1, s.hir[static_cast<size_t>(i)])}, {"zero", s.hir[static_cast<size_t>(i)] == 0}}));
    o.push_back({"hir", Value(std::move(hir))});
    o.push_back({"x", Value(xRegisterLine(s.xValue, s.xRaw))});
    o.push_back({"xColor", Value(hexColor(xRegisterColor(s.xValue, s.xRaw)))});
    o.push_back({"t", Value(tRegisterLine(s.tRegister))});
    o.push_back({"tZero", Value(s.tRegister == 0)});
    Value::Array vars;
    for (const auto& r : s.nonZeroRegs) vars.emplace_back(registerLine(r));
    o.push_back({"vars", Value(std::move(vars))});
    Value::Array scom;
    for (const auto& row : s.scomRows) scom.emplace_back(row);
    o.push_back({"scom", Value(std::move(scom))});
    return Value(std::move(o));
}

Value TIProgramListing::toValue() const {
    Value::Array a;
    for (const auto& e : steps) a.push_back(Value::object({{"text", programStepLine(e, true)}, {"current", e.isCurrent}}));
    return Value::object({{"steps", Value(std::move(a))},
                          {"currentIndex", currentIndex},
                          {"stepHighlight", hexColor(stepHighlightColor(source))}});
}

Value TICpuDebug::toValue() const {
    char buf[64];
    Value::Array rows;
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const CpuFrame& f = e.frame;
        Value::Object r{{"text", Value(inspectorLine(e))},
                        {"pc", Value(e.pc)},
                        {"history", Value(e.isHistory)},
                        {"current", Value(e.isCurrent)}};
        // The registers only for the rows the panel can show: the newest
        // while running, any while paused.
        if (paused || i + 1 == entries.size()) {
            Value::Array regs;
            const std::pair<const char*, const uint8_t*> named[] = {
                {"A", f.A}, {"B", f.B}, {"C", f.C}, {"D", f.D}, {"E", f.E}, {"Sout", f.Sout}};
            for (const auto& [name, nib] : named) regs.push_back(Value(Value::Array{Value(name), Value(registerNibbles(nib))}));
            r.push_back({"registers", Value(std::move(regs))});
            r.push_back({"KR", Value("KR: " + hexBin16(f.KR))});
            r.push_back({"fA", Value("fA: " + hexBin16(f.fA))});
            r.push_back({"fB", Value("fB: " + hexBin16(f.fB))});
            std::snprintf(buf, sizeof buf, "SR: %04X", f.SR);
            r.push_back({"SR", Value(std::string(buf))});
            r.push_back({"R5", Value("R5: " + std::to_string(f.R5))});
            r.push_back({"cond", Value(condFlag(f.flags))});
            r.push_back({"idle", Value(idleFlag(f.flags))});
            std::snprintf(buf, sizeof buf, "DIGIT: %2d", f.postDigit);
            r.push_back({"digit", Value(std::string(buf))});
            r.push_back({"lit", Value("Lit: " + std::to_string(f.maxDigitDecay))});
            r.push_back({"displayOn", Value(f.displayOn != 0)});
        }
        rows.emplace_back(std::move(r));
    }
    return Value(std::move(rows));
}

Value TIHeatmap::toValue() const {
    Value::Bytes steps(counts.size());
    for (size_t i = 0; i < counts.size(); ++i) steps[i] = heatStep(counts[i]);
    return Value(std::move(steps));
}

Value TIAsmOverlayState::toValue() const {
    return Value::object({{"fileName", fileName}, {"words", instructions}, {"status", status}, {"active", running}});
}

Value TIDebugView::toValue() const {
    return Value::object({{"view", std::string(debugPanelName(view))},
                          {"requests", static_cast<std::int64_t>(requests)},
                          {"logLevel", std::string(debugLevelName(logLevel))}});
}

// ── The part ─────────────────────────────────────────────────────────────────

TIDebugPart::TIDebugPart(TIDebugSetup setup) : m_setup(std::move(setup)) {
    m_asm.fileName = kNoFile;
    m_asm.status = kFirstStatus;
}

machine::Machine* TIDebugPart::machine() const { return m_ctx ? m_ctx->machine() : nullptr; }

debugger::RunControl* TIDebugPart::runControl() const { return m_debugger ? &m_debugger->runControl() : nullptr; }

bool TIDebugPart::paused() const {
    const debugger::RunControl* rc = runControl();
    return rc && rc->paused();
}

bool TIDebugPart::overlayRunning() const {
    machine::Machine* m = machine();
    const AsmOverlay* o = m ? calcu::machine::first<AsmOverlay>(*m) : nullptr;
    return o && o->running();
}

void TIDebugPart::attach(sessions::SessionSetup& s) {
    m_ctx = &s.context();
    m_liveTopic = &s.stateTopic(topics::kCalculatorDebug);
    m_listingTopic = &s.stateTopic(topics::kProgramListing);
    m_cpuTopic = &s.stateTopic(topics::kCpuDebug);
    m_heatTopic = &s.stateTopic(topics::kHeatmap);
    m_asmTopic = &s.stateTopic(topics::kAsmOverlay);
    m_viewTopic = &s.stateTopic(topics::kDebugView);

    using sessions::flagArg;
    using sessions::textArg;
    auto add = [this, &s](const sessions::CommandName& name, std::function<CommandOutcome(const Value&)> f) {
        s.command(name, [this, f = std::move(f)](const Value& a, sessions::SessionContext& c) {
            m_ctx = &c;
            follow();
            CommandOutcome out = f(a);
            follow();
            publish();
            return out;
        });
    };
    add(commands::kLiveDebug, [this](const Value& a) -> CommandOutcome {
        setLiveShown(flagArg(a, "on"));
        return Done{};
    });
    add(commands::kShowCpuDebug, [this](const Value& a) -> CommandOutcome {
        setCpuShown(flagArg(a, "on"));
        return Done{};
    });
    add(commands::kHeatmapReset, [this](const Value&) -> CommandOutcome {
        resetHeatmap();
        return Done{};
    });
    add(commands::kLogLevel, [this](const Value& a) -> CommandOutcome {
        const auto l = debugLevelNamed(textArg(a, "level"));
        if (!l) return Refused{"The log level is off, info or debug."};
        setLogLevel(*l);
        return Done{};
    });
    add(commands::kDump, [this](const Value& a) -> CommandOutcome {
        const std::string what = textArg(a, "what");
        if (what != "scom" && what != "memory" && what != "prog" && what != "vars")
            return Refused{"A dump is scom, memory, prog or vars."};
        if (!machine()) return Refused{sessions::kNoMachine};
        return Done{sessions::valuePayload("c59.dump", linesValue(dump(what)))};
    });
    add(commands::kAsmLoad, [this](const Value& a) -> CommandOutcome {
        const Value* t = a.find("text");
        std::optional<std::string> text;
        if (t && t->string()) text = *t->string();
        if (auto refusal = loadAsm(textArg(a, "name"), text)) return Refused{*refusal};
        return Done{sessions::valuePayload("c59.asm-load", Value(m_asm.instructions))};
    });
    add(commands::kAsmRun, [this](const Value&) -> CommandOutcome {
        if (auto refusal = runAsm()) return Refused{*refusal};
        return Done{sessions::valuePayload("c59.asm-run", Value(m_asm.status))};
    });
    add(commands::kAsmClear, [this](const Value&) -> CommandOutcome {
        clearAsm();
        return Done{};
    });
    publishAsm();
    m_viewTopic->set(std::make_shared<TIDebugView>(m_view));
}

void TIDebugPart::beforeCall(sessions::SessionContext& c, pacing::WallTime) {
    m_ctx = &c;
    follow();
    publish();
}

void TIDebugPart::afterCall(sessions::SessionContext& c, pacing::WallTime now) {
    m_ctx = &c;
    follow();
    machine::Machine* m = machine();
    if (!m) return;
    const bool live = m_liveShown || m_setup.alwaysShown, cpu = m_cpuShown || m_setup.alwaysShown;
    if (!paused()) {
        DebugModel::Update u = m_debug.tick(*m, live, cpu, false);
        m_liveDirty = m_liveDirty || u.live;
        m_cpuDirty = m_cpuDirty || u.cpu;
        if (u.diagnostic) c.log(*u.diagnostic);
    }
    // The machine's own lines, at their level (AC-11): one log message per call.
    if (auto* log = calcu::machine::first<MachineLog>(*m)) {
        std::vector<std::string> lines;
        for (auto& l : log->take()) lines.push_back(std::move(l.text));
        if (!lines.empty()) c.log(joined(lines));
    }
    // The heatmap about 10 Hz (today's rate), while it counts.
    if (cpu && now >= m_nextHeat) {
        m_nextHeat = now + std::chrono::milliseconds(100);
        m_heatDirty = true;
    }
    publish();
}

void TIDebugPart::machineChanged(sessions::SessionContext& c) {
    m_ctx = &c;
    follow();
    publish();
}

void TIDebugPart::machineRestarted(sessions::SessionContext& c, sessions::Restart r) {
    m_ctx = &c;
    machine::Machine* m = machine();
    if (!m) return;
    if (auto* h = calcu::machine::first<RomHeatmapCounter>(*m)) h->reset();   // AC-8: a Reset zeroes the counts
    m_heatDirty = true;
    if (r == sessions::Restart::ColdBoot && m_asm.instructions > 0) {   // AC-26
        m_asm.fileName = kNoFile;
        m_asm.instructions = 0;
        m_asm.status = "ASM overlay removed by the Cold boot.";
    }
    follow();
    publishAsm();
    publish();
}

void TIDebugPart::operationEnded(sessions::SessionContext& c) {
    m_ctx = &c;
    follow();
    publish();
}

void TIDebugPart::refreshModule() {
    machine::Machine* m = machine();
    std::string id;
    if (m)
        for (const auto& e : m->plugged().byPlace)
            if (e.place == kSlot) id = e.peripheral.value;
    m_moduleID = id;
    std::vector<uint8_t> image;
    if (!id.empty() && m_setup.roms)
        if (auto file = moduleRomFile(id)) image = moduleImage(*m_setup.roms, *file).value_or(std::vector<uint8_t>{});
    m_debug.moduleChanged(image);
    if (!m_debug.moduleImageError().empty() && m_ctx) m_ctx->log(m_debug.moduleImageError());
}

void TIDebugPart::follow() {
    machine::Machine* m = machine();
    if (!m || !m_ctx) return;
    // A new machine (a start, a preset, a model switch): the overlay is
    // removed with the old one (AC-26), the counts start at zero.
    if (m_ctx->machineSession().generation() != m_generation) {
        m_generation = m_ctx->machineSession().generation();
        m_debug.attach({});
        refreshModule();
        if (auto* log = calcu::machine::first<MachineLog>(*m)) log->setLevel(static_cast<int>(m_logLevel));
        if (auto* h = calcu::machine::first<RomHeatmapCounter>(*m)) h->setOn(m_cpuShown || m_setup.alwaysShown);
        if (m_asm.instructions > 0) {
            m_asm.fileName = kNoFile;
            m_asm.instructions = 0;
            m_asm.status = "ASM overlay removed with the machine.";
        }
        m_wasPaused = false;
        m_pausedAt.reset();
        m_liveDirty = m_listingDirty = m_cpuDirty = m_heatDirty = true;
        publishAsm();
    } else {
        std::string id;
        for (const auto& e : m->plugged().byPlace)
            if (e.place == kSlot) id = e.peripheral.value;
        if (id != m_moduleID) refreshModule();   // a module attached or detached
    }

    debugger::RunControl* rc = runControl();
    if (rc) rc->sync();   // a new machine first (G11): a pause of the old one is not shown on it
    const debugger::RunState now = rc ? rc->state() : debugger::RunState{};
    const bool isPaused = rc && rc->paused();
    if (auto* cs = calcu::machine::first<CalculatorState>(*m))   // the CPU tab's ring: shown, paused or a start armed
        cs->setFrameRing(m_cpuShown || m_setup.alwaysShown || isPaused || now.armed.has_value());
    if (isPaused) {
        const machine::Ticks at = m->tickCount();
        if (!m_wasPaused || m_pausedAt != at) {
            m_pausedAt = at;
            const FrozenViewReason reason = now.owner == debugger::kCpu && now.reason == debugger::RunState::Reason::Step
                                                ? FrozenViewReason::StepInstruction
                                                : FrozenViewReason::Freeze;
            DebugModel::Update u = m_debug.frozenView(*m, reason);
            if (u.diagnostic) m_ctx->log(*u.diagnostic);
            m_liveDirty = m_liveDirty || u.live;
            m_listingDirty = m_listingDirty || u.listing;
            m_cpuDirty = true;
        }
    } else {
        m_pausedAt.reset();
        if (m_wasPaused) {
            m_listingDirty = m_debug.clearListing() || m_listingDirty;
            m_cpuDirty = true;
        }
    }
    m_wasPaused = isPaused;
    if (const bool running = overlayRunning(); running != m_asm.running) {
        m_asm.running = running;
        publishAsm();
    }
}

void TIDebugPart::publish() {
    machine::Machine* m = machine();
    if (!m_liveTopic || !m) return;
    if (std::exchange(m_liveDirty, false)) {
        auto p = std::make_shared<TICalculatorDebug>();
        p->live = m_debug.live();
        m_liveTopic->set(std::move(p));
    }
    if (std::exchange(m_listingDirty, false)) {
        auto p = std::make_shared<TIProgramListing>();
        p->steps = m_debug.listing();
        p->source = m_debug.listingSource();
        p->currentIndex = m_debug.listingCurrentIndex();
        m_listingTopic->set(std::move(p));
    }
    if (std::exchange(m_cpuDirty, false)) {
        auto p = std::make_shared<TICpuDebug>();
        p->entries = m_debug.cpuHistory();
        p->paused = paused();
        p->serial = ++m_serial;
        m_cpuTopic->set(std::move(p));
    }
    if (std::exchange(m_heatDirty, false)) {
        auto p = std::make_shared<TIHeatmap>();
        if (auto* h = calcu::machine::first<RomHeatmapCounter>(*m)) p->counts = h->counts();
        p->serial = ++m_serial;
        m_heatTopic->set(std::move(p));
    }
}

void TIDebugPart::publishAsm() {
    if (m_asmTopic) m_asmTopic->set(std::make_shared<TIAsmOverlayState>(m_asm));
}

void TIDebugPart::setStatus(std::string s) {
    m_asm.status = std::move(s);
    publishAsm();
}

// ── The flows ────────────────────────────────────────────────────────────────

void TIDebugPart::showView(sessions::SessionContext& c, const std::string& view) {
    m_ctx = &c;
    const auto p = debugPanelNamed(view);
    if (!p) return;
    if (*p == DebugPanel::Calculator && overlayRunning()) return;   // AC-2: passed over without a word
    m_view.view = *p;
    ++m_view.requests;
    if (m_viewTopic) m_viewTopic->set(std::make_shared<TIDebugView>(m_view));
}

void TIDebugPart::setLiveShown(bool on) {
    m_liveShown = on;
    machine::Machine* m = machine();
    if (on && m && paused()) {   // shown while paused: the paused view may predate it
        DebugModel::Update u = m_debug.rebuildLive(*m, true);
        m_liveDirty = m_liveDirty || u.live;
        m_listingDirty = m_listingDirty || u.listing;
    }
}

void TIDebugPart::setCpuShown(bool on) {
    if (m_cpuShown == on) return;
    m_cpuShown = on;
    machine::Machine* m = machine();
    if (!m) return;
    if (auto* h = calcu::machine::first<RomHeatmapCounter>(*m)) {
        h->setOn(on || m_setup.alwaysShown);
        // The heatmap shows what ran since the tab came into view (AC-8); a
        // paused machine's heatmap is kept.
        if (on && !paused()) h->reset();
    }
    m_heatDirty = true;
    m_cpuDirty = true;
}

void TIDebugPart::resetHeatmap() {
    if (machine::Machine* m = machine())
        if (auto* h = calcu::machine::first<RomHeatmapCounter>(*m)) h->reset();
    m_heatDirty = true;
}

void TIDebugPart::setLogLevel(DebugLevel l) {
    m_logLevel = l;
    if (machine::Machine* m = machine())
        if (auto* log = calcu::machine::first<MachineLog>(*m)) log->setLevel(static_cast<int>(l));
    m_view.logLevel = l;
    if (m_viewTopic) m_viewTopic->set(std::make_shared<TIDebugView>(m_view));
}

std::vector<std::string> TIDebugPart::dump(std::string_view what) {
    machine::Machine* m = machine();
    if (!m) return {};
    std::vector<std::string> lines = what == "scom"     ? dumpScom(*m)
                                     : what == "memory" ? dumpMemory(*m)
                                     : what == "prog"   ? dumpProg(*m)
                                     : what == "vars"   ? dumpVars(*m)
                                                        : std::vector<std::string>{};
    if (!lines.empty() && m_ctx) m_ctx->log(joined(lines));   // AC-11: at every level
    return lines;
}

std::optional<std::string> TIDebugPart::loadAsm(const std::string& name, const std::optional<std::string>& text, bool notice) {
    std::string refusal;
    std::vector<std::uint16_t> words;
    if (!text) {
        refusal = name + " could not be read.";
    } else {
        auto r = readAsmOverlay(name, *text);
        if (auto* s = std::get_if<std::string>(&r)) refusal = *s;
        else words = std::move(std::get<std::vector<std::uint16_t>>(r));
    }
    machine::Machine* m = machine();
    AsmOverlay* o = m ? calcu::machine::first<AsmOverlay>(*m) : nullptr;
    if (refusal.empty() && !o) refusal = sessions::kNoMachine;
    if (refusal.empty() && !o->load(words))
        refusal = name + " holds " + std::to_string(words.size()) + " instructions; at most 2048 fit from 1800 to 1FFF.";
    if (!refusal.empty()) {   // EC-2: what was loaded before stays
        if (notice && m_ctx) m_ctx->notice(refusal);
        setStatus(refusal);
        return refusal;
    }
    m_asm.fileName = name;
    m_asm.instructions = static_cast<int>(words.size());
    m_asm.running = o->running();
    setStatus("Loaded " + std::to_string(words.size()) + " word(s) at 0x1800.");
    return std::nullopt;
}

std::optional<std::string> TIDebugPart::runAsm() {
    machine::Machine* m = machine();
    AsmOverlay* o = m ? calcu::machine::first<AsmOverlay>(*m) : nullptr;
    if (!o || o->loaded() == 0) {   // EC-1
        setStatus(kNoAsmOverlay);
        return std::string(kNoAsmOverlay);
    }
    if (m_ctx && m_ctx->busy()) return std::string(sessions::kAnotherOperationRuns);
    debugger::RunControl* rc = runControl();
    const bool armedCpu = rc && rc->state().armed == debugger::kCpu;
    if (rc && !rc->running()) rc->resume();   // Run while paused (AC-17): it goes on in the overlay
    const AsmOverlay::Entry e = o->enter(8192);
    if (!e.entered) {
        const std::string s = "ASM entry failed after " + std::to_string(e.instructions) + " step(s).";
        if (m_ctx) m_ctx->notice(s);
        setStatus(s);
        return s;
    }
    m_asm.running = true;
    setStatus("ASM entered at 0x1800 (" + std::to_string(e.instructions) + " step(s)).");
    // AC-18: the panel shows the CPU tab while the overlay runs.
    m_view.view = DebugPanel::Cpu;
    ++m_view.requests;
    if (m_viewTopic) m_viewTopic->set(std::make_shared<TIDebugView>(m_view));
    if (armedCpu && rc) rc->fireStart(debugger::kCpu);   // AC-17: paused at 1800, the reason pause-on-start
    m_cpuDirty = m_liveDirty = true;
    return std::nullopt;
}

void TIDebugPart::clearAsm() {
    machine::Machine* m = machine();
    AsmOverlay* o = m ? calcu::machine::first<AsmOverlay>(*m) : nullptr;
    const bool wasRunning = o && o->running();
    if (o) o->clear();
    m_asm.instructions = 0;
    m_asm.running = false;
    setStatus("ASM overlay cleared.");
    // A running overlay would now carry out the empty range: reset the machine (AC-16).
    if (wasRunning && m) {
        if (auto* rc = runControl(); rc && !rc->running()) rc->resume();
        m->reset();
        if (m_ctx) m_ctx->tellRestarted(sessions::Restart::Reset);
    }
}

// ── The product entries (AC-25) ──────────────────────────────────────────────

void TIDebugPart::productEntries(const machine::Machine& m, std::vector<control::ProductEntry>& out) const {
    auto entry = [&out](const char* name, bool available, const char* reason) {
        Value::Object o{{"available", Value(available)}};
        if (!available) o.push_back({"reason", Value(reason)});
        out.push_back(control::ProductEntry{name, sessions::valuePayload(name, Value(std::move(o)))});
    };
    const AsmOverlay* o = calcu::machine::first<AsmOverlay>(m);
    const bool loaded = o && o->loaded() > 0;
    const bool busy = m_ctx && m_ctx->busy();
    out.push_back(control::ProductEntry{
        entries::kAsmRun,
        sessions::valuePayload(entries::kAsmRun,
                               loaded && !busy ? Value::object({{"available", true}})
                                               : Value::object({{"available", false},
                                                                {"reason", loaded ? sessions::kAnotherOperationRuns : kNoAsmOverlay}}))});
    // TRACE can always be pressed: the trace folder is asked at the trace's
    // start, which is refused when it cannot be written (LIB-10 AC-53; C59-4
    // AC-12 as amended, row 59-62).
    entry(entries::kTrace, true, "");
    entry(entries::kCalculatorTab, !(o && o->running()), kAsmOverlayRuns);
}

} // namespace cu59
