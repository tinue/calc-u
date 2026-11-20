// The start, the
// built-in default, the model switch, the module and the printer, Reset and
// Cold boot as the library's commands on app/TIProductMachines, in the app
// session as a native frontend hosts it (tests/AppRig.hpp).

#include "TestHarness.hpp"
#include "testing/TempFolder.hpp"
#include "AppRig.hpp"
#include "../core/PC100C.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "../app/Bcd.hpp"

using apprig::attach;
using apprig::detach;
using apprig::Rig;
using apprig::Value;
namespace st = calcu::startup;

namespace {
// TRACE on, as the paper panel sends it (the library's set-control).
calcu::sessions::Value traceOnArgs() {
    return calcu::sessions::Value::object({{"peripheral", "PC-100C"}, {"control", "TRACE"}, {"on", true}});
}
const char* kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";
bool roms() { return testroms::romSet() != nullptr; }
std::string head(const std::string& model = "TI-59") { return "format-version: 2\nmodel: " + model + "\n"; }
bool traceOn(calcu::machine::Machine& m) {
    auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(m, calcu::machine::CapabilityId{"printer/controls"});
    if (!c) return false;
    for (const auto& i : c->list())
        if (i.id == PC100C::kTrace) return i.on;
    return false;
}
} // namespace

// ── Start, built-in default ─────────────────────────────────────────────────

TEST(no_settings_ti59) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("");   // no [startup] model
    CHECK(r.machine() && r.machine()->model().value == "TI-59");
    CHECK_EQ(r.settings.text("startup", "last-model"), std::string("TI-59"));   // StartupPart writes it
}

TEST(builtin_default_ml_no_printer) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* m : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(m);
        CHECK_EQ(r.machine()->model().value, std::string(m));
        CHECK_EQ(r.plugged("slot-1"), std::string("ML"));
        CHECK_EQ(r.plugged("printer"), std::string());
        CHECK(!r.part().printerConnected());
        CHECK_EQ(r.part().moduleID(), std::string("ML"));
    }
}

TEST(unreadable_startup_preset_gives_default_and_notice) {
    SKIP_UNLESS(roms(), kSkip);
    calcu::testing::TempFolder folder("calcu59-startup");
    folder.write("bad.ti59", "model: TI-59\nprinter: PC-100C\n");   // no format-version
    const std::string path = (folder.path() / "bad.ti59").string();
    Rig r("TI-59", std::make_shared<apprig::Files>(), [&](calcu::settings::SettingsFile& s) {
        s.set("preset", "TI-59", path);
    });
    CHECK_EQ(r.plugged("slot-1"), std::string("ML"));
    CHECK_EQ(r.plugged("printer"), std::string());
    bool noticed = false;
    for (const auto& n : r.notices()) noticed = noticed || n.find("could not be loaded") != std::string::npos;
    CHECK(noticed);
}

TEST(startup_preset_builds_its_machine) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-58", std::make_shared<apprig::Files>(), [](calcu::settings::SettingsFile& s) {
        s.set("preset", "TI-58", "examples:startup/default-ti58.ti58");   // the bundled startup preset
    });
    CHECK_EQ(r.machine()->model().value, std::string("TI-58"));
    CHECK_EQ(r.plugged("printer"), std::string("PC-100C"));
    CHECK(r.part().printerConnected());
}

// ── Constant memory at a build ──────────────────────────────────────────────

TEST(ti58c_without_startup_preset_gets_the_file_at_start_and_switch) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<apprig::Files>();
    files->files[apprig::kMemoryPath] =
        "format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n  bytes: |\n"
        "    R059: " + cu59::registerLineHex(*cu59::parseDecimal("42")) + "\n";
    {
        Rig r("TI-58C", files);
        CHECK(r.memory().readRaw(59) == *cu59::parseDecimal("42"));
    }
    Rig r("TI-59", files);
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58C"}})).done);
    CHECK(r.memory().readRaw(59) == *cu59::parseDecimal("42"));
}

