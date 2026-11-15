// LIB-5 design §6, L13: typing inside an operation (AC-14, AC-17; §3.6).
#include "TestHarness.hpp"
#include "TypingRig.hpp"
#include "typing/Awaitable.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;
using calcu::sessions::OperationContext;
using calcu::sessions::OperationResult;
using calcu::sessions::Task;

namespace {

Task<OperationResult> typing(OperationContext& c, TypingEngine* e, std::vector<Press> presses, TypeOptions o,
                             TypeResult* out) {
    *out = co_await typeAndWait(c, *e, std::move(presses), o);
    co_return OperationResult{};
}

Task<OperationResult> typingText(OperationContext& c, TypingEngine* e, std::string text, TypeResult* out) {
    *out = co_await typeTextAndWait(c, *e, std::move(text), true);
    co_return OperationResult{};
}

TypingProfile pc1600Like() {
    auto p = testProfile();
    p.queuedWaitsForReadiness = false;
    p.firstPressOfStep = StartRule::KeyReadiness;
    p.firstPressBound = 5000;
    return p;
}

} // namespace

TEST(ac14_first_press_rule_inside_an_operation) {
    TypingRig r(calcu::sessions::Hosting::VirtualTime, pc1600Like());
    r.run(100);
    r.m->readiness = [](const TypingTestMachine&) { return false; };
    TypeResult res{false, "unset"};
    TypingEngine* e = &r.part.engine();
    r.other.operation = [e, &res](OperationContext& c) { return typing(c, e, keys({"1"}), TypeOptions{}, &res); };
    const std::size_t from = r.m->keyEvents().size();
    const auto t = r.ticks();
    r.send("tp.op");   // in virtual time: runs to its end inside send()
    CHECK(res.done);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK(log.size() == 2 && tickOf(log[0]) >= t + 5000 && tickOf(log[0]) < t + 5005);   // at the bound
}

TEST(ac14_first_press_at_once_overrides_the_rule) {
    TypingRig r(calcu::sessions::Hosting::VirtualTime, pc1600Like());
    r.run(100);
    r.m->readiness = [](const TypingTestMachine&) { return false; };
    TypeResult res{false, "unset"};
    TypingEngine* e = &r.part.engine();
    TypeOptions o;
    o.firstPressAtOnce = true;   // LIB-6 AC-58: after skip-wait:
    r.other.operation = [e, &res, o](OperationContext& c) { return typing(c, e, keys({"1", "2"}), o, &res); };
    const std::size_t from = r.m->keyEvents().size();
    const auto t = r.ticks();
    r.send("tp.op");
    CHECK(res.done);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    CHECK(log.size() == 4 && tickOf(log[0]) == t);
}

TEST(type_and_wait_same_log_as_outside) {
    auto relative = [](const std::vector<std::string>& log) {
        std::vector<std::string> out;
        if (log.empty()) return out;
        const auto t0 = tickOf(log[0]);
        for (const auto& l : log) out.push_back(std::to_string(tickOf(l) - t0) + l.substr(l.find(' ')));
        return out;
    };
    TypingRig outside;
    outside.run(100);
    std::size_t from = outside.m->keyEvents().size();
    outside.type("1A2");
    outside.run(50'000);
    const auto a = relative(outside.log(from));

    TypingRig inside;
    inside.run(100);
    TypeResult res{false, "unset"};
    TypingEngine* e = &inside.part.engine();
    inside.other.operation = [e, &res](OperationContext& c) { return typingText(c, e, "1A2", &res); };
    from = inside.m->keyEvents().size();
    inside.send("tp.op");
    CHECK(res.done);
    const auto b = relative(inside.log(from));
    CHECK_EQ(a.size(), std::size_t(10));   // 1, shift, a, 2, enter
    CHECK(a == b);
}
