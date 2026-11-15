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

namespace {
template <class D>
std::optional<PlaceInfo> placeOf(const D* d, const PlaceName& name, const std::vector<PeripheralDescriptor>& all) {
    if (!d) return std::nullopt;
    const PlaceDescriptor* p = nullptr;
    for (const auto& x : d->places)
        if (x.name == name) p = &x;
    const ConnectorDescriptor* c = nullptr;
    if (p)
        for (const auto& x : d->connectors)
            if (x.id == p->connector) c = &x;
    if (!c) return std::nullopt;
    const PinOutDescriptor* pinOut = nullptr;
    for (const auto& o : c->pinOuts)
        if (o.id == p->pinOut) pinOut = &o;
    if (!pinOut) return std::nullopt;
    PlaceInfo info{p, c, pinOut, {}};
    for (const auto& per : all)
        if (per.fitsPinOut(p->pinOut)) info.fitting.push_back(per.id);
    return info;
}
} // namespace

std::optional<PlaceInfo> Catalogue::place(const ModelId& modelId, const PlaceName& name) const {
    return placeOf(model(modelId), name, peripherals_);
}

std::optional<PlaceInfo> Catalogue::place(const PeripheralId& id, const PlaceName& name) const {
    return placeOf(peripheral(id), name, peripherals_);
}

void Catalogue::addPeripheral(PeripheralDescriptor p) { peripherals_.push_back(std::move(p)); }

} // namespace calcu::machine
