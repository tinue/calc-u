// The start,
// the built-in default, the model and variant switches, a module, the
// peripherals and the host drive, the CE-1600P's ROM, Reset and Cold boot,
// a machine that is off -- on the library's StartupPart in Calc-U-1600's app
// session (app/tests/SessionFixture.hpp). Also, on the app: a module with a slot ROM is never saved. SKIP without roms/.
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
using Model = calcu::machine::ModelId;

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

// ── The start, the built-in default ──────────────────────────────────────

TEST(no_settings_pc1500a_new0_check) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start());
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1500A"});
    CHECK(screenHas(f, "NEW0?"));
}

TEST(builtin_default_variant_and_nothing_plugged) {
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

TEST(other_version_startup_preset_gives_default_and_notice) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    // A startup preset of another format: the built-in default and a notice.
    const fs::path v1 = f.root / "startup-v1.pc1600";
    std::ofstream(v1) << "format-version: 1\nmodel: PC-1600\nplotter: CE-1600P\nfloppy: Formatted\nslot-1: CE-1600M\n";
    f.settings.file().set("preset", "PC-1600", v1.generic_string());
    CHECK(f.start("PC-1600"));
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1600"});
    CHECK_EQ(pluggedCount(f), std::size_t{0});
    const auto n = f.notices();
    CHECK(!n.empty() && n.back().find("could not be loaded") != std::string::npos &&
          n.back().find("This release reads preset format 2, not format 1.") != std::string::npos);
}

// ── Model and variant ────────────────────────────────────────────────────

TEST(model_switch_keeps_no_module_or_peripheral) {
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

TEST(ce1600p_rom_choice_back_to_new) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(cu1600::commands::kCE1600PVariant, Value::object({{"version", "old"}})));
    CHECK(f.part().ce1600pVariant() == "Old");
    CHECK(f.model("PC-1500A"));
    CHECK(f.part().ce1600pVariant() == "New");
}

TEST(variant_switch_replugs_printer_interface_host_drive_modules) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const fs::path dir = f.root / "drive";
    fs::create_directories(dir);
    CHECK(f.attach("CE-150"));
    CHECK(f.attach("CE-158"));
    CHECK(f.ok(cu1600::commands::kMountHostDrive, Value::object({{"dir", dir.generic_string()}})));
    CHECK(f.choose("slot-2", "CE-1601M").done);
    CHECK(f.nameAndSave("slot-2", "Disk RAM").done);   // an instance: its content goes along
    CHECK(f.variant("Old"));
    CHECK(f.part().machine()->variant() && f.part().machine()->variant()->value == "Old");
    CHECK(plugged(f, "CE-150") && plugged(f, "CE-158") && plugged(f, "host-drive"));
    CHECK_EQ(f.part().hostDriveDirectory(), dir.generic_string());
    CHECK(f.part().slotModuleName(2) == "CE-1601M");
    CHECK(f.held("slot-2") && f.held("slot-2")->name == "Disk RAM" && f.held("slot-2")->instance());
}

TEST(disk_in_drive_again_same_side) {
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
TEST(no_startup_preset_loaded) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.settings.file().set("preset", "PC-1500", "examples:startup/default-pc1500.pc1500");
    CHECK(f.start("PC-1500"));
    CHECK(plugged(f, "CE-150"));   // the startup preset's
    CHECK(f.detach("CE-150"));
    CHECK(f.variant("A03"));
    CHECK(!plugged(f, "CE-150"));   // no startup preset at a variant switch
    CHECK(plugged(f, "CE-163F"));    // what was plugged in goes along
}

// ── Module, peripherals, host drive, a machine that is off ───────────────

TEST(module_replace_power_cycle_memory_kept_new0_check) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.choose("slot-1", "CE-155").done);
    CHECK(f.part().slotModuleName(1) == "CE-155");
    CHECK(screenHas(f, "NEW0?"));   // the ROM's check after the power cycle
    CHECK(f.choose("slot-1", "CE-151").done);   // a replace
    CHECK(f.part().slotModuleName(1) == "CE-151");
    CHECK(f.choose("slot-1", "").done);
    CHECK(f.part().slotModuleName(1).empty());
}

