#include "sessions/AppSession.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace calcu::sessions {

using machine::Ticks;
using pacing::Speed;

namespace {

// The audio rules, in one place: the session opens, primes and closes the
// sink; the sink writes.
constexpr int kSilenceThreshold = 8;   // a sample this small is silence
constexpr int kPrimeMs = 40;           // silence written first when the sink opens

bool silent(const std::vector<std::int16_t>& s) {
    return std::all_of(s.begin(), s.end(), [](std::int16_t x) { return std::abs(x) <= kSilenceThreshold; });
}

// The running operation: its job, and where the slices of the pacer go.
struct Running final : SliceSource {
    Operation op;   // kept alive while its coroutine runs
    std::unique_ptr<OperationContext> ctx;
    Job<OperationResult> job;
    ListenerId listener = 0;
    CallId call = 0;
    std::optional<OperationResult> failure;   // an exception outside the coroutine

    void step(std::uint64_t budget) {
        try {
            job.step(budget);
        } catch (const std::exception& e) {
            failure = OperationResult{false, e.what()};
        } catch (...) {
            failure = OperationResult{false, "The operation failed."};
        }
    }
    bool over() const { return failure.has_value() || job.done(); }

    OperationResult result() {
        if (failure) return *failure;
        try {
            return job.result();
        } catch (const std::exception& e) {
            return OperationResult{false, e.what()};
        } catch (...) {
            return OperationResult{false, "The operation failed."};
        }
    }

    // One slice: whole run requests of the operation until at least the
    // slice's ticks ran; the operation ending (or the machine changing)
    // ends the call.
    pacing::SliceResult runSlice(MachineSession& ms, const pacing::SliceRequest& r) override {
        pacing::SliceResult res;
        const std::uint64_t gen = ms.generation();
        const Ticks t0 = ms.machine() ? ms.machine()->tickCount() : 0;
        step(r.ticks);
        const bool same = ms.generation() == gen && ms.machine();
        res.ticksRun = same ? ms.machine()->tickCount() - t0 : 0;
        // A wait for the host's next call (hostWait) ends the host call too.
        res.stoppedEarly = over() || !same || job.waitsOnHost();
        // An operation runs as fast as the host allows: it never reports
        // waiting, so full speed is not paced while the machine waits for it.
        res.waitsForInput = false;
        return res;
    }
};

struct Listener {
    ListenerId id;
    MessageSink* sink;
    HostServices services;
};

struct Out {
    std::optional<ListenerId> to;   // nullopt: every listener
    Message message;
};

} // namespace

struct AppSession::Impl final : SessionContext, SessionSetup {
    explicit Impl(AppSessionSetup s);

    // SessionContext
    MachineSession& machineSession() override { return ms; }
    machine::Machine* machine() override { return ms.machine(); }
    void notice(std::string sentence) override { notices->append(std::make_shared<Notice>(std::move(sentence))); }
    void log(std::string line) override { logTopic->append(std::make_shared<LogLine>(std::move(line))); }
    LogLevel logLevel() const override { return level; }
    bool busy() const override { return running != nullptr; }
    bool hostOffers(HostService s) const override { return offerer(s) != nullptr; }
    std::optional<CallId> askHost(HostService, Value, std::function<void(const ServiceAnswer&)>) override;
    std::chrono::system_clock::time_point hostTime() const override {
        return setup.hostTime ? setup.hostTime() : std::chrono::system_clock::now();
    }
    bool clockFollowsHost() const override { return setup.clockFollowsHost; }
    Speed commandedSpeed() const override { return commanded; }
    void setCommandedSpeed(Speed sp) override {
        if (!running) commanded = sp;   // nothing while an operation runs
    }
    const machine::DisplayOutput* display() const override {
        const auto* d = payloadAs<DisplayState>(this->displayTopic());
        return d ? &d->output : nullptr;
    }
    const PayloadPtr& displayTopic() const { return displayState->current(); }
    CallId currentCall() const override { return current; }
    pacing::WallTime wallTime() const override { return setup.clock ? setup.clock() : pacing::WallTime{0}; }
    bool virtualTime() const override { return setup.hosting == Hosting::VirtualTime; }
    bool startOperation(Operation op) override {
        if (running) return false;
        start(0, 0, std::move(op));   // call 0: no reply (endOperation)
        return true;
    }
    void tellRestarted(Restart r) override {
        for (SessionPart* p : setup.parts) p->machineRestarted(*this, r);
    }
    void reply(CallId call, ReplyMessage m) override {
        const auto it = deferred.find(call);
        if (it == deferred.end()) return;
        const ListenerId l = it->second;
        deferred.erase(it);
        m.call = call;
        replies.push_back(Out{l, std::move(m)});
    }

