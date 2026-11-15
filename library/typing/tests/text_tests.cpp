// LIB-5 design §6, L9: typed text, paste, latches (AC-11, AC-12, AC-13;
// EC-4; D-14).
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

namespace {
std::vector<std::string> keysOf(const std::vector<std::string>& log) {
    std::vector<std::string> out;
    for (const auto& l : log)
        if (l.find(" down") != std::string::npos) out.push_back(l.substr(l.find(' ') + 1, l.rfind(' ') - l.find(' ') - 1));
    return out;
}
} // namespace

TEST(ac12_paste_first_line_skips_unknown_no_enter) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.paste("1z\t2\r\n3");
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"1", "2"}));   // no 3, no enter
    r.paste("12");
    r.paste("3");   // AC-34: a second paste is added after the first
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"1", "2", "1", "2", "3"}));
}

TEST(char_keys_without_support_same_has_a_key_answer) {
    const TypingProfile p = testProfile();
    TypingTestMachine m;
    const auto caps = m.capabilities(TypingSupport::kKind);
    auto* s = calcu::machine::capabilityAs<TypingSupport>(m, caps.at(0).id);
    for (char32_t c : {U'1', U'A', U'é', U'z', U'ü'}) {
        for (int latch = 0; latch < 3; ++latch) {
            m.latches = {{"shift", latch == 1}, {"kbii", latch == 2}};
            CHECK(p.charKeys(c, nullptr).has_value() == p.charKeys(c, s).has_value());   // D-14
        }
    }
    CHECK(p.charKeys(U'é', nullptr) == (std::optional<std::vector<Press>>(
                                                std::vector<Press>{Press{KeyId{"kbii"}}, Press{KeyId{"a"}}, Press{KeyId{"kbii"}, true}})));
    // A machine without typing support types as with every latch off.
    TypingRig r;
    r.m->offersSupport = false;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.paste("\xC3\xA9");
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"kbii", "a", "kbii"}));
}

TEST(ac13_latched_key_released_first_or_skipped) {
    TypingRig r;
    r.run(100);
    std::size_t from = r.m->keyEvents().size();
    r.m->latches = {{"shift", true}, {"kbii", false}};
    r.paste("\xC3\xA9");
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"shift", "kbii", "a", "kbii"}));
    from = r.m->keyEvents().size();
    r.m->latches = {{"shift", false}, {"kbii", true}};
    r.paste("1\xC3\xA9" "2");
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"1", "2"}));   // é left out
    // E13: decided when its first press is due, with the latches of then.
    from = r.m->keyEvents().size();
    r.m->latches = {{"shift", false}, {"kbii", true}};
    r.paste("1\xC3\xA9");
    r.run(200);   // 1 is down
    r.m->latches = {{"shift", false}, {"kbii", false}};
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"1", "kbii", "a", "kbii"}));
}

TEST(ec4_committed_text_each_char_one_press) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.send("host-text", calcu::sessions::Value::object({{"text", "12"}}));
    r.run(20'000);
    const auto log = r.log(from);
    CHECK(keysOf(log) == (std::vector<std::string>{"1", "2"}));
    if (log.size() == 4) {
        CHECK(tickOf(log[1]) - tickOf(log[0]) >= 1000 && tickOf(log[1]) - tickOf(log[0]) < 1005);
        CHECK(tickOf(log[3]) - tickOf(log[2]) >= 1000 && tickOf(log[3]) - tickOf(log[2]) < 1005);
    }
}

TEST(ac16_type_text_and_enter_refuses_unknown_nothing_typed) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    const auto bad = r.type("12z");
    CHECK(r.reply(bad) && !r.reply(bad)->done && r.reply(bad)->reason == "No key types the character \"z\".");
    r.run(5000);
    CHECK(r.log(from).empty());
    const auto ok = r.type("1B");
    CHECK(r.reply(ok) == nullptr);   // deferred: the reply comes when ENTER is up
    r.run(20'000);
    CHECK(keysOf(r.log(from)) == (std::vector<std::string>{"1", "shift", "b", "enter"}));
    const auto* rep = r.reply(ok);
    CHECK(rep && rep->done);
}
