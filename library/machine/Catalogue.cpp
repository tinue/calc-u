#include "machine/Catalogue.hpp"

namespace calcu::machine {

Catalogue::Catalogue(std::vector<MachineDescriptor> models, std::vector<PeripheralDescriptor> peripherals)
    : models_(std::move(models)), peripherals_(std::move(peripherals)) {}

const MachineDescriptor* Catalogue::model(const ModelId& id) const {
    for (const auto& m : models_)
        if (m.model == id) return &m;
    return nullptr;
}

const PeripheralDescriptor* Catalogue::peripheral(const PeripheralId& id) const {
    for (const auto& p : peripherals_)
        if (p.id == id) return &p;
    return nullptr;
}

std::optional<PlaceInfo> Catalogue::place(const ModelId& modelId, const PlaceName& name) const {
    const MachineDescriptor* m = model(modelId);
    const PlaceDescriptor* p = m ? m->place(name) : nullptr;
    const ConnectorDescriptor* c = p ? m->connector(p->connector) : nullptr;
    if (!c) return std::nullopt;
    const PinOutDescriptor* pinOut = nullptr;
    for (const auto& o : c->pinOuts)
        if (o.id == p->pinOut) pinOut = &o;
    if (!pinOut) return std::nullopt;
    PlaceInfo info{p, c, pinOut, {}};
    for (const auto& per : peripherals_)
        if (per.fitsPinOut(p->pinOut)) info.fitting.push_back(per.id);
    return info;
}

void Catalogue::addPeripheral(PeripheralDescriptor p) { peripherals_.push_back(std::move(p)); }

} // namespace calcu::machine
