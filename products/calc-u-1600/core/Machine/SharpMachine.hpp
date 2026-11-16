#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../Connector/ExpansionCard.hpp"
#include "../Debug/SharpDebugging.hpp"
#include "Machine/PowerCycleSequence.hpp"
#include "Machine/SharpMediaPlaces.hpp"
#include "Machine/StateRecords.hpp"
#include "harness/MachineState.hpp"
#include "machine/Machine.hpp"
#include "machine/ProductData.hpp"
#include "peripherals/Controls.hpp"

// What the two Sharp machines behind the machine interface share (C1600-1
// design §3.2-§3.4): the tick count, the power cycle sequence, the keys,
// places and attach, the capability list. PC1500InterfaceMachine and
// PC1600InterfaceMachine fill in the model's part over the old class each
// owns. Nothing above the core names any of these classes: they are
// reached as calcu::machine::Machine (C1600-1, Technical Requirements).
//
// Attach on a running machine (design D-2): plugged() and the capabilities
// show the change at once; the bus changes at the off point of the power
// cycle that follows, as today's powerCycleAround did. On a machine that is
// switched off, or not yet started (just built), the change is made at
// once and nothing follows. A reset or cold boot makes a waiting change
// first.

namespace sharp {

// A module handed in with its card already built (the module bay's own
// instance, read from its file): PeripheralRequest::content. The card is
// taken out when the module is plugged in.
class ModuleCard final : public calcu::machine::ProductData {
public:
    explicit ModuleCard(std::unique_ptr<ExpansionCard> c) : card(std::move(c)) {}
    mutable std::unique_ptr<ExpansionCard> card;
};

// The host drive's folder (UTF-8): PeripheralRequest::content of the host drive.
class HostDriveRequest final : public calcu::machine::ProductData {
public:
    explicit HostDriveRequest(std::string f) : folder(std::move(f)) {}
    std::string folder;
};

// A ROM on the 60-pin connector (C1600-4 AC-9, AC-10): the content of a
// request for the peripheral bus-rom at the place bus-rom. `bank` (4-7, the
// PC-1600's system bus) or `address` (the PC-1500's connector or the
// PC-1600's LH5803 side) with ME1 and the PV/PU gates (-1: either).
class BusRomRequest final : public calcu::machine::ProductData {
public:
    std::vector<std::uint8_t> bytes;
    int bank = -1;
    std::uint16_t address = 0;
    bool me1 = false;
    int pv = -1;
    int pu = -1;
    std::string name;   // the file's name, shown in the inspector
};

// The bytes that replace a ROM module's ROM (C1600-4 AC-8, `slot-N-rom:`):
// the content of a module request; the card is built from the module's
// definition with these bytes (moduleCardOf).
class SlotRomRequest final : public calcu::machine::ProductData {
public:
    explicit SlotRomRequest(std::vector<std::uint8_t> b) : bytes(std::move(b)) {}
    std::vector<std::uint8_t> bytes;
};

// The image of the `index`-th ROM entry of the request's variant, or nullptr.
const std::vector<std::uint8_t>* romImageOf(const calcu::machine::PeripheralRequest&,
                                            const calcu::machine::PeripheralDescriptor&, std::size_t index);
// The card of a module request: the one handed in (ModuleCard), or one
// built from the descriptor's module definition; nullptr if neither.
std::unique_ptr<ExpansionCard> moduleCardOf(const calcu::machine::PeripheralRequest&,
                                            const calcu::machine::PeripheralDescriptor&);

class SharpMachine : public calcu::machine::Machine, private PowerCycleSequence::Actions {
public:
    const calcu::machine::MachineDescriptor& descriptor() const override { return m_descriptor; }
    std::optional<calcu::machine::VariantId> variant() const override { return m_variant; }
    calcu::machine::Ticks advance(calcu::machine::Ticks atLeast) override;
    void reset() override;
    void powerCycle() override;
    void coldBoot() override;
    bool isKeyReady() const override { return !m_sequence.active() && coreKeyReady(); }
    calcu::machine::KeyCall keyDown(const calcu::machine::KeyId&) override;
    calcu::machine::KeyCall keyUp(const calcu::machine::KeyId&) override;
    calcu::machine::Plugged plugged() const override;
    std::vector<calcu::machine::CapabilityEntry> capabilities() const override;
    calcu::machine::Capability* capability(const calcu::machine::CapabilityId&) override;
    std::vector<calcu::machine::DebugTargetId> debugTargets() const override { return m_descriptor.debugTargets; }
    // LIB-10 (design §10.4, §10.5): the model's debug targets; advance()
    // honours their Checks.
    calcu::machine::Debugging* debugging() override { return sharpDebugging(); }
    virtual SharpDebugging* sharpDebugging() = 0;

