#pragma once
#include <optional>
#include <string>
#include <vector>

#include "machine/Attach.hpp"
#include "machine/Catalogue.hpp"

// Adapter until M-27 (LIB-12 1600; C1600-1 design §5): the ROM images a
// descriptor names, read from the bundled folders as today
// (BundledRoms::resolveBundledRomPath and its cached read). M-27 takes
// every image through the library's ROM lookup.

namespace sharp {

// The images of the model's variant (std::nullopt: the default). False when
// a file is missing or unreadable; `error` then says which, in today's
// words ("bundled ROM '<file>' not found in any of the given directories").
bool modelRomImages(const calcu::machine::Catalogue&, const calcu::machine::ModelId&,
                    const std::optional<calcu::machine::VariantId>&, const std::vector<std::string>& dirs,
                    std::vector<calcu::machine::RomImage>* out, std::string* error);
bool peripheralRomImages(const calcu::machine::Catalogue&, const calcu::machine::PeripheralId&,
                         const std::optional<calcu::machine::VariantId>&, const std::vector<std::string>& dirs,
                         std::vector<calcu::machine::RomImage>* out, std::string* error);

} // namespace sharp
