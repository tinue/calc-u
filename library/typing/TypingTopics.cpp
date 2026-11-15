#include "typing/TypingTopics.hpp"

namespace calcu::typing {

sessions::Value HeldKeys::toValue() const {
    sessions::Value::Array keys;
    for (const auto& k : live) keys.push_back(sessions::Value(k.value));
    return sessions::Value::object({{"live", sessions::Value(std::move(keys))}});
}

sessions::Value TypingState::toValue() const {
    return sessions::Value::object({{"idle", idle}, {"waiting", static_cast<std::int64_t>(waiting)}});
}

} // namespace calcu::typing
