#pragma once
#include "pacing/WallClock.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include "sessions/SessionPart.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

// The app session:
// one per window, for as long as it is open. It holds one machine session,
// the topics, commands and requests of the parts, the running operation, the
// busy state and the audio policy. Single-threaded: it holds no lock, starts
// no thread and throws nothing across its interface; everything
// crosses it as messages.

namespace calcu::sessions {

class MachineSession;

enum class Hosting { Paced, VirtualTime };   // VirtualTime: the harness and tests

struct AppSessionSetup {
    pacing::WallClock clock;   // the host's; the pacer reads it within a call
    // The host time: local date and time in the products, a fixed instant
    // in the harness. The session sets the real-time clocks with it at build
    // and, while clockFollowsHost, at the end of every operation and resume.
    std::function<std::chrono::system_clock::time_point()> hostTime;
    bool clockFollowsHost = true;
    std::vector<SessionPart*> parts;   // library features first, then the product's
    Hosting hosting = Hosting::Paced;
    std::optional<std::uint32_t> jitterSeed;   // same seed, same slice lengths; none: differs per run
    // The parts and what they need (settings, paths), when the session owns
    // them: released first when the session goes, while its machine is still
    // there (a part saves its media into the machine's files), and never
    // called again.
    std::shared_ptr<void> owned;
};

class AppSession {
public:
    explicit AppSession(AppSessionSetup);
    ~AppSession();
    AppSession(const AppSession&) = delete;
    AppSession& operator=(const AppSession&) = delete;

    // The new listener is sent every state topic once and the
    // stream topics with history since their last reset.
    ListenerId listen(MessageSink&, HostServices = {});
    void stopListening(ListenerId);

    void call(pacing::WallTime now);           // Paced only
    CallId send(ListenerId, Command);          // carried out at once, its reply and
                                               // changed topics sent at once
    CallId ask(ListenerId, Request);
    void answer(CallId ask, ServiceAnswer);
    machine::Ticks advanceUnpaced(machine::Ticks);   // virtual-time hosts (the harness's `wait`)
    // The product quits or the window closes.
    // Every part's closing() once (a part writes what it has to save); no
    // message goes out after it. The host calls it while its parts are
    // there, before it destroys the session; the destructor calls no part.
    void close();

    // The session's context, for a host's own lines and checks: a log line
    // about its setup, what a listener offers (debug builds), tests.
    SessionContext& context();
    MachineSession& machineSession();
    bool busy() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

// A product's session as its factory makes it: the session, which owns the
// product's part (AppSessionSetup::owned), and the part for the host's own
// calls until their topics and commands exist.
template <class Part> struct ProductSession {
    std::unique_ptr<AppSession> session;
    Part* part = nullptr;
};

} // namespace calcu::sessions
