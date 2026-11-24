// Commands, requests and replies; product names carried unread.
#include "TestHarness.hpp"
#include "TestParts.hpp"
#include <algorithm>
#include <stdexcept>

using namespace calcu::sessions;
using namespace calcu::sessions::test;
namespace mt = calcu::machine::test;

TEST(request_gets_exactly_one_reply) {
    Rig r;
    const CallId id = r.ask("tp.echo", Value::object({{"value", 42}}));
    r.call();
    r.send("tp.set", Value::object({{"value", 1}}));
    CHECK_EQ(r.fe.replies(id), 1);
    const ReplyMessage* rep = r.fe.reply(id);
    CHECK(rep && rep->done);
    CHECK(rep && payloadAs<TestValue>(rep->result) && payloadAs<TestValue>(rep->result)->v == 42);
}

TEST(request_without_answer_says_why) {
    Rig r;   // no machine yet
    const CallId id = r.ask("ticks");
    CHECK_EQ(r.fe.replies(id), 1);
    CHECK(!r.fe.reply(id)->done);
    CHECK_EQ(r.fe.reply(id)->reason, std::string(kNoMachine));
    const CallId unknown = r.ask("nothing-like-it");
    CHECK(!r.fe.reply(unknown)->done);
    CHECK_EQ(r.fe.reply(unknown)->reason, std::string("Unknown request: nothing-like-it."));
}

TEST(command_done_or_refused) {
    Rig r;
    const CallId done = r.send("tp.set", Value::object({{"value", 3}}));
    CHECK(r.fe.reply(done)->done);
    const std::size_t mark = r.fe.log.size();
    const CallId refused = r.send("tp.refuse");
    CHECK_EQ(r.fe.replies(refused), 1);
    CHECK(!r.fe.reply(refused)->done);
    CHECK_EQ(r.fe.reply(refused)->reason, std::string("Not now."));
    CHECK_EQ(r.fe.since(mark).size(), std::size_t(1));   // the reply only: nothing changed
    // A library command refused for an unknown key.
    r.build();
    const CallId key = r.send("key-down", Value::object({{"key", "nope"}}));
    CHECK(!r.fe.reply(key)->done);
    CHECK_EQ(r.fe.reply(key)->reason, std::string("Unknown key: nope."));
    CHECK(r.m->keysDown().empty());
}

TEST(unknown_command_refused_changes_nothing) {
    Rig r;
    r.build();
    r.call();
    const auto ticks = r.ticks();
    const std::size_t mark = r.fe.log.size();
    const CallId id = r.send("c99.no-such-command", Value::object({{"x", 1}}));
    const auto got = r.fe.since(mark);
    CHECK_EQ(got.size(), std::size_t(1));
    CHECK(!r.fe.reply(id)->done);
    CHECK_EQ(r.fe.reply(id)->reason, std::string("Unknown command: c99.no-such-command."));
    CHECK_EQ(r.ticks(), ticks);
    CHECK(!r.session->busy());
}

TEST(ticks_request_gives_ticks_and_seconds) {
    Rig r;
    r.build();
    r.call();
    r.call();
    const CallId id = r.ask("ticks");
    const auto* t = payloadAs<EmulatedTimeReply>(r.fe.reply(id)->result);
    CHECK(t != nullptr);
    CHECK(t && t->ticks == r.ticks() && t->ticks > 0);
    CHECK(t && t->seconds == static_cast<double>(r.ticks()) / mt::kSegRate);
}