    // SessionSetup
    SessionContext& context() override { return *this; }
    StateTopic& stateTopic(TopicName) override;
    StreamTopic& streamTopic(TopicName, History) override;
    void command(CommandName name, Handler h, bool ends = false) override {
        // One owner per command: a second
        // declaration of a name would silently replace the first.
        const std::string text = name.value;
        if (!commands.emplace(std::move(name), CommandEntry{std::move(h), ends}).second)
            throw std::logic_error("The command " + text + " is declared twice.");
    }
    void request(RequestName name, RequestHandler h) override { requests[name] = std::move(h); }

    void registerLibraryCommands();
    const Listener* offerer(HostService) const;
    void reply(ListenerId l, CallId id, bool done, std::string reason, PayloadPtr result = nullptr) {
        replies.push_back(Out{l, ReplyMessage{id, done, std::move(reason), std::move(result)}});
    }
    void runCommand(ListenerId, CallId, const Command&);
    void start(ListenerId, CallId, Operation);
    void endOperation();
    void stopOperation();
    void stepOperation(pacing::PaceResult&);
    void syncMachine();
    void readDisplay();
    void sound(bool discard);
    void flush();
    void deliver(const Out&);

    AppSessionSetup setup;
    MachineSession ms;
    std::deque<StateTopic> states;    // deque: references stay valid
    std::deque<StreamTopic> streams;
    std::vector<TopicEntry> declared;
    StateTopic* displayState = nullptr;
    StateTopic* busyTopic = nullptr;
    StreamTopic* notices = nullptr;
    StreamTopic* logTopic = nullptr;
    StateTopic* logLevelTopic = nullptr;
    LogLevel level = LogLevel::Off;

    struct CommandEntry {
        Handler handler;
        bool endsRunningOperation = false;
    };
    std::map<CommandName, CommandEntry> commands;
    std::map<RequestName, RequestHandler> requests;

    std::vector<Listener> listeners;
    ListenerId nextListener = 0;
    CallId nextCall = 0;
    std::vector<Out> outbox, replies, asks;
    std::map<CallId, std::function<void(const ServiceAnswer&)>> waiting;   // asks without an answer yet
    CallId current = 0;                          // the command whose handler runs (currentCall)
    std::map<CallId, ListenerId> deferred;       // commands whose reply comes later (Deferred)

    Speed commanded = Speed::Regular;
    bool closed = false;   // close() was called
    std::unique_ptr<Running> running;
    std::uint64_t seenGeneration = 0;

    // Sound.
    std::optional<ListenerId> sinkListener;
    bool sinkOpen = false;
    std::int64_t silentSamples = 0;
    std::vector<ListenerId> recordersStarted;
    std::vector<std::int16_t> drained;
};

AppSession::Impl::Impl(AppSessionSetup s) : setup(std::move(s)), ms(setup.clock, setup.jitterSeed) {
    displayState = &stateTopic(kDisplayTopic);
    busyTopic = &stateTopic(kBusyTopic);
    notices = &streamTopic(kNoticesTopic, History::None);
    logTopic = &streamTopic(kLogTopic, History::SinceReset);
    logLevelTopic = &stateTopic(kLogLevelTopic);
    logLevelTopic->set(std::make_shared<LogLevelState>(level));
    registerLibraryCommands();
    ms.setAboutToChange([this] {
        for (SessionPart* p : setup.parts) p->beforeMachineChange(*this);
    });
    for (SessionPart* p : setup.parts) p->attach(*this);
    syncMachine();   // a machine a part set at attach
    flush();
}