    // ---- The machine state (C1600-5 AC-13; LIB-11 AC-24) ----
    // The records of the whole machine (StateRecords.hpp): the power cycle
    // under way, the write-protect switches, then the model's (the core's
    // parts, the serial ports' lines, the cassette place). Saving only
    // reads. A restore that does not fit puts the state from before back
    // and gives the reason; a state saved while a change waited for the
    // off point of a power cycle (an attach on a running machine) is
    // refused, since the change is not a part of the machine.
    std::vector<std::uint8_t> saveState() const;
    std::optional<std::string> restoreState(const std::vector<std::uint8_t>&);

    // ---- For the core and its tests (tests/BootRun.hpp, SharpTestSupport.hpp) ----
    bool powerCycleActive() const { return m_sequence.active(); }
    // Ticks to the sequence's next phase change: an advance of exactly this
    // runs the phase as one run of the core, as today's job did.
    calcu::machine::Ticks powerCyclePhaseTicks() const { return m_sequence.remaining(); }
    // Runs `f` at the off point of the coming power cycle, after its
    // changes, or now when no change waits.
    void atOffPoint(std::function<void()> f);
    // The module in a slot as plugged() shows it: one that waits for the off
    // point already, one that waits to come out no longer (design D-2).
    ExpansionCard* moduleNow(const calcu::machine::PlaceName&);

protected:
    SharpMachine(calcu::machine::MachineDescriptor d, std::optional<calcu::machine::VariantId> v)
        : m_descriptor(std::move(d)), m_variant(std::move(v)),
          m_slotMedia{SlotModuleMedium([this] { return moduleNow(calcu::machine::PlaceName{"slot-1"}); }),
                      SlotModuleMedium([this] { return moduleNow(calcu::machine::PlaceName{"slot-2"}); })} {}

    // The machine-state capability (LIB-11 AC-24), offered by the model on
    // its main processor.
    calcu::machine::Capability& stateCapability() { return m_stateKind; }
    // The model's records (saveState()).
    virtual void coreSaveState(StateWriter&) const = 0;
    virtual void coreRestoreState(StateReader&) = 0;

    // ---- The model's part ----
    virtual calcu::machine::Ticks runCore(calcu::machine::Ticks) = 0;   // the core's own path, all checks off
    virtual bool coreSwitchedOff() const = 0;
    virtual bool coreKeyReady() const = 0;
    virtual void coreKey(const std::string& keyId, bool down) = 0;   // a key of the matrix
    virtual void coreOnKey(bool down) = 0;
    virtual void coreReset() = 0;
    virtual void coreColdBoot() = 0;   // the machine's own memory empty
    // The card in a module slot, or nullptr.
    virtual ExpansionCard* moduleIn(const calcu::machine::PlaceName&) = 0;
    // The electrical change on the core.
    virtual void corePlug(const calcu::machine::PeripheralRequest&, const calcu::machine::PeripheralDescriptor&) = 0;
    virtual void coreUnplug(const calcu::machine::PlaceName&, const calcu::machine::PeripheralId&) = 0;
    // The capabilities of the model, present now (`plugged` is what plugged() gives).
    struct Offered {
        calcu::machine::CapabilityEntry entry;
        calcu::machine::Capability* capability;
    };
    virtual void offer(std::vector<Offered>& out, const std::vector<calcu::machine::PlugEntry>& plugged) = 0;
    // The rule of a product kind that is the model's own (CE-1600P with CE-158).
    virtual std::optional<std::string> modelRefusal(const calcu::machine::PeripheralRequest&,
                                                    const std::optional<calcu::machine::PeripheralId>&) const {
        return std::nullopt;
    }

