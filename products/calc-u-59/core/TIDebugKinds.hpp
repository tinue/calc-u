#pragma once
#include "TraceTypes.hpp"
#include "machine/Capability.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

// Calc-U-59's product kinds of the debug panel (C59-4 design §2, §3.2 to
// §3.5; capabilities.md, product kinds), all owned by the processor
// (`TMC0501/c59.…`) on the TI-59, TI-58 and TI-58C. They replace the debug
// panel adapter of C59-1 (TIDebugPanelAdapter, gone in M-56): what the
// generic debug target (core/TIDebugging.hpp, LIB-10 AC-28) does not give.
//  - `c59.asm-overlay` (AC-13): up to 2048 13-bit instructions over the ROM
//    addresses 1800 to 1FFF, taken away again, entered at 1800 with COND set;
//    it runs from a successful entry until a Reset, a Cold boot or clear().
//    A Cold boot also takes the instructions away (AC-26).
//  - `c59.rom-heatmap` (AC-8, D-4): while on, how often each ROM address
//    0000 to 13FF was carried out; one increment per instruction.
//  - `c59.calculator-state` (AC-3, AC-4, AC-6, D-3): the frame latched at the
//    last program step boundary, the bytes of the ROM's own programs, the
//    processor frame now and the frame ring the CPU tab shows (its frames
//    hold what the target names no register for: Sout, the display state).
//  - `c59.machine-log` (AC-11): the machine's own log lines at their level
//    (1 info, 2 debug) and the level set (0 off).
// Read and called between two advances, on the session's thread.

namespace cu59 {

class AsmOverlay : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.asm-overlay"};
    calcu::machine::KindId kind() const override { return kKind; }
    static constexpr std::size_t kMaxInstructions = 2048;   // 1800 to 1FFF
    static constexpr std::uint16_t kEntry = 0x1800;

    /// false: more than 2048, nothing changed.
    virtual bool load(const std::vector<std::uint16_t>& words) = 0;
    virtual void clear() = 0;
    virtual std::size_t loaded() const = 0;
    struct Entry {
        bool entered = false;
        std::uint32_t instructions = 0;
    };
    /// To the next program step boundary (at most 50000 weighted cycles),
    /// then on at 1800 with COND set, within `maxInstructions` (AC-17).
    virtual Entry enter(std::uint32_t maxInstructions) = 0;
    virtual bool running() const = 0;
};

class RomHeatmapCounter : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.rom-heatmap"};
    calcu::machine::KindId kind() const override { return kKind; }
    static constexpr std::size_t kAddresses = 0x1400;

    virtual void setOn(bool) = 0;
    virtual bool on() const = 0;
    virtual const std::vector<std::uint32_t>& counts() const = 0;   // kAddresses
    virtual void reset() = 0;
};

class CalculatorState : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.calculator-state"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::uint16_t pc() const = 0;
    virtual CpuFrame snapshotCPU() const = 0;
    /// The latched module byte address of the running module program (0xFFFF: none; M-55, was the card reader adapter's).
    virtual std::uint16_t libExecPC() const = 0;
    virtual TIStepBoundary lastStepBoundary() const = 0;
    /// A byte of the ROM's own programs, 0 to 383.
    virtual std::uint8_t readROMKeycode(int addr) const = 0;
    /// The newest `max` frames of the ring, oldest first; not consumed.
    virtual std::uint32_t readCpuFrames(CpuFrame* out, std::uint32_t max) const = 0;
    /// The ring on for the panel (while the CPU tab shows, paused, a start armed).
    virtual void setFrameRing(bool on) = 0;
};

class MachineLog : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.machine-log"};
    calcu::machine::KindId kind() const override { return kKind; }

    struct Line {
        int level;   // 1 info, 2 debug
        std::string text;
    };
    virtual void setLevel(int level) = 0;   // 0 off, 1 info, 2 debug
    virtual int level() const = 0;
    virtual std::vector<Line> take() = 0;
};

/// A 16-nibble register of the TI's BCD layout as a number (the panel's reads).
double decodeRegister(const std::uint8_t* nibbles16);

} // namespace cu59