StateTopic& AppSession::Impl::stateTopic(TopicName name) {
    for (auto& t : states)
        if (t.name() == name) return t;
    declared.push_back(TopicEntry{name, false});
    return states.emplace_back(std::move(name));
}

StreamTopic& AppSession::Impl::streamTopic(TopicName name, History h) {
    for (auto& t : streams)
        if (t.name() == name) return t;
    declared.push_back(TopicEntry{name, true});
    return streams.emplace_back(std::move(name), h);
}

void AppSession::Impl::registerLibraryCommands() {
    command(kSuspendCommand, [this](const Value&, SessionContext&) -> CommandOutcome {
        if (!ms.suspended()) {
            for (SessionPart* p : setup.parts) p->suspending(*this);
            ms.setSuspended(true);
        }
        return Done{};
    });
    command(kContinueCommand, [this](const Value&, SessionContext&) -> CommandOutcome {
        ms.setSuspended(false);
        return Done{};
    });
    command(kSpeedCommand, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const Value* full = args.find("full");
        if (!full || !full->boolean()) return Refused{"The speed command needs full: true or false."};
        if (!running) commanded = *full->boolean() ? Speed::Full : Speed::Regular;   // nothing while an operation runs
        return Done{};
    });
    command(kLogLevelCommand, [this](const Value& args, SessionContext&) -> CommandOutcome {
        const Value* l = args.find("level");
        const auto named = l && l->string() ? logLevelNamed(*l->string()) : std::nullopt;
        if (!named) return Refused{"The log level is off, info or debug."};
        level = *named;
        logLevelTopic->set(std::make_shared<LogLevelState>(level));
        return Done{};
    });
    auto key = [this](bool down) {
        return [this, down](const Value& args, SessionContext&) -> CommandOutcome {
            const Value* k = args.find("key");
            if (!k || !k->string()) return Refused{"The key command needs key: <key ID>."};
            machine::Machine* m = ms.machine();
            if (!m) return Refused{kNoMachine};
            const machine::KeyId id{*k->string()};
            if ((down ? m->keyDown(id) : m->keyUp(id)) == machine::KeyCall::UnknownKey)
                return Refused{"Unknown key: " + *k->string() + "."};
            return Done{};
        };
    };
    command(kKeyDownCommand, key(true));
    command(kKeyUpCommand, key(false));

    request(kTicksRequest, [this](const Value&, SessionContext&) {
        const machine::Machine* m = ms.machine();
        if (!m) return replyRefused(kNoMachine);
        return replyGiven(std::make_shared<EmulatedTimeReply>(m->tickCount(), m->elapsed().seconds().count()));
    });
    request(kCapabilitiesRequest, [this](const Value&, SessionContext&) {
        const machine::Machine* m = ms.machine();
        if (!m) return replyRefused(kNoMachine);
        auto offer = std::make_shared<MachineOffer>();
        offer->capabilities = m->capabilities();
        offer->plugged = m->plugged();
        for (const auto& p : m->descriptor().places) offer->places.push_back(p.name);
        return replyGiven(std::move(offer));
    });
    request(kTopicsRequest, [this](const Value&, SessionContext&) {
        auto list = std::make_shared<TopicList>();
        list->topics = declared;
        return replyGiven(std::move(list));
    });
}

const Listener* AppSession::Impl::offerer(HostService s) const {
    for (const Listener& l : listeners)
        if (l.services.offers(s)) return &l;
    return nullptr;
}

std::optional<CallId> AppSession::Impl::askHost(HostService s, Value args,
                                                std::function<void(const ServiceAnswer&)> then) {
    const Listener* l = offerer(s);
    if (!l) return std::nullopt;   // not asked
    const CallId id = ++nextCall;
    asks.push_back(Out{l->id, ServiceAsk{id, s, std::move(args)}});
    waiting[id] = std::move(then);
    return id;
}

// ── Commands and operations ──────────────────────────────────────────────