    // ---- calcu::machine::Machine hooks (SharpPlaces.cpp) ----
    std::optional<std::string> productRefusal(const calcu::machine::PeripheralRequest&,
                                              const std::optional<calcu::machine::PeripheralId>&) const override;
    void plug(calcu::machine::PeripheralRequest, const calcu::machine::PeripheralDescriptor&,
              calcu::machine::TickRate) override;
    void unplug(const calcu::machine::PlacePath&, const calcu::machine::PeripheralId&) override;
    calcu::machine::AfterChange followsNow(const calcu::machine::PinOutDescriptor&) const override;

    // A peripheral plugged in at `place` now (by plugged()), or nullptr.
    const calcu::machine::PlugEntry* at(const char* place) const;
    // A capability owned by a component: ID "<component>/<kind>".
    static Offered byComponent(const char* component, calcu::machine::Capability& c);
    // A capability owned by the peripheral at `place`: ID "<place>/<kind>".
    static Offered byPeripheral(const calcu::machine::PlugEntry& e, calcu::machine::Capability& c);
    // One of several of a kind there (the media places): ID "<place>/<kind>/<name>".
    static Offered byPeripheral(const calcu::machine::PlugEntry& e, calcu::machine::Capability& c, const std::string& name);
    // The write-protect switch of the module at a slot (LIB-2 AC-32), made
    // when the module is plugged in.
    calcu::peripherals::ControlTable* switchesAt(const calcu::machine::PlaceName&);

private:
    // runCore(), or the debug targets' run while a check is on (it ends
    // early at a stop: stopPending()).
    calcu::machine::Ticks runChecked(calcu::machine::Ticks);
    bool stopPending();
    // The machine has run and is not switched off: a change waits for the
    // off point and a power cycle follows.
    bool running() const;
    void makeWaitingChanges();
    void makeSwitches(const calcu::machine::PlaceName&, const std::vector<calcu::machine::SwitchPosition>&);

    // PowerCycleSequence::Actions
    void offKey(bool down) override { coreKey("off", down); }
    void onKey(bool down) override { coreOnKey(down); }
    bool switchedOff() const override { return coreSwitchedOff(); }
    void atOffPoint() override { makeWaitingChanges(); }

    const calcu::machine::MachineDescriptor m_descriptor;
    const std::optional<calcu::machine::VariantId> m_variant;
    PowerCycleSequence m_sequence;
    std::vector<calcu::machine::PlugEntry> m_plugged;   // what plugged() gives, by place
    std::vector<std::function<void()>> m_waiting;       // changes for the off point (D-2)
    std::vector<std::function<void()>> m_afterWaiting;  // atOffPoint()
    // The modules of the waiting changes, by slot (nullptr: one comes out).
    std::vector<std::pair<calcu::machine::PlaceName, ExpansionCard*>> m_waitingModules;
    std::vector<std::pair<calcu::machine::PlaceName, std::unique_ptr<calcu::peripherals::ControlTable>>> m_switches;
    // LIB-7 D-1: a battery-backed module in slot 1 or 2 is a module medium.
    SlotModuleMedium m_slotMedia[2];
    // The capabilities every Sharp machine offers besides the model's: the
    // write-protect switches and the module media.
    void offerCommon(std::vector<Offered>&);

    class StateKind final : public calcu::harness::MachineState {
    public:
        explicit StateKind(SharpMachine& m) : m_machine(m) {}
        std::vector<std::uint8_t> save() const override { return m_machine.saveState(); }
        std::optional<std::string> restore(const std::vector<std::uint8_t>& b) override { return m_machine.restoreState(b); }

    private:
        SharpMachine& m_machine;
    };
    StateKind m_stateKind{*this};
    template <class Io>
    void stateIo(Io& io);
};

} // namespace sharp
