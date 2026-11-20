#pragma once
#include "../PC1500/PC1500Machine.hpp"
#include "../PC1600/PC1600Machine.hpp"
#include "Machine/SharpCatalogue.hpp"

// Inside the core only (the include check, CalcU1600MachineIncludeCheck):
// the model class behind a Sharp machine, for the core's own parts that
// read a model's hardware (the LCD font, the text of the display). By the
// descriptor's model and a static_cast; no RTTI.

namespace sharp {

inline PC1500Machine* pc1500Of(calcu::machine::Machine& m) {
    if (m.descriptor().family.value != kSharpFamily || m.model().value == kPC1600) return nullptr;
    return static_cast<PC1500Machine*>(&m);
}

inline PC1600Machine* pc1600Of(calcu::machine::Machine& m) {
    if (m.descriptor().family.value != kSharpFamily || m.model().value != kPC1600) return nullptr;
    return static_cast<PC1600Machine*>(&m);
}

} // namespace sharp