TEST(ti58c_from_preset_cold_booted) {
    SKIP_UNLESS(roms(), kSkip);
    auto files = std::make_shared<apprig::Files>();
    files->files[apprig::kMemoryPath] =
        "format: ti58c-constant-memory\nformat-version: 1\nram:\n  encoding: register-hex\n  bytes: |\n"
        "    R059: " + cu59::registerLineHex(*cu59::parseDecimal("42")) + "\n";
    Rig r("TI-59", files);
    CHECK(r.load(head("TI-58C") + "data:\n  00: 1\n", "c.ti58c").done);
    CHECK(r.memory().readRaw(59) != *cu59::parseDecimal("42"));
}

// ── Model switch ────────────────────────────────────────────────────────────

TEST(switch_carries_no_module_no_printer) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);
    CHECK(r.send(st::kAttachCommand, attach("slot-1", "ST")).done);
    CHECK_EQ(r.plugged("slot-1"), std::string("ST"));
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58"}})).done);
    CHECK_EQ(r.plugged("slot-1"), std::string("ML"));   // the built-in default: nothing of the old machine
    CHECK_EQ(r.plugged("printer"), std::string());
    CHECK(r.part().model() == cu59::Model::TI58);
}

TEST(log_and_debug_settings_stay) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    r.part().debugPart().setLogLevel(cu59::DebugLevel::Info);
    r.part().debugPart().setCpuShown(true);
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58C"}})).done);
    CHECK(r.part().debugPart().logLevel() == cu59::DebugLevel::Info);
    CHECK(r.part().debugPart().cpuShown());
    CHECK(r.part().hasMachine());
}

TEST(last_model_follows_a_preset_build) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    CHECK(r.load(head("TI-58") + "data:\n  00: 1\n", "x.ti58").done);
    CHECK_EQ(r.machine()->model().value, std::string("TI-58"));
    CHECK(r.part().model() == cu59::Model::TI58);
    CHECK_EQ(r.settings.text("startup", "last-model"), std::string("TI-58"));
}

// ── Module and printer ──────────────────────────────────────────────────────

TEST(module_attach_detach_power_cycle_ti59_loses_ti58c_keeps) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* m : {"TI-59", "TI-58C"}) {
        Rig r(m);
        r.memory().writeRegister(5, *cu59::parseDecimal("7"));
        const cu59::RegisterNibbles before = r.memory().readRegister(5);
        CHECK(r.send(st::kAttachCommand, attach("slot-1", "ST")).done);   // a replace of ML
        CHECK_EQ(r.plugged("slot-1"), std::string("ST"));
        CHECK_EQ(r.part().moduleID(), std::string("ST"));
        const bool kept = r.memory().readRegister(5) == before;
        CHECK(kept == (std::string(m) == "TI-58C"));   // a power cycle: only the TI-58C keeps its memory
        CHECK(r.send(st::kDetachCommand, detach("slot-1")).done);
        CHECK_EQ(r.plugged("slot-1"), std::string());
        CHECK_EQ(r.part().moduleID(), std::string());
    }
}

TEST(printer_attach_detach_power_cycle) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    r.memory().writeRegister(5, *cu59::parseDecimal("7"));
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);
    CHECK(r.part().printerConnected());
    CHECK(r.memory().readRegister(5) != *cu59::parseDecimal("7"));   // the power cycle
    CHECK(r.send(st::kDetachCommand, detach("printer")).done);
    CHECK(!r.part().printerConnected());
}

TEST(trace_set_again_in_the_attached_pc100c) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);
    CHECK(!r.part().printerTrace());
    CHECK(r.send(calcu::peripherals::kSetControlCommand, traceOnArgs()).done);
    CHECK(traceOn(*r.machine()));
    CHECK(r.send(st::kDetachCommand, detach("printer")).done);
    CHECK(r.part().printerTrace());   // the app session keeps it
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);
    r.runFor(std::chrono::milliseconds(50));
    CHECK(traceOn(*r.machine()));      // set again in the new PC-100C
    CHECK(r.part().printerTrace());
}

