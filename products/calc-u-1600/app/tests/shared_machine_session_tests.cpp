// The machine side of SharpAppPart on the library's app session (ported
// from 0.8.1's MachineSession tests in M-31): model switches, typing through
// SharpTypingHook, peripherals kept over a rebuild. Against the real ROMs
// (roms/, run from the product folder); SKIP without them.

#include <cstdio>
#include <string>

#include "SessionFixture.hpp"
#include "TestHarness.hpp"

namespace {

using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using shared::Model;
namespace c = shared::commands;

const char* settingsName(Model m) { return shared::settingsModelName(m); }

void test_switch_and_paste(Model model) {
    SessionFixture f;
    CHECK(f.start(settingsName(model)));
    CHECK(f.part().currentModel() == model);
    const auto* machine = f.state<shared::MachinePayload>(shared::topics::kMachine);
    CHECK(machine && machine->model == model && machine->builds == 1 && machine->modelSwitches == 1);
    CHECK(f.settings.lastUsedModel() == model);
    CHECK(f.host->display() && f.host->display()->on);
    CHECK(f.part().slotCount() == (model == Model::PC1600 ? 2 : 1));
    CHECK(f.part().slotCard(1) == nullptr && f.part().slotCard(2) == nullptr && f.part().slotCard(3) == nullptr);

    shared::SharpTypingHook& t = f.part().typing();
    // A cold PC-1500 shows "NEW0? :CHECK": CL, then NEW0.
    if (model != Model::PC1600) {
        t.tapKey("cl");
        f.runFor(0.5);
        t.typeCommand("NEW0");
        f.runFor(2.0);
    }
    t.typeCommand("PRINT 6*7");
    CHECK(t.pasteActive());
    f.runFor(4.0);
    CHECK(!t.pasteActive());
    const std::string text = f.screen();
    CHECK(text.find("42") != std::string::npos);
    if (text.find("42") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
}

TEST(test_timed_tap_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    // tapKey on the PC-1600 presses now and releases 30 ms of emulated time
    // later: one "A" on the input line. A key never released would repeat.
    f.part().typing().tapKey("a");
    f.runFor(1.5);
    const std::string text = f.screen();
    CHECK(text.find('A') != std::string::npos);
    CHECK(text.find("AA") == std::string::npos);
    // tapShiftedKey: SHIFT latched, then the base key -- shifted "1" is "'".
    f.part().typing().tapShiftedKey("1");
    f.runFor(1.0);
    CHECK(f.screen().find("A'") != std::string::npos);
}

// A key released before the key scan could see it (both events between two
// host calls) stays down for the minimum hold: still exactly one key.
void test_minimum_key_hold(Model model) {
    SessionFixture f;
    CHECK(f.start(settingsName(model)));
    shared::SharpTypingHook& t = f.part().typing();
    if (model != Model::PC1600) {
        t.tapKey("cl");
        f.runFor(0.5);
    }
    t.pressKey("a");
    t.releaseKey("a");
    f.runFor(1.5);
    std::string text = f.screen();
    CHECK(text.find('A') != std::string::npos);
    CHECK(text.find("AA") == std::string::npos);
    // A quick double tap: two taps, the second not cut short by the first's release.
    t.pressKey("b");
    t.releaseKey("b");
    t.pressKey("b");
    f.call(20);
    t.releaseKey("b");
    f.runFor(1.5);
    text = f.screen();
    CHECK(text.find("ABB") != std::string::npos);
    CHECK(text.find("BBB") == std::string::npos);
    if (text.find("ABB") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
    // A fast typist: the next key waits for the last one's deferred release.
    for (const char* k : {"c", "d", "d", "e"}) {
        t.pressKey(k);
        t.releaseKey(k);
    }
    f.runFor(1.5);
    text = f.screen();
    CHECK(text.find("ABBCDDE") != std::string::npos);
    if (text.find("ABBCDDE") == std::string::npos) std::fprintf(stderr, "  screen: %s\n", text.c_str());
}

// hostKeyDown() routes a host key the way the model needs.
TEST(test_host_keys) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    using shared::ResolvedKey;
    {
        SessionFixture f;
        CHECK(f.start("PC-1600"));
        shared::SharpTypingHook& t = f.part().typing();
        CHECK(t.hostKeyDown(ResolvedKey{"a"}, /*physical=*/true));
        f.runFor(0.1);
        t.hostKeyUp("a");
        CHECK(!t.hostKeyDown(ResolvedKey{"b"}, /*physical=*/false));
        f.runFor(0.5);
        CHECK(!t.hostKeyDown(ResolvedKey{"1", /*needsShift=*/true}, /*physical=*/true));
        f.runFor(1.0);
        CHECK(f.screen().find("AB'") != std::string::npos);
        CHECK(!t.hostKeyDown(ResolvedKey{"e", false, /*needsKbii=*/true}, /*physical=*/true));
        CHECK(t.liveTypingActive());
        CHECK(!t.hostKeyDown(ResolvedKey{"a"}, /*physical=*/true));
    }
    {
        SessionFixture g("calcu-session-test-g");
        CHECK(g.start("PC-1600"));
        g.part().typing().typeCommand("PRINT 123456789");
        CHECK(g.part().typing().pasteActive());
        g.part().typing().hostShiftTap();   // a key stops a paste
        CHECK(!g.part().typing().pasteActive());
    }
    {
        SessionFixture f;
        CHECK(f.start("PC-1500A"));
        shared::SharpTypingHook& t = f.part().typing();
        CHECK(t.hostKeyDown(ResolvedKey{"left"}, /*physical=*/true));
        t.hostKeyUp("left");
        CHECK(!t.hostKeyDown(ResolvedKey{"left"}, /*physical=*/false));
        CHECK(!t.hostKeyDown(ResolvedKey{"a"}, /*physical=*/true));
        CHECK(!t.hostKeyDown(ResolvedKey{"1", /*needsShift=*/true}, /*physical=*/true));
    }
}

TEST(test_rebuild_keeps_plotter) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.ok(c::kTogglePeripheral, calcu::sessions::Value::object({{"peripheral", "CE-150"}})));
    CHECK(f.ok(c::kTogglePeripheral, calcu::sessions::Value::object({{"peripheral", "CE-158"}})));
    CHECK(f.part().ce150Attached() && f.part().ce158Attached());
    const auto builds = f.state<shared::MachinePayload>(shared::topics::kMachine)->builds;
    CHECK(f.ok(c::kPC1500Rom, calcu::sessions::Value::object({{"revision", "A03"}})));   // a rebuild (the PC-1500A stays A04)
    CHECK(f.state<shared::MachinePayload>(shared::topics::kMachine)->builds == builds + 1);
    CHECK(f.part().ce150Attached() && f.part().ce158Attached());
    CHECK(f.ok(c::kPickModel, calcu::sessions::Value::object({{"model", "PC-1600"}})));   // a model switch starts bare
    CHECK(!f.part().ce150Attached() && !f.part().ce158Attached());

    // PC-1600: the CE-1600P survives a ROM rebuild.
    CHECK(f.ok(c::kTogglePeripheral, calcu::sessions::Value::object({{"peripheral", "CE-1600P"}})));
    CHECK(f.ok(c::kPC1600Rom, calcu::sessions::Value::object({{"version", "old"}})));
    CHECK(f.part().ce1600pAttached());
}

} // namespace

TEST(test_switch_and_paste_pc1500a) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_switch_and_paste(Model::PC1500A); }
TEST(test_switch_and_paste_pc1600) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_switch_and_paste(Model::PC1600); }
TEST(test_minimum_key_hold_pc1500a) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_minimum_key_hold(Model::PC1500A); }
TEST(test_minimum_key_hold_pc1600) { SKIP_UNLESS(romsPresent(), "no ROMs in roms/"); test_minimum_key_hold(Model::PC1600); }
