// LIB-3 L17: the topic mirror (support for AC-29, AC-38).
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;

TEST(mirror_keeps_last_state_and_appended) {
    TopicMirror mirror;
    int changes = 0;
    mirror.onChange = [&](const TopicName&) { ++changes; };
    const TopicName state{"tp.state"}, stream{"tp.stream"};
    mirror.deliver(StateMessage{state, std::make_shared<TestValue>(1)});
    mirror.deliver(StateMessage{state, std::make_shared<TestValue>(2)});
    CHECK_EQ(payloadAs<TestValue>(*mirror.state(state))->v, 2);   // the last

    mirror.deliver(StreamMessage{stream, false, {std::make_shared<TestText>("a")}});
    mirror.deliver(StreamMessage{stream, false, {std::make_shared<TestText>("b"), std::make_shared<TestText>("c")}});
    auto got = mirror.takeAppended(stream);
    CHECK_EQ(got.size(), std::size_t(3));
    CHECK_EQ(payloadAs<TestText>(got[2])->text, std::string("c"));
    CHECK(mirror.takeAppended(stream).empty());   // taken
    CHECK(!mirror.takeReset(stream));

    mirror.deliver(StreamMessage{stream, false, {std::make_shared<TestText>("d")}});
    mirror.deliver(StreamMessage{stream, true, {std::make_shared<TestText>("e")}});   // starts over
    CHECK(mirror.takeReset(stream));
    got = mirror.takeAppended(stream);
    CHECK_EQ(got.size(), std::size_t(1));
    CHECK_EQ(payloadAs<TestText>(got[0])->text, std::string("e"));
    CHECK_EQ(changes, 6);
    CHECK(mirror.takeAppended(TopicName{"nothing"}).empty());
}
