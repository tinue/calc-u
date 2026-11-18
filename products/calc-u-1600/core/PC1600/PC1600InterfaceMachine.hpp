#pragma once
#include <memory>
#include <string>

#include "../Machine/SharpMachine.hpp"
#include "../Machine/SharpProductKinds.hpp"
#include "../Serial/SerialPortCapability.hpp"
#include "PC1600Machine.hpp"

// A PC-1600 behind the machine interface. It owns the
// core's PC1600Machine; above the core it is a calcu::machine::Machine.
// Ticks are SC7852 T-states at 3,580,000 per second; the LH5803 and
// everything else are counted in them. An advance runs whole steps
// of whichever CPU has the bus, and steps of 256 while the machine
// is switched off. Built by SharpMachineBuilder.
class PC1600InterfaceMachine final : public sharp::SharpMachine {
public:
    PC1600InterfaceMachine(const calcu::machine::MachineDescriptor&, const calcu::machine::VariantId&);
    ~PC1600InterfaceMachine() override;

    PC1600Machine& core() { return *m_core; }
    const PC1600Machine& core() const { return *m_core; }

    calcu::machine::Ticks tickCount() const override { return m_core->elapsedTStates(); }
    calcu::machine::DisplayOutput readDisplay() override;
    calcu::machine::Capability* capability(const calcu::machine::CapabilityId&) override;
    using SharpMachine::capability;
    std::size_t takeSound(std::span<std::int16_t> out) override { return m_core->drainAudio(out.data(), out.size()); }

private:
    calcu::machine::Ticks runCore(calcu::machine::Ticks n) override { return m_core->runCycles(n); }
    sharp::SharpDebugging* sharpDebugging() override { return m_debugging.get(); }
    bool coreSwitchedOff() const override { return m_core->isPoweredOff(); }
    bool coreKeyReady() const override { return m_core->keyReady(); }
    void coreKey(const std::string& id, bool down) override;
    void coreOnKey(bool down) override { m_core->setOnKeyPressed(down); }
    void coreReset() override { m_core->reset(); }
    void coreColdBoot() override { m_core->allReset(); }
    ExpansionCard* moduleIn(const calcu::machine::PlaceName&) override;
    void corePlug(const calcu::machine::PeripheralRequest&, const calcu::machine::PeripheralDescriptor&) override;
    void coreUnplug(const calcu::machine::PlaceName&, const calcu::machine::PeripheralId&) override;
    void offer(std::vector<Offered>&, const std::vector<calcu::machine::PlugEntry>&) override;
    void coreSaveState(sharp::StateWriter&) const override;
    void coreRestoreState(sharp::StateReader&) override;
    template <class Io>
    void interfaceStateIo(Io&);
    std::optional<std::string> modelRefusal(const calcu::machine::PeripheralRequest&,
                                            const std::optional<calcu::machine::PeripheralId>&) const override;

    struct Kinds;

    std::unique_ptr<PC1600Machine> m_core;
    SerialPortCapability m_rs232c;      // the TC8576F's peer (component rs232c)
    SerialPortCapability m_ce158Port;   // the CE-158's UART peer; kept across detach and attach
    std::unique_ptr<Kinds> m_kinds;
    std::unique_ptr<sharp::SharpDebugging> m_debugging;   // the debugger: after the core, gone before it
};