void AppSession::Impl::runCommand(ListenerId l, CallId id, const Command& c) {
    const auto it = commands.find(c.name);
    if (it == commands.end()) {
        reply(l, id, false, "Unknown command: " + c.name.value + ".");
        return;
    }
    CommandOutcome outcome;
    // A handler may reply to its own call before it returns Deferred (a key
    // that is up at once): the call is known as deferred from the start.
    deferred[id] = l;
    const CallId outer = std::exchange(current, id);
    try {
        outcome = it->second.handler(c.args, *this);
    } catch (const std::exception& e) {
        current = outer;
        deferred.erase(id);
        reply(l, id, false, e.what());
        return;
    } catch (...) {
        current = outer;
        deferred.erase(id);
        reply(l, id, false, "The command failed.");
        return;
    }
    current = outer;
    if (std::holds_alternative<Deferred>(outcome)) return;   // replied later (or already)
    deferred.erase(id);
    if (auto* d = std::get_if<Done>(&outcome)) reply(l, id, true, {}, std::move(d->result));
    else if (auto* r = std::get_if<Refused>(&outcome)) reply(l, id, false, std::move(r->reason));
    else {
        // A command declared endsRunningOperation stops the
        // running operation only when it starts one itself; Done (nothing to
        // do) and Refused leave it running.
        if (it->second.endsRunningOperation && running) stopOperation();
        start(l, id, std::move(std::get<StartOperation>(outcome).op));
    }
}

void AppSession::Impl::start(ListenerId l, CallId id, Operation op) {
    if (running) {   // whoever's command it is
        reply(l, id, false, kAnotherOperationRuns);
        return;
    }
    for (SessionPart* p : setup.parts) p->beforeOperation(*this);
    auto r = std::make_unique<Running>();
    r->op = std::move(op);
    r->ctx = std::make_unique<OperationContext>(*this);
    r->listener = l;
    r->call = id;
    try {
        r->job = Job<OperationResult>(r->op(*r->ctx));
    } catch (const std::exception& e) {
        if (id) reply(l, id, false, e.what());
        else notice(e.what());
        return;
    }
    running = std::move(r);
    running->step(0);   // up to its first run request
    if (setup.hosting == Hosting::VirtualTime && !running->over()) {   // in one go
        running->step(std::numeric_limits<std::uint64_t>::max());
        if (!running->over()) running->failure = OperationResult{false, "The operation did not finish."};
        if (ms.generation() == seenGeneration) readDisplay();   // else syncMachine reads it
    }
    if (running->over()) endOperation();
}

// The reply says how it ended and goes out at the end of the operation, a
// failure is also a notice, and pacing starts afresh.
void AppSession::Impl::endOperation() {
    std::unique_ptr<Running> r = std::move(running);
    OperationResult res = r->result();
    if (!res.ok && res.message.empty()) res.message = "The operation failed.";
    if (r->call) reply(r->listener, r->call, res.ok, res.ok ? std::string() : res.message, res.ok ? res.result : nullptr);
    if (!res.ok) notice(res.message);
    ms.startAfresh();
    for (SessionPart* p : setup.parts) p->operationEnded(*this);
}

void AppSession::Impl::stopOperation() {   // replied "Stopped."
    std::unique_ptr<Running> r = std::move(running);
    if (r->call) reply(r->listener, r->call, false, kStopped);
    ms.startAfresh();
    for (SessionPart* p : setup.parts) p->operationEnded(*this);
}

// One part of the running operation: the pacer at the operation's
// speed, with the slices taken from its run requests.
void AppSession::Impl::stepOperation(pacing::PaceResult& res) {
    if (!running->over() && !ms.machine()) running->failure = OperationResult{false, kNoMachine};
    if (!running->over() && ms.runs()) {
        ms.setSliceSource(running.get());
        res = ms.pace(running->ctx->speed());
        ms.setSliceSource(nullptr);
    } else if (!running->over() && ms.machine() && running->job.waitsOnHost()) {
        running->step(1);   // a host wait goes on while the machine is paused
    }
    if (running->over()) endOperation();
}

// ── Machine output, sound and the flush ─────────────────────────────────

void AppSession::Impl::readDisplay() {
    if (machine::Machine* m = ms.machine()) displayState->set(std::make_shared<DisplayState>(m->readDisplay()));
}