TEST(capabilities_request_gives_kind_id_owner_places_plugged) {
    Rig r;
    auto mm = mt::buildTestMachine(mt::kSeg, std::nullopt,
                                   {mt::peripheralAt(mt::testCatalogue(), "printer", mt::kPrinter)});
    r.m = mm.get();
    r.session->machineSession().setMachine(std::move(mm), {}, 8000);
    const CallId id = r.ask("capabilities");
    const auto* o = payloadAs<MachineOffer>(r.fe.reply(id)->result);
    CHECK(o != nullptr);
    if (!o) return;
    CHECK_EQ(o->capabilities.size(), std::size_t(3));   // with the controls of the printer
    const auto counter = std::find_if(o->capabilities.begin(), o->capabilities.end(),
                                      [](const auto& c) { return c.kind == mt::TestCounter::kKind; });
    CHECK(counter != o->capabilities.end());
    if (counter != o->capabilities.end()) {
        CHECK_EQ(counter->id.value, std::string("printer/test.counter"));
        const auto* owner = std::get_if<calcu::machine::PeripheralOwner>(&counter->owner);
        CHECK(owner && owner->peripheral == mt::kPrinter);
    }
    CHECK_EQ(o->places.size(), std::size_t(3));
    CHECK_EQ(o->places[0].value, std::string("slot-1"));
    CHECK_EQ(o->plugged.byPlace.size(), std::size_t(1));
    CHECK(o->plugged.byPlace[0].peripheral == mt::kPrinter);
    // A translating host gets the same as a value.
    const Value v = o->toValue();
    CHECK(v.find("capabilities") && v.find("capabilities")->elements()->size() == 3);
}

TEST(topics_request_lists_declared_topics) {
    Rig r;
    const auto* list = payloadAs<TopicList>(r.fe.reply(r.ask("topics"))->result);
    CHECK(list != nullptr);
    if (!list) return;
    std::vector<std::string> names;
    for (const auto& t : list->topics) names.push_back(t.name.value + (t.stream ? ":stream" : ":state"));
    CHECK(names == (std::vector<std::string>{"display:state", "busy:state", "notices:stream", "log:stream",
                                             "log-level:state", "tp.state:state", "tp.stream:stream"}));
}

TEST(product_topic_command_request_travel_like_generic) {
    Rig r;
    const std::size_t mark = r.fe.log.size();   // after the topics sent at listen
    // A product state topic: the very payload the part set arrives, unread.
    const CallId set = r.send("tp.set", Value::object({{"value", 5}}));
    CHECK(r.fe.reply(set)->done);
    const PayloadPtr* got = r.fe.mirror.state(r.part.stateName());
    CHECK(got && got->get() == r.part.lastSet.get());
    // A product stream topic, a product request.
    r.send("tp.append", Value::object({{"text", "line"}}));
    CHECK_EQ(r.fe.mirror.takeAppended(r.part.streamName()).size(), std::size_t(1));
    CHECK(r.fe.reply(r.ask("tp.echo", Value::object({{"value", 8}})))->done);
    // The same shapes as the library's own.
    int states = 0, streams = 0;
    for (const auto& m : r.fe.since(mark)) {
        if (const auto* s = std::get_if<StateMessage>(&m); s && s->topic == r.part.stateName()) ++states;
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == r.part.streamName()) ++streams;
    }
    CHECK_EQ(states, 1);
    CHECK_EQ(streams, 1);
}

namespace {
struct Strange final : PayloadOf<Strange> {
    static constexpr PayloadKind kKind{"c77.strange"};
    bool same(const Strange&) const { return true; }
    Value toValue() const override { return Value::object({{"strange", true}}); }
};
} // namespace

TEST(unknown_topic_ignored_by_mirror) {
    TopicMirror mirror;
    std::vector<std::string> changed;
    mirror.onChange = [&](const TopicName& t) { changed.push_back(t.value); };
    mirror.deliver(StateMessage{kBusyTopic, std::make_shared<BusyState>(false, calcu::pacing::Speed::Regular)});
    mirror.deliver(StateMessage{TopicName{"c77.unknown"}, std::make_shared<Strange>()});
    mirror.deliver(StreamMessage{TopicName{"c77.stream"}, false, {std::make_shared<Strange>()}});
    mirror.deliver(ReplyMessage{1, true, {}, nullptr});   // not a topic
    // Kept opaquely, nothing else touched; a part that knows only busy
    // finds busy as it was.
    const PayloadPtr* busy = mirror.state(kBusyTopic);
    CHECK(busy && payloadAs<BusyState>(*busy) && !payloadAs<BusyState>(*busy)->operationRuns);
    CHECK(payloadAs<BusyState>(*mirror.state(TopicName{"c77.unknown"})) == nullptr);
    CHECK(changed == (std::vector<std::string>{"busy", "c77.unknown", "c77.stream"}));
    CHECK(mirror.state(TopicName{"display"}) == nullptr);
}

