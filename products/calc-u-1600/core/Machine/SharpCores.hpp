#pragma once
#include "../PC1500/PC1500InterfaceMachine.hpp"
#include "../PC1600/PC1600InterfaceMachine.hpp"
#include "Machine/SharpCatalogue.hpp"

// Inside the core only (the include check, CalcU1600MachineIncludeCheck):
// the old class behind a Sharp machine, for the core's own parts and the
// core's tests. By the descriptor's model and a
// static_cast; no RTTI.

namespace sharp {

inline SharpMachine* sharpOf(calcu::machine::Machine& m) {
    return m.descriptor().family.value == kSharpFamily ? static_cast<SharpMachine*>(&m) : nullptr;
}

inline PC1500Machine* pc1500Of(calcu::machine::Machine& m) {
    if (m.descriptor().family.value != kSharpFamily || m.model().value == kPC1600) return nullptr;
    return &static_cast<PC1500InterfaceMachine&>(m).core();
}

inline PC1600Machine* pc1600Of(calcu::machine::Machine& m) {
    if (m.descriptor().family.value != kSharpFamily || m.model().value != kPC1600) return nullptr;
    return &static_cast<PC1600InterfaceMachine&>(m).core();
}

} // namespace sharp
