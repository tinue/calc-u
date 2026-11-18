#pragma once

// The opaque payload a product hands through the library: a module
// definition behind a PeripheralDescriptor, the content of a memory module,
// a TI-58C constant memory handed in at build. The library carries it without looking inside.

namespace calcu::machine {

class ProductData {
public:
    virtual ~ProductData() = default;
};

} // namespace calcu::machine
