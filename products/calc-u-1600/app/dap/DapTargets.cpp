#include "app/dap/DapTargets.hpp"

#include <utility>

#include "Debug/SharpDebugging.hpp"

namespace cu1600::dap {

DapTargets::DapTargets(calcu::machine::Debugging* debugging) : m_debugging(debugging) {}

std::size_t DapTargets::indexOf(int thread) const {
    const std::size_t n = m_debugging ? m_debugging->targetCount() : 0;
    return thread == 1 || n == 0 ? 0 : n - 1;
}

sharp::SharpDebugTarget& DapTargets::target(int thread) const {
    return static_cast<sharp::SharpDebugTarget&>(m_debugging->target(indexOf(thread)));
}

calcu::machine::DebugTargetId DapTargets::targetId(int thread) const { return target(thread).info().name; }

std::vector<Thread> DapTargets::threads() const {
    std::vector<Thread> list;
    if (!valid()) return list;
    for (std::size_t i = 0; i < m_debugging->targetCount(); ++i) {
        const auto& t = static_cast<const sharp::SharpDebugTarget&>(m_debugging->target(i));
        list.push_back({threadOf(i), t.kind(), t.view().name()});
    }
    return list;
}

int DapTargets::busOwner() const { return valid() ? threadOf(m_debugging->current()) : 1; }

debug::CpuKind DapTargets::kindOf(int thread) const { return target(thread).kind(); }
std::vector<debug::Register> DapTargets::registers(int thread) const { return target(thread).registers(); }
bool DapTargets::readRegister(int thread, const std::string& name, std::uint32_t* value) const {
    return target(thread).readRegister(name, value);
}
bool DapTargets::writeRegister(int thread, const std::string& name, std::uint32_t value) const {
    return target(thread).writeRegister(name, value);
}
std::uint16_t DapTargets::pc(int thread) const { return target(thread).pc(); }
std::uint16_t DapTargets::sp(int thread) const { return target(thread).sp(); }
bool DapTargets::halted(int thread) const { return target(thread).halted(); }
bool DapTargets::peek(int thread, debug::Space space, std::uint16_t addr, std::uint8_t* value) const {
    return target(thread).peekByte(space, addr, value);
}
bool DapTargets::poke(int thread, debug::Space space, std::uint16_t addr, std::uint8_t value) const {
    return target(thread).pokeByte(space, addr, value);
}
int DapTargets::bankAt(int thread, std::uint16_t addr) const { return target(thread).bankAt(addr); }
std::vector<debug::BankField> DapTargets::bankState(int thread) const { return target(thread).bankState(); }
bool DapTargets::bankMatches(int thread, const debug::BankKey& key, std::uint16_t addr) const {
    return target(thread).bankMatches(key, addr);
}
std::uint32_t DapTargets::historySize(int thread) const { return target(thread).historySize(); }
debug::HistoryEntry DapTargets::history(int thread, std::uint32_t age) const { return target(thread).historyEntry(age); }
std::uint64_t DapTargets::retired(int thread) const { return target(thread).retired(); }
disasm::Decoded DapTargets::decode(int thread, std::uint16_t addr, const disasm::SymbolFn& symbols) const {
    return target(thread).decodeAt(addr, symbols);
}
disasm::Decoded DapTargets::decode(int thread, const debug::HistoryEntry& entry, const disasm::SymbolFn& symbols) const {
    return target(thread).decodeEntry(entry, symbols);
}
debug::ExpressionContext DapTargets::expressionContext(int thread,
                                                       std::function<bool(const std::string&, std::int64_t*)> symbols) const {
    return target(thread).expressionContext(std::move(symbols));
}

debug::BankMatch DapTargets::bankMatch() const {
    return [self = *this](int t, const debug::BankKey& k, std::uint16_t a) { return self.valid() && self.bankMatches(t, k, a); };
}
debug::CodePeek DapTargets::codePeek() const {
    return [self = *this](int t, std::uint16_t a, std::uint8_t* v) { return self.valid() && self.peek(t, debug::kSpaceMain, a, v); };
}

} // namespace cu1600::dap
