#pragma once
#include "machine/Capability.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The generic kind machine state: the full
// state of a machine and its plugged peripherals in a form internal to the
// product and not documented. The library carries the bytes without reading
// them; the harness writes a header before them (product, version, model,
// variant, plugged peripherals) and refuses a file of another configuration
// before the machine sees it. Files of the host (the save folder, a host
// drive's folder, a serial port file) are not part of the state.

namespace calcu::harness {

class MachineState : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"machine-state"};
    machine::KindId kind() const override { return kKind; }

    virtual std::vector<std::uint8_t> save() const = 0;
    // The reason of a refusal, or none; a refused restore changes nothing.
    virtual std::optional<std::string> restore(const std::vector<std::uint8_t>&) = 0;
};

} // namespace calcu::harness
