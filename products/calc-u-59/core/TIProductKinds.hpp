#pragma once
#include "machine/Capability.hpp"
#include <array>
#include <cstdint>
#include <optional>

// Calc-U-59's product kinds of memory (capabilities.md, product kinds; C-3):
//  - `c59.program-memory` (C59-2 AC-17, design §3.2), capability
//    `RAM/c59.program-memory` on the TI-59, TI-58 and TI-58C: program steps,
//    data registers, the TI-58C's H00–H03, the partition and the raw RAM
//    registers, read and written between two advances;
//  - `c59.constant-memory` (C59-3 AC-1, design §3.1), capability
//    `RAM/c59.constant-memory` on the TI-58C only: the 64 raw registers of
//    its constant memory and a counter that changes with them.
// Declared in the core (the product's core holds its kinds, LIB-1 design
// §2a; the app reaches them through calcu::machine::Capability only); where a
// step, a register and the partition lie in the RAM stays the core's.

namespace cu59 {

/// A RAM register: 16 nibbles (app/Bcd.hpp's layout).
using RegisterNibbles = std::array<std::uint8_t, 16>;

class ProgramMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.program-memory"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual int lastStep() const = 0;                                // 959 (TI-59), 479 (TI-58/58C)
    virtual std::uint8_t readStep(int step) const = 0;               // keycode 00–99
    virtual void writeStep(int step, std::uint8_t keycode) = 0;
    virtual int dataRegisterCount() const = 0;                       // 100 (TI-59), 60 (TI-58/58C)
    virtual RegisterNibbles readRegister(int n) const = 0;           // data register NN, as STO/RCL number it
    virtual void writeRegister(int n, const RegisterNibbles&) = 0;
    virtual int extraCount() const = 0;                              // 4 on the TI-58C (H00–H03), else 0
    virtual RegisterNibbles readExtra(int n) const = 0;
    virtual void writeExtra(int n, const RegisterNibbles&) = 0;
    virtual int partitionSteps() const = 0;                          // steps of program memory (a multiple of 80)
    virtual void setPartition(int steps) = 0;                        // where the ROM keeps it (SCOM)
    virtual RegisterNibbles readRaw(int r) const = 0;                // raw RAM register R000…
    virtual int rawCount() const = 0;                                // 120
    /// The ROM's top of the data registers (BCD in SCOM[13]): R00 is raw
    /// register top - 1, Hnn on the TI-58C top + nn; 0 before the ROM booted.
    virtual int registerTop() const = 0;
};

class ConstantMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.constant-memory"};
    calcu::machine::KindId kind() const override { return kKind; }
    static constexpr int kRegisters = 64;   // R000–R063; H00–H03 are R060–R063

    virtual std::array<RegisterNibbles, kRegisters> content() const = 0;
    /// Changes whenever a nibble of R000–R063 changed since it was last read.
    virtual std::uint64_t changeCount() const = 0;
};

/// `c59.library-program` (C59-7 AC-21, design §3.6), capability
/// `TMC0501/c59.library-program` on every TI model: the number of the library
/// program of the Solid State ROM module that is selected now (`Pgm nn`), or
/// none, from the calculator's own registers.
class LibraryProgram : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.library-program"};
    calcu::machine::KindId kind() const override { return kKind; }
    virtual std::optional<int> selected() const = 0;
};

} // namespace cu59
