// The typing part in the session.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;
using calcu::sessions::Value;

TEST(live_dropped_while_busy_not_pressed_after) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.send("tp.op");   // an operation of 10 000 ticks
    CHECK(r.session->busy());
    r.screenDown("a");
    r.hostDown(letter('b', "KeyB"));
    r.paste("12");
    r.send("host-text", Value::object({{"text", "3"}}));
    for (int i = 0; i < 1000 && r.session->busy(); ++i) r.call(1ms);
    CHECK(!r.session->busy());
    r.run(10'000);   // a key still held when the operation ended is not pressed then
    CHECK(r.log(from).empty());
    r.screenUp("a");   // releases pass
    r.hostUp("KeyB");
    r.run(2000);
    CHECK(r.log(from).empty());
}

TEST(key_and_type_not_busy_stopped_by_live) {
    TypingRig r(calcu::sessions::Hosting::Paced);
    r.run(100);
    const auto k = r.key({"1", "2", "3"});
    CHECK(!r.session->busy());
    CHECK(r.fe.busy() && !r.fe.busy()->operationRuns);
    r.call(1ms);
    const auto t = r.type("12");
    CHECK(!r.session->busy());
    r.call(1ms);
    r.screenDown("c");
    r.screenUp("c");
    const auto* rk = r.reply(k);
    const auto* rt = r.reply(t);
    CHECK(rk && !rk->done && rk->reason == kStoppedByKey);
    CHECK(rt && !rt->done && rt->reason == kStoppedByKey);
}

TEST(raw_key_down_not_engines) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.send("key-down", Value::object({{"key", "a"}}));   // the harness's raw command
    CHECK_EQ(r.m->keysDown().size(), std::size_t(1));
    CHECK(r.part.engine().idle());
    CHECK(r.held().empty());   // not in held-keys
    const auto k = r.key({"1"});   // does not wait for it
    CHECK_EQ(r.m->keysDown().size(), std::size_t(2));
    r.send("focus-lost");   // does not release it
    r.run(5000);
    CHECK(r.reply(k) && r.reply(k)->done);
    CHECK_EQ(r.m->keysDown().size(), std::size_t(1));
    CHECK(r.m->keysDown().size() == 1 && r.m->keysDown()[0] == KeyId{"a"});
    r.send("key-up", Value::object({{"key", "a"}}));
    CHECK(r.m->keysDown().empty());
    CHECK_EQ(r.log(from).size(), std::size_t(4));
}

TEST(held_keys_lists_live_only) {
    TypingRig r;
    r.run(100);
    r.screenDown("a");
    CHECK(r.held() == std::vector<KeyId>{KeyId{"a"}});
    r.hostDown(digit('2', "Digit2"));
    CHECK(r.held() == (std::vector<KeyId>{KeyId{"a"}, KeyId{"2"}}));
    r.screenUp("a");   // let go: no longer held by the user (still down for its minimum)
    CHECK(r.held() == std::vector<KeyId>{KeyId{"2"}});
    r.hostUp("Digit2");
    r.run(5000);
    CHECK(r.held().empty());
    r.key({"1"});   // a command's press is not live input
    CHECK_EQ(r.m->keysDown().size(), std::size_t(1));
    CHECK(r.held().empty());
    r.run(5000);
    r.paste("3");
    CHECK(r.held().empty());
    r.run(5000);
    r.hostDown(letter('a', "KeyA", true));   // a live sequence: shown while each is down
    CHECK(r.held() == std::vector<KeyId>{KeyId{"shift"}});
    r.run(20'000);
    CHECK(r.held().empty());
}

TEST(type_absent_without_char_profile) {
    TypingRig r(calcu::sessions::Hosting::VirtualTime, testProfile(), false);   // a product without text
    const auto c = r.type("1");
    CHECK(r.reply(c) && !r.reply(c)->done && r.reply(c)->reason == "Unknown command: type.");
    auto p = testProfile();
    p.charKeys = nullptr;
    TypingRig s(calcu::sessions::Hosting::VirtualTime, p, true);   // a model without characters
    const auto d = s.type("1");
    CHECK(s.reply(d) && !s.reply(d)->done && s.reply(d)->reason == "Unknown command: type.");
    const auto e = s.paste("1");   // nothing to type
    CHECK(s.reply(e) && s.reply(e)->done);
    s.run(5000);
    CHECK(s.m->keyEvents().empty());
}

TEST(key_names_refused_nothing_typed_screen_key_unknown) {
    TypingRig r;
    r.run(100);
    const auto bad = r.key({"1", "nokey"});
    CHECK(r.reply(bad) && !r.reply(bad)->done && r.reply(bad)->reason == "Unknown key: nokey.");
    const auto empty = r.send("key", Value::object({{"names", Value(Value::Array{})}}));
    CHECK(r.reply(empty) && !r.reply(empty)->done);
    const auto sk = r.screenDown("nokey");
    CHECK(r.reply(sk) && !r.reply(sk)->done && r.reply(sk)->reason == "Unknown key: nokey.");
    r.run(5000);
    CHECK(r.m->keyEvents().empty());
    const auto ok = r.send("key", Value::object({{"names", "SUM clr"}}));   // names in one string; any case
    r.run(20'000);
    CHECK(r.reply(ok) && r.reply(ok)->done);
    CHECK_EQ(r.m->keyEvents().size(), std::size_t(6));   // shift, 1, clr
}

TEST(typing_request_says_idle_and_waiting) {
    TypingRig r;
    r.run(100);
    r.m->readiness = [](const TypingTestMachine&) { return false; };
    r.key({"1", "2"});
    const auto q = r.session->ask(r.id, calcu::sessions::Request{kTypingRequest, {}});
    const auto* rep = r.reply(q);
    const auto* st = rep ? calcu::sessions::payloadAs<TypingState>(rep->result) : nullptr;
    CHECK(st && !st->idle && st->waiting == 2);
}
