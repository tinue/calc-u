#pragma once
#include "machine/Descriptor.hpp"
#include <optional>
#include <vector>

// A product's catalogue: one machine
// descriptor per model, one peripheral descriptor per peripheral, available
// without a machine and without ROM files.

namespace calcu::machine {

struct PlaceInfo {
    const PlaceDescriptor* place;
    const ConnectorDescriptor* connector;
    const PinOutDescriptor* pinOut;
    std::vector<PeripheralId> fitting;   // by the pin-out rule
};

class Catalogue {
public:
    Catalogue(std::vector<MachineDescriptor> models, std::vector<PeripheralDescriptor> peripherals);
    const std::vector<MachineDescriptor>& models() const { return models_; }
    const std::vector<PeripheralDescriptor>& peripherals() const { return peripherals_; }
    const MachineDescriptor* model(const ModelId&) const;
    const PeripheralDescriptor* peripheral(const PeripheralId&) const;
    // The connector behind a place of a model and the peripherals that
    // fit it, including those added later.
    std::optional<PlaceInfo> place(const ModelId&, const PlaceName&) const;
    // A place a peripheral offers, the same way.
    std::optional<PlaceInfo> place(const PeripheralId&, const PlaceName&) const;
    // A module definition read after the catalogue was built.
    void addPeripheral(PeripheralDescriptor);

private:
    std::vector<MachineDescriptor> models_;
    std::vector<PeripheralDescriptor> peripherals_;
};

} // namespace calcu::machine