void AppSession::Impl::syncMachine() {
    if (ms.generation() == seenGeneration) return;
    seenGeneration = ms.generation();
    if (sinkOpen && sinkListener) outbox.push_back(Out{*sinkListener, SoundStop{}});
    sinkOpen = false;
    silentSamples = 0;
    recordersStarted.clear();
    readDisplay();
    for (SessionPart* p : setup.parts) p->machineChanged(*this);
}

// A recorder gets every sample; the audio sink gets the
// sound of regular speed outside an operation, opened on the first sound
// after silence (primed) and closed after a second of silence.
void AppSession::Impl::sound(bool discard) {
    machine::Machine* m = ms.machine();
    if (!m) return;
    drained.clear();
    std::int16_t buf[1024];
    for (;;) {
        const std::size_t n = m->takeSound(std::span<std::int16_t>(buf));
        if (n == 0) break;
        drained.insert(drained.end(), buf, buf + n);
    }
    const int rate = ms.soundRate();

    for (const Listener& l : listeners) {
        if (!l.services.recordsSound || drained.empty()) continue;
        if (std::find(recordersStarted.begin(), recordersStarted.end(), l.id) == recordersStarted.end()) {
            recordersStarted.push_back(l.id);
            outbox.push_back(Out{l.id, SoundStart{rate}});
        }
        outbox.push_back(Out{l.id, SoundSamples{drained}});
    }

    const Listener* sink = nullptr;
    for (const Listener& l : listeners)
        if (l.services.offers(HostService::AudioSink) && !l.services.recordsSound) {
            sink = &l;
            break;
        }
    if (!sink || sink->id != sinkListener) {
        sinkListener = sink ? std::optional<ListenerId>(sink->id) : std::nullopt;
        sinkOpen = false;
        silentSamples = 0;
    }
    if (!sink || discard || drained.empty()) return;
    const bool quiet = silent(drained);
    if (!sinkOpen) {
        if (quiet) return;
        sinkOpen = true;
        silentSamples = 0;
        outbox.push_back(Out{sink->id, SoundStart{rate}});
        outbox.push_back(Out{sink->id, SoundSamples{std::vector<std::int16_t>(std::size_t(rate * kPrimeMs / 1000), 0)}});
    }
    outbox.push_back(Out{sink->id, SoundSamples{drained}});
    silentSamples = quiet ? silentSamples + static_cast<std::int64_t>(drained.size()) : 0;
    if (silentSamples >= rate) {
        outbox.push_back(Out{sink->id, SoundStop{}});
        sinkOpen = false;
        silentSamples = 0;
    }
}

// The parts' beforeFlush, sound, then the state topics
// in declaration order, then the stream topics, then the replies, then the
// asks.
void AppSession::Impl::flush() {
    for (SessionPart* p : setup.parts) p->beforeFlush(*this);
    const bool runs = running != nullptr;
    const Speed speed = runs ? running->ctx->speed() : commanded;
    const auto* b = payloadAs<BusyState>(busyTopic->current());
    if (!b || b->operationRuns != runs || b->speed != speed) busyTopic->set(std::make_shared<BusyState>(runs, speed));

    for (auto& t : states)
        if (auto m = t.flush()) outbox.push_back(Out{std::nullopt, std::move(*m)});
    for (auto& t : streams)
        if (auto m = t.flush()) outbox.push_back(Out{std::nullopt, std::move(*m)});
    for (auto& r : replies) outbox.push_back(std::move(r));
    replies.clear();
    for (auto& a : asks) outbox.push_back(std::move(a));
    asks.clear();

    std::vector<Out> out;
    out.swap(outbox);
    for (const Out& o : out) deliver(o);
}

void AppSession::Impl::deliver(const Out& o) {
    if (closed) return;   // no message goes out after close()
    for (const Listener& l : listeners)
        if (!o.to || *o.to == l.id) l.sink->deliver(o.message);
}

// ── AppSession ──────────────────────────────────────────────────────────

AppSession::AppSession(AppSessionSetup s) : impl_(std::make_unique<Impl>(std::move(s))) {}
AppSession::~AppSession() {
    // No part is called from here: a part the session does not own may be
    // gone already; a host calls close() first.
    impl_->ms.setAboutToChange(nullptr);
    impl_->setup.owned.reset();   // the parts first, while the machine is there
}