TEST(printer_ce158_attach_detach_power_cycle_memory_kept) {
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

TEST(host_drive_mount_unmount_power_cycle) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const fs::path a = f.root / "a", b = f.root / "b";
    fs::create_directories(a);
    fs::create_directories(b);
    CHECK(f.ok(cu1600::commands::kMountHostDrive, Value::object({{"dir", a.generic_string()}})));
    CHECK(f.part().hostDriveAttached() && f.part().hostDriveDirectory() == a.generic_string());
    CHECK(f.ok(cu1600::commands::kMountHostDrive, Value::object({{"dir", b.generic_string()}})));   // a replace
    CHECK(f.part().hostDriveDirectory() == b.generic_string());
    CHECK(f.ok(cu1600::commands::kMountHostDrive, Value::object({{"dir", ""}})));
    CHECK(!f.part().hostDriveAttached());
}

TEST(off_machine_change_nothing_follows_found_on_switch_on) {
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

// ── The CE-1600P's ROM ───────────────────────────────────────────────────

TEST(plugged_is_replace_by_other_variant) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.ok(cu1600::commands::kCE1600PVariant, Value::object({{"version", "old"}})));
    const calcu::machine::Machine* m = f.part().machine();
    bool old = false;
    for (const auto& e : m->plugged().byPlace)
        old = old || (e.peripheral.value == "CE-1600P" && e.variant && e.variant->value == "Old");
    CHECK(old);
    CHECK(f.part().ce1600pVariant() == "Old");
}

TEST(unplugged_choice_holds_for_next_attach) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.ok(cu1600::commands::kCE1600PVariant, Value::object({{"version", "old"}})));
    CHECK(f.part().ce1600pVariant() == "Old");
    // The frontends attach with the choice (MainWindow::togglePeripheral, engine.js).
    const std::string variant = f.part().ce1600pVariant();
    CHECK(f.attach("CE-1600P", variant.c_str()));
    CHECK(f.part().ce1600pVariant() == "Old");
}

// ── Reset, Cold boot ─────────────────────────────────────────────────────

TEST(session_reset_keeps_memory) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    type(f, "B=815");
    CHECK(f.reset());
    type(f, "PRINT B");
    CHECK(screenHas(f, "815"));
}

TEST(cold_boot_empties_memory_ce151_ce155_battery_module_kept) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    type(f, "B=815");
    CHECK(f.coldBoot());
    type(f, "PRINT B");
    CHECK(f.screen().find("815") == std::string::npos);   // the memory emptied
    // The command is Machine::coldBoot(): a RAM module without a battery
    // forgets, a battery-backed one keeps (the contents:
    // sharp_machine_tests.cpp, cold_boot_*). The
    // module stays in its slot.
    for (const char* name : {"CE-151", "CE-1600M"}) {
        CHECK(f.choose("slot-1", name).done);
        CHECK(f.coldBoot());
        CHECK(f.part().slotModuleName(1) == name);
    }
}

TEST(reset_and_model_switch_stop_a_preset) {
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

// ── On the app: a module with a slot ROM is never saved ──────────────────

TEST(module_with_slot_rom_never_saved) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    fs::create_directories(f.root / "build");
    std::ofstream(f.root / "build" / "rom.bin", std::ios::binary) << std::string(0x4000, '\0');
    const fs::path preset = f.root / "dev.pc1500";
    std::ofstream(preset) << "format-version: 2\nmodel: PC-1500\nslot-1: { name: CE-502B }\nslot-1-rom: { file: build/rom.bin }\n";
    CHECK(f.loadPreset(preset.generic_string()).done);
    CHECK(f.part().slotModuleName(1) == "CE-502B");
    CHECK(f.held("slot-1") == nullptr);   // never a medium
    const auto r = f.nameAndSave("slot-1", "Copy");
    CHECK(!r.done && r.reason.find("no medium") != std::string::npos);
}
