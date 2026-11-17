#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "Debug/CpuRegisters.hpp"
#include "Debug/DebugExpression.hpp"
#include "Debug/Disasm/Disassembly.hpp"
#include "Debug/SourceMap.hpp"
#include "machine/DebugTarget.hpp"

namespace sharp {
class SharpDebugTarget;
}

// The DAP's view of the machine's debug targets (C1600-3 design §3.6;
// LIB-10 design §10.3, D-15; built in M-45, kept by M-50): threads, their
// registers by name with their halves and flags, banks, memory, decoding and
// history, over the library's machine::Debugging and Calc-U-1600's
// SharpDebugTarget (core/Debug/SharpDebugging). No Qt type.
//
// Threads are the debug targets in the machine's order, from 1: the LH5801;
// the SC7852 (1) and the LH5803 (2). A thread id other than 1 names the last
// target, as the DAP's references always did.

namespace cu1600::dap {

struct Thread {
    int id = 0;
    debug::CpuKind kind = debug::CpuKind::LH5801;
    std::string name;   // "LH5801", "Z80 (SC7852)", "LH5803"
};

class DapTargets {
public:
    // nullptr: no machine, or one without targets (every call is then empty).
    explicit DapTargets(calcu::machine::Debugging* debugging);

    bool valid() const { return m_debugging && m_debugging->targetCount() > 0; }
    calcu::machine::Debugging* debugging() const { return m_debugging; }
    std::vector<Thread> threads() const;
    int busOwner() const;   // the thread that carries out the next instruction
    sharp::SharpDebugTarget& target(int thread) const;
    // The library's name of the thread's target ("SC7852").
    calcu::machine::DebugTargetId targetId(int thread) const;
    int threadOf(std::size_t targetIndex) const { return static_cast<int>(targetIndex) + 1; }

    debug::CpuKind kindOf(int thread) const;
    std::vector<debug::Register> registers(int thread) const;
    bool readRegister(int thread, const std::string& name, std::uint32_t* value) const;
    bool writeRegister(int thread, const std::string& name, std::uint32_t value) const;
    std::uint16_t pc(int thread) const;
    std::uint16_t sp(int thread) const;
    bool halted(int thread) const;
    bool peek(int thread, debug::Space space, std::uint16_t addr, std::uint8_t* value) const;
    bool poke(int thread, debug::Space space, std::uint16_t addr, std::uint8_t value) const;
    int bankAt(int thread, std::uint16_t addr) const;
    std::vector<debug::BankField> bankState(int thread) const;
    bool bankMatches(int thread, const debug::BankKey& key, std::uint16_t addr) const;
    std::uint32_t historySize(int thread) const;
    debug::HistoryEntry history(int thread, std::uint32_t age) const;
    std::uint64_t retired(int thread) const;
    disasm::Decoded decode(int thread, std::uint16_t addr, const disasm::SymbolFn& symbols = {}) const;
    disasm::Decoded decode(int thread, const debug::HistoryEntry& entry, const disasm::SymbolFn& symbols = {}) const;
    debug::ExpressionContext expressionContext(int thread,
                                               std::function<bool(const std::string&, std::int64_t*)> symbols = {}) const;

    // For the source map: bank state and side-effect-free code reads.
    debug::BankMatch bankMatch() const;
    debug::CodePeek codePeek() const;

private:
    std::size_t indexOf(int thread) const;
    calcu::machine::Debugging* m_debugging;
};

} // namespace cu1600::dap
