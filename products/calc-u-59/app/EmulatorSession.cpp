#include "EmulatorSession.hpp"
#include "KeyIds.hpp"
#include "ModuleCatalog.hpp"
#include "../core/PrinterOutput.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cu59 {

uint32_t traceFlagsFor(const TraceConsumers& c) {
    uint32_t flags = TRACE_NONE;
    if (c.cpuPanelVisible || c.frozen || c.manualTrace || c.freezeArmed || c.scriptedTrace)
        flags |= TRACE_PC | TRACE_REGS_FULL;
    return flags;
}

namespace {
const calcu::machine::PlacePath kSlot{{kTIModulePlace}};
const calcu::machine::PlacePath kPrinter{{kTIPrinterPlace}};

// A module ID the set holds the file of, or none (an empty slot).
std::optional<std::string> knownModule(const std::optional<std::string>& id, const calcu::roms::RomSet& roms) {
    if (!id) return std::nullopt;
    auto file = moduleRomFile(*id);
    return file && roms.bytes(*file) ? id : std::nullopt;
}
MachinePeripherals checked(MachinePeripherals p, const calcu::roms::RomSet& roms) {
    p.module = knownModule(p.module, roms);
    return p;
}
} // namespace

EmulatorSession::EmulatorSession(Model model, const calcu::roms::RomSet& roms, MachinePeripherals peripherals,
                                 Listener& listener)
    : m_model(model),
      m_listener(listener),
      m_machine(buildMachine(model, roms, checked(peripherals, roms))),
      m_manualTrace(model),
      m_scriptedTrace(model) {
    if (auto id = knownModule(peripherals.module, roms))
        m_moduleImage = cu59::moduleImage(roms, *moduleRomFile(*id)).value_or(std::vector<uint8_t>{});

    access().debugTarget().setRunControl(&m_freeze);
}

EmulatorSession::~EmulatorSession() { access().debugTarget().setRunControl(nullptr); }

void TIKeyTraceObserver::keyChanged(const calcu::machine::KeyId& id, bool down, calcu::machine::Ticks) {
    const auto code = matrixCodeOf(id);
    if (!code) return;
    const auto row = static_cast<uint8_t>(*code / 10 - 1), col = static_cast<uint8_t>(*code % 10 - 1);
    if (down) m_w.writeKeyDown(row, col);
    else m_w.writeKeyUp(row, col);
}

// A KEYSTROKES key: one press of the typing engine, origin Preset (LIB-5 §4).
bool EmulatorSession::typeKey(uint8_t matrixCode) {
    const auto id = keyIdAt(matrixCode);
    if (!id || !m_typing) return false;
    return !m_typing->typeKeys({calcu::typing::Press{*id}}, calcu::typing::Origin::Preset).refused;
}

calcu::machine::ChangeResult EmulatorSession::setModule(const std::optional<std::string>& moduleID,
                                                        const calcu::roms::RomSet& roms) {
    using namespace calcu::machine;
    auto a = access();
    Machine& m = a.machine();
    std::optional<PeripheralId> now;
    for (const auto& e : m.plugged().byPlace)
        if (e.place == kSlot) now = e.peripheral;
    ChangeResult r;
    if (!moduleID) {
        if (now) r = m.detach(kSlot, *now, tiCatalogue());
    } else {
        PeripheralRequest req = moduleRequest(*moduleID, roms);
        r = now ? m.replace(*now, std::move(req), tiCatalogue()) : m.attach(std::move(req), tiCatalogue());
    }
    if (!r.refused) {
        m_moduleImage.clear();
        if (moduleID) m_moduleImage = cu59::moduleImage(roms, *moduleRomFile(*moduleID)).value_or(std::vector<uint8_t>{});
    }
    return r;
}

