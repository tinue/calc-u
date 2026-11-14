#pragma once
#include "ScriptedClock.hpp"
#include "TestFrontend.hpp"
#include "TestMachines.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// A test product part (LIB-3 design §6): its names carry a product's prefix
// ("tp." by default). One product topic of each shape, product commands that
// start an operation and that do not, one product request; and the rig the
// session tests share: a scripted clock, the session, a test frontend and a
// machine built for the tests.

namespace calcu::sessions::test {

using machine::Ticks;
using machine::test::TestMachine;
using namespace std::chrono_literals;

struct TestValue final : PayloadOf<TestValue> {
    static constexpr PayloadKind kKind{"tp.value"};
    explicit TestValue(std::int64_t x) : v(x) {}
    std::int64_t v;
    bool same(const TestValue& o) const { return v == o.v; }
    Value toValue() const override { return Value::object({{"v", v}}); }
};

struct TestText final : PayloadOf<TestText> {
    static constexpr PayloadKind kKind{"tp.text"};
    explicit TestText(std::string t) : text(std::move(t)) {}
    std::string text;
    bool same(const TestText& o) const { return text == o.text; }
    Value toValue() const override { return Value::object({{"text", text}}); }
};

// Operations for the tests. Parameters by value (Operation.hpp, rules).
inline Task<OperationResult> runFor(OperationContext& c, Ticks n) {
    co_await c.run(n);
    co_return OperationResult{};
}

inline Operation runningFor(Ticks n) {
    return [n](OperationContext& c) { return runFor(c, n); };
}

inline std::int64_t intArg(const Value& args, const char* key, std::int64_t fallback = 0) {
    const Value* v = args.find(key);
    return v && v->integer() ? *v->integer() : fallback;
}
inline std::string textArg(const Value& args, const char* key) {
    const Value* v = args.find(key);
    return v && v->string() ? *v->string() : std::string();
}

class TestPart : public SessionPart {
public:
    explicit TestPart(std::string prefix = "tp") : p_(std::move(prefix) + ".") {}

    TopicName stateName() const { return TopicName{p_ + "state"}; }
    TopicName streamName() const { return TopicName{p_ + "stream"}; }

    void attach(SessionSetup& s) override {
        state_ = &s.stateTopic(stateName());
        stream_ = &s.streamTopic(streamName(), History::SinceReset);
        s.command(CommandName{p_ + "set"}, [this](const Value& a, SessionContext&) -> CommandOutcome {
            set(intArg(a, "value"));
            return Done{};
        });
        s.command(CommandName{p_ + "append"}, [this](const Value& a, SessionContext&) -> CommandOutcome {
            stream_->append(std::make_shared<TestText>(textArg(a, "text")));
            return Done{};
        });
        s.command(CommandName{p_ + "reset"}, [this](const Value&, SessionContext&) -> CommandOutcome {
            stream_->reset();
            return Done{};
        });
        s.command(CommandName{p_ + "refuse"},
                  [](const Value&, SessionContext&) -> CommandOutcome { return Refused{"Not now."}; });
        s.command(CommandName{p_ + "op"}, [this](const Value&, SessionContext&) -> CommandOutcome {
            ++opsStarted;
            return StartOperation{operation};
        });
        s.command(
            CommandName{p_ + "stop"}, [](const Value&, SessionContext&) -> CommandOutcome { return Done{}; }, true);
        s.command(CommandName{p_ + "log"}, [](const Value& a, SessionContext& c) -> CommandOutcome {
            c.log(textArg(a, "text"));
            return Done{};
        });
        s.command(CommandName{p_ + "notice"}, [](const Value& a, SessionContext& c) -> CommandOutcome {
            c.notice(textArg(a, "text"));
            return Done{};
        });
        s.command(CommandName{p_ + "ask"}, [this](const Value& a, SessionContext& c) -> CommandOutcome {
            const auto id = c.askHost(static_cast<HostService>(intArg(a, "service")), a, [this](const ServiceAnswer& ans) {
                answers.push_back(ans);
                set(ans.given ? 1 : 0);
            });
            if (!id) return Refused{"Nobody offers it."};
            return Done{std::make_shared<TestValue>(static_cast<std::int64_t>(*id))};
        });
        s.request(RequestName{p_ + "echo"}, [](const Value& a, SessionContext&) {
            return replyGiven(std::make_shared<TestValue>(intArg(a, "value")));
        });
    }
    void beforeCall(SessionContext&, pacing::WallTime) override { ++beforeCalls; }
    void afterCall(SessionContext& c, pacing::WallTime now) override {
        ++afterCalls;
        if (onAfterCall) onAfterCall(c, now);
    }
    void machineChanged(SessionContext&) override { ++machineChanges; }
    void suspending(SessionContext&) override { ++suspendings; }

    void set(std::int64_t v) {
        lastSet = std::make_shared<TestValue>(v);
        state_->set(lastSet);
    }

    Operation operation = runningFor(10'000);
    std::function<void(SessionContext&, pacing::WallTime)> onAfterCall;
    std::shared_ptr<const TestValue> lastSet;
    std::vector<ServiceAnswer> answers;
    int beforeCalls = 0, afterCalls = 0, machineChanges = 0, suspendings = 0, opsStarted = 0;
    StateTopic* state_ = nullptr;
    StreamTopic* stream_ = nullptr;

private:
    std::string p_;
};

struct RigOptions {
    Hosting hosting = Hosting::Paced;
    HostServices services;
    TestFrontend::Delivery delivery = TestFrontend::Delivery::AtOnce;
    std::vector<SessionPart*> extraParts;
};

// One window: a session with the test part, its frontend listening.
class Rig {
public:
    explicit Rig(RigOptions o = {}) : fe(o.delivery) {
        AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [] { return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50)); };
        s.parts = {&part};
        for (SessionPart* p : o.extraParts) s.parts.push_back(p);
        s.hosting = o.hosting;
        s.jitterSeed = 1;
        session = std::make_unique<AppSession>(std::move(s));
        id = session->listen(fe, o.services);
    }
    Rig(const Rig&) = delete;
    Rig& operator=(const Rig&) = delete;

    // A machine built for the tests; "the call" profile unless given.
    TestMachine* build(const machine::ModelId& model = machine::test::kSeg, int soundRate = 8000,
                       pacing::PacingProfile profile = {}) {
        auto mm = machine::test::buildTestMachine(model, std::nullopt, {}, soundRate);
        m = mm.get();
        session->machineSession().setMachine(std::move(mm), profile, soundRate);
        return m;
    }
    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, Command{CommandName{name}, std::move(args)});
    }
    CallId ask(const std::string& name, Value args = Value()) {
        return session->ask(id, Request{RequestName{name}, std::move(args)});
    }
    // One host call, a frame after the last.
    void call(pacing::WallTime frame = 16ms) {
        clock.now += frame;
        session->call(clock.now);
    }
    // Host calls until no operation runs; the calls made.
    int callWhileBusy(int max = 10'000) {
        int n = 0;
        while (session->busy() && n < max) {
            call();
            ++n;
        }
        return n;
    }
    Ticks ticks() const { return m ? m->tickCount() : 0; }

    ScriptedClock clock;
    TestPart part;
    TestFrontend fe;
    std::unique_ptr<AppSession> session;
    ListenerId id = 0;
    TestMachine* m = nullptr;
};

} // namespace calcu::sessions::test
