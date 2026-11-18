#pragma once
#include "PC100C.hpp"
#include "SolidStateRomModule.hpp"
#include "TI59Machine.hpp"
#include "TITypingSupport.hpp"
#include "TICardReaderPlace.hpp"
#include "TIDebugKindsImpl.hpp"
#include "TIDebugging.hpp"
#include "TIMachineState.hpp"
#include "TIMemoryKinds.hpp"
#include "TICatalogue.hpp"
#include "TIRomRanges.hpp"
#include "TIRomText.hpp"
#include "machine/Machine.hpp"
#include <memory>
#include <optional>
#include <vector>

// A TI-59, TI-58 or TI-58C behind the machine interface.
// The only class above the core is calcu::machine::Machine: build one with
// calcu::machine::build(tiCatalogue(), TIMachineBuilder, request). It owns the
// core-internal TI59Machine, the tick count that goes on across every
// operation, the Reset restore of the TI-59/58, the peripherals plugged into
// its places `printer` (the PC-100C) and `slot-1` (a Solid State ROM module),
// and its capabilities (the product kinds, the debug target, the
// typing support). It holds no lock and starts no
// thread.
//
// Ticks: one instruction is 32 ticks, 128 in the ROM's IDLE state on the
// TI-59 and TI-58 (TMC0501's step weight × 32).

class TIMachine final : public calcu::machine::Machine {
public:
    /// Build order: the model with its defaults and zeroed
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
    /// Always `TMC0501/typing-support`,
    /// `TMC0501/c59.trace-records` and `RAM/c59.program-memory`;
    /// on the TI-58C `RAM/c59.constant-memory`; with the PC-100C:
    /// the debug panel's kinds `TMC0501/c59.asm-overlay`, `TMC0501/c59.rom-heatmap`,
    /// `TMC0501/c59.calculator-state`, `TMC0501/c59.machine-log`;
    /// `printer/controls` (PRINT, ADV, TRACE) and
    /// `printer/paper-output`; always
    /// `TMC0501/c59.library-program`; on the TI-59 the card
    /// reader's media place `card-reader/media-place`.
    std::vector<calcu::machine::CapabilityEntry> capabilities() const override;
    calcu::machine::Capability* capability(const calcu::machine::CapabilityId&) override;
    using calcu::machine::Machine::capability;
    std::vector<calcu::machine::DebugTargetId> debugTargets() const override { return m_descriptor.debugTargets; }
    /// The one target TMC0501 (core/TIDebugging.hpp).
    calcu::machine::Debugging* debugging() override { return &m_debugging; }

    /// The PC-100C's character buffer, not yet printed (debug view; empty
    /// without a PC-100C).
    std::string printerBufferContent() const { return m_printer ? m_printer->bufferContent() : std::string(); }

protected:
    // The PC-100C into `printer`, a module into `slot-1`.
    // What follows is the pin-out's power cycle, which the
    // caller runs; a second module is refused by the library's attach
    // (rule 6, library/machine/Attach.cpp).
    void plug(calcu::machine::PeripheralRequest, const calcu::machine::PeripheralDescriptor&,
              calcu::machine::TickRate hostRate) override;
    void unplug(const calcu::machine::PlacePath&, const calcu::machine::PeripheralId&) override;

private:
    friend class TICardReaderPlace;
    friend class TILibraryProgram;
    friend class TIDebugTarget;
    friend class TIDebugging;
    friend class TITraceRecords;
    friend class TIAsmOverlay;
    friend class TICalculatorState;
    friend class TIMachineLog;
    friend class TITypingSupport;
    friend class TIProgramMemory;
    friend class TIConstantMemoryCapability;
    friend struct TIMachineTesting;
    friend class TIMachineState;

    /// The machine state (core/TIMachineState.cpp): the records of the machine's
    /// parts, both ways; a refused restore puts the state from before back.
    template <class Io>
    void stateIo(Io& io);
    std::vector<std::uint8_t> saveState();
    std::optional<std::string> restoreState(const std::vector<std::uint8_t>&);

    /// One instruction; then the pending Reset restore. Every path that runs an instruction goes through here.
    std::uint32_t stepOne();
    /// The processor reset (TI59Machine::reset), with the tick count carried;
    /// the peripherals are told of it.
    void cpuReset();
    /// The connectors to the CPU: the TI-58C has the printer's D0 pad on digit 10.
    void rewire();
    /// The core's frame ring is on while the debug panel, a trace or the run
    /// control's checks want it (TRACE_PC | TRACE_REGS_FULL), else off.
    void applyTraceFlags();

    struct SavedMemory {
        std::vector<std::uint8_t> ram;
        int partitionProgramRegs;
    };

    const calcu::machine::MachineDescriptor m_descriptor;
    const MachineVariant m_variant;
    const TIRomRanges m_ranges;
    TI59Machine m_hw;
    calcu::machine::Ticks m_ticksBeforeReset = 0;
    std::optional<SavedMemory> m_restore;   ///< RAM and partition until key readiness
    std::uint64_t m_retired = 0;          ///< instructions carried out (DebugTarget::retired)
    bool m_panelTrace = false;            ///< the debug panel wants the frame ring (c59.calculator-state)
    bool m_recordTrace = false;           ///< a trace runs (c59.trace-records)
    bool m_checksTrace = false;           ///< the run control's checks are set

    std::unique_ptr<PC100C> m_printer;
    bool m_recordCodeLines = false;   ///< the PC-100C keeps its code lines (TIMachineTesting)
    std::unique_ptr<SolidStateRomModule> m_module;
    calcu::machine::PeripheralId m_moduleId;
    std::unique_ptr<TICardReaderPlace> m_cardReader;   ///< the TI-59 only
    TIAsmOverlay m_asmOverlay{*this};
    TIRomHeatmap m_heatmap{*this};
    TICalculatorState m_calculatorState{*this};
    TIMachineLog m_machineLog{*this};
    TIDebugging m_debugging{*this};
    TITraceRecords m_traceRecords{*this};
    TITypingSupport m_typing{*this};
    TIProgramMemory m_programMemory{*this};
    TIConstantMemoryCapability m_constantMemory{*this};   ///< TI-58C
    TILibraryProgram m_libraryProgram{*this};
    TIMachineState m_machineState{*this};
};
