#pragma once
#include "machine/Catalogue.hpp"
#include "machine/Descriptor.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Names written by people (a preset, a harness command, a setting) matched
// with the catalogue's without regard to case (LIB-6 AC-1, LIB-18 D-16);
// the catalogue's spelling back. Part D simplification P-6: one set of these
// for every area.

namespace calcu::machine {

// The letters A to Z lowered; every other byte as it is (UTF-8 untouched,
// whatever the C locale).
std::string lowerAscii(std::string_view);
bool sameIgnoringCase(std::string_view, std::string_view);

const MachineDescriptor* modelNamed(const Catalogue&, std::string_view name);
const PeripheralDescriptor* peripheralNamed(const Catalogue&, std::string_view name);
std::optional<VariantId> variantNamed(const std::vector<VariantDescriptor>&, std::string_view name);

} // namespace calcu::machine
