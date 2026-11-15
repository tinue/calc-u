// LIB-5 design §6, L1: the test machine `test-type`.
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;
using calcu::machine::KeyId;

TEST(test_type_logs_keys_with_ticks) {
    TypingTestMachine m;
    CHECK(m.descriptor().model == kTypeModel);
    m.advance(10);
    CHECK(m.keyDown(KeyId{"a"}) == calcu::machine::KeyCall::Done);
    const auto at = m.tickCount();
    m.advance(10);
    m.keyUp(KeyId{"a"});
    const auto log = m.log();
    CHECK_EQ(log.size(), std::size_t(2));
    CHECK_EQ(log[0], std::to_string(at) + " a down");
    CHECK_EQ(log[1], std::to_string(m.tickCount()) + " a up");
    CHECK(m.keyDown(KeyId{"zz"}) == calcu::machine::KeyCall::UnknownKey);
}

TEST(test_type_scripted_readiness_samples_and_latches) {
    TypingTestMachine m;
    CHECK(!m.isKeyReady());   // test-seg's start-up of 50 ticks
    m.advance(60);
    CHECK(m.isKeyReady());
    m.readiness = [](const TypingTestMachine&) { return false; };
    CHECK(!m.isKeyReady());
    m.samplePeriod = 100;
    m.keyDown(KeyId{"b"});
    m.advance(250);
    CHECK_EQ(m.samplesOf(KeyId{"b"}), std::uint64_t(2));
    const auto caps = m.capabilities(calcu::typing::TypingSupport::kKind);
    CHECK_EQ(caps.size(), std::size_t(1));
    auto* s = calcu::machine::capabilityAs<calcu::typing::TypingSupport>(m, caps.at(0).id);
    CHECK(s != nullptr);
    m.latches["kbii"] = true;
    CHECK(s && s->latched("kbii") && !s->latched("shift"));
    CHECK(s && s->samples(KeyId{"b"}) == std::optional<std::uint64_t>(2));
}
