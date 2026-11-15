#pragma once
#include "machine/Ids.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include <cstddef>
#include <vector>

// The names and payloads of typing in the app session (LIB-5 design §3.5;
// spec AC-17, AC-19, AC-38). Library names, without a prefix (LIB-3 §3.8).

namespace calcu::typing {

inline const sessions::CommandName kHostKeyDown{"host-key-down"};       // HostKeyEvent (HostKey.hpp); dropped while busy
inline const sessions::CommandName kHostKeyUp{"host-key-up"};           // {physical}
inline const sessions::CommandName kHostText{"host-text"};              // {text}: EC-4; dropped while busy
inline const sessions::CommandName kScreenKeyDown{"screen-key-down"};   // {key, pointer}; dropped while busy
inline const sessions::CommandName kScreenKeyUp{"screen-key-up"};       // {key, pointer}
inline const sessions::CommandName kFocusLost{"focus-lost"};            // AC-10
inline const sessions::CommandName kPaste{"paste"};                     // {text}: AC-12, AC-34; dropped while busy
inline const sessions::CommandName kKey{"key"};                         // {names: [..]}: AC-15, AC-17; deferred
inline const sessions::CommandName kType{"type"};                       // {text}: AC-16, AC-17; deferred
inline const sessions::TopicName kHeldKeysTopic{"held-keys"};           // HeldKeys: AC-19
inline const sessions::RequestName kTypingRequest{"typing"};            // TypingState

// AC-19: the keys live input holds down on the machine, by key ID.
struct HeldKeys final : sessions::PayloadOf<HeldKeys> {
    static constexpr sessions::PayloadKind kKind{"held-keys"};
    std::vector<machine::KeyId> live;
    bool same(const HeldKeys& o) const { return live == o.live; }
    sessions::Value toValue() const override;
};

// What the engine has left to do (frontends that wait for it, tests).
struct TypingState final : sessions::PayloadOf<TypingState> {
    static constexpr sessions::PayloadKind kKind{"typing-state"};
    bool idle = true;
    std::size_t waiting = 0;
    bool same(const TypingState& o) const { return idle == o.idle && waiting == o.waiting; }
    sessions::Value toValue() const override;
};

} // namespace calcu::typing
