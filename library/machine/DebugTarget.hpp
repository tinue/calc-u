#pragma once
#include "machine/Ids.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Debug targets: what a machine offers the run control for each processor
// it models. Part of the machine interface, so it names no processor and
// no model and stays on the standard library. Register and cell values are
// 64-bit unsigned.
//
// Everything is called on the session's thread, between two advances or
// from inside one (the observer); nothing holds a lock or throws.

namespace calcu::machine {

struct RegisterInfo {
    std::string name;
    int bits = 8;            // 1..64
    bool writable = true;
};

// Cells [first, first + count) of a space are reached by bank (and,
// with vbanks, by vertical bank) whichever is switched in.
struct BankWindow {
    std::uint64_t first = 0, count = 0;
    std::uint32_t banks = 0, vbanks = 0;   // vbanks 0: none
};

struct SpaceInfo {
    std::string name;                  // "memory", "io", "io-written"
    int cellBits = 8;                  // 1..64
    std::uint64_t size = 0;            // cells
    bool writtenView = false;          // "<space>-written", read-only, a cell may hold none
    std::vector<BankWindow> banked;
};

struct QualifierField {
    std::string name;                  // "bank"
    std::vector<std::string> values;   // "0".."3"
};

struct TargetOffers {
    bool watches = false, stepOverOut = false, decode = false;
    std::size_t history = 0;                   // entries kept; 0: no history
    std::vector<QualifierField> qualifier;     // empty: none
};

struct TargetInfo {
    DebugTargetId name;                        // as the descriptor names it
    std::vector<RegisterInfo> registers;
    std::vector<SpaceInfo> spaces;
    std::size_t codeSpace = 0;                 // index into spaces: what a breakpoint's address means
    TargetOffers offers;
};

struct BankSelect {
    std::optional<std::uint32_t> bank, vbank;  // none: as the processor sees it now
};

struct CellRead {
    enum Kind { Value, Unreadable, NoneWritten } kind = Value;
    std::uint64_t value = 0;
};

struct HistoryEntry {
    std::uint64_t address = 0;
    std::vector<std::uint64_t> registers;      // before the instruction, in the order of info().registers
    std::optional<std::string> text;
};

struct Decoded {
    std::string text;
    std::uint32_t cells = 1;
};

enum class Flow { Other, Call, Return };       // what step over and out need

struct WatchRange {
    std::size_t space = 0;
    std::uint64_t first = 0, count = 1;
    bool read = false, write = true;
};

class DebugTarget {
public:
    virtual const TargetInfo& info() const = 0;
    virtual std::uint64_t nextAddress() const = 0;
    virtual std::uint64_t readRegister(std::size_t) const = 0;                       // pre-execution
    virtual bool writeRegister(std::size_t, std::uint64_t) = 0;                      // false: refused, nothing changed
    virtual CellRead peek(std::size_t space, std::uint64_t address, BankSelect) const = 0;   // no effect
    virtual bool poke(std::size_t space, std::uint64_t address, std::uint64_t value, BankSelect) = 0;
    virtual std::vector<HistoryEntry> history(std::size_t count) const {   // newest first
        (void)count;
        return {};
    }
    virtual std::optional<Decoded> decode(std::uint64_t address) const {
        (void)address;
        return std::nullopt;
    }
    virtual std::vector<std::string> qualifierNow() const { return {}; }   // a value per field
    // A wanted qualifier holds "" for a field left free; the target checks it.
    virtual bool qualifierMatches(const std::vector<std::string>& wanted, std::uint64_t address) const {
        (void)wanted;
        (void)address;
        return true;
    }
    virtual Flow flowAt(std::uint64_t address) const {   // stepOverOut only
        (void)address;
        return Flow::Other;
    }
    virtual std::uint64_t callDepth() const { return 0; }   // grows with a call; stepOverOut only
    virtual std::uint64_t retired() const = 0;              // instructions carried out
    virtual void setBreakpoints(std::vector<std::uint64_t> addresses) = 0;   // checked only while Checks say so
    virtual bool setWatches(std::vector<WatchRange>) { return false; }       // false: no watches

protected:
    ~DebugTarget() = default;
};

// ── The machine's side ────────────────────────────────────────────

struct InstructionEvent {
    std::size_t target = 0;
    std::uint64_t address = 0, nextAddress = 0;
    std::uint32_t marks = 0;   // a product's facts about the instruction, passed on unread
};

class InstructionObserver {   // the run control
public:
    virtual bool stopAfter(const InstructionEvent&) = 0;   // true: the advance ends after it

protected:
    ~InstructionObserver() = default;
};

struct Checks {
    bool breakpoints = false, watches = false;
    InstructionObserver* observer = nullptr;
};

struct WatchHit {
    std::size_t space = 0;
    std::uint64_t address = 0;
    bool write = false;
};

struct DebugStop {
    enum Kind { Breakpoint, Watch, Observer } kind = Observer;
    std::size_t target = 0;
    std::uint64_t address = 0;        // Breakpoint: where the target stands
    std::vector<WatchHit> hits;       // Watch: every hit of the instruction
};

// A machine's debug targets with the checks of its advance.
// Machine::advance(atLeast) keeps its signature: while Checks are set it
// stops early, before an instruction at a breakpoint of its target, after
// an instruction that hit a watch, or after one for which the observer
// said stop, and returns the ticks run; the stop is read with takeStop().
// Fetching an instruction is no access for a watch.
class Debugging {
public:
    virtual std::size_t targetCount() const = 0;
    virtual DebugTarget& target(std::size_t) = 0;   // in the descriptor's order
    virtual const DebugTarget& target(std::size_t i) const = 0;
    virtual std::size_t current() const = 0;        // the target that carries out the next instruction
    virtual void setChecks(const Checks&) = 0;      // read by advance(); all off: the fast path
    virtual std::optional<DebugStop> takeStop() = 0;   // why the last advance ended early; clears it
    // Its breakpoint where it stands is passed once; a pass another target
    // had is dropped.
    virtual void passOnce(std::size_t target) = 0;

protected:
    ~Debugging() = default;
};

} // namespace calcu::machine
