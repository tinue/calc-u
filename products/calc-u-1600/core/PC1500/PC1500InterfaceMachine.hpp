#pragma once
#include <memory>

#include "../Machine/SharpMachine.hpp"
#include "../Machine/SharpProductKinds.hpp"
#include "../Serial/SerialPortCapability.hpp"
#include "PC1500Machine.hpp"
#include "peripherals/RealTimeClock.hpp"

// A PC-1500 or PC-1500A behind the machine interface (C1600-1 design
// §3.2). It owns the core's PC1500Machine, which keeps its name, its tests
// and its callers inside the core; above the core it is a
// calcu::machine::Machine. Ticks are LH5801 cycles at 1,300,000 per second
// (AC-1). Built by SharpMachineBuilder.
class PC1500InterfaceMachine final : public sharp::SharpMachine {
public:
    PC1500InterfaceMachine(const calcu::machine::MachineDescriptor&, const calcu::machine::VariantId&);

    PC1500Machine& core() { return *m_core; }
    const PC1500Machine& core() const { return *m_core; }

    calcu::machine::Ticks tickCount() const override { return m_core->elapsedCycles(); }
    calcu::machine::DisplayOutput readDisplay() override;
    std::size_t takeSound(std::span<std::int16_t> out) override { return m_core->drainAudio(out.data(), out.size()); }

private:
    calcu::machine::Ticks runCore(calcu::machine::Ticks n) override { return m_core->runCycles(n); }
    sharp::SharpDebugging* sharpDebugging() override { return m_debugging.get(); }
    bool coreSwitchedOff() const override { return m_core->cpu().poweredOff(); }
    bool coreKeyReady() const override { return m_core->keyReady(); }
    void coreKey(const std::string& id, bool down) override;
    void coreOnKey(bool down) override { m_core->setOnKeyPressed(down); }
    void coreReset() override { m_core->reset(); }
    void coreColdBoot() override { m_core->allReset(); }
    ExpansionCard* moduleIn(const calcu::machine::PlaceName&) override;
    void corePlug(const calcu::machine::PeripheralRequest&, const calcu::machine::PeripheralDescriptor&) override;
    void coreUnplug(const calcu::machine::PlaceName&, const calcu::machine::PeripheralId&) override;
    std::vector<calcu::machine::PlugEntry> coreHasPlugged() const override;
    void offer(std::vector<Offered>&, const std::vector<calcu::machine::PlugEntry>&) override;

    struct Kinds;   // the capability objects (PC1500InterfaceMachine.cpp)

    std::unique_ptr<PC1500Machine> m_core;
    SerialPortCapability m_ce158Port;   // the CE-158's UART peer; kept across detach and attach, as today
    std::unique_ptr<Kinds> m_kinds;
    std::unique_ptr<sharp::SharpDebugging> m_debugging;   // LIB-10: after the core, gone before it

public:
    ~PC1500InterfaceMachine() override;
};
