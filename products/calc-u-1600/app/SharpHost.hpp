#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>

#include "app/SharpAppPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/TopicMirror.hpp"

namespace shared {

// ── SharpHost: a frontend's side of Calc-U-1600's app session ────────────
//
// Not a session: the wiring every host of the library's app session needs
// (LIB-3 design §3.9) -- the product's part, the session with its parts,
// one listener that keeps the last message of each topic (a TopicMirror)
// and the replies to its commands. The Qt6 frontend, the Web engine, the
// tests and tools/wasm_smoke host the session through it; each calls
// session().call(now) from its own timer. Single-threaded.
class SharpHost final : public calcu::sessions::MessageSink {
public:
    struct Options {
        bool hostDirectories = true;   // states a folder picker: File ▸ Mount Directory offered
        calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
        calcu::pacing::WallClock clock;                                      // default: the steady clock
        std::function<std::chrono::system_clock::time_point()> hostTime;    // default: the system clock
        calcu::sessions::HostServices services;                              // what the frontend states (LIB-3 AC-26)
    };
    SharpHost(Settings& settings, AppPaths& paths, Options options);
    ~SharpHost();
    SharpHost(const SharpHost&) = delete;
    SharpHost& operator=(const SharpHost&) = delete;

    calcu::sessions::AppSession& session() { return *m_session; }
    SharpAppPart& part() { return *m_part; }
    const SharpAppPart& part() const { return *m_part; }
    calcu::sessions::TopicMirror& mirror() { return m_mirror; }
    calcu::sessions::ListenerId listener() const { return m_listener; }

    // A command from this listener; its reply, once it came, from takeReply().
    calcu::sessions::CallId send(const calcu::sessions::CommandName& name, calcu::sessions::Value args = {});
    std::optional<calcu::sessions::ReplyMessage> takeReply(calcu::sessions::CallId call);
    // A command, then `step` (one host call) until its reply arrives: a test
    // on a scripted clock, a frozen frontend (the --shots runner).
    calcu::sessions::ReplyMessage runToEnd(const calcu::sessions::CommandName& name, calcu::sessions::Value args,
                                           const std::function<void()>& step);
    calcu::sessions::ReplyMessage ask(const calcu::sessions::RequestName& name, calcu::sessions::Value args = {});

    // The last state of a topic, as P, or nullptr.
    template <class P> const P* state(const calcu::sessions::TopicName& topic) const {
        const calcu::sessions::PayloadPtr* p = m_mirror.state(topic);
        return p ? calcu::sessions::payloadAs<P>(*p) : nullptr;
    }
    const calcu::machine::DisplayOutput* display() const;
    bool busy() const;
    // Every message, after the mirror took it (sound, notices, replies).
    std::function<void(const calcu::sessions::Message&)> onMessage;

    void deliver(const calcu::sessions::Message& message) override;

private:
    std::unique_ptr<SharpAppPart> m_part;
    std::unique_ptr<calcu::sessions::AppSession> m_session;
    calcu::sessions::TopicMirror m_mirror;
    calcu::sessions::ListenerId m_listener = 0;
    std::map<calcu::sessions::CallId, calcu::sessions::ReplyMessage> m_replies;
};

} // namespace shared