EmulatorSession::PrinterChange EmulatorSession::setPrinter(bool connected, bool trace) {
    using namespace calcu::machine;
    auto a = access();
    Machine& m = a.machine();
    PrinterChange out;
    auto* paper = capabilityAs<PrinterOutput>(m, CapabilityId{"printer/c59.printer-output"});
    if (connected == (paper != nullptr)) return out;   // as it is: nothing changes, nothing follows
    if (connected) {
        out.result = m.attach(printerRequest(trace), tiCatalogue());
    } else {
        paper->endLineHere();
        out.lines = paper->takeLines();
        out.codeLines = paper->takeCodeLines();
        out.result = m.detach(kPrinter, kPC100C, tiCatalogue());
    }
    return out;
}

bool EmulatorSession::printerControl(const calcu::machine::ControlId& id, bool on) {
    auto a = access();
    auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(a.machine(),
                                                                         calcu::machine::CapabilityId{"printer/controls"});
    if (!c) return false;
    for (const auto& info : c->list()) {
        if (info.id != id) continue;
        if (info.form == calcu::machine::ControlForm::Switch) return c->set(id, on) == calcu::peripherals::ControlCall::Done;
        return (on ? c->press(id) : c->release(id)) == calcu::peripherals::ControlCall::Done;
    }
    return false;
}

void EmulatorSession::runQueuedSteps() {
    while (!m_steps.empty()) {
        auto step = std::move(m_steps.front());
        m_steps.pop_front();
        step();
    }
}

// Playback advances at the end of every slice (sliceEnded), at the time the
// slice was due (its Wait: is wall time); with no slice (frozen), at `now`.
// Its keys are held by the typing engine in emulated time (LIB-5).
void EmulatorSession::pump(std::chrono::steady_clock::time_point now) {
    runQueuedSteps();
    if (!m_sliceRan) m_playback.advance(now);
    m_sliceRan = false;
    runQueuedSteps();
}

calcu::machine::Ticks EmulatorSession::advanceHosted(calcu::machine::Ticks atLeast, bool& stopped) {
    stopped = false;
    if (m_halted) {
        stopped = true;
        return 0;
    }
    std::optional<FreezeOwner> fired;
    bool breakpoint = false;
    const calcu::machine::Ticks ran = m_machineRef.advance(atLeast);
    switch (access().debugTarget().takeStop()) {
    case TIStop::Breakpoint: breakpoint = true; break;   // core breakpoints: set by headless tools only
    case TIStop::Observer: fired = m_freeze.takeFired().value_or(FreezeOwner::Cpu); break;
    case TIStop::None: break;
    }
    m_ramDirty = true;   // TI-58C persist debounce
    if (breakpoint || fired) {
        m_halted = true;
        stopped = true;
        if (fired) onFreezeFired(*fired);
    }
    return ran;
}

void EmulatorSession::setFullSpeed(bool on) {
    if (m_fullSpeed == on) return;
    m_fullSpeed = on;
    m_listener.fullSpeedChanged(on);
}

// ── Debugger ─────────────────────────────────────────────────────────────────

DebugState EmulatorSession::debugState() const { return m_state; }

uint64_t EmulatorSession::setFrozenState(bool frozen, std::optional<FreezeOwner> owner) {
    if (frozen) {
        m_freeze.disarmPcChange();
        m_freeze.disarmScanLoopExit();
    }
    m_state.frozen = frozen;
    m_state.owner = frozen ? owner : std::nullopt;
    if (frozen) m_state.pcChangeArmed = m_state.scanLoopExitArmed = false;
    const uint64_t generation = ++m_state.generation;
    applyTraceFlags();
    return generation;
}

// The advance has stopped; the freeze is applied here, before any listener runs,
// so the key sequencer never sees a stopped machine as running.
void EmulatorSession::onFreezeFired(FreezeOwner owner) {
    uint64_t generation = setFrozenState(true, owner);
    // CALCULATOR freezes at a step dispatch: prepare the next step's display state.
    if (owner == FreezeOwner::Calculator) access().debugTarget().beginNextStep();
    m_listener.debugStateChanged();
    m_listener.frozenViewReady(generation, FrozenViewReason::FreezeFired);
}

