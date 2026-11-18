#pragma once
#include "sessions/Messages.hpp"
#include <optional>
#include <vector>

// Topics. set(), append() and reset() only store; the app session flushes at the end
// of each host call, command and service answer.

namespace calcu::sessions {

class StateTopic {
public:
    explicit StateTopic(TopicName name) : name_(std::move(name)) {}
    const TopicName& name() const { return name_; }
    void set(PayloadPtr state);                 // a null state is ignored
    const PayloadPtr& lastSent() const { return sent_; }
    const PayloadPtr& current() const { return current_; }
    // The message of a flush: the stored state when it is not equal to the
    // last one sent; several sets send the last, a change back sends
    // nothing.
    std::optional<StateMessage> flush();

private:
    TopicName name_;
    PayloadPtr current_, sent_;
    bool dirty_ = false;   // set() since the last flush: no comparison otherwise
};

// Notices: None (a notice is about the moment); log, paper: SinceReset.
enum class History { None, SinceReset };

class StreamTopic {
public:
    StreamTopic(TopicName name, History history) : name_(std::move(name)), history_(history) {}
    const TopicName& name() const { return name_; }
    History history() const { return history_; }
    void append(PayloadPtr item);
    void reset();
    // What was appended since the last flush, in order, with the reset marker
    // when the stream started over in between.
    std::optional<StreamMessage> flush();
    // For a listener that starts late: everything since the last reset, with
    // the reset marker; nothing for History::None.
    std::optional<StreamMessage> replay() const;

private:
    TopicName name_;
    History history_;
    std::vector<PayloadPtr> pending_, kept_;
    bool resetPending_ = false;
};

} // namespace calcu::sessions
