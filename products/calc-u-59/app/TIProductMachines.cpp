#include "TIProductMachines.hpp"
#include "MachineModelInfo.hpp"
#include "PacingProfiles.hpp"

namespace cu59 {

calcu::startup::MachinePlan TIProductMachines::builtInDefault(const calcu::machine::ModelId& model) {
    calcu::startup::MachinePlan plan;
    plan.model = model;
    calcu::machine::PeripheralRequest module;
    module.place = calcu::machine::PlacePath{{kTIModulePlace}};
    module.peripheral = calcu::machine::PeripheralId{kDefaultModule};
    plan.peripherals.push_back(std::move(module));
    if (model.value == "TI-58C" && m_constantMemory) plan.content = m_constantMemory();
    return plan;
}

calcu::pacing::PacingProfile TIProductMachines::pacingProfile(const calcu::machine::ModelId& model) const {
    return cu59::pacingProfile(modelFrom(model).value_or(Model::TI59));
}

} // namespace cu59
