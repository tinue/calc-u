#include "machine/Machine.hpp"

namespace calcu::machine {

std::vector<CapabilityEntry> Machine::capabilities(const KindId& kind) const {
    std::vector<CapabilityEntry> out;
    for (auto& e : capabilities())
        if (e.kind == kind) out.push_back(std::move(e));
    return out;
}

DebugTarget* Machine::debugTarget(const DebugTargetId& id) {
    Debugging* d = debugging();
    if (!d) return nullptr;
    for (std::size_t i = 0; i < d->targetCount(); ++i)
        if (d->target(i).info().name == id) return &d->target(i);
    return nullptr;
}

} // namespace calcu::machine
