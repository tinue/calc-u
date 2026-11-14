#include "machine/Machine.hpp"

namespace calcu::machine {

std::vector<CapabilityEntry> Machine::capabilities(const KindId& kind) const {
    std::vector<CapabilityEntry> out;
    for (auto& e : capabilities())
        if (e.kind == kind) out.push_back(std::move(e));
    return out;
}

} // namespace calcu::machine
