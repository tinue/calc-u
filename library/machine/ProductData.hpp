#pragma once

// The opaque payload a product hands through the library (LIB-1 design §3.9;
// capabilities.md Q-2, machine.md M-7): a module definition behind a
// PeripheralDescriptor, the content of a memory module, a TI-58C constant
// memory handed in at build. The library carries it without looking inside.

namespace calcu::machine {

class ProductData {
public:
    virtual ~ProductData() = default;
};

} // namespace calcu::machine
