#pragma once
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "typing/TypingSupport.hpp"
#include <array>
#include <cstdint>
#include <optional>

// The typing support of the TI machines and the model's key-detect range.
// The memory readers are the product kind c59.program-memory
// (TIProductKinds.hpp); the core tests use TIMachineTesting.

class TIMachine;

/// Typing support of the TI machines: a capability
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
/// (extra empty when lo > hi): the addresses `c59 coverage` leaves out
/// without `all`, the run probe's idle loop, and the tests.
std::array<std::uint16_t, 4> idleAndKeyDetectRange(const calcu::machine::ModelId&);
