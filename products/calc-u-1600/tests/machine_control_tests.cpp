// LIB-18, the Calc-U-1600 column (design §10.6, P16-1 to P16-5): the start,
// the built-in default, the model and variant switches, a module, the
// peripherals and the host drive, the CE-1600P's ROM, Reset and Cold boot,
// a machine that is off -- on the library's StartupPart in Calc-U-1600's app
// session (app/tests/SessionFixture.hpp). Also C1600-4 AC-8's clause on the
// app: a module with a slot ROM is never saved. SKIP without roms/.
#include "../app/tests/SessionFixture.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "Loaders.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "TestHarness.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using calcu::sessions::Value;
using shared::Model;

namespace {

bool plugged(SessionFixture& f, const char* peripheral) {
    const calcu::machine::Machine* m = f.part().machine();
    if (!m) return false;
    for (const auto& e : m->plugged().byPlace)
        if (e.peripheral.value == peripheral) return true;
    return false;
}

std::size_t pluggedCount(SessionFixture& f) {
    const calcu::machine::Machine* m = f.part().machine();
    return m ? m->plugged().byPlace.size() : 0;
}

bool screenHas(SessionFixture& f, const std::string& text) {
    f.runFor(0.1);
    const std::string s = f.screen();
    if (s.find(text) == std::string::npos) {
        std::fprintf(stderr, "  \"%s\" not on \"%s\"\n", text.c_str(), s.c_str());
        return false;
    }
    return true;
}

// Types `line` and ENTER and waits until the machine is idle again.
void type(SessionFixture& f, const std::string& line) {
    f.typeLine(line);
    f.runUntilTyped(20);
    f.ok(calcu::presets::kWaitIdleCommand);
}

} // namespace

// ── P16-1: the start, the built-in default (AC-25, AC-26) ───────────────

TEST(lib18_ac25_no_settings_pc1500a_new0_check) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start());
    CHECK(f.part().currentModel() == Model::PC1500A);
    CHECK(screenHas(f, "NEW0?"));
}

TEST(lib18_ac26_builtin_default_variant_and_nothing_plugged) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        SessionFixture f;
        CHECK(f.start(model));
        const calcu::machine::Machine* m = f.part().machine();
        CHECK(m && m->model().value == model);
        CHECK(m && m->variant() && m->variant()->value == (std::string(model) == "PC-1600" ? "New" : "A04"));
        CHECK_EQ(pluggedCount(f), std::size_t{0});
    }
}

TEST(lib18_ac26_version1_startup_preset_gives_default_and_notice) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    // A startup preset of format 1 (row 1600-1): the built-in default and EC-1's notice.
    f.settings.setDefaultPresetPath(Model::PC1600, "tests/fixtures/presets/startup-pc1600.pc1600");
    CHECK(f.start("PC-1600"));
    CHECK(f.part().currentModel() == Model::PC1600);
    CHECK_EQ(pluggedCount(f), std::size_t{0});
    const auto n = f.notices();
    CHECK(!n.empty() && n.back().find("could not be loaded") != std::string::npos &&
          n.back().find("upgraded by hand to preset format 2") != std::string::npos);
}

// ── P16-2: model and variant (AC-27, AC-28) ─────────────────────────────

TEST(lib18_ac27_model_switch_keeps_no_module_or_peripheral) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("slot-1", "CE-1600M").done);
    CHECK(pluggedCount(f) == 2);
    CHECK(f.model("PC-1500A"));
    CHECK(f.model("PC-1600"));
    CHECK_EQ(pluggedCount(f), std::size_t{0});
    CHECK(f.held("slot-1") == nullptr);
}

TEST(lib18_ac27_ce1600p_rom_choice_back_to_new) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(shared::commands::kCE1600PRom, Value::object({{"version", "old"}})));
    CHECK(f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::Old);
    CHECK(f.model("PC-1500A"));
    CHECK(f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::New);
}

