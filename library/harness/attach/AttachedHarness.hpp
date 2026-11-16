#pragma once
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "harness/attach/AttachEndpoint.hpp"
#include "harness/attach/SessionParts.hpp"
#include "sessions/AppSession.hpp"
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>

// Harness attach, the session's side (C59-5 AC-11, AC-12, AC-14, EC-2,
// EC-3; design §3.2): on the thread that hosts the app session. While a
// client is attached it is a second listener of the session (HarnessHost in
// its attached form, no host services) and runs LIB-11's commands on it as
// `calcu serve` does, with one difference: time is the app's. `wait <d>`
// replies when the app's own host calls have let d of emulated time pass;
// the harness never advances the machine (AC-14). The bare `wait` replies
// when the app's machine is idle (LIB-6 AC-34; LIB-14 D-27: the session's
// `wait-idle` operation, at most an hour). The commands are those of
// `calcu serve` (D-27): the first, startup, preset, typing, paper, debug,
// media and open commands, then the product's; the preset steps and the
// media commands use the session's parts named in SessionParts. Replies and
// the listener's messages go back as lines (LIB-11 design §3.9).

namespace calcu::harness {

class AttachedHarness {
public:
    static constexpr const char* kSuspended = "the app is suspended";

    AttachedHarness(sessions::AppSession&, HarnessProduct&, AttachEndpoint&, std::function<bool()> callsSuspended,
                    SessionParts parts = {});
    ~AttachedHarness();

    void handle(const AttachEvent&);
    void afterHostCall();   // a pending `wait` or reply that is due goes out
    bool attached() const { return host_ != nullptr; }
    const CommandRegistry& commands() const { return registry_; }   // D-27: the serve form's

private:
    struct Pending {
        bool wait = false;
        bool idle = false;   // the bare `wait`: the reply of `wait-idle` with the time it took
        machine::Ticks start = 0, target = 0;
        machine::TickRate rate = 1;
        sessions::CallId call = 0;
    };
    void process();
    void runLine(const std::string&);
    void reply(const Reply&);
    void lines(const std::string& text);
    void message(const sessions::Message&);
    std::optional<machine::Ticks> ticks();
    void detach();

    sessions::AppSession& session_;
    HarnessProduct& product_;
    AttachEndpoint& endpoint_;
    std::function<bool()> suspended_;
    CommandRegistry registry_;
    std::unique_ptr<HarnessHost> host_;
    std::uint64_t client_ = 0;
    bool json_ = false;
    std::deque<std::string> queue_;
    std::optional<Pending> pending_;
    std::set<std::string> held_;   // keys down through this harness (EC-2)
};

} // namespace calcu::harness
