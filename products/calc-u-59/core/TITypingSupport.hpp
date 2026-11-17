#pragma once
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "typing/TypingSupport.hpp"
#include <array>
#include <cstdint>
#include <optional>

// What was left of core/TIAdapters.hpp once the adapters of C59-1 were gone
// (LIB-11 design §11.2, D-17; M-59): the typing support of the TI machines
// and the model's key-detect range. Neither is an adapter. The memory
// adapter's readers moved onto the product kind c59.program-memory
// (TIProductKinds.hpp) and the core tests onto TIMachineTesting (M-59).

class TIMachine;

/// Typing support of the TI machines (LIB-5 design §3.4, D-3): a capability
/// of the generic kind `typing-support`, `TMC0501/typing-support`, owner
/// component TMC0501. How often the ROM's keyboard-scan instruction found a
/// key pressed; no prompt readiness, no latches.
class TITypingSupport final : public calcu::typing::TypingSupport {
public:
    explicit TITypingSupport(TIMachine& m) : m_(m) {}
    std::optional<std::uint64_t> samples(const calcu::machine::KeyId&) const override;

private:
    TIMachine& m_;
};

/// The full keyboard scan/detect cycle of a model: {lo, hi, extraLo, extraHi}
/// (extra empty when lo > hi): the addresses `ti59 coverage` leaves out
/// without `all` (C59-9 D-7), the run probe's idle loop, and the tests.
std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId&);
