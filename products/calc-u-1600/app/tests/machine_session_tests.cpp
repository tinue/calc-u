// The machine side of SharpAppPart on the library's app session (ported
// from 0.8.1's MachineSession tests in M-31): model switches, typing through
// the library's typing engine (LIB-5), peripherals kept over a rebuild. Against the real ROMs
// (roms/, run from the product folder); SKIP without them.

#include <cctype>
#include <cstdio>
#include <string>

#include "SessionFixture.hpp"
#include "control/ControlState.hpp"
#include "TestHarness.hpp"

namespace {

using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using Model = calcu::machine::ModelId;

namespace c = cu1600::commands;

const char* settingsName(Model m) { return m.value.c_str(); }

void test_switch_and_paste(Model model) {
    SessionFixture f;
    CHECK(f.start(settingsName(model)));
    CHECK(f.part().model() == model);
    const auto* cs = f.state<calcu::control::ControlState>(calcu::control::kControlStateTopic);
    CHECK(cs && cs->model == model && f.session().machineSession().generation() == 1);
    CHECK(cu1600::sharpModel(f.settings.file().text("startup", "last-model")) == model);
    CHECK(f.listener.display() && f.listener.display()->on);
    CHECK(f.part().slotCount() == (model == calcu::machine::ModelId{"PC-1600"} ? 2 : 1));
    CHECK(f.part().slotModuleName(1).empty() && f.part().slotModuleName(2).empty() && f.part().slotModuleName(3).empty());

    // A cold PC-1500 shows "NEW0? :CHECK": CL, then NEW0.
    if (model != calcu::machine::ModelId{"PC-1600"}) {
        f.tap("cl");
        f.runFor(0.5);
        f.typeLine("NEW0");
        f.runFor(2.0);
    }
    f.typeLine("PRINT 6*7");
    CHECK(!f.engine().idle());
    f.runFor(4.0);
    CHECK(f.engine().idle());
    const std::string text = f.screen();
    CHECK(text.find("42") != std::string::npos);
    if (text.find("42") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
}

// LIB-5 AC-37 (instant press and release is one key): a tap on the PC-1600
// presses now and releases after the minimum hold: one "A" on the input
// line. A key never released would repeat.
TEST(test_timed_tap_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.tap("a");
    f.runFor(1.5);
    const std::string text = f.screen();
    CHECK(text.find('A') != std::string::npos);
    CHECK(text.find("AA") == std::string::npos);
    // A shifted host key: SHIFT, then the base key -- shifted "1" is "'".
    calcu::typing::HostKeyEvent e;
    e.physical = "Quote";
    e.produced = '\'';
    f.hostDown(e);
    f.hostUp("Quote");
    f.runFor(1.0);
    CHECK(f.screen().find("A'") != std::string::npos);
}

// AC-37: a key released before the key scan could see it (both events
// between two host calls) stays down for the minimum hold: still exactly
// one key; a double tap is two; a fast sequence comes out in order.
void test_minimum_key_hold(Model model) {
    SessionFixture f;
    CHECK(f.start(settingsName(model)));
    if (model != calcu::machine::ModelId{"PC-1600"}) {
        f.tap("cl");
        f.runFor(0.5);
    }
    f.tap("a");
    f.runFor(1.5);
    std::string text = f.screen();
    CHECK(text.find('A') != std::string::npos);
    CHECK(text.find("AA") == std::string::npos);
    // A quick double tap: two taps, the second not cut short by the first's release.
    f.screenDown("b");
    f.screenUp("b");
    f.screenDown("b");
    f.call(20);
    f.screenUp("b");
    f.runFor(1.5);
    text = f.screen();
    CHECK(text.find("ABB") != std::string::npos);
    CHECK(text.find("BBB") == std::string::npos);
    if (text.find("ABB") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
    // A fast typist: the next key waits for the last one's deferred release.
    for (const char* k : {"c", "d", "d", "e"}) f.tap(k);
    f.runFor(1.5);
    text = f.screen();
    CHECK(text.find("ABBCDDE") != std::string::npos);
    if (text.find("ABBCDDE") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
}

namespace {
calcu::typing::HostKeyEvent hostChar(char32_t c, const char* physical) {
    calcu::typing::HostKeyEvent e;
    e.physical = physical;
    e.produced = c;
    if (c < 0x80 && std::isalpha(static_cast<int>(c))) e.carried = static_cast<char32_t>(std::tolower(static_cast<int>(c)));
    return e;
}
} // namespace

// Host keys through the product's keyboard map the way the model needs.
TEST(test_host_keys) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    {
        SessionFixture f;
        CHECK(f.start("PC-1600"));
        f.hostDown(hostChar('a', "KeyA"));   // held until let go
        f.runFor(0.1);
        CHECK(!f.engine().liveHeld().empty());
        f.hostUp("KeyA");
        f.hostDown(hostChar('b', "KeyB"));
        f.hostUp("KeyB");
        f.runFor(0.5);
        f.hostDown(hostChar('\'', "Quote"));   // shift, then 1
        f.runFor(1.0);
        CHECK(f.screen().find("AB'") != std::string::npos);
        f.hostDown(hostChar(U'\u00e9', "KeyE"));   // é: KBII, E, KBII
        CHECK(!f.engine().idle());
    }
    {
        SessionFixture g("calcu-session-test-g");
        CHECK(g.start("PC-1600"));
        g.paste("PRINT 123456789");
        CHECK(!g.engine().idle());
        g.runFor(0.1);
        g.screenDown("shift");   // a key stops a paste
        g.screenUp("shift");
        CHECK(g.engine().waiting() <= 1);
    }
    {
        SessionFixture f;
        CHECK(f.start("PC-1500A"));
        calcu::typing::HostKeyEvent left;
        left.physical = "ArrowLeft";
        left.named = calcu::typing::NamedKey::Left;
        f.hostDown(left);   // held: the ROM's own repeat (AC-31)
        CHECK(f.engine().liveHeld().size() == 1);
        f.hostUp("ArrowLeft");
        CHECK(f.engine().liveHeld().empty());
    }
}

TEST(test_rebuild_keeps_plotter) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    CHECK(f.toggle("CE-150"));
    CHECK(f.toggle("CE-158"));
    CHECK(f.part().ce150Attached() && f.part().ce158Attached());
    const auto builds = f.session().machineSession().generation();
    CHECK(f.ok(calcu::startup::kVariantCommand, calcu::sessions::Value::object({{"name", "A03"}})));   // a rebuild
    CHECK(f.session().machineSession().generation() == builds + 1);
    CHECK(f.part().ce150Attached() && f.part().ce158Attached());
    CHECK(f.model("PC-1600"));   // a model switch starts bare
    CHECK(!f.part().ce150Attached() && !f.part().ce158Attached());

    // PC-1600: the CE-1600P survives a ROM rebuild.
    CHECK(f.toggle("CE-1600P"));
    CHECK(f.ok(calcu::startup::kVariantCommand, calcu::sessions::Value::object({{"name", "Old"}})));
    CHECK(f.part().ce1600pAttached());
}

} // namespace

TEST(test_switch_and_paste_pc1500a) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_switch_and_paste(calcu::machine::ModelId{"PC-1500A"}); }
TEST(test_switch_and_paste_pc1600) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_switch_and_paste(calcu::machine::ModelId{"PC-1600"}); }
TEST(test_minimum_key_hold_pc1500a) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_minimum_key_hold(calcu::machine::ModelId{"PC-1500A"}); }
TEST(test_minimum_key_hold_pc1600) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_minimum_key_hold(calcu::machine::ModelId{"PC-1600"}); }
