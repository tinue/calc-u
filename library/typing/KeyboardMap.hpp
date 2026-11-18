#pragma once
#include "typing/HostKey.hpp"
#include "typing/TypingProfile.hpp"
#include <optional>
#include <vector>

// The keyboard map of a model: a host key event → a key, a sequence of keys, or nothing. The
// product supplies one per model; no frontend holds one.

namespace calcu::typing {

struct Mapped {
    std::vector<Press> presses;
    // One key that stays down until the host key comes up; otherwise
    // a sequence typed press by press, whose key-up does nothing.
    bool held = false;
};

class KeyboardMap {
public:
    virtual std::optional<Mapped> map(const HostKeyEvent&) const = 0;   // nullopt: nothing

protected:
    ~KeyboardMap() = default;
};

} // namespace calcu::typing
