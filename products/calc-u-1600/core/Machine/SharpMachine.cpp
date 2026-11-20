#include "Machine/SharpMachine.hpp"

#include <algorithm>
#include <cctype>

#include "../Connector/SoftwareDefinedCard.hpp"
#include "Machine/SharpCatalogue.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

bool isSlot(const PlaceName& p) { return p.value == kSlot1 || p.value == kSlot2; }

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
    if (auto* m = dynamic_cast<const ModuleContent*>(req.content.get()))   // a module medium
        return std::make_unique<SoftwareDefinedCard>(m->definition);
    if (auto* def = dynamic_cast<const ModuleDefinitionData*>(d.productData.get())) {
        if (auto* rom = dynamic_cast<const SlotRomRequest*>(req.content.get())) {
            MemoryCardDefinition withRom = def->definition;
            std::string error;
            if (!replaceRomContent(&withRom, rom->bytes, &error)) return nullptr;   // the reader checked the size
            return std::make_unique<SoftwareDefinedCard>(std::move(withRom));
        }
        return std::make_unique<SoftwareDefinedCard>(def->definition);
    }
    return nullptr;
}

// ---- Emulated time and the power cycle ----

// With all checks off, the core's own path.
Ticks SharpMachine::runChecked(Ticks n) {
    SharpDebugging* d = sharpDebugging();
    if (d && d->checking()) return d->run(n);
    return runCore(n);
}

bool SharpMachine::stopPending() {
    SharpDebugging* d = sharpDebugging();
    return d && d->stopPending();
}

Ticks SharpMachine::advance(Ticks atLeast) {
    if (atLeast == 0) return 0;
    Ticks ran = 0;
    while (ran < atLeast && m_sequence.active()) {
        const Ticks chunk = std::min(atLeast - ran, m_sequence.remaining());
        const Ticks r = runChecked(chunk);
        ran += r;
        m_sequence.consumed(r, *this);
        if (r == 0 || stopPending()) return ran;   // a debugger stop
    }
    if (ran < atLeast) ran += runChecked(atLeast - ran);
    // Switched off by its own ROM (the `off` key, auto power-off): that is
    // an off point too, and the waiting changes reach the bus.
    if (!m_waiting.empty() && !m_sequence.active() && coreSwitchedOff()) makeWaitingChanges();
    return ran;
}

