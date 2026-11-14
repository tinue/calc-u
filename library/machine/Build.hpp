#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "machine/ProductData.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Building a machine (LIB-1 design §3.8; AC-5, AC-6, EC-1).

namespace calcu::machine {

struct RomImage {
    std::string fileName;
    std::vector<std::uint8_t> bytes;   // the file as bundled; the core decodes (D-4)
};
struct PeripheralRequest {
    PlacePath place;
    PeripheralId peripheral;
    std::optional<VariantId> variant;
    std::vector<RomImage> roms;
    std::shared_ptr<const ProductData> content;
};
struct BuildRequest {
    ModelId model;
    std::optional<VariantId> variant;          // std::nullopt: the default (or none)
    std::vector<RomImage> roms;
    std::vector<PeripheralRequest> peripherals;
    int soundRate = 0;                         // samples per second of takeSound() (AC-19)
    std::shared_ptr<const ProductData> content;   // handed in at build (AC-6)
};
struct BuildResult {
    std::unique_ptr<Machine> machine;          // null when nothing was built
    std::vector<std::string> missingRoms;      // the files not handed in (EC-1)
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
