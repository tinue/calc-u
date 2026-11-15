#include "app/SharpHost.hpp"

#include <chrono>

namespace shared {

namespace sessions = calcu::sessions;

SharpHost::SharpHost(Settings& settings, AppPaths& paths, Options options)
    : m_part(std::make_unique<SharpAppPart>(settings, paths)) {
    sessions::AppSessionSetup setup;
    setup.clock = options.clock ? std::move(options.clock) : calcu::pacing::WallClock([] {
        return std::chrono::duration_cast<calcu::pacing::WallTime>(std::chrono::steady_clock::now().time_since_epoch());
    });
    setup.hostTime = std::move(options.hostTime);
    setup.parts = m_part->sessionParts();
    setup.hosting = options.hosting;
    m_session = std::make_unique<sessions::AppSession>(std::move(setup));
    m_part->bind(*m_session);
    if (options.hostDirectories) options.services.offered.insert(sessions::HostService::FolderPicker);
    m_listener = m_session->listen(*this, std::move(options.services));
}

SharpHost::~SharpHost() {
    if (m_session) m_session->stopListening(m_listener);
    m_part.reset();      // first: it saves the bays and the tape into the live machine's files
    m_session.reset();   // its parts and hooks are not called again
}

void SharpHost::deliver(const sessions::Message& message) {
    m_mirror.deliver(message);
    if (const auto* r = std::get_if<sessions::ReplyMessage>(&message)) m_replies[r->call] = *r;
    if (onMessage) onMessage(message);
}

sessions::CallId SharpHost::send(const sessions::CommandName& name, sessions::Value args) {
    return m_session->send(m_listener, sessions::Command{name, std::move(args)});
}

std::optional<sessions::ReplyMessage> SharpHost::takeReply(sessions::CallId call) {
    const auto it = m_replies.find(call);
    if (it == m_replies.end()) return std::nullopt;
    sessions::ReplyMessage r = std::move(it->second);
    m_replies.erase(it);
    return r;
}

sessions::ReplyMessage SharpHost::runToEnd(const sessions::CommandName& name, sessions::Value args,
                                           const std::function<void()>& step) {
    const sessions::CallId id = send(name, std::move(args));
    for (;;) {
        if (auto r = takeReply(id)) return *r;
        step();
    }
}

sessions::ReplyMessage SharpHost::ask(const sessions::RequestName& name, sessions::Value args) {
    const sessions::CallId id = m_session->ask(m_listener, sessions::Request{name, std::move(args)});
    if (auto r = takeReply(id)) return *r;
    return sessions::replyRefused("No reply.");
}

const calcu::machine::DisplayOutput* SharpHost::display() const {
    const auto* d = state<sessions::DisplayState>(sessions::kDisplayTopic);
    return d ? &d->output : nullptr;
}

bool SharpHost::busy() const { return m_session->busy(); }

} // namespace shared
