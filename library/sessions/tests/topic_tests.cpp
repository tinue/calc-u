// LIB-3 L2, L3: topics, the flush and the late listener (AC-5, AC-6, AC-7,
// EC-1, EC-2).
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;

namespace {
Value valueArg(std::int64_t v) { return Value::object({{"value", v}}); }
Value textArgs(const char* t) { return Value::object({{"text", t}}); }
} // namespace

TEST(ac5_state_sent_only_when_it_differs) {
    Rig r;
    const TopicName t = r.part.stateName();
    r.send("tp.set", valueArg(1));
    CHECK_EQ(r.fe.stateMessages(t), 1);
    r.send("tp.set", valueArg(1));   // equal: none
    CHECK_EQ(r.fe.stateMessages(t), 1);
    r.send("tp.set", valueArg(2));
    CHECK_EQ(r.fe.stateMessages(t), 2);
    CHECK_EQ(r.fe.state<TestValue>(t)->v, 2);
    r.call();   // nothing changed in the topic
    CHECK_EQ(r.fe.stateMessages(t), 2);
}

TEST(ac5_command_reply_and_changed_state_at_once) {
    Rig r;
    const std::size_t mark = r.fe.log.size();
    const CallId id = r.send("tp.set", valueArg(7));   // between two host calls
    const auto got = r.fe.since(mark);
    CHECK_EQ(got.size(), std::size_t(2));
    CHECK(std::holds_alternative<StateMessage>(got[0]));   // the state before the reply
    CHECK(std::holds_alternative<ReplyMessage>(got[1]));
    CHECK_EQ(std::get<ReplyMessage>(got[1]).call, id);
    CHECK(std::get<ReplyMessage>(got[1]).done);
}

TEST(ac5_machine_output_only_with_host_calls) {
    Rig r;
    r.build();
    r.call();
    const int before = r.fe.stateMessages(kDisplayTopic);
    CHECK(before >= 1);
    // A key changes the machine's display output, but the command does not
    // read it: the display goes out with the next host call.
    CHECK(r.fe.reply(r.send("key-down", Value::object({{"key", "1"}})))->done);
    CHECK_EQ(r.fe.stateMessages(kDisplayTopic), before);
    r.call();
    CHECK_EQ(r.fe.stateMessages(kDisplayTopic), before + 1);
}

TEST(ac7_stream_sends_exactly_what_was_appended) {
    Rig r;
    const TopicName t = r.part.streamName();
    r.part.onAfterCall = [&](SessionContext&, calcu::pacing::WallTime) {
        r.part.stream_->append(std::make_shared<TestText>("a"));
        r.part.stream_->append(std::make_shared<TestText>("b"));
    };
    const std::size_t mark = r.fe.log.size();
    r.call();
    r.part.onAfterCall = nullptr;
    r.call();   // nothing appended: no message
    r.send("tp.append", textArgs("c"));
    std::vector<std::string> seen;
    int messages = 0;
    for (const auto& m : r.fe.since(mark))
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == t) {
            ++messages;
            CHECK(!s->reset);
            for (const auto& p : s->appended) seen.push_back(payloadAs<TestText>(p)->text);
        }
    CHECK_EQ(messages, 2);
    CHECK(seen == (std::vector<std::string>{"a", "b", "c"}));
}

TEST(ac7_stream_reset_marker) {
    Rig r;
    const TopicName t = r.part.streamName();
    r.send("tp.append", textArgs("old"));
    r.part.onAfterCall = [&](SessionContext&, calcu::pacing::WallTime) {
        r.part.stream_->reset();
        r.part.stream_->append(std::make_shared<TestText>("new"));
    };
    const std::size_t mark = r.fe.log.size();
    r.call();
    const auto got = r.fe.since(mark);
    int found = 0;
    for (const auto& m : got)
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == t) {
            ++found;
            CHECK(s->reset);   // one message: the marker and what came after it
            CHECK_EQ(s->appended.size(), std::size_t(1));
            CHECK_EQ(payloadAs<TestText>(s->appended[0])->text, std::string("new"));
        }
    CHECK_EQ(found, 1);
    CHECK(r.fe.mirror.takeReset(t));
}

TEST(ec1_several_changes_send_the_last) {
    Rig r;
    const TopicName t = r.part.stateName();
    r.part.onAfterCall = [&](SessionContext&, calcu::pacing::WallTime) {
        r.part.set(1);
        r.part.set(2);
        r.part.set(3);
    };
    r.call();
    CHECK_EQ(r.fe.stateMessages(t), 1);
    CHECK_EQ(r.fe.state<TestValue>(t)->v, 3);
}

