// LIB-4 design L10, L11: the part in the app session (AC-4, AC-6, AC-8).
#include "TestControlRules.hpp"
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "control/ControlPart.hpp"
#include "control/ControlState.hpp"
#include "sessions/LibraryTopics.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace calcu::control;
using namespace calcu::control::test;
using calcu::sessions::Value;
namespace st = calcu::sessions::test;

namespace {

calcu::machine::PeripheralRequest plug(const char* place, const PeripheralId& id) {
    return mt::peripheralAt(mt::testCatalogue(), place, id);
}

// A window with the control state among its parts.
struct ControlRig {
    explicit ControlRig(std::vector<const ControlRules*> rules = {}) : control(testSetup(std::move(rules))) {
        st::RigOptions o;
        o.extraParts = {&control};
        rig = std::make_unique<st::Rig>(o);
    }
    int sent() const { return rig->fe.stateMessages(kControlStateTopic); }
    const ControlState* state() const { return rig->fe.state<ControlState>(kControlStateTopic); }
    bool plugged(const char* place, const PeripheralId& id) const {
        const ControlState* s = state();
        const PlaceEntry* p = s ? s->place(at({place})) : nullptr;
        const PeripheralChoice* c = p ? choice(*p, id) : nullptr;
        return c && c->plugged;
    }
    ControlPart control;
    std::unique_ptr<st::Rig> rig;
};

calcu::sessions::Task<calcu::sessions::OperationResult> attachUartThenRun(calcu::sessions::OperationContext& c) {
    if (calcu::machine::Machine* m = c.session().machine()) m->attach(plug("aux", mt::kUart), mt::testCatalogue());
    co_await c.run(10'000);
    co_return calcu::sessions::OperationResult{};
}

} // namespace

// ── L10: the part (AC-4, AC-6) ──────────────────────────────────────────

TEST(ac4_computed_at_end_of_call_and_command) {
    ControlRig c;
    c.rig->build();
    c.rig->call();   // the end of a host call
    CHECK_EQ(c.sent(), 1);
    CHECK(c.state() && c.state()->model == mt::kSeg && !c.plugged("printer", mt::kPrinter));
    // The machine changes between two calls; the end of the next command sends it.
    c.rig->m->attach(plug("printer", mt::kPrinter), mt::testCatalogue());
    c.rig->send("tp.set", Value::object({{"value", 1}}));
    CHECK_EQ(c.sent(), 2);
    CHECK(c.plugged("printer", mt::kPrinter));
    // And the end of a host call.
    c.rig->m->detach(at({"printer"}), mt::kPrinter, mt::testCatalogue());
    c.rig->call();
    CHECK_EQ(c.sent(), 3);
    CHECK(!c.plugged("printer", mt::kPrinter));
    // And the end of a request.
    c.rig->m->attach(plug("slot-1", mt::kModule), mt::testCatalogue());
    c.rig->ask("tp.echo", Value::object({{"value", 2}}));
    CHECK_EQ(c.sent(), 4);
    CHECK(c.plugged("slot-1", mt::kModule));
}

TEST(ac4_sent_only_when_it_differs) {
    ControlRig c;
    c.rig->build();
    c.rig->call();
    CHECK_EQ(c.sent(), 1);
    for (int i = 0; i < 5; ++i) c.rig->call();   // host calls that change nothing of it
    c.rig->send("key-down", Value::object({{"key", "1"}}));
    c.rig->send("key-up", Value::object({{"key", "1"}}));
    c.rig->send("tp.set", Value::object({{"value", 3}}));
    CHECK_EQ(c.sent(), 1);
    // An operation starts: the entries change, and change back at its end.
    c.rig->send("tp.op");
    CHECK_EQ(c.sent(), 2);
    c.rig->callWhileBusy();
    CHECK_EQ(c.sent(), 3);
    // A late listener gets it, as any state topic (LIB-3 AC-6).
    st::TestFrontend late;
    c.rig->session->listen(late);
    CHECK_EQ(late.stateMessages(kControlStateTopic), 1);
    CHECK(late.state<ControlState>(kControlStateTopic)->equals(*c.state()));
}

TEST(ac6_no_machine_no_control_state) {
    ControlRig c;   // no machine is built: a product that could not start (LIB-12 AC-5)
    for (int i = 0; i < 3; ++i) c.rig->call();
    c.rig->send("tp.set", Value::object({{"value", 1}}));
    c.rig->ask("tp.echo", Value::object({{"value", 1}}));
    CHECK_EQ(c.sent(), 0);
    CHECK(c.state() == nullptr);
    st::TestFrontend late;
    c.rig->session->listen(late);
    CHECK_EQ(late.stateMessages(kControlStateTopic), 0);
    // The topic is declared all the same.
    const auto r = c.rig->ask("topics");
    const auto* reply = c.rig->fe.reply(r);
    CHECK(reply && reply->done && reply->result &&
          calcu::sessions::toText(reply->result->toValue()).find("control-state") != std::string::npos);
}

// ── L11: two frontends (AC-8) ───────────────────────────────────────────

TEST(ac8_two_sessions_same_machine_same_control_state) {
    TestControlRules rules;   // the same product in both
    ControlRig one({&rules}), two({&rules});
    one.rig->part.operation = two.rig->part.operation = [](calcu::sessions::OperationContext& c) {
        return attachUartThenRun(c);
    };
    auto same = [&] {
        const ControlState* a = one.state();
        const ControlState* b = two.state();
        return a && b && a->equals(*b) && a->toValue() == b->toValue();
    };
    // One script of host calls and commands, on both.
    std::vector<std::function<void(st::Rig&)>> script = {
        [](st::Rig& r) { r.build(); },
        [](st::Rig& r) { r.call(); },
        [](st::Rig& r) { r.send("key-down", Value::object({{"key", "2"}})); },
        [](st::Rig& r) { r.send("tp.op"); },
        [](st::Rig& r) { r.call(); },
        [](st::Rig& r) { r.callWhileBusy(); },
        [](st::Rig& r) { r.send("key-up", Value::object({{"key", "2"}})); },
        [](st::Rig& r) { r.call(); },
    };
    for (std::size_t i = 0; i < script.size(); ++i) {
        script[i](*one.rig);
        script[i](*two.rig);
        if (i >= 1) CHECK(same());
    }
    CHECK(one.plugged("aux", mt::kUart) && two.plugged("aux", mt::kUart));
    CHECK_EQ(one.sent(), two.sent());
}
