#include "Machine/SharpMachine.hpp"

#include <algorithm>

#include "../Connector/SoftwareDefinedCard.hpp"
#include "Machine/SharpCatalogue.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

bool isSlot(const PlaceName& p) { return p.value == kSlot1 || p.value == kSlot2; }

// The place each 60-pin peripheral goes in (C1600-1 §3.4): they all fit the
// one pin-out of the connector, so the place is the product's rule.
const char* placeOf(const PeripheralId& id) {
    if (id.value == kCE150 || id.value == kCE1600P) return kPrinter;
    if (id.value == kCE158) return kInterface;
    if (id.value == kHostDrive) return kHostDrivePlace;
    return nullptr;   // a module: the slots' pin-outs decide
}

} // namespace

const std::vector<std::uint8_t>* romImageOf(const PeripheralRequest& req, const PeripheralDescriptor& d, std::size_t index) {
    const auto& entries = d.romsFor(req.variant);
    if (index >= entries.size()) return nullptr;
    for (const RomImage& image : req.roms)
        if (image.fileName == entries[index].fileName) return &image.bytes;
    return nullptr;
}

std::unique_ptr<ExpansionCard> moduleCardOf(const PeripheralRequest& req, const PeripheralDescriptor& d) {
    if (auto* m = dynamic_cast<const ModuleCard*>(req.content.get()); m && m->card) return std::move(m->card);
    if (auto* def = dynamic_cast<const ModuleDefinitionData*>(d.productData.get()))
        return std::make_unique<SoftwareDefinedCard>(def->definition);
    return nullptr;
}

// ---- Emulated time and the power cycle ----

Ticks SharpMachine::advance(Ticks atLeast) {
    if (atLeast == 0) return 0;
    Ticks ran = 0;
    while (ran < atLeast && m_sequence.active()) {
        const Ticks chunk = std::min(atLeast - ran, m_sequence.remaining());
        const Ticks r = runCore(chunk);
        ran += r;
        m_sequence.consumed(r, *this);
        if (r == 0) return ran;   // a debugger stop: nothing ran
    }
    if (ran < atLeast) ran += runCore(atLeast - ran);
    return ran;
}

void SharpMachine::powerCycle() {
    if (m_sequence.active()) return;
    if (!running()) {
        // Off already (or never started): the change is made and the
        // machine stays as it is, as today (design D-4).
        makeWaitingChanges();
        return;
    }
    m_sequence.start(m_descriptor.tickRate / 60, *this);
}

void SharpMachine::reset() {
    m_sequence.cancel(*this);
    makeWaitingChanges();
    coreReset();
}

void SharpMachine::coldBoot() {
    m_sequence.cancel(*this);
    makeWaitingChanges();
    // A module without a battery of its own forgets its RAM (AC-6, 1600-16).
    for (const PlugEntry& e : m_plugged)
        if (e.place.steps.size() == 1 && isSlot(e.place.steps[0]))
            if (ExpansionCard* card = moduleIn(e.place.steps[0])) card->powerLoss();
    coreColdBoot();
}

bool SharpMachine::running() const { return tickCount() > 0 && !coreSwitchedOff(); }

void SharpMachine::makeWaitingChanges() {
    std::vector<std::function<void()>> changes = std::move(m_waiting);
    m_waiting.clear();
    for (auto& c : changes) c();
    std::vector<std::function<void()>> after = std::move(m_afterWaiting);
    m_afterWaiting.clear();
    for (auto& f : after) f();
}

void SharpMachine::atOffPoint(std::function<void()> f) {
    if (m_waiting.empty()) f();
    else m_afterWaiting.push_back(std::move(f));
}

// ---- Keys ----

KeyCall SharpMachine::keyDown(const KeyId& id) {
    if (!m_descriptor.hasKey(id)) return KeyCall::UnknownKey;
    if (id.value == "on") coreOnKey(true);
    else coreKey(id.value, true);
    return KeyCall::Done;
}

KeyCall SharpMachine::keyUp(const KeyId& id) {
    if (!m_descriptor.hasKey(id)) return KeyCall::UnknownKey;
    if (id.value == "on") coreOnKey(false);
    else coreKey(id.value, false);
    return KeyCall::Done;
}

// ---- Places ----

Plugged SharpMachine::plugged() const {
    Plugged p;
    p.byPlace = m_plugged;
    for (const PlugEntry& e : m_plugged) {
        const PlaceDescriptor* place = e.place.steps.size() == 1 ? m_descriptor.place(e.place.steps[0]) : nullptr;
        if (!place) continue;
        auto it = std::find_if(p.byConnector.begin(), p.byConnector.end(),
                               [&](const ConnectorPlugs& c) { return c.connector == place->connector; });
        if (it == p.byConnector.end()) {
            p.byConnector.push_back({PlacePath{}, place->connector, {}});
            it = std::prev(p.byConnector.end());
        }
        it->plugged.push_back(e);
    }
    return p;
}

const PlugEntry* SharpMachine::at(const char* place) const {
    for (const PlugEntry& e : m_plugged)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return &e;
    return nullptr;
}

