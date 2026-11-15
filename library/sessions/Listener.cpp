#include "sessions/Listener.hpp"
#include <utility>

namespace calcu::sessions {

Listener::~Listener() { stop(); }

void Listener::listen(AppSession& s, HostServices services) {
    stop();
    session_ = &s;
    id_ = s.listen(*this, std::move(services));
}

void Listener::stop() {
    if (session_) session_->stopListening(id_);
    session_ = nullptr;
    id_ = 0;
    handlers_.clear();
}

void Listener::deliver(const Message& m) {
    if (holding_) held_.push_back(m);
    else take(m);
}

void Listener::take(const Message& m) {
    mirror_.deliver(m);
    if (const auto* r = std::get_if<ReplyMessage>(&m)) {
        const auto it = handlers_.find(r->call);
        if (it != handlers_.end()) {
            ReplyHandler h = std::move(it->second);
            handlers_.erase(it);
            if (h) h(*r);
            return;
        }
    }
    if (onMessage) onMessage(m);
}

// Runs `call` (a send or an ask of the session, giving the call's id) with
// the messages held, then takes them in order. The reply of that call, when
// among them, is returned; it goes to a handler only when one waits for it.
template <class Call> std::optional<ReplyMessage> Listener::held(Call&& call, CallId& id) {
    holding_ = true;
    try {
        id = call();
    } catch (...) {
        holding_ = false;
        held_.clear();
        throw;
    }
    holding_ = false;
    std::vector<Message> messages;
    messages.swap(held_);
    std::optional<ReplyMessage> atOnce;
    for (const Message& m : messages) {
        if (const auto* r = std::get_if<ReplyMessage>(&m); r && r->call == id) {
            atOnce = *r;
            if (!handlers_.count(id)) continue;   // the caller has it
        }
        take(m);
    }
    return atOnce;
}

Listener::Sent Listener::send(const CommandName& name, Value args, ReplyHandler then) {
    Sent out;
    out.reply = held(
        [&] {
            const CallId id = session_->send(id_, Command{name, std::move(args)});
            if (then) handlers_[id] = std::move(then);   // before the messages are taken: a reply at once goes to it
            return id;
        },
        out.call);
    return out;
}

void Listener::onReply(CallId call, ReplyHandler h) { handlers_[call] = std::move(h); }

ReplyMessage Listener::runToEnd(const CommandName& name, Value args, const std::function<void()>& step) {
    std::optional<ReplyMessage> got;
    send(name, std::move(args), [&got](const ReplyMessage& r) { got = r; });
    while (!got) step();
    return *got;
}

ReplyMessage Listener::ask(const RequestName& name, Value args) {
    CallId id = 0;
    auto r = held([&] { return session_->ask(id_, Request{name, std::move(args)}); }, id);
    return r ? *r : ReplyMessage{id, false, "No reply.", nullptr};
}

CallId Listener::ask(const RequestName& name, Value args, ReplyHandler then) {
    CallId id = 0;
    held(
        [&] {
            const CallId c = session_->ask(id_, Request{name, std::move(args)});
            if (then) handlers_[c] = std::move(then);
            return c;
        },
        id);
    return id;
}

} // namespace calcu::sessions
