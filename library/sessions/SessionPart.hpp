#pragma once
#include "machine/Machine.hpp"
#include "pacing/WallClock.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/LogLevel.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Operation.hpp"
#include "sessions/Topics.hpp"
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <variant>

// Session parts: what a library feature or a product's app layer plugs into the app
// session. A product's names carry its prefix ("c59.", "c1600."); the
// library carries its topics, commands and requests without reading them.

namespace calcu::sessions {

class MachineSession;

struct Done { PayloadPtr result; };
struct Refused { std::string reason; };   // a refused command changes nothing
struct StartOperation { Operation op; };  // refused while another runs
// The reply comes later, through SessionContext::reply(currentCall(), …),
// once; the session is not busy meanwhile (the typing's `key` and `type`,
// the debugger's `pause` and `step`).
struct Deferred {};
using CommandOutcome = std::variant<Done, Refused, StartOperation, Deferred>;

// A restart of the machine that is running: told after Machine::reset() or
// coldBoot(), before the run to key readiness. A new machine is machineChanged, not a restart.
enum class Restart { Reset, ColdBoot };

// What parts, handlers and operations see, on the session's thread.
class SessionContext {
public:
    virtual MachineSession& machineSession() = 0;
    virtual machine::Machine* machine() = 0;
    virtual void notice(std::string sentence) = 0;
    virtual void log(std::string line) = 0;   // at every level
    // The level the `log-level` command set; a line at a level above it is
    // left out.
    virtual LogLevel logLevel() const = 0;
    void logAt(LogLevel at, std::string line) {
        if (at <= logLevel()) log(std::move(line));
    }
    virtual bool busy() const = 0;                    // an operation runs
    virtual bool hostOffers(HostService) const = 0;
    // Asks the first listener that offers the service; `then` runs
    // when the answer arrives. std::nullopt: nobody offers it, nothing asked.
    virtual std::optional<CallId> askHost(HostService, Value args, std::function<void(const ServiceAnswer&)> then) = 0;
    virtual std::chrono::system_clock::time_point hostTime() const = 0;   // local time; a fixed instant in the harness
    // The real-time clocks follow the host time after every operation and
    // resume; false (a fixed instant): only at a build and `sync-clock`, so a
    // program that sets its clock keeps it.
    virtual bool clockFollowsHost() const = 0;
    // The speed outside an operation, as the `speed` command sets it.
    // A part sets it for a speed its product asks for itself (a preset or a
    // start that ends at regular speed); like the command, it changes
    // nothing while an operation runs.
    virtual pacing::Speed commandedSpeed() const = 0;
    virtual void setCommandedSpeed(pacing::Speed) = 0;
    // The display as the `display` topic holds it now (read in this host
    // call before afterCall), or nullptr: for a part that sends the display
    // in a product's own form (Calc-U-1600's Web page). Nobody else
    // reads the machine's display: it has one reader.
    virtual const machine::DisplayOutput* display() const { return nullptr; }
    // A deferred reply: the call of the command whose handler
    // runs now (0 outside one), and its reply later, once; a second reply,
    // or one to a call that did not defer, is ignored.
    virtual CallId currentCall() const { return 0; }
    virtual void reply(CallId, ReplyMessage) {}
    // The session's wall clock now, and whether the session is hosted in
    // virtual time (the harness: no wall clock); OperationContext::hostWait.
    virtual pacing::WallTime wallTime() const { return pacing::WallTime{0}; }
    virtual bool virtualTime() const { return true; }
    // SessionPart::machineRestarted on every part, in the order of the parts
    // (the startup's MachineControl calls it).
    virtual void tellRestarted(Restart) {}
    // An operation started by a part, not by a command (a file kept for later, opened in afterCall). False, and nothing
    // started, while one runs. Its end sends no reply; a failure is a
    // notice.
    virtual bool startOperation(Operation) { return false; }

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
    // command first stops a running one (Reset, Cold boot, a model switch); a handler that gives Done or Refused stops nothing.
    // A name has one owner: declaring it a second time throws
    // std::logic_error.
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
    // whole session, set on its topic here (the control part's state). It sets topics
    // only and calls nothing of the session.
    virtual void beforeFlush(SessionContext&) {}
    virtual void machineChanged(SessionContext&) {}
    // The machine was reset or cold booted (the debugger's run control has
    // restart rules).
    virtual void machineRestarted(SessionContext&, Restart) {}
    // An operation ended or was stopped; e.g. the real-time clocks are
    // set from the host time.
    virtual void operationEnded(SessionContext&) {}
    virtual void suspending(SessionContext&) {}
    // Before an operation's
    // first step; before the machine or a peripheral of it changes (a build,
    // an attach, detach or replace, MachineSession::aboutToChange); and when
    // the product quits or the window closes (AppSession::close(), which the
    // host calls before it destroys the session; no message goes out after it).
    virtual void beforeOperation(SessionContext&) {}
    virtual void beforeMachineChange(SessionContext&) {}
    virtual void closing(SessionContext&) {}

protected:
    ~SessionPart() = default;
};

} // namespace calcu::sessions
