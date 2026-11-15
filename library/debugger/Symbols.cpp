#include "debugger/Symbols.hpp"
#include <cstdio>

namespace calcu::debugger {

void SymbolTable::add(std::string name, std::uint64_t address) {
    byAddress_.emplace(address, name);   // the first name of an address is shown
    byName_[std::move(name)] = address;
}

std::optional<std::uint64_t> SymbolTable::address(std::string_view name) const {
    const auto it = byName_.find(name);
    if (it == byName_.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string> SymbolTable::nameAt(std::uint64_t address) const {
    auto it = byAddress_.upper_bound(address);
    if (it == byAddress_.begin()) return std::nullopt;
    --it;
    if (it->first == address) return it->second;
    char buf[32];
    std::snprintf(buf, sizeof buf, "+0x%llx", static_cast<unsigned long long>(address - it->first));
    return it->second + buf;
}

} // namespace calcu::debugger
