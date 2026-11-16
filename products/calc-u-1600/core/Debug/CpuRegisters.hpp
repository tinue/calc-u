#pragma once

#include <cstdint>
#include <string>
#include <vector>

class LH5801;
class SC7852;
struct LH5801HistoryFrame;
struct Z80HistoryFrame;

// Register tables of the two CPU families: the display list, name lookup
// (incl. 8-bit halves and flags) and history-frame conversion, and the
// types the disassembler, the expressions, the source map and the DAP's
// layer share (LIB-10 design §10.3: moved here from the old
// core/Debug/DebugTarget.hpp when the debugger became the library's
// machine::DebugTarget, core/Debug/SharpDebugging). Names are lower case.

namespace debug {

enum class CpuKind : uint8_t { LH5801, LH5803, Z80 };

/// Address spaces a memory reference can name: the Z-80's single memory
/// space and the LH580x's ME0 share 0; ME1 is 1 (same values as
/// WatchHit::space).
enum Space : uint8_t { kSpaceMain = 0, kSpaceME1 = 1 };

struct Register {
    std::string name;      ///< lower case, as the expression evaluator knows it
    uint32_t value = 0;
    uint8_t bits = 8;      ///< 1 (a flag or PU/PV), 8 or 16
    bool status = false;   ///< the status register: its low byte holds the flags (T / AF)
};

/// One line of a CPU's bank state, for display: {"Page B (4000-7FFF)",
/// "bank 3"}, {"PV", "1"}.
struct BankField {
    std::string name;
    std::string value;
};

/// One entry of a CPU's instruction history (see HistoryRing.hpp).
struct HistoryEntry {
    uint16_t pc = 0;            ///< the instruction's address
    uint8_t bytes[5] = {};
    uint8_t len = 0;            ///< 0 for an interrupt entry
    bool interrupt = false;
    std::vector<Register> registers; ///< before the instruction ran
};

/// Bank qualifier of a listing or breakpoint; -1 = don't care. `bank`: the
/// PC-1600 Z-80's page bank (0-7) at the address (vertical banks are not
/// modelled). `me`: LH580x ME0/ME1 -- code is always fetched from ME0.
/// `pu` / `pv`: the LH580x PU/PV flags.
struct BankKey {
    int bank = -1, me = -1, pu = -1, pv = -1;
    bool any() const { return bank >= 0 || me >= 0 || pu >= 0 || pv >= 0; }
    std::string describe() const; ///< "bank 3", "pv=1", "" when unqualified
};

/// Flag bit names of the status register, least significant first:
/// LH580x T = c ie z v h; Z-80 F = c n pv - h - z s (an empty name marks an
/// undocumented bit).
const std::vector<std::string>& flagNames(CpuKind kind);

/// The flags byte of a register list (live or history): the low byte of
/// the register marked `status`. False if there is none.
bool statusFlags(const std::vector<Register>& regs, uint8_t* flags);

std::vector<Register> lhRegisters(const LH5801& cpu);
bool lhReadRegister(const LH5801& cpu, const std::string& name, uint32_t* value);
bool lhWriteRegister(LH5801& cpu, const std::string& name, uint32_t value);
HistoryEntry lhHistoryEntry(const LH5801HistoryFrame& f);

std::vector<Register> z80Registers(const SC7852& cpu);
bool z80ReadRegister(const SC7852& cpu, const std::string& name, uint32_t* value);
bool z80WriteRegister(SC7852& cpu, const std::string& name, uint32_t value);
HistoryEntry z80HistoryEntry(const Z80HistoryFrame& f);

} // namespace debug