void EmulatorSession::freeze(FreezeOwner owner) {
    m_halted = true;
    uint64_t generation = setFrozenState(true, owner);
    // The step to the next keycode boundary runs after the host call.
    m_steps.push_back([this, generation] {
        {
            auto a = access();
            a.debugTarget().stepUntilNextKeycode();
            a.debugTarget().beginNextStep();
        }
        m_listener.frozenViewReady(generation, FrozenViewReason::Freeze);
    });
}

void EmulatorSession::freezeInPlace(FreezeOwner owner) { setFrozenState(true, owner); }

void EmulatorSession::resume() {
    setFrozenState(false, std::nullopt);
    m_halted = false;
}

void EmulatorSession::clearFreeze() { setFrozenState(false, std::nullopt); }

void EmulatorSession::armPcChange() {
    m_freeze.armPcChange();  // disarms the scan-loop exit
    m_state.pcChangeArmed = true;
    m_state.scanLoopExitArmed = false;
    applyTraceFlags();
}

void EmulatorSession::armScanLoopExit() {
    m_freeze.armScanLoopExit(access().debugTarget().pc());  // disarms the step-boundary freeze
    m_state.scanLoopExitArmed = true;
    m_state.pcChangeArmed = false;
    applyTraceFlags();
}

void EmulatorSession::disarmPcChange() {
    m_freeze.disarmPcChange();
    m_state.pcChangeArmed = false;
    applyTraceFlags();
}

void EmulatorSession::disarmScanLoopExit() {
    m_freeze.disarmScanLoopExit();
    m_state.scanLoopExitArmed = false;
    applyTraceFlags();
}

void EmulatorSession::stepKeycode() {
    DebugState s = debugState();
    if (!s.frozen) return;
    m_steps.push_back([this, generation = s.generation] {
        {
            auto a = access();
            a.debugTarget().stepUntilNextKeycode();
            a.debugTarget().beginNextStep();
        }
        m_listener.frozenViewReady(generation, FrozenViewReason::StepKeycode);
    });
}

void EmulatorSession::stepInstruction() {
    DebugState s = debugState();
    if (!s.frozen) return;
    m_steps.push_back([this, generation = s.generation] {
        {
            auto a = access();
            a.debugTarget().stepInstruction();   // through a HOLD (WAIT Dn) of up to 32
            a.debugTarget().beginNextStep();
        }
        m_listener.frozenViewReady(generation, FrozenViewReason::StepInstruction);
    });
}

void EmulatorSession::setCpuPanelVisible(bool visible) {
    m_cpuPanelVisible = visible;
    applyTraceFlags();
}

// The one place the core's trace flags are set.
void EmulatorSession::applyTraceFlags() {
    TraceConsumers c;
    c.cpuPanelVisible = m_cpuPanelVisible;
    c.frozen = m_state.frozen;
    c.freezeArmed = m_state.pcChangeArmed || m_state.scanLoopExitArmed;
    c.manualTrace = m_manualTrace.isOpen();
    c.scriptedTrace = m_scriptedTrace.isOpen();
    uint32_t flags = traceFlagsFor(c);
    auto a = access();
    if (a.debugTarget().traceFlags() != flags) a.debugTarget().setTraceFlags(flags);
}

bool EmulatorSession::openManualTrace(const std::filesystem::path& path, uint64_t maxBytes) {
    bool ok = m_manualTrace.open(path, maxBytes);
    applyTraceFlags();
    return ok;
}

void EmulatorSession::closeManualTrace() {
    if (!m_manualTrace.isOpen()) return;
    drainTrace();
    m_manualTrace.close();
    applyTraceFlags();
}

void EmulatorSession::setTraceLocation(const std::filesystem::path& dir, uint64_t maxBytes) {
    m_traceDir = dir;
    m_traceMaxBytes = maxBytes;
}