std::optional<std::string> SharpMachine::productRefusal(const PeripheralRequest& req,
                                                        const std::optional<PeripheralId>& replacing) const {
    const std::string place = req.place.steps.empty() ? std::string() : req.place.steps[0].value;
    if (const char* own = placeOf(req.peripheral); own && place != own)
        return "The " + req.peripheral.value + " goes in the place " + own + ".";
    return modelRefusal(req, replacing);
}

AfterChange SharpMachine::followsNow(const PinOutDescriptor& p) const {
    return running() ? p.after : AfterChange::Nothing;   // switched off: nothing (LIB-2 AC-4)
}

void SharpMachine::plug(PeripheralRequest req, const PeripheralDescriptor& d, TickRate) {
    const PlaceName place = req.place.steps.at(0);
    m_plugged.push_back(PlugEntry{req.place, req.peripheral, req.variant});
    if (isSlot(place) && !d.controls.empty()) {
        std::vector<SwitchPosition> positions = req.switches;
        if (positions.empty())
            if (auto* m = dynamic_cast<const ModuleCard*>(req.content.get()); m && m->card)
                positions.push_back({ControlId{"write-protect"}, m->card->writeProtected()});
        makeSwitches(place, positions);
    }
    auto request = std::make_shared<PeripheralRequest>(std::move(req));
    auto descriptor = std::make_shared<PeripheralDescriptor>(d);
    auto change = [this, request, descriptor, place] {
        corePlug(*request, *descriptor);
        // The switch's position reaches the card it belongs to.
        if (auto* table = switchesAt(place))
            if (ExpansionCard* card = moduleIn(place)) card->setWriteProtected(table->on(ControlId{"write-protect"}));
    };
    if (running()) m_waiting.push_back(std::move(change));
    else change();
}

void SharpMachine::unplug(const PlacePath& path, const PeripheralId& id) {
    m_plugged.erase(std::remove_if(m_plugged.begin(), m_plugged.end(),
                                   [&](const PlugEntry& e) { return e.place == path && e.peripheral == id; }),
                    m_plugged.end());
    const PlaceName place = path.steps.at(0);
    m_switches.erase(std::remove_if(m_switches.begin(), m_switches.end(), [&](const auto& s) { return s.first == place; }),
                     m_switches.end());
    auto change = [this, place, id] { coreUnplug(place, id); };
    if (running()) m_waiting.push_back(std::move(change));
    else change();
}

void SharpMachine::makeSwitches(const PlaceName& place, const std::vector<SwitchPosition>& positions) {
    auto table = std::make_unique<calcu::peripherals::ControlTable>(
        std::vector<ControlDescriptor>{{ControlId{"write-protect"}, ControlForm::Switch}},
        [this, place](const ControlId&, bool on) {
            if (ExpansionCard* card = moduleIn(place)) card->setWriteProtected(on);
        });
    table->restore(positions);
    m_switches.emplace_back(place, std::move(table));
}

calcu::peripherals::ControlTable* SharpMachine::switchesAt(const PlaceName& place) {
    for (auto& [p, t] : m_switches)
        if (p == place) return t.get();
    return nullptr;
}

void SharpMachine::syncFromCore() {
    m_waiting.clear();
    m_afterWaiting.clear();
    m_plugged = coreHasPlugged();
    m_switches.clear();
    for (const PlugEntry& e : m_plugged) {
        const PlaceName& place = e.place.steps.at(0);
        if (!isSlot(place)) continue;
        if (ExpansionCard* card = moduleIn(place); card && card->hasWriteProtectSwitch())
            makeSwitches(place, {{ControlId{"write-protect"}, card->writeProtected()}});
    }
}

// ---- Capabilities ----

SharpMachine::Offered SharpMachine::byComponent(const char* component, Capability& c) {
    return {CapabilityEntry{c.kind(), CapabilityId{std::string(component) + "/" + std::string(c.kind().value)},
                            ComponentOwner{ComponentId{component}}},
            &c};
}

SharpMachine::Offered SharpMachine::byPeripheral(const PlugEntry& e, Capability& c) {
    return {CapabilityEntry{c.kind(), CapabilityId{placeText(e.place) + "/" + std::string(c.kind().value)},
                            PeripheralOwner{e.peripheral, e.place}},
            &c};
}

std::vector<CapabilityEntry> SharpMachine::capabilities() const {
    std::vector<Offered> offered;
    auto* self = const_cast<SharpMachine*>(this);   // offer() hands out non-const pointers; nothing changes
    self->offer(offered, m_plugged);
    for (const PlugEntry& e : m_plugged)
        if (auto* t = self->switchesAt(e.place.steps.at(0))) offered.push_back(byPeripheral(e, *t));
    std::vector<CapabilityEntry> out;
    for (const auto& o : offered) out.push_back(o.entry);
    return out;
}

Capability* SharpMachine::capability(const CapabilityId& id) {
    std::vector<Offered> offered;
    offer(offered, m_plugged);
    for (const PlugEntry& e : m_plugged)
        if (auto* t = switchesAt(e.place.steps.at(0))) offered.push_back(byPeripheral(e, *t));
    for (const auto& o : offered)
        if (o.entry.id == id) return o.capability;
    return nullptr;
}

} // namespace sharp
