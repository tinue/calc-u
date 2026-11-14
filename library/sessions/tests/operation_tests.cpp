// LIB-3 L9 to L12: operations (AC-14 to AC-19, EC-3, EC-6).
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
    co_await coldBootAndWait(c);   // AC-17: awaited, not started
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

TEST(ac14_busy_while_operation_runs) {
    Rig r;
    r.build();
    r.part.operation = runningFor(200'000);   // two seconds of emulated time
    const CallId id = r.send("tp.op");
    CHECK(r.fe.busy()->operationRuns);   // at once
    CHECK_EQ(r.fe.replies(id), 0);       // D-2: the reply comes at the end
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

TEST(ac14_one_part_per_call_within_allowance) {
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
        // LIB-17 AC-5: within the allowance of a call at full speed, plus the
        // readings of the one slice that crosses it.
        CHECK(used <= calcu::pacing::Pacer::kFullSpeedAllowance + std::chrono::milliseconds(3));
        CHECK(r.ticks() > t0);
    }
    CHECK(calls > 3);   // in parts, so the frontend repaints between them
}

TEST(ac14_full_speed_unless_regular_stretch) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return regularThenFull(c, 8'000, 100'000); };
    r.send("tp.op");
    // The regular stretch: paced like regular-speed running, a frame per call
    // (the first call may carry one piece more, LIB-17 AC-3).
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

TEST(ac14_busy_says_speed_now) {
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

TEST(ac15_changes_sent_at_end_of_call_during_operation) {
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

TEST(ac16_second_operation_refused) {
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

TEST(ac16_product_command_operation_refused_unread) {
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

TEST(ac16_other_commands_go_through) {
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

TEST(ac16_ends_running_operation_command) {
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
    CHECK_EQ(r.session->machineSession().freshStarts(), fresh + 1);
    CHECK(r.fe.noticeSentences().empty());   // a stop is not a failure
    // The order: the stopped operation's reply before the stopping command's.
    std::vector<CallId> order;
    for (const auto& m : r.fe.log)
        if (const auto* rep = std::get_if<ReplyMessage>(&m)) order.push_back(rep->call);
    CHECK(order.size() >= 2 && order[order.size() - 2] == op && order.back() == stop);
}

TEST(ac17_nested_operation_not_refused) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return presetLike(c); };
    const CallId id = r.send("tp.op");
    r.callWhileBusy();
    CHECK(r.fe.reply(id) && r.fe.reply(id)->done);
    CHECK(r.fe.noticeSentences().empty());
    CHECK_EQ(r.m->keyEvents().size(), std::size_t(2));
}

TEST(ec6_request_answered_while_busy) {
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

TEST(ac18_end_busy_reply_notice_start_afresh) {
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

TEST(ec3_failure_halfway_then_commands_accepted) {
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

TEST(ac19_one_go_equals_parts) {
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