void SharpMachine::powerCycle() {
    if (m_sequence.active()) return;
    if (!running()) {
        // Off already (or never started): the change is made and the
        // machine stays as it is.
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
    // A module without a battery of its own forgets its RAM.
    for (const PlugEntry& e : m_plugged)
        if (e.place.steps.size() == 1 && isSlot(e.place.steps[0]))
            if (ExpansionCard* card = moduleIn(e.place.steps[0])) card->powerLoss();
    coreColdBoot();
}

bool SharpMachine::running() const { return tickCount() > 0 && !coreSwitchedOff(); }

void SharpMachine::makeWaitingChanges() {
    std::vector<std::function<void()>> changes = std::move(m_waiting);
    m_waiting.clear();
    m_waitingModules.clear();
    for (auto& c : changes) c();
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
    if (req.peripheral.value == kBusRom) {
        const auto* rom = dynamic_cast<const BusRomRequest*>(req.content.get());
        if (!rom) return std::string("A bus ROM is plugged in by a preset only.");
        if (rom->bank >= 0 && m_descriptor.model.value != kPC1600)
            return "The " + m_descriptor.model.value + " takes a bus ROM at an address, not in a bank.";
    }
    return modelRefusal(req, replacing);
}

AfterChange SharpMachine::followsNow(const PinOutDescriptor& p) const {
    return running() ? p.after : AfterChange::Nothing;   // switched off: nothing
}

void SharpMachine::plug(PeripheralRequest req, const PeripheralDescriptor& d, TickRate) {
    const PlaceName place = req.place.steps.at(0);
    if (!running()) makeWaitingChanges();   // off: at an off point, in order
    m_plugged.push_back(PlugEntry{req.place, req.peripheral, req.variant});
    if (isSlot(place) && !d.controls.empty()) {
        std::vector<SwitchPosition> positions = req.switches;
        if (positions.empty())
            if (auto* m = dynamic_cast<const ModuleCard*>(req.content.get()); m && m->card)
                positions.push_back({ControlId{"write-protect"}, m->card->writeProtected()});
        if (positions.empty())
            if (auto* m = dynamic_cast<const ModuleContent*>(req.content.get()))
                positions.push_back({ControlId{"write-protect"}, m->writeProtected()});
        makeSwitches(place, positions);
    }
    auto request = std::make_shared<PeripheralRequest>(std::move(req));
    auto descriptor = std::make_shared<PeripheralDescriptor>(d);
    if (isSlot(place) && running()) {
        // The card is made now, so the slot shows it before the off point.
        auto card = moduleCardOf(*request, d);
        m_waitingModules.emplace_back(place, card.get());
        request->content = std::make_shared<ModuleCard>(std::move(card));
    }
    auto change = [this, request, descriptor, place] {
        corePlug(*request, *descriptor);
        // The switch's position reaches the card it belongs to.
        if (auto* table = switchesAt(place))
            if (ExpansionCard* card = moduleNow(place)) card->setWriteProtected(table->on(ControlId{"write-protect"}));
    };
    if (running()) m_waiting.push_back(std::move(change));
    else change();
}

void SharpMachine::unplug(const PlacePath& path, const PeripheralId& id) {
    m_plugged.erase(std::remove_if(m_plugged.begin(), m_plugged.end(),
                                   [&](const PlugEntry& e) { return e.place == path && e.peripheral == id; }),
                    m_plugged.end());
    const PlaceName place = path.steps.at(0);
    if (!running()) makeWaitingChanges();   // off: at an off point, in order
    m_switches.erase(std::remove_if(m_switches.begin(), m_switches.end(), [&](const auto& s) { return s.first == place; }),
                     m_switches.end());
    if (isSlot(place) && running()) m_waitingModules.emplace_back(place, nullptr);
    auto change = [this, place, id] { coreUnplug(place, id); };
    if (running()) m_waiting.push_back(std::move(change));
    else change();
}

void SharpMachine::makeSwitches(const PlaceName& place, const std::vector<SwitchPosition>& positions) {
    auto table = std::make_unique<calcu::peripherals::ControlTable>(
        std::vector<ControlDescriptor>{{ControlId{"write-protect"}, ControlForm::Switch}},
        [this, place](const ControlId&, bool on) {
            if (ExpansionCard* card = moduleNow(place)) card->setWriteProtected(on);
        });
    table->restore(positions);
    m_switches.emplace_back(place, std::move(table));
}

ExpansionCard* SharpMachine::moduleNow(const PlaceName& place) {
    for (auto it = m_waitingModules.rbegin(); it != m_waitingModules.rend(); ++it)
        if (it->first == place) return it->second;
    return moduleIn(place);
}

calcu::peripherals::ControlTable* SharpMachine::switchesAt(const PlaceName& place) {
    for (auto& [p, t] : m_switches)
        if (p == place) return t.get();
    return nullptr;
}

// ---- Capabilities ----

SharpMachine::Offered SharpMachine::byComponent(const char* component, Capability& c) {
    return {CapabilityEntry{c.kind(), CapabilityId{std::string(component) + "/" + std::string(c.kind().value)},
                            ComponentOwner{ComponentId{component}}},
            &c};
}

SharpMachine::Offered SharpMachine::byPeripheral(const PlugEntry& e, Capability& c, const std::string& name) {
    return {CapabilityEntry{c.kind(), CapabilityId{placeText(e.place) + "/" + std::string(c.kind().value) + "/" + name},
                            PeripheralOwner{e.peripheral, e.place}},
            &c};
}

SharpMachine::Offered SharpMachine::byPeripheral(const PlugEntry& e, Capability& c) {
    return {CapabilityEntry{c.kind(), CapabilityId{placeText(e.place) + "/" + std::string(c.kind().value)},
                            PeripheralOwner{e.peripheral, e.place}},
            &c};
}

void SharpMachine::offerCommon(std::vector<Offered>& offered) {
    for (const PlugEntry& e : m_plugged) {
        const PlaceName& place = e.place.steps.at(0);
        if (auto* t = switchesAt(place)) offered.push_back(byPeripheral(e, *t));
        SlotModuleMedium* medium = place.value == kSlot1 ? &m_slotMedia[0] : place.value == kSlot2 ? &m_slotMedia[1] : nullptr;
        if (medium && medium->present()) offered.push_back(byPeripheral(e, *medium));   // a module medium
    }
}

std::vector<CapabilityEntry> SharpMachine::capabilities() const {
    std::vector<Offered> offered;
    auto* self = const_cast<SharpMachine*>(this);   // offer() hands out non-const pointers; nothing changes
    self->offer(offered, m_plugged);
    self->offerCommon(offered);
    std::vector<CapabilityEntry> out;
    for (const auto& o : offered) out.push_back(o.entry);
    return out;
}

Capability* SharpMachine::capability(const CapabilityId& id) {
    std::vector<Offered> offered;
    offer(offered, m_plugged);
    offerCommon(offered);
    for (const auto& o : offered)
        if (o.entry.id == id) return o.capability;
    return nullptr;
}

// ---- The machine state ----

template <class Io>
void SharpMachine::stateIo(Io& io) {
    io.begin("SHRP");
    bool waiting = !m_waiting.empty();
    io(waiting);
    if constexpr (Io::kRestoring)
        if (waiting)
            io.fail("The state was saved while a change of the peripherals waited for the power cycle; "
                    "save it again once the machine is ready.");
    m_sequence.stateIo(io);
    std::uint64_t switches = m_switches.size();
    io(switches);
    if constexpr (Io::kRestoring)
        if (switches != m_switches.size()) io.fail("the state has other write-protect switches than this machine");
    for (auto& [place, table] : m_switches) {
        bool on = table->on(ControlId{"write-protect"});
        io(on);
        if constexpr (Io::kRestoring) table->restore({SwitchPosition{ControlId{"write-protect"}, on}});
    }
    io.end();
    if constexpr (Io::kRestoring) coreRestoreState(io);
    else coreSaveState(io);
}

std::vector<std::uint8_t> SharpMachine::saveState() const {
    StateWriter w;
    const_cast<SharpMachine*>(this)->stateIo(w);   // only reads
    return w.take();
}

std::optional<std::string> SharpMachine::restoreState(const std::vector<std::uint8_t>& bytes) {
    std::vector<std::uint8_t> before = saveState();
    StateReader r(bytes);
    stateIo(r);
    if (r.ok() && !r.atEnd()) r.fail("the state has more records than this machine has parts");
    if (r.ok()) return std::nullopt;
    StateReader back(before);
    stateIo(back);   // the machine as it was (a refused restore changes nothing)
    std::string why = r.error();
    if (why.rfind("The ", 0) == 0) return why;   // a sentence of its own
    if (!why.empty() && why.back() != '.') why += '.';
    if (!why.empty()) why[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(why[0])));
    return "The state does not fit this machine: " + why;
}

} // namespace sharp
