// The library's own topics.
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;

namespace {
Task<OperationResult> logging(OperationContext& c) {
    c.session().log("preset: line 1");
    co_await c.run(100);
    c.session().log("preset: line 2");
    co_return OperationResult{};
}
} // namespace

TEST(display_busy_notices_log_and_their_shapes) {
    Rig r;
    const auto* list = payloadAs<TopicList>(r.fe.reply(r.ask("topics"))->result);
    CHECK(list && list->topics.size() >= 4);
    if (!list || list->topics.size() < 4) return;
    CHECK(list->topics[0].name == kDisplayTopic && !list->topics[0].stream);
    CHECK(list->topics[1].name == kBusyTopic && !list->topics[1].stream);
    CHECK(list->topics[2].name == kNoticesTopic && list->topics[2].stream);
    CHECK(list->topics[3].name == kLogTopic && list->topics[3].stream);
    // busy carries whether an operation runs and the speed now.
    CHECK(r.fe.busy() && !r.fe.busy()->operationRuns && r.fe.busy()->speed == calcu::pacing::Speed::Regular);
    r.send("speed", Value::object({{"full", true}}));
    CHECK(r.fe.busy()->speed == calcu::pacing::Speed::Full);
    // display carries the machine's display output.
    r.build(calcu::machine::test::kDot);
    r.call();
    const auto* d = r.fe.state<DisplayState>(kDisplayTopic);
    CHECK(d && std::holds_alternative<calcu::machine::DotMatrixFrame>(d->output.frame));
    CHECK(d && sameDisplay(d->output, r.m->readDisplay()));
    const Value v = d->toValue();
    CHECK(v.find("frame") && *v.find("frame")->find("kind")->string() == "dots");
}

TEST(notice_is_the_sentence_as_written) {
    Rig r;
    const std::string sentence = "The card could not be read: the file is empty.";
    r.send("tp.notice", Value::object({{"text", sentence}}));
    CHECK(r.fe.noticeSentences() == std::vector<std::string>{sentence});
    // Notices are not kept: a late listener gets none.
    TestFrontend late;
    r.session->listen(late);
    CHECK(late.noticeSentences().empty());
}

TEST(log_lines_from_library_and_product_part) {
    Rig r;
    r.build();
    r.part.operation = [](OperationContext& c) { return logging(c); };
    r.send("tp.log", Value::object({{"text", "outdated: a.yaml"}}));
    r.send("tp.op");
    r.callWhileBusy();
    std::vector<std::string> lines;
    for (const auto& p : r.fe.mirror.takeAppended(kLogTopic)) lines.push_back(payloadAs<LogLine>(p)->text);
    CHECK(lines == (std::vector<std::string>{"outdated: a.yaml", "preset: line 1", "preset: line 2"}));
}
