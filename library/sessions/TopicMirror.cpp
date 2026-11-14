#include "sessions/TopicMirror.hpp"

namespace calcu::sessions {

void TopicMirror::deliver(const Message& m) {
    if (const auto* s = std::get_if<StateMessage>(&m)) {
        states_[s->topic] = s->state;
        if (onChange) onChange(s->topic);
    } else if (const auto* st = std::get_if<StreamMessage>(&m)) {
        Stream& k = streams_[st->topic];
        if (st->reset) {
            k.appended.clear();   // what was not taken is of the old stream
            k.reset = true;
        }
        k.appended.insert(k.appended.end(), st->appended.begin(), st->appended.end());
        if (onChange) onChange(st->topic);
    }
}

const PayloadPtr* TopicMirror::state(const TopicName& t) const {
    const auto it = states_.find(t);
    return it == states_.end() ? nullptr : &it->second;
}

std::vector<PayloadPtr> TopicMirror::takeAppended(const TopicName& t) {
    const auto it = streams_.find(t);
    if (it == streams_.end()) return {};
    std::vector<PayloadPtr> out;
    out.swap(it->second.appended);
    return out;
}

bool TopicMirror::takeReset(const TopicName& t) {
    const auto it = streams_.find(t);
    if (it == streams_.end()) return false;
    const bool r = it->second.reset;
    it->second.reset = false;
    return r;
}

} // namespace calcu::sessions