TEST(lib18_ac28_variant_switch_replugs_printer_interface_host_drive_modules) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const fs::path dir = f.root / "drive";
    fs::create_directories(dir);
    CHECK(f.attach("CE-150"));
    CHECK(f.attach("CE-158"));
    CHECK(f.ok(shared::commands::kMountHostDrive, Value::object({{"dir", dir.generic_string()}})));
    CHECK(f.choose("slot-2", "CE-1601M").done);
    CHECK(f.nameAndSave("slot-2", "Disk RAM").done);   // an instance: its content goes along
    CHECK(f.variant("Old"));
    CHECK(f.part().pc1600RomVersion() == shared::PC1600RomVersion::Old);
    CHECK(plugged(f, "CE-150") && plugged(f, "CE-158") && plugged(f, "host-drive"));
    CHECK_EQ(f.part().hostDriveDirectory(), dir.generic_string());
    CHECK(f.part().slotCard(2) && f.part().slotCard(2)->moduleName() == "CE-1601M");
    CHECK(f.held("slot-2") && f.held("slot-2")->name == "Disk RAM" && f.held("slot-2")->instance());
}

TEST(lib18_ac28_disk_in_drive_again_same_side) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("floppy", "Formatted").done);
    CHECK(f.run(calcu::media::kTurnCommand, Value::object({{"place", "floppy"}})).done);
    auto side = [&]() -> std::optional<int> {
        const calcu::media::PlaceState* p = f.mediaPlace("floppy");
        return p && p->holds ? p->holds->side : std::nullopt;
    };
    CHECK(side() == 1);
    CHECK(f.variant("Old"));
    CHECK(side() == 1);
    CHECK(f.held("floppy") && f.held("floppy")->name == "Formatted");
}
TEST(lib18_ac28_no_startup_preset_loaded) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.settings.setDefaultPresetPath(Model::PC1500, "examples:startup/default-pc1500.pc1500");
    CHECK(f.start("PC-1500"));
    CHECK(plugged(f, "CE-150"));   // the startup preset's
    CHECK(f.detach("CE-150"));
    CHECK(f.variant("A03"));
    CHECK(!plugged(f, "CE-150"));   // no startup preset at a variant switch (AC-7)
    CHECK(plugged(f, "CE-163F"));    // what was plugged in goes along
}

// ── P16-3: module, peripherals, host drive, a machine that is off (AC-29, AC-30, AC-33) ──

TEST(lib18_ac29_module_replace_power_cycle_memory_kept_new0_check) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.choose("slot-1", "CE-155").done);
    CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == "CE-155");
    CHECK(screenHas(f, "NEW0?"));   // the ROM's check after the power cycle (row 1600-4)
    CHECK(f.choose("slot-1", "CE-151").done);   // a replace
    CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == "CE-151");
    CHECK(f.choose("slot-1", "").done);
    CHECK(f.part().slotCard(1) == nullptr);
}

TEST(lib18_ac30_printer_ce158_attach_detach_power_cycle_memory_kept) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    type(f, "A=4711");
    const auto ticks = f.part().machine()->tickCount();
    CHECK(f.attach("CE-158"));
    CHECK(f.part().machine()->tickCount() > ticks);   // a power cycle ran
    type(f, "PRINT A");
    CHECK(screenHas(f, "4711"));   // the memory kept
    CHECK(f.detach("CE-158"));
    CHECK(!plugged(f, "CE-158"));
    type(f, "PRINT A");
    CHECK(screenHas(f, "4711"));
}

TEST(lib18_ac30_host_drive_mount_unmount_power_cycle) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const fs::path a = f.root / "a", b = f.root / "b";
    fs::create_directories(a);
    fs::create_directories(b);
    CHECK(f.ok(shared::commands::kMountHostDrive, Value::object({{"dir", a.generic_string()}})));
    CHECK(f.part().hostDriveAttached() && f.part().hostDriveDirectory() == a.generic_string());
    CHECK(f.ok(shared::commands::kMountHostDrive, Value::object({{"dir", b.generic_string()}})));   // a replace
    CHECK(f.part().hostDriveDirectory() == b.generic_string());
    CHECK(f.ok(shared::commands::kMountHostDrive, Value::object({{"dir", ""}})));
    CHECK(!f.part().hostDriveAttached());
}

