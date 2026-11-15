// LIB-5 design §6, L7: the keyboard map and sequences (AC-7, AC-8; EC-2).
#include "TestHarness.hpp"
#include "TypingRig.hpp"

using namespace calcu::typing;
using namespace calcu::typing::test;

TEST(ac7_frontend_event_mapped_by_model_map) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    // The frontend sends what the host says; the product's map decides.
    HostKeyEvent e = letter('b', "KeyB");
    r.hostDown(e);
    CHECK_EQ(r.log(from).size(), std::size_t(1));
    CHECK(r.log(from).size() == 1 && r.log(from)[0].find("b down") != std::string::npos);
    r.hostUp("KeyB");
    r.run(2000);
    HostKeyEvent cmd = letter('b', "KeyB");
    cmd.mods.command = true;   // the map gives nothing
    r.hostDown(cmd);
    r.run(2000);
    CHECK_EQ(r.log(from).size(), std::size_t(2));
    // The event crosses the session as a Value and back.
    HostKeyEvent full;
    full.physical = "KeyQ";
    full.named = NamedKey::PageDown;
    full.produced = U'é';
    full.carried = 'q';
    full.mods = Modifiers{true, false, true, false, true, true};
    full.autoRepeat = true;
    const HostKeyEvent back = hostKeyEventOf(toValue(full));
    CHECK(back.physical == "KeyQ" && back.named == NamedKey::PageDown && back.produced == full.produced &&
          back.carried == 'q' && back.mods.shift && !back.mods.control && back.mods.alt && !back.mods.command &&
          back.mods.meta && back.mods.macHost && back.autoRepeat);
    for (int k = 0; k <= static_cast<int>(NamedKey::PageDown); ++k)
        CHECK(namedKeyOf(namedKeyName(static_cast<NamedKey>(k))) == static_cast<NamedKey>(k));
}

TEST(ac8_sequence_presses_key_up_does_nothing) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    r.hostDown(letter('a', "KeyA", true));   // Shift+A: shift, then a
    r.hostUp("KeyA");                        // at once: does nothing
    r.run(10'000);
    const auto log = r.log(from);
    CHECK_EQ(log.size(), std::size_t(4));
    if (log.size() == 4) {
        CHECK(log[0].find("shift down") != std::string::npos && log[2].find("a down") != std::string::npos);
        CHECK(tickOf(log[1]) - tickOf(log[0]) >= 1000);   // each a press of its own
        CHECK(tickOf(log[3]) - tickOf(log[2]) >= 1000);
    }
    CHECK(r.held().empty());
}

TEST(ec2_unmapped_types_nothing) {
    TypingRig r;
    r.run(100);
    const std::size_t from = r.m->keyEvents().size();
    HostKeyEvent z;
    z.physical = "KeyZ";
    z.carried = 'z';
    z.produced = 'z';
    r.hostDown(z);
    r.hostUp("KeyZ");
    CHECK_EQ(r.part.engine().waiting(), std::size_t(0));
    r.run(2000);
    CHECK_EQ(r.log(from).size(), std::size_t(0));
    CHECK(r.part.engine().idle());
}
