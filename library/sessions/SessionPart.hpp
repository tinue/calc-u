#pragma once
#include "machine/Machine.hpp"
#include "pacing/WallClock.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Operation.hpp"
#include "sessions/Topics.hpp"
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <variant>

// Session parts (LIB-3 design §3.8; docs/contracts/README.md L-1; spec AC-12,
// FE-6): what a library feature or a product's app layer plugs into the app
// session. A product's names carry its prefix ("c59.", "c1600."); the
// library carries its topics, commands and requests without reading them.

namespace calcu::sessions {

class MachineSession;

struct Done { PayloadPtr result; };
struct Refused { std::string reason; };   // a refused command changes nothing (AC-13)
struct StartOperation { Operation op; };  // refused while another runs (AC-16)
// The reply comes later, through SessionContext::reply(currentCall(), …),
// once; the session is not busy meanwhile (LIB-5 design §3.5, D-4: `key`
// and `type`; LIB-10's `pause` and `step` from M-40).
struct Deferred {};
using CommandOutcome = std::variant<Done, Refused, StartOperation, Deferred>;

// What parts, handlers and operations see, on the session's thread.
class SessionContext {
public:
    virtual MachineSession& machineSession() = 0;
    virtual machine::Machine* machine() = 0;
    virtual void notice(std::string sentence) = 0;   // AC-10
    virtual void log(std::string line) = 0;          // AC-11
    virtual bool busy() const = 0;                    // an operation runs
    virtual bool hostOffers(HostService) const = 0;   // AC-26
    // AC-27: asks the first listener that offers the service; `then` runs
    // when the answer arrives. std::nullopt: nobody offers it, nothing asked.
    virtual std::optional<CallId> askHost(HostService, Value args, std::function<void(const ServiceAnswer&)> then) = 0;
    virtual std::chrono::system_clock::time_point hostTime() const = 0;   // A-7
    // The speed outside an operation, as the `speed` command sets it (AC-21).
    // A part sets it for a speed its product asks for itself (Calc-U-59's
    // KEYSTROKES FullSpeed:, until LIB-6 59); like the command, it changes
    // nothing while an operation runs (D-6).
    virtual pacing::Speed commandedSpeed() const = 0;
    virtual void setCommandedSpeed(pacing::Speed) = 0;
    // The display as the `display` topic holds it now (read in this host
    // call before afterCall), or nullptr: for a part that sends the display
    // in a product's own form (Calc-U-1600's Web page, M-31). Nobody else
    // reads the machine's display (LIB-1 §3.5: one reader).
    virtual const machine::DisplayOutput* display() const { return nullptr; }
    // A deferred reply (LIB-5 §3.5): the call of the command whose handler
    // runs now (0 outside one), and its reply later, once; a second reply,
    // or one to a call that did not defer, is ignored.
    virtual CallId currentCall() const { return 0; }
    virtual void reply(CallId, ReplyMessage) {}

protected:
    ~SessionContext() = default;
};

// At attach: what a part declares, and the context it keeps for later (its
// slice hook, gate and wait probe go on context().machineSession() here).
class SessionSetup {
public:
    using Handler = std::function<CommandOutcome(const Value& args, SessionContext&)>;
    using RequestHandler = std::function<ReplyMessage(const Value& args, SessionContext&)>;

    virtual SessionContext& context() = 0;

    virtual StateTopic& stateTopic(TopicName) = 0;
    virtual StreamTopic& streamTopic(TopicName, History) = 0;
    // endsRunningOperation: when its handler starts an operation, the
    // command first stops a running one (Reset, Cold boot, a model switch;
    // LIB-18 AC-16, D-6); a handler that gives Done or Refused stops nothing.
    virtual void command(CommandName, Handler, bool endsRunningOperation = false) = 0;
    virtual void request(RequestName, RequestHandler) = 0;

protected:
    ~SessionSetup() = default;
};

class SessionPart {
public:
    // Once, while the session is built, in the order of the parts. A part may
    // set the first machine here; the session reads its display and tells
    // every part (machineChanged) before it is first called.
    virtual void attach(SessionSetup&) = 0;
    virtual void beforeCall(SessionContext&, pacing::WallTime) {}
    virtual void afterCall(SessionContext&, pacing::WallTime) {}   // per-call duties
    // At the start of every flush (the end of every host call, command,
    // request and service answer, of advanceUnpaced and of listen), after the
    // parts' afterCall, on the session's thread: a state computed from the
    // whole session, set on its topic here (LIB-4's control state, LIB-4
    // design §3.5). It sets topics only and calls nothing of the session.
    virtual void beforeFlush(SessionContext&) {}
    virtual void machineChanged(SessionContext&) {}
    // An operation ended or was stopped (AC-18); e.g. LIB-2's real-time
    // clocks set from the host (LIB-2 AC-23).
    virtual void operationEnded(SessionContext&) {}
    virtual void suspending(SessionContext&) {}

protected:
    ~SessionPart() = default;
};

} // namespace calcu::sessions