// A command whose reply comes later, once,
// while the session is not busy.
namespace {
struct DeferPart final : SessionPart {
    SessionContext* ctx = nullptr;
    std::vector<CallId> pending;
    void attach(SessionSetup& s) override {
        ctx = &s.context();
        s.command(CommandName{"tp.later"}, [this](const Value&, SessionContext& c) -> CommandOutcome {
            pending.push_back(c.currentCall());
            return Deferred{};
        });
        s.command(CommandName{"tp.now-and-later"}, [](const Value&, SessionContext& c) -> CommandOutcome {
            c.reply(c.currentCall(), ReplyMessage{0, true, {}, nullptr});   // replied inside its own handler
            return Deferred{};
        });
    }
    void afterCall(SessionContext& c, calcu::pacing::WallTime) override {
        for (CallId id : pending) {
            c.reply(id, ReplyMessage{0, false, "Later.", nullptr});
            c.reply(id, ReplyMessage{0, true, {}, nullptr});   // a second reply is ignored
        }
        pending.clear();
    }
};
} // namespace

TEST(deferred_reply_comes_later_and_not_busy) {
    DeferPart p;
    RigOptions o;
    o.extraParts = {&p};
    Rig r(o);
    r.build();
    const CallId id = r.send("tp.later");
    CHECK_EQ(r.fe.replies(id), 0);
    CHECK(!r.session->busy());
    CHECK(r.fe.busy() && !r.fe.busy()->operationRuns);
    CHECK_EQ(p.ctx->currentCall(), CallId(0));   // outside a handler
    r.call();
    CHECK_EQ(r.fe.replies(id), 1);
    CHECK(r.fe.reply(id) && !r.fe.reply(id)->done && r.fe.reply(id)->reason == "Later.");
    r.call();
    CHECK_EQ(r.fe.replies(id), 1);
    p.ctx->reply(id, ReplyMessage{0, true, {}, nullptr});      // after the reply: ignored
    p.ctx->reply(12345, ReplyMessage{0, true, {}, nullptr});   // not a deferred call: ignored
    r.call();
    CHECK_EQ(r.fe.replies(id), 1);
    const CallId both = r.send("tp.now-and-later");
    CHECK_EQ(r.fe.replies(both), 1);
    CHECK(r.fe.reply(both) && r.fe.reply(both)->done);
    const CallId done = r.send("tp.set", Value::object({{"value", 1}}));   // other commands as before
    CHECK_EQ(r.fe.replies(done), 1);
    p.ctx->reply(done, ReplyMessage{0, false, "x", nullptr});
    r.call();
    CHECK_EQ(r.fe.replies(done), 1);
}

// A handler, a request or a host's answer that throws: refused or told, and
// the app log has an error line with what it threw.
namespace {
struct ThrowingPart final : SessionPart {
    void attach(SessionSetup& s) override {
        s.command(CommandName{"tp.throw"}, [](const Value&, SessionContext&) -> CommandOutcome {
            throw std::runtime_error("The handler broke.");
        });
        s.request(RequestName{"tp.throw-request"}, [](const Value&, SessionContext&) -> ReplyMessage {
            throw std::runtime_error("The request broke.");
        });
    }
};
} // namespace

TEST(throwing_handler_refused_and_app_log_error) {
    ThrowingPart p;
    RigOptions o;
    o.extraParts = {&p};
    Rig r(o);
    const CallId c = r.send("tp.throw");
    CHECK(r.fe.reply(c) && !r.fe.reply(c)->done && r.fe.reply(c)->reason == "The handler broke.");
    CHECK(r.appLog.has(calcu::applog::AppLogLevel::Error, "The command tp.throw threw: The handler broke."));
    const CallId q = r.ask("tp.throw-request");
    CHECK(r.fe.reply(q) && !r.fe.reply(q)->done);
    CHECK(r.appLog.has(calcu::applog::AppLogLevel::Error, "The request tp.throw-request threw: The request broke."));
}
