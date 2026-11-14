#pragma once
#include "sessions/Messages.hpp"
#include <functional>
#include <map>
#include <vector>

// The topic mirror (LIB-3 design §3.10): a MessageSink that keeps the last
// message of each state topic and what each stream topic appended. It reads
// no payload, so a topic it does not know is kept like any other and changes
// nothing else (AC-12). It serves the test frontend, the Qt6 frontends and
// the harness. Replies, asks and sound are not topics: it ignores them.

namespace calcu::sessions {

class TopicMirror : public MessageSink {
public:
    void deliver(const Message&) override;

    const PayloadPtr* state(const TopicName&) const;           // the last of a state topic, or nullptr
    std::vector<PayloadPtr> takeAppended(const TopicName&);    // what a stream appended since the last take
    bool takeReset(const TopicName&);                          // a reset marker arrived since the last take

    std::function<void(const TopicName&)> onChange;            // a Qt frontend turns this into its repaint signals

private:
    struct Stream {
        std::vector<PayloadPtr> appended;
        bool reset = false;
    };
    std::map<TopicName, PayloadPtr> states_;
    std::map<TopicName, Stream> streams_;
};

} // namespace calcu::sessions
