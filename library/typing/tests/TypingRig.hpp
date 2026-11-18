#pragma once
#include "TestParts.hpp"
#include "TestProfiles.hpp"
#include "TypingTestMachine.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/TypingPart.hpp"
#include "typing/TypingTopics.hpp"
#include <memory>
#include <string>

// The typing tests' window: an app session with the
// typing part (the test profile and map) and the sessions' test part (an
// operation `tp.op` of 10 000 ticks), its test frontend listening, a
// `test-type` machine, and the scripted clock of the session tests. Paced
// or in virtual time.

namespace calcu::typing::test {

using sessions::CallId;
using sessions::Value;
using namespace std::chrono_literals;

struct TypingRig {
    explicit TypingRig(sessions::Hosting h = sessions::Hosting::VirtualTime, TypingProfile p = testProfile(),
                       bool text = true)
        : product(std::move(p), text), part(product, h), hosting(h) {
        sessions::AppSessionSetup s;
        s.clock = clock.fn();
        s.hostTime = [] { return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50)); };
        s.parts = {&part, &other};
        s.hosting = h;
        s.jitterSeed = 1;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        id = session->listen(fe);
        build();
    }

    TypingTestMachine* build() {
        auto mm = makeTypingMachine();
        m = mm.get();
        session->machineSession().setMachine(std::move(mm), pacing::PacingProfile{}, 0);
        session->advanceUnpaced(0);   // the session tells its parts
        return m;
    }
    CallId send(const std::string& name, Value args = Value()) {
        return session->send(id, sessions::Command{sessions::CommandName{name}, std::move(args)});
    }
    CallId screenDown(const char* key, int pointer = 1) {
        return send("screen-key-down", Value::object({{"key", key}, {"pointer", pointer}}));
    }
    CallId screenUp(const char* key, int pointer = 1) {
        return send("screen-key-up", Value::object({{"key", key}, {"pointer", pointer}}));
    }
    CallId hostDown(const HostKeyEvent& e) { return send("host-key-down", toValue(e)); }
    CallId hostUp(const std::string& physical) { return send("host-key-up", Value::object({{"physical", physical}})); }
    CallId key(std::initializer_list<const char*> names) {
        Value::Array a;
        for (const char* n : names) a.push_back(Value(n));
        return send("key", Value::object({{"names", Value(std::move(a))}}));
    }
    CallId type(const std::string& text) { return send("type", Value::object({{"text", text}})); }
    CallId paste(const std::string& text) { return send("paste", Value::object({{"text", text}})); }

    // Emulated time: in virtual time an unpaced advance (cut at the engine's
    // due ticks), paced host calls of 1 ms of wall time otherwise.
    void run(machine::Ticks n) {
        const machine::Ticks end = m->tickCount() + n;
        if (hosting == sessions::Hosting::VirtualTime) {
            session->advanceUnpaced(n);
            return;
        }
        for (int i = 0; i < 1'000'000 && m->tickCount() < end; ++i) call(1ms);
    }
    void runUntil(machine::Ticks tick) {
        if (m->tickCount() < tick) run(tick - m->tickCount());
    }
    void call(pacing::WallTime frame = 16ms) {
        clock.now += frame;
        session->call(clock.now);
    }
    machine::Ticks ticks() const { return m->tickCount(); }
    std::vector<std::string> log(std::size_t from = 0) const { return m->log(from); }
    std::vector<machine::KeyId> held() const {
        const auto* h = fe.state<HeldKeys>(kHeldKeysTopic);
        return h ? h->live : std::vector<machine::KeyId>{};
    }
    const sessions::ReplyMessage* reply(CallId c) const { return fe.reply(c); }

    sessions::test::ScriptedClock clock;
    TestTypingProduct product;
    TypingPart part;
    sessions::test::TestPart other;
    sessions::test::TestFrontend fe;
    std::unique_ptr<sessions::AppSession> session;
    sessions::ListenerId id = 0;
    TypingTestMachine* m = nullptr;
    sessions::Hosting hosting;
};

// The tick a logged event happened at ("<tick> <key> down").
inline machine::Ticks tickOf(const std::string& line) { return std::stoull(line.substr(0, line.find(' '))); }

} // namespace calcu::typing::test
