#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../CPU/DebugStop.hpp"
#include "../CPU/WatchSet.hpp"
#include "CpuRegisters.hpp"
#include "CpuViews.hpp"
#include "DebugExpression.hpp"
#include "Disasm/Disassembly.hpp"
#include "machine/DebugTarget.hpp"
#include "machine/Machine.hpp"

class PC1500Machine;
class PC1600Machine;

// ── The Sharp machines' debug targets ─────────────────────────────────────
//
// One machine::DebugTarget per CPU, named as the model descriptors name
// them: the LH5801 of a PC-1500 and PC-1500A; the SC7852 (its Z-80) and the
// LH5803 of a PC-1600, in that order. Each is a CpuView (CpuViews.hpp) plus
// the machine's memory access:
//
//   LH5801, LH5803  spaces me0, me1, me1-written; code space me0;
//                   qualifier me, pu, pv; banks: me0 and me1 by PU and PV
//                   (bank = PU * 2 + PV)
//   SC7852          spaces memory, io, io-written; code space memory;
//                   qualifier bank; banks: memory by the page bank of
//                   port 31H (page A and D 0-1, page B and C 0-7) and page
//                   C's vertical bank (port 28H, a module in Slot 2)
//
// A peek or poke with bank= or vbank= sets the bank registers for that one
// access and puts them back: no bank is switched for the machine.
// The -written views hold the value the CPU last wrote to each ME1 cell or
// I/O port since it was built, or none.
//
// SharpDebugging is the machine's Debugging: SharpMachine::advance runs its
// run() while checks are set; with all checks off the core runs its own
// path (the golden runs see no change). Breakpoints and watches are the
// CPUs' own lists (BreakpointSet, WatchSet), checked only while the Checks
// say so; the observer runs the machine one machine step at a time.
//
// The SharpDebugTarget's own functions (registers by name with their halves
// and flags, bank state, history entries with their bytes, decoding with
// symbols) are what the DAP's layer (app/dap/) needs beside the generic
// interface; they name no machine class.

namespace sharp {

class SharpDebugging;

class SharpDebugTarget final : public calcu::machine::DebugTarget {
public:
    SharpDebugTarget(SharpDebugging& owner, std::size_t index, std::string name, std::unique_ptr<debug::CpuView> view,
                     std::vector<calcu::machine::SpaceInfo> spaces, std::vector<calcu::machine::QualifierField> qualifier);
    ~SharpDebugTarget();

    // ---- calcu::machine::DebugTarget ----
    const calcu::machine::TargetInfo& info() const override { return m_info; }
    std::uint64_t nextAddress() const override { return m_view->pc(); }
    std::uint64_t readRegister(std::size_t) const override;
    bool writeRegister(std::size_t, std::uint64_t) override;
    calcu::machine::CellRead peek(std::size_t space, std::uint64_t address, calcu::machine::BankSelect) const override;
    bool poke(std::size_t space, std::uint64_t address, std::uint64_t value, calcu::machine::BankSelect) override;
    std::vector<calcu::machine::HistoryEntry> history(std::size_t count) const override;
    std::optional<calcu::machine::Decoded> decode(std::uint64_t address) const override;
    std::vector<std::string> qualifierNow() const override;
    bool qualifierMatches(const std::vector<std::string>& wanted, std::uint64_t address) const override;
    calcu::machine::Flow flowAt(std::uint64_t address) const override;
    std::uint64_t callDepth() const override { return 0x10000u - m_view->sp(); }
    std::uint64_t retired() const override { return m_view->retired(); }
    void setBreakpoints(std::vector<std::uint64_t> addresses) override;
    bool setWatches(std::vector<calcu::machine::WatchRange>) override;

