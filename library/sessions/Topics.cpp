#include "sessions/Topics.hpp"

namespace calcu::sessions {

void StateTopic::set(PayloadPtr state) {
    if (!state) return;
    current_ = std::move(state);
    dirty_ = true;
}

std::optional<StateMessage> StateTopic::flush() {
    if (!dirty_) return std::nullopt;
    dirty_ = false;
    if (sent_ && (sent_ == current_ || current_->equals(*sent_))) return std::nullopt;
    sent_ = current_;
    return StateMessage{name_, sent_};
}

void StreamTopic::append(PayloadPtr item) {
    if (!item) return;
    pending_.push_back(item);
    if (history_ == History::SinceReset) kept_.push_back(std::move(item));
}

void StreamTopic::reset() {
    pending_.clear();
    kept_.clear();
    resetPending_ = true;
}

std::optional<StreamMessage> StreamTopic::flush() {
    if (!resetPending_ && pending_.empty()) return std::nullopt;
    StreamMessage m{name_, resetPending_, std::move(pending_)};
    pending_.clear();
    resetPending_ = false;
    return m;
}

std::optional<StreamMessage> StreamTopic::replay() const {
    if (history_ == History::None) return std::nullopt;
    return StreamMessage{name_, true, kept_};   // the app session flushes before it replays
}

} // namespace calcu::sessions
