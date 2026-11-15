#pragma once
#include "PC100C.hpp"
#include "SolidStateRomModule.hpp"
#include "TI59Machine.hpp"
#include "TIAdapters.hpp"
#include "TICatalogue.hpp"
#include "TIRomRanges.hpp"
#include "TIRomText.hpp"
#include "machine/Machine.hpp"
#include <memory>
#include <optional>
#include <vector>

// A TI-59, TI-58 or TI-58C behind the machine interface (C59-1; design §3.1).
// The only class above the core is calcu::machine::Machine: build one with
// calcu::machine::build(tiCatalogue(), TIMachineBuilder, request). It owns the
// core-internal TI59Machine, the tick count that goes on across every
// operation, the Reset restore of the TI-59/58, the peripherals plugged into
// its places `printer` (the PC-100C) and `slot-1` (a Solid State ROM module;
// C59-6), and the adapters (TIAdapters.hpp). It holds no lock and starts no
// thread (ADR 0009).
//
// Ticks: one instruction is 32 ticks, 128 in the ROM's IDLE state on the
// TI-59 and TI-58 (TMC0501's step weight × 32).

class TIMachine final : public calcu::machine::Machine {
public:
    /// Build order (C59-1 AC-6, AC-7): the model with its defaults and zeroed
    /// memory; then `content` into the RAM (a TI-58C); then the processor
    /// reset, so the ROM's start-up runs in the advances that follow.
    TIMachine(const calcu::machine::MachineDescriptor&, MachineVariant, const TIRoms& roms,
              std::shared_ptr<const TIConstantMemory> content);

    const calcu::machine::MachineDescriptor& descriptor() const override { return m_descriptor; }
    std::optional<calcu::machine::VariantId> variant() const override { return std::nullopt; }
    calcu::machine::Ticks advance(calcu::machine::Ticks atLeast) override;
    calcu::machine::Ticks tickCount() const override;
    void reset() override;
    void powerCycle() override;
    void coldBoot() override;
    bool isKeyReady() const override;
    calcu::machine::KeyCall keyDown(const calcu::machine::KeyId&) override;
    calcu::machine::KeyCall keyUp(const calcu::machine::KeyId&) override;
    calcu::machine::DisplayOutput readDisplay() override;
    std::size_t takeSound(std::span<std::int16_t>) override { return 0; }
    calcu::machine::Plugged plugged() const override;
    /// With the PC-100C: `printer/controls` (PRINT, ADV, TRACE) and
    /// `printer/c59.printer-output` (C59-6 AC-1, AC-5; LIB-2 AC-26). The card
    /// reader's media place comes in M-55 (LIB-1 D-1).
    std::vector<calcu::machine::CapabilityEntry> capabilities() const override;
    calcu::machine::Capability* capability(const calcu::machine::CapabilityId&) override;
    std::vector<calcu::machine::DebugTargetId> debugTargets() const override { return m_descriptor.debugTargets; }

    /// The PC-100C's character buffer, not yet printed (debug view; empty
    /// without a PC-100C).
    std::string printerBufferContent() const { return m_printer ? m_printer->bufferContent() : std::string(); }

protected:
    // LIB-2 design §3.1: the PC-100C into `printer`, a module into `slot-1`.
    // What follows is the pin-out's power cycle (LIB-2 AC-24), which the
    // caller runs; a second module is refused by the library's rule 6 (AC-25).
    void plug(calcu::machine::PeripheralRequest, const calcu::machine::PeripheralDescriptor&,
              calcu::machine::TickRate hostRate) override;
    void unplug(const calcu::machine::PlacePath&, const calcu::machine::PeripheralId&) override;

private:
    friend class TICardReaderAdapter;
    friend class TIMemoryAdapter;
    friend class TIDebugTargetAdapter;
    friend class TIDebugPanelAdapter;
    friend struct TIMachineTesting;
    friend TICardReaderAdapter* cardReaderOf(calcu::machine::Machine&);
    friend TIMemoryAdapter* memoryOf(calcu::machine::Machine&);
    friend TIDebugTargetAdapter* debugTargetOf(calcu::machine::Machine&);
    friend TIDebugPanelAdapter* debugPanelOf(calcu::machine::Machine&);

    /// One instruction; then the pending Reset restore. Every path that runs an instruction goes through here.
    std::uint32_t stepOne();
    /// The processor reset (TI59Machine::reset), with the tick count carried;
    /// the peripherals are told of it (LIB-2 AC-10).
    void cpuReset();
    /// The connectors to the CPU: the TI-58C has the printer's D0 pad on digit 10.
    void rewire();

    struct SavedMemory {
        std::vector<std::uint8_t> ram;
        int partitionProgramRegs;
    };

    const calcu::machine::MachineDescriptor m_descriptor;
    const MachineVariant m_variant;
    const TIRomRanges m_ranges;
    TI59Machine m_hw;
    calcu::machine::Ticks m_ticksBeforeReset = 0;
    std::optional<SavedMemory> m_restore;   ///< AC-19: RAM and partition until key readiness
    TIStepObserver* m_observer = nullptr;
    TIStop m_stop = TIStop::None;

    std::unique_ptr<PC100C> m_printer;
    std::unique_ptr<SolidStateRomModule> m_module;
    calcu::machine::PeripheralId m_moduleId;
    TICardReaderAdapter m_cardReader{*this};
    TIMemoryAdapter m_memory{*this};
    TIDebugTargetAdapter m_debugTarget{*this};
    TIDebugPanelAdapter m_debugPanel{*this};
};