void AppSession::close() {
    Impl& s = *impl_;
    if (s.closed) return;
    for (SessionPart* p : s.setup.parts) p->closing(s);   // what is to be saved, written
    s.closed = true;
}

ListenerId AppSession::listen(MessageSink& sink, HostServices services) {
    Impl& s = *impl_;
    s.flush();   // what is pending goes to the listeners there are
    const ListenerId id = ++s.nextListener;
    s.listeners.push_back(Listener{id, &sink, std::move(services)});
    for (const auto& t : s.states)
        if (t.lastSent()) sink.deliver(StateMessage{t.name(), t.lastSent()});
    for (const auto& t : s.streams)
        if (auto m = t.replay()) sink.deliver(*m);
    return id;
}

void AppSession::stopListening(ListenerId id) {
    auto& ls = impl_->listeners;
    ls.erase(std::remove_if(ls.begin(), ls.end(), [id](const Listener& l) { return l.id == id; }), ls.end());
}

void AppSession::call(pacing::WallTime now) {
    Impl& s = *impl_;
    if (s.setup.hosting != Hosting::Paced || s.ms.suspended()) return;   // a suspend holds a running operation too
    for (SessionPart* p : s.setup.parts) p->beforeCall(s, now);
    pacing::PaceResult res;
    const bool operation = s.running != nullptr;
    // Full speed throws its sound away in every slice: also where the pacer
    // runs a waiting machine at the pace of regular speed.
    const bool full = s.commanded == Speed::Full;
    if (operation) s.stepOperation(res);
    else res = s.ms.pace(s.commanded);
    const bool changed = s.ms.generation() != s.seenGeneration;
    s.syncMachine();
    if (!changed && res.ticksRun > 0) s.readDisplay();
    for (SessionPart* p : s.setup.parts) p->afterCall(s, now);
    s.syncMachine();
    s.sound(full || operation);
    s.flush();
}

CallId AppSession::send(ListenerId l, Command c) {
    Impl& s = *impl_;
    const CallId id = ++s.nextCall;
    s.runCommand(l, id, c);
    s.syncMachine();
    s.flush();
    return id;
}

CallId AppSession::ask(ListenerId l, Request q) {
    Impl& s = *impl_;
    const CallId id = ++s.nextCall;
    ReplyMessage m;
    const auto it = s.requests.find(q.name);
    if (it == s.requests.end()) m = replyRefused("Unknown request: " + q.name.value + ".");
    else {
        try {
            m = it->second(q.args, s);
        } catch (const std::exception& e) {
            m = replyRefused(e.what());
        }
    }
    m.call = id;
    s.replies.push_back(Out{l, std::move(m)});
    s.flush();
    return id;
}

void AppSession::answer(CallId a, ServiceAnswer ans) {
    Impl& s = *impl_;
    const auto it = s.waiting.find(a);
    if (it == s.waiting.end()) return;
    auto then = std::move(it->second);
    s.waiting.erase(it);
    if (then) {
        try {
            then(ans);
        } catch (const std::exception& e) {
            s.notice(e.what());
        }
    }
    s.syncMachine();
    s.flush();
}

Ticks AppSession::advanceUnpaced(Ticks ticks) {
    Impl& s = *impl_;
    if (s.running) return 0;
    const Ticks ran = s.ms.advanceUnpaced(ticks);
    const bool changed = s.ms.generation() != s.seenGeneration;
    s.syncMachine();
    if (!changed && ran > 0) s.readDisplay();
    // The parts' per-call duties, as call() runs them. A host in
    // virtual time has no wall clock to read.
    const pacing::WallTime now = s.setup.hosting == Hosting::VirtualTime ? pacing::WallTime{0} : s.wallTime();
    for (SessionPart* p : s.setup.parts) p->afterCall(s, now);
    s.syncMachine();
    s.sound(true);   // not real time: only a recorder gets it
    s.flush();
    return ran;
}

SessionContext& AppSession::context() { return *impl_; }
MachineSession& AppSession::machineSession() { return impl_->ms; }
bool AppSession::busy() const { return impl_->running != nullptr; }

} // namespace calcu::sessions
