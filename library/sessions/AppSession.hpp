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

// The app session (LIB-3 design §3.9; spec AC-1, AC-2, AC-4, AC-6, AC-26):
// one per window, for as long as it is open. It holds one machine session,
// the topics, commands and requests of the parts, the running operation, the
// busy state and the audio policy. Single-threaded: it holds no lock, starts
// no thread and throws nothing across its interface (ADR 0009, 0015;
// frontend.md I-1); everything crosses it as messages.

namespace calcu::sessions {

class MachineSession;

enum class Hosting { Paced, VirtualTime };   // VirtualTime: the harness, AC-19 tests

struct AppSessionSetup {
    pacing::WallClock clock;   // the host's; the pacer reads it within a call
    // A-7: local date and time in the products, a fixed instant in the
    // harness (for LIB-2's real-time clocks).
    std::function<std::chrono::system_clock::time_point()> hostTime;
    std::vector<SessionPart*> parts;   // library features first, then the product's
    Hosting hosting = Hosting::Paced;
    std::optional<std::uint32_t> jitterSeed;   // LIB-17 AC-10
};

class AppSession {
public:
    explicit AppSession(AppSessionSetup);
    ~AppSession();
    AppSession(const AppSession&) = delete;
    AppSession& operator=(const AppSession&) = delete;

    // AC-6, AC-26: the new listener is sent every state topic once and the
    // stream topics with history since their last reset.
    ListenerId listen(MessageSink&, HostServices = {});
    void stopListening(ListenerId);

    void call(pacing::WallTime now);           // AC-2; Paced only
    CallId send(ListenerId, Command);          // carried out at once (spec OQ-3)
    CallId ask(ListenerId, Request);
    void answer(CallId ask, ServiceAnswer);    // AC-27
    machine::Ticks advanceUnpaced(machine::Ticks);   // virtual-time hosts (LIB-11 `wait`)

    SessionContext& context();                 // for the host's own adapters until their steps
    MachineSession& machineSession();
    bool busy() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace calcu::sessions
