#pragma once
#include "machine/Attach.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "machine/ProductData.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Building a machine.

namespace calcu::machine {

// RomImage and PeripheralRequest: Attach.hpp.
struct BuildRequest {
    ModelId model;
    std::optional<VariantId> variant;          // std::nullopt: the default (or none)
    std::vector<RomImage> roms;
    std::vector<PeripheralRequest> peripherals;
    int soundRate = 0;                         // samples per second of takeSound()
    std::shared_ptr<const ProductData> content;   // handed in at build
};
struct BuildResult {
    std::unique_ptr<Machine> machine;          // null when nothing was built
    std::vector<std::string> missingRoms;      // the files not handed in
    std::string refusal;                       // why nothing was built, otherwise
};

// A product's builder. It gets a request that build() has checked against
// the catalogue, with the variants resolved to the default where none was
// named.
class MachineBuilder {
public:
    virtual ~MachineBuilder() = default;
    virtual BuildResult build(const BuildRequest&, const Catalogue&) = 0;
};

// Checks the request against the catalogue (the model is known, the variant
// belongs to it, every ROM entry of the model's and each peripheral's
// variant has an image of that file name), then calls the builder.
BuildResult build(const Catalogue&, MachineBuilder&, const BuildRequest&);

} // namespace calcu::machine
