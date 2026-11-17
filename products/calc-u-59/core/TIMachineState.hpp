#pragma once
#include "harness/MachineState.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The generic kind `machine-state` (LIB-11 AC-24) on the TI-59, TI-58 and
// TI-58C (C59-9 AC-15, design §3.7): capability `TMC0501/machine-state`.
// The form is the product's (core/TIStateRecords.hpp), one record per part
// in the machine's order: the machine (its tick count, a pending Reset
// restore, what is plugged), the RAM, the TMC0501, the PC-100C and the
// Solid State ROM module when plugged, the card reader with the card in it.
// The library writes the header (product, version, model, variant, plugged
// peripherals) and refuses a file of another configuration first. A restore
// that does not fit puts the state from before back and is refused: it
// changes nothing.

class TIMachine;

class TIMachineState final : public calcu::harness::MachineState {
public:
    explicit TIMachineState(TIMachine& m) : m_(m) {}
    std::vector<std::uint8_t> save() const override;
    std::optional<std::string> restore(const std::vector<std::uint8_t>&) override;

private:
    TIMachine& m_;
};