EmulatorSession::DrainResult EmulatorSession::drainTrace() {
    DrainResult r;
    auto& frames = m_drainBuf;
    uint32_t lost = 0;
    uint32_t n = access().debugTarget().drainCpuFrames(frames.data(), static_cast<uint32_t>(frames.size()), &lost);
    if (lost) {
        m_manualTrace.writeLostGap(lost);
        m_scriptedTrace.writeLostGap(lost);
    }
    for (uint32_t i = 0; i < n; ++i) {
        bool manual = m_manualTrace.isOpen(), scripted = m_scriptedTrace.isOpen();
        if (!manual && !scripted) break;
        if (manual && !m_manualTrace.write(frames[i])) r.manualLimit = true;
        if (scripted && !m_scriptedTrace.write(frames[i])) r.scriptedLimit = true;
    }
    if (r.manualLimit || r.scriptedLimit) applyTraceFlags();  // a writer closed itself
    return r;
}

std::string EmulatorSession::elapsedReport() const {
    const uint64_t now = m_machineRef.tickCount();
    const uint64_t rate = m_machineRef.descriptor().tickRate;
    const uint64_t base = m_elapseBaseline;
    uint64_t delta = now >= base ? now - base : 0;
    double seconds = static_cast<double>(delta) / static_cast<double>(rate);
    long totalMs = std::lround(seconds * 1000);
    char buf[64];
    std::snprintf(buf, sizeof buf, "Elapsed: %ld:%02ld.%03ld", totalMs / 60000, (totalMs / 1000) % 60, totalMs % 1000);
    return buf;
}

// Switching files (or turning off) first flushes and closes the open scripted
// session, for a clean SESSION_END.
void EmulatorSession::scriptedTrace(const std::optional<std::string>& fileName) {
    if (m_scriptedTrace.isOpen()) {
        drainTrace();
        m_scriptedTrace.close();
    }
    bool opened = false;
    if (fileName) {
        opened = m_scriptedTrace.open(m_traceDir / *fileName, m_traceMaxBytes);
        if (!opened) m_listener.scriptedTraceFailed(*fileName);
    }
    applyTraceFlags();
}

// A script always ends at regular speed, and never leaves a scripted trace
// running without its "Trace: Off".
void EmulatorSession::applyReload(const std::shared_ptr<const ReloadStage>& stage) {
    ++m_hostPending;
    m_listener.reloadRequested(stage);
}

void EmulatorSession::queueCard(const KeystrokeEvent& event) {
    QueuedCard card{event.cardName, event.card, event.kind == KeystrokeEvent::Kind::InsertCard, event.cardSide};
    if (card.insertNow) ++m_hostPending;
    m_queuedCards.push_back(std::move(card));
}

void EmulatorSession::hostDone() {
    if (m_hostPending > 0) --m_hostPending;
}

std::optional<QueuedCard> EmulatorSession::takeQueuedCard() {
    if (m_state.frozen) return std::nullopt;
    auto a = access();
    const bool keyReady = a.machine().isKeyReady();
    const bool cardWait = a.cardReader().isInCardWait() && a.cardReader().isWaitingForCard();
    // Two queues in one: InsertCard cards for an idle calculator, ArmCard
    // cards for the card wait, each in order; neither holds up the other.
    bool insertNow;
    if (keyReady) insertNow = true;
    else if (cardWait) insertNow = false;
    else return std::nullopt;
    auto it = std::find_if(m_queuedCards.begin(), m_queuedCards.end(),
                           [&](const QueuedCard& c) { return c.insertNow == insertNow; });
    if (it == m_queuedCards.end()) return std::nullopt;
    QueuedCard card = std::move(*it);
    m_queuedCards.erase(it);
    return card;
}

std::vector<std::string> EmulatorSession::clearQueuedCards() {
    std::vector<std::string> names;
    for (const auto& c : m_queuedCards) names.push_back(c.name);
    m_queuedCards.clear();
    return names;
}

void EmulatorSession::playbackFinished() {
    // Queued cards stay: a Write keyed in last, or a run the user starts
    // later, still gets its card. An InsertCard nobody waits for any more is
    // done.
    m_hostPending = 0;
    setFullSpeed(false);
    if (m_scriptedTrace.isOpen()) {
        drainTrace();
        m_scriptedTrace.close();
        applyTraceFlags();
    }
    m_listener.playbackFinished();
}

} // namespace cu59
