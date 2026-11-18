#pragma once
#include <memory>
#include <optional>
#include <utility>

#include "pacing/WallClock.hpp"
#include "qt6/SessionHost.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/LibraryTopics.hpp"

// A Qt6 frontend's product session and the SessionHost over it: the common
// half of the products' controllers. Made from the product's session
// (shell::makeHostSession, with the shell's SettingsPart); start() starts the
// host's timer, then sends `start`; the end stops the timer and closes the
// session (the parts' closing(): what a medium still has to write) before the
// host and the session go, so no message goes out after it. On the GUI
// thread.

namespace calcu::qt6 {

template <class Part> class ProductHost {
public:
    ProductHost(sessions::ProductSession<Part> app, sessions::HostServices services,
                pacing::WallClock clock = pacing::steadyWallTime)
        : app_(std::move(app)),
          host_(std::make_unique<SessionHost>(*app_.session, std::move(services), std::move(clock))) {}
    ~ProductHost() {
        host_->stop();
        app_.session->close();
        host_.reset();   // stops listening before the session goes
        app_ = {};
    }
    ProductHost(const ProductHost&) = delete;
    ProductHost& operator=(const ProductHost&) = delete;

    SessionHost& host() { return *host_; }
    const SessionHost& host() const { return *host_; }
    Part& part() { return *app_.part; }
    const Part& part() const { return *app_.part; }
    sessions::AppSession& session() { return *app_.session; }

    void start() {
        host_->start();
        host_->send(sessions::kStartCommand);
    }

    // A request, answered at once (requests do not wait); an empty
    // reply if none came.
    sessions::ReplyMessage ask(const sessions::RequestName& name, sessions::Value args = {}) {
        auto got = std::make_shared<std::optional<sessions::ReplyMessage>>();
        host_->ask(name, std::move(args), [got](const sessions::ReplyMessage& r) { *got = r; });
        return *got ? **got : sessions::ReplyMessage{};
    }

private:
    sessions::ProductSession<Part> app_;
    std::unique_ptr<SessionHost> host_;
};

} // namespace calcu::qt6
