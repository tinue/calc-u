#include "EmulatorSession.hpp"
#include "KeyIds.hpp"
#include "ModuleCatalog.hpp"
#include "../core/TICatalogue.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cu59 {

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
      m_machine(buildMachine(model, roms, checked(peripherals, roms))) {
    if (auto id = knownModule(peripherals.module, roms))
        m_moduleImage = cu59::moduleImage(roms, *moduleRomFile(*id)).value_or(std::vector<uint8_t>{});
}

EmulatorSession::~EmulatorSession() = default;

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

calcu::machine::ChangeResult EmulatorSession::setPrinter(bool connected, bool trace) {
    using namespace calcu::machine;
    auto a = access();
    Machine& m = a.machine();
    bool there = false;
    for (const auto& e : m.plugged().byPlace)
        if (e.place == kPrinter) there = true;
    if (connected == there) return {};   // as it is: nothing changes, nothing follows
    return connected ? m.attach(printerRequest(trace), tiCatalogue()) : m.detach(kPrinter, kPC100C, tiCatalogue());
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

// Playback advances at the end of every slice (sliceEnded), at the time the
// slice was due (its Wait: is wall time); with no slice (frozen), at `now`.
// Its keys are held by the typing engine in emulated time (LIB-5).
void EmulatorSession::pump(std::chrono::steady_clock::time_point now) {
    if (!m_sliceRan) m_playback.advance(now);
    m_sliceRan = false;
}

void EmulatorSession::setFullSpeed(bool on) {
    if (m_fullSpeed == on) return;
    m_fullSpeed = on;
    m_listener.fullSpeedChanged(on);
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
    if (m_listener.paused()) return std::nullopt;
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

// A script always ends at regular speed.
void EmulatorSession::playbackFinished() {
    // Queued cards stay: a Write keyed in last, or a run the user starts
    // later, still gets its card. An InsertCard nobody waits for any more is
    // done.
    m_hostPending = 0;
    setFullSpeed(false);
    m_listener.playbackFinished();   // the preset's trace ends with its operation (LIB-10 T3)
}

} // namespace cu59
