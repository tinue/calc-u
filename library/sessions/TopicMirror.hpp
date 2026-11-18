#pragma once
#include "sessions/Messages.hpp"
#include <functional>
#include <map>
#include <vector>

// The topic mirror: a MessageSink that keeps the last
// message of each state topic and what each stream topic appended. It reads
// no payload, so a topic it does not know is kept like any other and changes
// nothing else. It serves every Listener and the test frontend.
// Replies, asks and sound are not topics: it ignores them.

namespace calcu::sessions {

class TopicMirror : public MessageSink {
public:
    void deliver(const Message&) override;

    const PayloadPtr* state(const TopicName&) const;           // the last of a state topic, or nullptr
    template <class P> const P* stateOf(const TopicName& t) const {   // the last as P, or nullptr
        const PayloadPtr* p = state(t);
        return p ? payloadAs<P>(*p) : nullptr;
    }
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
