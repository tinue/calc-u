#pragma once
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Messages.hpp"
#include "sessions/TopicMirror.hpp"
#include <functional>
#include <map>
#include <optional>
#include <vector>

// A listener of an app session (glossary): what a
// frontend, the harness or a tool uses to talk to the session on its thread.
// It keeps the topics in a TopicMirror, sends commands and requests, and
// hands each reply to the handler given with its call, else to onMessage.
//
// Order: the messages that arrive inside send() and ask() are taken in the
// order the session sent them, once the call has returned (so the call's id
// is known); a reply that came at once is also returned to the caller. A
// handler or onMessage may send again. Single-threaded, as the session.

namespace calcu::sessions {

class Listener final : public MessageSink {
public:
    using ReplyHandler = std::function<void(const ReplyMessage&)>;

    Listener() = default;
    ~Listener();   // stops listening
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    // Starts to listen; the session sends every state topic
    // once and the stream topics with their history.
    void listen(AppSession&, HostServices = {});
    void stop();
    bool listening() const { return session_ != nullptr; }
    AppSession& session() { return *session_; }
    ListenerId id() const { return id_; }

    struct Sent {
        CallId call = 0;
        std::optional<ReplyMessage> reply;   // the reply, when it came at once (no operation started)
    };
    // A command. `then` gets its reply where it comes among the messages:
    // inside send() when it came at once, at the operation's end otherwise,
    // with its result. Without `then` (or a later onReply) the reply of an
    // operation goes to onMessage.
    Sent send(const CommandName&, Value args = {}, ReplyHandler then = {});
    // A handler for the reply of a call that has not come yet.
    void onReply(CallId, ReplyHandler);
    // A command, then `step` (one host call) until its reply came: a test on
    // a scripted clock, a frozen frontend.
    ReplyMessage runToEnd(const CommandName&, Value args, const std::function<void()>& step);
    // A request; its reply always comes at once.
    ReplyMessage ask(const RequestName&, Value args = {});
    // A request whose reply goes to `then`, where it comes among the messages.
    CallId ask(const RequestName&, Value args, ReplyHandler then);

    TopicMirror& mirror() { return mirror_; }
    const TopicMirror& mirror() const { return mirror_; }
    template <class P> const P* state(const TopicName& t) const { return mirror_.stateOf<P>(t); }
    const machine::DisplayOutput* display() const {   // the `display` topic's last state
        const auto* d = state<DisplayState>(kDisplayTopic);
        return d ? &d->output : nullptr;
    }

    // Every message that went to no reply handler, after the mirror took it:
    // sound, notices and log lines, asks, the replies nobody waits for.
    std::function<void(const Message&)> onMessage;

    void deliver(const Message&) override;

private:
    template <class Call> std::optional<ReplyMessage> held(Call&& call, CallId& id);
    void take(const Message&);

    AppSession* session_ = nullptr;
    ListenerId id_ = 0;
    TopicMirror mirror_;
    std::map<CallId, ReplyHandler> handlers_;
    bool holding_ = false;
    std::vector<Message> held_;
};

} // namespace calcu::sessions