// ── Reset, Cold boot ────────────────────────────────────────────────────────

TEST(reset_keeps_program_and_data_ti58c_power_cycle) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* m : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(m);
        r.memory().writeStep(3, 42);
        r.memory().writeRegister(5, *cu59::parseDecimal("7"));
        CHECK(r.send(st::kResetCommand).done);
        CHECK(r.machine()->isKeyReady());   // it ends at key readiness
        CHECK_EQ(int(r.memory().readStep(3)), 42);
        CHECK(r.memory().readRegister(5) == *cu59::parseDecimal("7"));
    }
}

TEST(cold_boot_empties_all_constant_memory_too) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* m : {"TI-59", "TI-58C"}) {
        Rig r(m);
        r.memory().writeStep(3, 42);
        r.memory().writeRegister(5, *cu59::parseDecimal("7"));
        CHECK(r.send(st::kColdBootCommand).done);
        CHECK_EQ(int(r.memory().readStep(3)), 0);
        CHECK(r.memory().readRegister(5) != *cu59::parseDecimal("7"));
    }
}

TEST(paper_module_printer_trace_own_card_kept) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    CHECK(r.load(head() + "slot-1: { name: ST }\nprinter: PC-100C\ncue-card:\n  title: Mine\nkeys:\n  - printer-trace: on\n  - key: 5 Prt\n").done);
    r.runFor(std::chrono::seconds(1));
    const auto paperBefore = r.part().paperPart().papers().size();
    for (const auto& cmd : {st::kResetCommand, st::kColdBootCommand}) {
        CHECK(r.send(cmd).done);
        CHECK_EQ(r.plugged("slot-1"), std::string("ST"));
        CHECK_EQ(r.plugged("printer"), std::string("PC-100C"));
        CHECK(traceOn(*r.machine()));
        CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Mine");   // the fitted cue card stays
        CHECK_EQ(r.part().paperPart().papers().size(), paperBefore);
    }
}

TEST(power_cycle_keeps_own_card) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    CHECK(r.load(head() + "slot-1: { name: ML }\ncue-card:\n  title: Mine\n").done);
    CHECK(r.send(st::kAttachCommand, attach("printer", "PC-100C")).done);   // a power cycle
    CHECK(r.part().ownCueCard() && r.part().ownCueCard()->title == "Mine");
}

TEST(reset_and_model_switch_stop_a_preset) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r("TI-59");
    const std::string loop = head() + "program:\n  keycodes: |\n    76 11 61 00 00\nkeys:\n  - regular-speed:\n  - key: A\n  - let-it-finish:\n";
    const auto id = r.session().send(r.me, calcu::sessions::Command{calcu::presets::kLoadPresetCommand,
                                                                     Value::object({{"content", loop}, {"name", "l.ti59"}})});
    r.runFor(std::chrono::milliseconds(500));
    CHECK(r.session().context().busy());
    CHECK(r.send(st::kResetCommand).done);   // ends the preset at once, without an error
    CHECK(r.fe.reply(id) != nullptr);
    CHECK(!r.session().context().busy());
    // A module change while busy is refused.
    const auto id2 = r.session().send(r.me, calcu::sessions::Command{calcu::presets::kLoadPresetCommand,
                                                                      Value::object({{"content", loop}, {"name", "l.ti59"}})});
    r.runFor(std::chrono::milliseconds(500));
    const auto refused = r.session().send(r.me, calcu::sessions::Command{st::kAttachCommand, attach("slot-1", "ST")});
    CHECK(r.fe.reply(refused) && !r.fe.reply(refused)->done);
    CHECK(r.send(st::kModelCommand, Value::object({{"name", "TI-58"}})).done);   // a model switch ends it too
    CHECK(r.fe.reply(id2) != nullptr);
}