    // ---- The Sharp target (app/dap/) ----
    std::size_t index() const { return m_index; }
    debug::CpuKind kind() const { return m_view->kind(); }
    std::vector<debug::Register> registers() const { return m_view->registers(); }
    // Any name registers() lists, the 8-bit halves and the flags as <flag>f.
    bool readRegister(const std::string& name, std::uint32_t* value) const { return m_view->readRegister(name, value); }
    bool writeRegister(const std::string& name, std::uint32_t value) { return m_view->writeRegister(name, value); }
    std::uint16_t pc() const { return m_view->pc(); }
    std::uint16_t sp() const { return m_view->sp(); }
    bool halted() const { return m_view->halted(); }
    // A byte of ME0 / the Z-80's memory (kSpaceMain) or ME1 as the CPU sees it
    // now, without side effects; false: unreadable.
    bool peekByte(debug::Space space, std::uint16_t address, std::uint8_t* value) const;
    bool pokeByte(debug::Space space, std::uint16_t address, std::uint8_t value);
    // What selects the memory the CPU sees now (PU and PV; the Z-80's page banks).
    std::vector<debug::BankField> bankState() const;
    // The PC-1600 page bank (0-7) of the code at `address` now; -1 on the LH580x.
    int bankAt(std::uint16_t address) const;
    // Whether code at `address` satisfies `key` now (the DAP's listings and
    // breakpoints; qualifierMatches() over a BankKey).
    bool bankMatches(const debug::BankKey& key, std::uint16_t address) const;
    std::vector<std::string> qualifierOf(const debug::BankKey& key) const;
    std::uint32_t historySize() const { return m_view->historySize(); }
    debug::HistoryEntry historyEntry(std::uint32_t age) const { return m_view->history(age); }
    disasm::Decoded decodeAt(std::uint16_t address, const disasm::SymbolFn& symbols = {}) const;
    disasm::Decoded decodeEntry(const debug::HistoryEntry& entry, const disasm::SymbolFn& symbols = {}) const;
    // Registers and flags first, then `symbols`; memory through peekByte().
    debug::ExpressionContext expressionContext(std::function<bool(const std::string&, std::int64_t*)> symbols = {}) const;

    // For SharpDebugging.
    debug::CpuView& view() { return *m_view; }
    const debug::CpuView& view() const { return *m_view; }
    const std::vector<std::uint16_t>& breakpoints() const { return m_breakpoints; }
    WatchSet& watchSet() { return m_watches; }

private:
    SharpDebugging& m_owner;
    std::size_t m_index;
    std::unique_ptr<debug::CpuView> m_view;
    calcu::machine::TargetInfo m_info;
    std::vector<std::uint16_t> m_breakpoints;   // sorted
    WatchSet m_watches;
};

class SharpDebugging : public calcu::machine::Debugging {
public:
    virtual ~SharpDebugging();

    // ---- calcu::machine::Debugging ----
    std::size_t targetCount() const override { return m_targets.size(); }
    calcu::machine::DebugTarget& target(std::size_t i) override { return *m_targets.at(i); }
    const calcu::machine::DebugTarget& target(std::size_t i) const override { return *m_targets.at(i); }
    std::size_t current() const override { return busOwner(); }
    void setChecks(const calcu::machine::Checks&) override;
    std::optional<calcu::machine::DebugStop> takeStop() override;
    void passOnce(std::size_t target) override;

    // ---- SharpMachine::advance ----
    // Whether any check is on (else the core's own path runs).
    bool checking() const { return m_checks.breakpoints || m_checks.watches || m_checks.observer; }
    // Runs at least... up to `ticks` with the checks; ends early at a stop.
    calcu::machine::Ticks run(calcu::machine::Ticks ticks);
    bool stopPending() const { return m_stop.has_value(); }

    SharpDebugTarget& sharpTarget(std::size_t i) { return *m_targets.at(i); }
    const SharpDebugTarget& sharpTarget(std::size_t i) const { return *m_targets.at(i); }

    // ---- The model's part (SharpDebugTarget asks) ----
    virtual calcu::machine::CellRead peekCell(std::size_t target, std::size_t space, std::uint64_t address,
                                              calcu::machine::BankSelect) const = 0;
    virtual bool pokeCell(std::size_t target, std::size_t space, std::uint64_t address, std::uint8_t value,
                          calcu::machine::BankSelect) = 0;
    virtual int bankAt(std::size_t target, std::uint16_t address) const = 0;
    virtual std::vector<debug::BankField> bankState(std::size_t target) const = 0;

protected:
    void addTarget(std::string name, std::unique_ptr<debug::CpuView> view, std::vector<calcu::machine::SpaceInfo> spaces,
                   std::vector<calcu::machine::QualifierField> qualifier);
    // Takes every CPU's breakpoints and watches off (the destructor of the
    // model's class, while the machine is still there).
    void detachAll();
    virtual std::size_t busOwner() const = 0;
    virtual calcu::machine::Ticks runCore(calcu::machine::Ticks) = 0;   // the core's runCycles()
    virtual void stepCore() = 0;                                         // one machine step
    virtual std::uint64_t elapsed() const = 0;                           // ticks so far
    virtual ::DebugStop consumeCoreStop() = 0;
    virtual void attachWatchSet(std::size_t target, WatchSet*) = 0;

private:
    void armChecks();
    void latch(const ::DebugStop&);

    std::vector<std::unique_ptr<SharpDebugTarget>> m_targets;
    calcu::machine::Checks m_checks;
    std::optional<calcu::machine::DebugStop> m_stop;
};

std::unique_ptr<SharpDebugging> makeSharpDebugging(PC1500Machine&);
std::unique_ptr<SharpDebugging> makeSharpDebugging(PC1600Machine&);

} // namespace sharp