TEST(lib18_ac33_off_machine_change_nothing_follows_found_on_switch_on) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    f.tap("off");
    f.runFor(1.0);
    const calcu::machine::DisplayOutput* d = f.listener.display();
    CHECK(d && !d->on);
    CHECK(f.attach("CE-150"));
    f.runFor(0.5);
    d = f.listener.display();
    CHECK(d && !d->on);              // it stays off: nothing follows
    CHECK(plugged(f, "CE-150"));
    f.screenDown("on");
    f.runFor(0.5);
    f.screenUp("on");
    f.runFor(15.0);   // the CE-150's power-on (the pen's home run) first
    CHECK(f.part().ce150Attached());   // found on switching on
    d = f.listener.display();
    CHECK(d && d->on);
}

// ── P16-4: the CE-1600P's ROM (AC-31) ───────────────────────────────────

TEST(lib18_ac31_plugged_is_replace_by_other_variant) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.ok(shared::commands::kCE1600PRom, Value::object({{"version", "old"}})));
    const calcu::machine::Machine* m = f.part().machine();
    bool old = false;
    for (const auto& e : m->plugged().byPlace)
        old = old || (e.peripheral.value == "CE-1600P" && e.variant && e.variant->value == "Old");
    CHECK(old);
    CHECK(f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::Old);
}

TEST(lib18_ac31_unplugged_choice_holds_for_next_attach) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(shared::commands::kCE1600PRom, Value::object({{"version", "old"}})));
    CHECK(f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::Old);
    // The frontends attach with the choice (MainWindow::togglePeripheral, engine.js).
    const char* variant = f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::Old ? "Old" : "New";
    CHECK(f.attach("CE-1600P", variant));
    CHECK(f.part().ce1600pRomVersion() == shared::CE1600PRomVersion::Old);
}

// ── P16-5: Reset, Cold boot (AC-32, AC-16; row 1600-9) ──────────────────

TEST(lib18_ac32_reset_keeps_memory) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    type(f, "B=815");
    CHECK(f.reset());
    type(f, "PRINT B");
    CHECK(screenHas(f, "815"));
}

TEST(lib18_ac32_cold_boot_empties_memory_ce151_ce155_battery_module_kept) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    type(f, "B=815");
    CHECK(f.coldBoot());
    type(f, "PRINT B");
    CHECK(f.screen().find("815") == std::string::npos);   // the memory emptied
    // The command is Machine::coldBoot(): a RAM module without a battery
    // forgets, a battery-backed one keeps (C1600-1 AC-6, row 1600-16; the
    // contents: sharp_machine_tests.cpp, c1600_1_ac6_cold_boot_*). The
    // module stays in its slot.
    for (const char* name : {"CE-151", "CE-1600M"}) {
        CHECK(f.choose("slot-1", name).done);
        CHECK(f.coldBoot());
        CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == name);
    }
}

TEST(ledger_1600_9_reset_and_model_switch_stop_a_preset) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const int how : {0, 1}) {
        SessionFixture f;
        CHECK(f.start("PC-1600"));
        f.readCost = std::chrono::milliseconds(1);
        const calcu::sessions::CallId preset = f.send(calcu::presets::kLoadPresetCommand,
                                                      Value::object({{"ref", "tests/fixtures/presets/maxed-out-mem.pc1600"}}));
        f.call();
        CHECK(f.busy());
        const calcu::sessions::ReplyMessage r = how == 0 ? f.run(calcu::startup::kResetCommand)
                                                         : f.run(calcu::startup::kModelCommand, Value::object({{"name", "PC-1500A"}}));
        CHECK(r.done);
        const auto stopped = f.takeReply(preset);
        CHECK(stopped && !stopped->done && stopped->reason == calcu::sessions::kStopped);
    }
}

// ── C1600-4 AC-8 on the app: a module with a slot ROM is never saved ────

TEST(c1600_4_ac8_module_with_slot_rom_never_saved) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    fs::create_directories(f.root / "build");
    std::ofstream(f.root / "build" / "rom.bin", std::ios::binary) << std::string(0x4000, '\0');
    const fs::path preset = f.root / "dev.pc1500";
    std::ofstream(preset) << "format-version: 2\nmodel: PC-1500\nslot-1: { name: CE-502B }\nslot-1-rom: { file: build/rom.bin }\n";
    CHECK(f.loadPreset(preset.generic_string()).done);
    CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == "CE-502B");
    CHECK(f.held("slot-1") == nullptr);   // never a medium (LIB-7 §10.5)
    const auto r = f.nameAndSave("slot-1", "Copy");
    CHECK(!r.done && r.reason.find("no medium") != std::string::npos);
}
