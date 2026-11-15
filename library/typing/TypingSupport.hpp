#pragma once
#include "machine/Capability.hpp"
#include "machine/Ids.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Typing support (LIB-5 design §3.4; spec AC-13, AC-35; capabilities.md C-1):
// the generic kind `typing-support`, what a machine tells the typing engine
// beyond key readiness: whether it waits at its prompt, which keys it holds
// latched, and on the TI models how often the ROM has read a key.

namespace calcu::typing {

class TypingSupport : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"typing-support"};
    machine::KindId kind() const override { return kKind; }

    // AC-35: the machine waits at its prompt; nullopt: the model has none.
    virtual std::optional<bool> promptReady() const { return std::nullopt; }
    // AC-13: the latches by name and whether each is on (PC-1600: shift, kbii).
    virtual std::vector<std::pair<std::string, bool>> latches() const { return {}; }
    // §3.4: how many times the ROM's key scan found the key down so far (it
    // only grows); nullopt: the model does not count (Sharp).
    virtual std::optional<std::uint64_t> samples(const machine::KeyId&) const { return std::nullopt; }

    // The latch of that name (false when the machine has none).
    bool latched(const std::string& name) const {
        for (const auto& [n, on] : latches())
            if (n == name) return on;
        return false;
    }
};

} // namespace calcu::typing