TEST(ec2_change_and_back_sends_nothing) {
    Rig r;
    const TopicName t = r.part.stateName();
    r.send("tp.set", valueArg(4));
    CHECK_EQ(r.fe.stateMessages(t), 1);
    r.part.onAfterCall = [&](SessionContext&, calcu::pacing::WallTime) {
        r.part.set(5);
        r.part.set(4);   // a new payload equal to the one last sent
    };
    r.call();
    CHECK_EQ(r.fe.stateMessages(t), 1);
}

TEST(ac6_new_listener_gets_every_state_topic_once) {
    Rig r;
    r.build();
    r.call();
    r.send("tp.set", valueArg(9));
    TestFrontend late;
    r.session->listen(late);
    for (const TopicName& t : {kDisplayTopic, kBusyTopic, r.part.stateName()}) CHECK_EQ(late.stateMessages(t), 1);
    CHECK_EQ(late.count<StateMessage>(), 3);
    CHECK_EQ(late.state<TestValue>(r.part.stateName())->v, 9);
    CHECK(late.busy() && !late.busy()->operationRuns);
    // It holds the whole current state without asking back.
    const auto* mine = r.fe.state<DisplayState>(kDisplayTopic);
    const auto* its = late.state<DisplayState>(kDisplayTopic);
    CHECK(mine && its && sameDisplay(mine->output, its->output));
}

TEST(ac6_late_listener_gets_log_since_reset_and_no_notices) {
    Rig r;
    r.send("tp.notice", textArgs("Something happened."));
    r.send("tp.log", textArgs("first"));
    r.send("tp.append", textArgs("x"));
    r.send("tp.reset");
    r.send("tp.append", textArgs("y"));
    r.send("tp.log", textArgs("second"));
    TestFrontend late;
    r.session->listen(late);
    std::vector<std::string> log;
    for (const auto& p : late.mirror.takeAppended(kLogTopic)) log.push_back(payloadAs<LogLine>(p)->text);
    CHECK(log == (std::vector<std::string>{"first", "second"}));
    auto stream = late.mirror.takeAppended(r.part.streamName());
    CHECK_EQ(stream.size(), std::size_t(1));   // from its last reset on
    CHECK_EQ(payloadAs<TestText>(stream[0])->text, std::string("y"));
    CHECK(late.noticeSentences().empty());
    for (const auto& m : late.log)
        if (const auto* s = std::get_if<StreamMessage>(&m)) CHECK(s->topic != kNoticesTopic);
}

// LIB-4 L3 (LIB-4 design §3.5): SessionPart::beforeFlush runs at the start of
// every flush, after the parts' afterCall.
namespace {
struct FlushCounter final : SessionPart {
    void attach(SessionSetup&) override {}
    void beforeFlush(SessionContext&) override {
        ++flushes;
        if (test) afterCallsSeen = test->afterCalls;
    }
    const TestPart* test = nullptr;
    int flushes = 0, afterCallsSeen = -1;
};
} // namespace

TEST(before_flush_runs_at_every_flush) {
    FlushCounter counter;
    RigOptions o;
    o.services.offered = {HostService::FilePicker};
    o.extraParts = {&counter};
    Rig r(o);
    counter.test = &r.part;
    CHECK(counter.flushes >= 1);   // the session's first flush and the listen
    r.build();
    const int n = counter.flushes;
    r.call();   // a host call
    CHECK_EQ(counter.flushes, n + 1);
    CHECK_EQ(counter.afterCallsSeen, r.part.afterCalls);   // after the parts' afterCall
    r.send("tp.set", valueArg(1));   // a command
    CHECK_EQ(counter.flushes, n + 2);
    r.ask("tp.echo", valueArg(2));   // a request
    CHECK_EQ(counter.flushes, n + 3);
    const CallId cmd = r.send("tp.ask", Value::object({{"service", static_cast<int>(HostService::FilePicker)}}));
    CHECK_EQ(counter.flushes, n + 4);
    CHECK(r.fe.reply(cmd) && r.fe.reply(cmd)->done);
    if (!r.fe.reply(cmd) || !r.fe.reply(cmd)->result) return;
    const CallId askId = static_cast<CallId>(payloadAs<TestValue>(r.fe.reply(cmd)->result)->v);
    r.session->answer(askId, ServiceAnswer{true, Value(), ""});   // a service answer
    CHECK_EQ(counter.flushes, n + 5);
    r.session->advanceUnpaced(100);
    CHECK_EQ(counter.flushes, n + 6);
    TestFrontend late;
    r.session->listen(late);
    CHECK_EQ(counter.flushes, n + 7);
}
