// Operations.
#include "TestHarness.hpp"
#include "TestParts.hpp"
#include <stdexcept>

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;
using calcu::machine::KeyId;
using calcu::pacing::Speed;
using calcu::pacing::WallTime;

namespace {
Task<OperationResult> regularThenFull(OperationContext& c, Ticks regular, Ticks full) {
    c.setSpeed(Speed::Regular);
    co_await c.run(regular);
    c.setSpeed(Speed::Full);
    co_await c.run(full);
    co_return OperationResult{};
}

Task<OperationResult> failingHalfway(OperationContext& c, bool throwIt) {
    co_await c.run(20'000);
    if (throwIt) throw std::runtime_error("The tape broke.");
    co_return OperationResult{false, "The file is not a preset."};
}

Task<Ticks> coldBootAndWait(OperationContext& c) {
    c.session().machine()->coldBoot();
    Ticks ran = 0;
    while (!c.session().machine()->isKeyReady()) ran += co_await c.run(10);
    co_return ran;
}

// A preset as a nested operation: cold boot, type, a stretch at regular speed.
Task<OperationResult> presetLike(OperationContext& c) {
    co_await coldBootAndWait(c);   // a nested operation: awaited, not started
    co_await c.run(3'000);
    c.session().machine()->keyDown(KeyId{"1"});
    co_await c.run(500);
    c.session().machine()->keyUp(KeyId{"1"});
    c.setSpeed(Speed::Regular);
    co_await c.run(2'000);
    c.setSpeed(Speed::Full);
    co_await c.run(400'000);
    c.session().log("typed");
    co_return OperationResult{};
}

std::int64_t value(Rig& r) {
    const auto* v = r.fe.state<TestValue>(r.part.stateName());
    return v ? v->v : -1;
}
} // namespace

TEST(busy_while_operation_runs) {
    Rig r;
    r.build();
    r.part.operation = runningFor(200'000);   // two seconds of emulated time
    const CallId id = r.send("tp.op");
    CHECK(r.fe.busy()->operationRuns);   // at once
    CHECK_EQ(r.fe.replies(id), 0);       // the reply comes at the end
    int calls = 0;
    while (r.session->busy() && calls < 1000) {
        r.call();
        ++calls;
        if (r.session->busy()) CHECK(r.fe.busy()->operationRuns);
    }
    CHECK(calls > 3);
    CHECK(!r.fe.busy()->operationRuns);
    CHECK(r.ticks() >= 200'000);
    CHECK_EQ(r.fe.replies(id), 1);
}

TEST(one_part_per_call_within_allowance) {
    Rig r;
    r.build();
    r.part.operation = runningFor(300'000);
    r.send("tp.op");
    int calls = 0;
    while (r.session->busy() && calls < 1000) {
        r.clock.now += std::chrono::milliseconds(16);
        const WallTime start = r.clock.now;
        const Ticks t0 = r.ticks();
        r.session->call(start);
        ++calls;
        const WallTime used = r.clock.now - start;
        // Within the allowance of a call at full speed, plus the
        // readings of the one slice that crosses it.
        CHECK(used <= calcu::pacing::Pacer::kFullSpeedAllowance + std::chrono::milliseconds(3));
        CHECK(r.ticks() > t0);
    }
    CHECK(calls > 3);   // in parts, so the frontend repaints between them
}

TEST(full_speed_unless_regular_stretch) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return regularThenFull(c, 8'000, 100'000); };
    r.send("tp.op");
    // The regular stretch: paced like regular-speed running, a frame per call
    // (the first call may carry one piece more).
    CHECK(r.fe.busy()->operationRuns && r.fe.busy()->speed == Speed::Regular);
    r.call();
    Ticks t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 < 2'500);
    while (r.fe.busy()->speed == Speed::Regular && r.session->busy()) r.call();
    t0 = r.ticks();
    r.call();
    CHECK(r.ticks() - t0 > 6'000);   // full speed again
}

TEST(busy_says_speed_now) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return regularThenFull(c, 5'000, 100'000); };
    r.send("tp.op");
    r.callWhileBusy();
    std::vector<std::pair<bool, Speed>> seen;
    for (const auto& m : r.fe.log)
        if (const auto* s = std::get_if<StateMessage>(&m); s && s->topic == kBusyTopic) {
            const auto* b = payloadAs<BusyState>(s->state);
            seen.emplace_back(b->operationRuns, b->speed);
        }
    // idle at the commanded speed → busy at regular (set before its first
    // run) → busy at full → idle at the commanded speed again.
    const std::vector<std::pair<bool, Speed>> want{
        {false, Speed::Regular}, {true, Speed::Regular}, {true, Speed::Full}, {false, Speed::Regular}};
    CHECK(seen == want);
}

TEST(changes_sent_at_end_of_call_during_operation) {
    Rig r;
    r.build();
    r.part.operation = runningFor(300'000);
    r.part.onAfterCall = [&](SessionContext& c, WallTime) {
        if (c.busy()) r.part.set(static_cast<std::int64_t>(r.ticks()));   // e.g. the paper
    };
    r.send("tp.op");
    int callsWithDisplay = 0, callsWithProduct = 0, calls = 0;
    while (r.session->busy()) {
        const std::size_t mark = r.fe.log.size();
        r.call();
        ++calls;
        if (r.fe.stateMessages(kDisplayTopic, mark) == 1) ++callsWithDisplay;
        if (r.fe.stateMessages(r.part.stateName(), mark) == 1) ++callsWithProduct;
        if (r.session->busy()) CHECK_EQ(value(r), static_cast<std::int64_t>(r.ticks()));
    }
    CHECK(calls > 3);
    CHECK(callsWithDisplay >= calls / 2);   // the tick digit moves with most parts
    CHECK_EQ(callsWithProduct, calls - 1);
}

TEST(second_operation_refused) {
    Rig r;
    r.build();
    r.part.operation = runningFor(200'000);
    const CallId first = r.send("tp.op");
    const CallId second = r.send("tp.op");
    CHECK(!r.fe.reply(second)->done);
    CHECK_EQ(r.fe.reply(second)->reason, std::string(kAnotherOperationRuns));
    r.callWhileBusy();
    CHECK(r.fe.reply(first)->done);
    CHECK_EQ(r.part.opsStarted, 2);   // the handler ran; the session refused the start
    CHECK(r.fe.reply(r.send("tp.op")) == nullptr);   // accepted again: its reply comes at its end
}

TEST(product_command_operation_refused_unread) {
    TestPart other("tq");   // another product's names
    RigOptions o;
    o.extraParts = {&other};
    Rig r(o);
    r.build();
    r.part.operation = runningFor(200'000);
    r.send("tp.op");
    const CallId id = r.send("tq.op");
    CHECK(!r.fe.reply(id)->done);
    CHECK_EQ(r.fe.reply(id)->reason, std::string(kAnotherOperationRuns));
    CHECK(r.session->busy());
}

TEST(other_commands_go_through) {
    Rig r;
    r.build();
    r.part.operation = runningFor(200'000);
    r.send("tp.op");
    r.call();
    CHECK(r.fe.reply(r.send("tp.set", Value::object({{"value", 3}})))->done);
    CHECK_EQ(value(r), 3);
    CHECK(r.fe.reply(r.send("key-down", Value::object({{"key", "2"}})))->done);
    CHECK(r.fe.reply(r.send("tp.append", Value::object({{"text", "x"}})))->done);
    CHECK(r.fe.reply(r.ask("ticks"))->done);
    CHECK(r.session->busy());
}

TEST(ends_running_operation_command) {
    Rig r;
    r.build();
    r.part.operation = runningFor(500'000);
    const CallId op = r.send("tp.op");
    r.call();
    const auto fresh = r.session->machineSession().freshStarts();
    const CallId stop = r.send("tp.stop");   // declared endsRunningOperation (Reset, Cold boot, model switch)
    CHECK(!r.fe.reply(op)->done);
    CHECK_EQ(r.fe.reply(op)->reason, std::string(kStopped));
    CHECK(r.fe.reply(stop)->done);
    CHECK(!r.session->busy());
    CHECK(!r.fe.busy()->operationRuns);
    CHECK_EQ(r.session->machineSession().freshStarts(), fresh + 2);   // the stop, then the end of its own
    CHECK(r.fe.noticeSentences().empty());   // a stop is not a failure
    // The order: the stopped operation's reply before the stopping command's.
    std::vector<CallId> order;
    for (const auto& m : r.fe.log)
        if (const auto* rep = std::get_if<ReplyMessage>(&m)) order.push_back(rep->call);
    CHECK(order.size() >= 2 && order[order.size() - 2] == op && order.back() == stop);
}

// A command declared endsRunningOperation whose
// handler starts nothing (Done) or is refused leaves the operation running.
TEST(ending_command_that_starts_nothing_stops_nothing) {
    Rig r;
    r.build();
    r.part.operation = runningFor(500'000);
    const CallId op = r.send("tp.op");
    r.call();
    const CallId nothing = r.send("tp.stop-nothing");
    CHECK(r.fe.reply(nothing) && r.fe.reply(nothing)->done);
    CHECK(r.fe.reply(op) == nullptr);   // still running, no reply yet
    CHECK(r.session->busy());
    r.callWhileBusy();
    CHECK(r.fe.reply(op) && r.fe.reply(op)->done);   // it ran to its end
}

TEST(nested_operation_not_refused) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return presetLike(c); };
    const CallId id = r.send("tp.op");
    r.callWhileBusy();
    CHECK(r.fe.reply(id) && r.fe.reply(id)->done);
    CHECK(r.fe.noticeSentences().empty());
    CHECK_EQ(r.m->keyEvents().size(), std::size_t(2));
}

TEST(request_answered_while_busy) {
    Rig r;
    r.build();
    r.part.operation = runningFor(200'000);
    r.send("tp.op");
    r.call();
    const CallId t = r.ask("ticks");
    const CallId e = r.ask("tp.echo", Value::object({{"value", 6}}));
    CHECK(r.fe.reply(t) && r.fe.reply(t)->done);
    CHECK_EQ(payloadAs<EmulatedTimeReply>(r.fe.reply(t)->result)->ticks, r.ticks());
    CHECK(r.fe.reply(e) && r.fe.reply(e)->done);
    CHECK(r.session->busy());
}

TEST(end_busy_reply_notice_start_afresh) {
    Rig r;
    r.build();
    r.part.operation = runningFor(50'000);
    const CallId ok = r.send("tp.op");
    const auto fresh = r.session->machineSession().freshStarts();
    r.callWhileBusy();
    CHECK(!r.fe.busy()->operationRuns);
    CHECK(r.fe.reply(ok)->done);
    CHECK(r.fe.noticeSentences().empty());
    CHECK_EQ(r.session->machineSession().freshStarts(), fresh + 1);

    r.part.operation = [](OperationContext& c) { return failingHalfway(c, false); };
    const CallId bad = r.send("tp.op");
    r.callWhileBusy();
    CHECK(!r.fe.reply(bad)->done);
    CHECK_EQ(r.fe.reply(bad)->reason, std::string("The file is not a preset."));
    CHECK(r.fe.noticeSentences() == std::vector<std::string>{"The file is not a preset."});   // told once
    CHECK_EQ(r.session->machineSession().freshStarts(), fresh + 2);
}

TEST(failure_halfway_then_commands_accepted) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return failingHalfway(c, true); };
    const CallId bad = r.send("tp.op");
    r.callWhileBusy();
    CHECK(r.ticks() >= 20'000);   // it got halfway
    CHECK(!r.fe.busy()->operationRuns);
    CHECK(!r.fe.reply(bad)->done);
    CHECK_EQ(r.fe.reply(bad)->reason, std::string("The tape broke."));
    CHECK(r.fe.noticeSentences() == std::vector<std::string>{"The tape broke."});
    // The session accepts commands and operations again.
    CHECK(r.fe.reply(r.send("tp.set", Value::object({{"value", 1}})))->done);
    r.part.operation = runningFor(1'000);
    const CallId again = r.send("tp.op");
    r.callWhileBusy();
    CHECK(r.fe.reply(again)->done);
    // An operation without a machine fails the same way.
    auto taken = r.session->machineSession().takeMachine();
    const CallId none = r.send("tp.op");
    r.callWhileBusy();
    CHECK(!r.fe.reply(none)->done);
    CHECK_EQ(r.fe.reply(none)->reason, std::string(kNoMachine));
}

TEST(one_go_equals_parts) {
    RigOptions virt;
    virt.hosting = Hosting::VirtualTime;
    Rig oneGo(virt);
    Rig parts;
    for (Rig* r : {&oneGo, &parts}) {
        r->build(mt::kDot);
        r->part.operation = [](OperationContext& c) { return presetLike(c); };
    }
    const CallId a = oneGo.send("tp.op");
    CHECK(!oneGo.session->busy());   // in one go, inside the command
    const CallId b = parts.send("tp.op");
    const int calls = parts.callWhileBusy();
    CHECK(calls > 1);

    CHECK(oneGo.fe.reply(a) && oneGo.fe.reply(a)->done);
    CHECK(parts.fe.reply(b) && parts.fe.reply(b)->done);
    CHECK_EQ(oneGo.ticks(), parts.ticks());
    CHECK(oneGo.m->keyEvents() == parts.m->keyEvents());
    const auto* da = oneGo.fe.state<DisplayState>(kDisplayTopic);
    const auto* db = parts.fe.state<DisplayState>(kDisplayTopic);
    CHECK(da && db && sameDisplay(da->output, db->output));

    // From then on the machine answers the same calls in the same way.
    for (Rig* r : {&oneGo, &parts}) {
        r->send("key-down", Value::object({{"key", "b"}}));
        r->session->advanceUnpaced(12'345);
        r->send("key-up", Value::object({{"key", "b"}}));
        r->session->advanceUnpaced(777);
    }
    const auto* ta = payloadAs<EmulatedTimeReply>(oneGo.fe.reply(oneGo.ask("ticks"))->result);
    const auto* tb = payloadAs<EmulatedTimeReply>(parts.fe.reply(parts.ask("ticks"))->result);
    CHECK(ta && tb && ta->ticks == tb->ticks);
    CHECK(oneGo.m->keyEvents() == parts.m->keyEvents());
    da = oneGo.fe.state<DisplayState>(kDisplayTopic);
    db = parts.fe.state<DisplayState>(kDisplayTopic);
    CHECK(da && db && sameDisplay(da->output, db->output));
}

// OperationContext::hostWait, the one wall time of an operation
// (a preset's `wait:` while the machine is paused).
namespace {
Task<OperationResult> waitInHostTime(OperationContext& c, WallTime d, bool* waited) {
    *waited = co_await c.hostWait(d);
    c.session().log("waited");
    co_return OperationResult{};
}
} // namespace

TEST(host_wait_passes_in_wall_time_while_paused) {
    Rig r;
    r.build();
    r.session->machineSession().setPaused(true);
    bool waited = false;
    r.part.operation = [&waited](OperationContext& c) { return waitInHostTime(c, 100ms, &waited); };
    const CallId id = r.send("tp.op");
    const Ticks t0 = r.ticks();
    for (int i = 0; i < 3; ++i) r.call();   // about 50 ms of wall time
    CHECK(r.session->busy());
    for (int i = 0; i < 10 && r.session->busy(); ++i) r.call();
    CHECK(!r.session->busy());
    CHECK(waited);
    CHECK(r.fe.reply(id) && r.fe.reply(id)->done);
    CHECK_EQ(r.ticks(), t0);   // no emulated time passed
    CHECK(r.session->machineSession().paused());
}

TEST(host_wait_ends_at_once_in_virtual_time) {
    RigOptions o;
    o.hosting = Hosting::VirtualTime;
    Rig r(o);
    r.build();
    r.session->machineSession().setPaused(true);
    bool waited = true;
    r.part.operation = [&waited](OperationContext& c) { return waitInHostTime(c, std::chrono::seconds(5), &waited); };
    const CallId id = r.send("tp.op");
    CHECK(!r.session->busy());
    CHECK(!waited);
    CHECK(r.fe.reply(id) && r.fe.reply(id)->done);
}

// The payload of an operation's result in its
// reply, and an operation a part starts without a command.
namespace {
Task<OperationResult> withPayload(OperationContext& c, bool ok) {
    co_await c.run(1'000);
    OperationResult r{ok, ok ? std::string() : std::string("It went wrong."), nullptr};
    r.result = std::make_shared<TestValue>(42);
    co_return r;
}
} // namespace

TEST(operation_result_payload_in_reply) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return withPayload(c, true); };
    const CallId id = r.send("tp.op");
    r.callWhileBusy();
    const auto* reply = r.fe.reply(id);
    CHECK(reply && reply->done);
    const auto* v = reply ? payloadAs<TestValue>(reply->result) : nullptr;
    CHECK(v && v->v == 42);
    r.part.operation = [](OperationContext& c) { return withPayload(c, false); };
    const CallId bad = r.send("tp.op");
    r.callWhileBusy();
    CHECK(r.fe.reply(bad) && !r.fe.reply(bad)->done && !r.fe.reply(bad)->result);   // no payload with a failure
}

TEST(part_starts_operation_without_command) {
    Rig r;
    r.build();
    int started = 0;
    r.part.onAfterCall = [&](SessionContext& c, WallTime) {
        if (started == 0 && c.startOperation([](OperationContext& oc) { return withPayload(oc, false); })) ++started;
    };
    r.call();
    CHECK_EQ(started, 1);
    r.callWhileBusy();
    CHECK(!r.session->busy());
    const auto n = r.fe.noticeSentences();
    CHECK(std::find(n.begin(), n.end(), std::string("It went wrong.")) != n.end());   // a failure is a notice
    CHECK(r.ticks() >= 1'000);
}

TEST(part_operation_refused_while_one_runs) {
    Rig r;
    r.build();
    r.part.operation = runningFor(500'000);
    const CallId id = r.send("tp.op");
    CHECK(r.session->busy());
    CHECK(!r.session->context().startOperation(runningFor(10)));
    r.callWhileBusy();
    CHECK(r.fe.reply(id) && r.fe.reply(id)->done);
    CHECK(r.session->context().startOperation(runningFor(10)));
    r.callWhileBusy();
    CHECK(!r.session->busy());
}
