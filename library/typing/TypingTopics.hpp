#pragma once
#include "machine/Ids.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include <cstddef>
#include <vector>

// The names and payloads of typing in the app session. Library names,
// without a prefix.

namespace calcu::typing {

inline const sessions::CommandName kHostKeyDown{"host-key-down"};       // HostKeyEvent (HostKey.hpp); dropped while busy
inline const sessions::CommandName kHostKeyUp{"host-key-up"};           // {physical}
inline const sessions::CommandName kHostText{"host-text"};              // {text}; dropped while busy
inline const sessions::CommandName kScreenKeyDown{"screen-key-down"};   // {key, pointer}; dropped while busy
inline const sessions::CommandName kScreenKeyUp{"screen-key-up"};       // {key, pointer}
inline const sessions::CommandName kFocusLost{"focus-lost"};
inline const sessions::CommandName kPaste{"paste"};                     // {text}; dropped while busy
inline const sessions::CommandName kKey{"key"};                         // {names: [..]}; deferred
inline const sessions::CommandName kType{"type"};                       // {text}; deferred
inline const sessions::TopicName kHeldKeysTopic{"held-keys"};           // HeldKeys
inline const sessions::RequestName kTypingRequest{"typing"};            // TypingState

// The keys live input holds down on the machine, by key ID, so that a
// frontend can draw them pressed; keys of typed text and steps are not in it.
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
