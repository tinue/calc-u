// Calc-U-1600's app layer (SharpAppPart) on the library's app session
// (ported in M-31 from 0.8.1's AppSession tests): the Sharp operations as
// product commands run in parts on a scripted clock, the bays, the control
// state topic, the loads. SKIP without ROMs.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "Machine/SharpProductKinds.hpp"
#include "SessionFixture.hpp"
#include "app/CardYamlDump.hpp"
#include "app/MediaLibrary.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = shared::commands;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;
using shared::Model;

Value toggle(const char* p) { return Value::object({{"peripheral", p}}); }
Value preset(const char* path) { return Value::object({{"path", path}}); }
Value module(int slot, const char* name) { return Value::object({{"slot", slot}, {"name", name}}); }

int busyMessages(const SessionFixture& f, bool runs) {
    int n = 0;
    for (const auto& m : f.messages)
        if (const auto* s = std::get_if<sessions::StateMessage>(&m))
            if (const auto* b = sessions::payloadAs<sessions::BusyState>(s->state); b && b->operationRuns == runs) ++n;
    return n;
}

TEST(test_cooperative_preset) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.part().currentModel() == Model::PC1500A);
    f.messages.clear();
    f.readCost = std::chrono::milliseconds(1);   // the host's work: the allowance of a call runs out

    // The default PC-1600 preset (CE-1600P + floppy, two slot cards, a
    // dozen typed lines), a host call at a time.
    const sessions::CallId id = f.send(c::kLoadPreset, preset("tests/fixtures/presets/startup-pc1600.pc1600"));
    CHECK(f.busy());
    // A second operation is refused while one runs.
    const sessions::CallId second = f.send(c::kDefaultPreset);
    const auto refused = f.takeReply(second);
    CHECK(refused && !refused->done && refused->reason == sessions::kAnotherOperationRuns);
    int calls = 0;
    std::optional<sessions::ReplyMessage> r;
    while (!(r = f.takeReply(id)) && calls < 100000) {
        f.call();
        ++calls;
    }
    CHECK(r && r->done);
    if (r && !r->done) std::fprintf(stderr, "  preset: %s\n", r->reason.c_str());
    CHECK(calls > 10);   // it really ran in parts
    CHECK(!f.busy());
    CHECK(busyMessages(f, true) == 1 && busyMessages(f, false) == 1);
    CHECK(f.listener.mirror().takeAppended(sessions::kLogTopic).size() > 10);   // the preset's step log
    CHECK(f.part().currentModel() == Model::PC1600);
    CHECK(f.part().ce1600pAttached());
    CHECK(f.part().modules().selectedModuleName(1) == "CE-1600M");
    CHECK(f.part().modules().selectedModuleName(2) == "CE-1601M");
    CHECK(f.part().floppy().selectedDiskName() == "Formatted");
    CHECK(f.screen().find("Memory:") != std::string::npos);
}

TEST(test_operations_and_selection) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start());
    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1600"}})));
    CHECK(f.part().currentModel() == Model::PC1600);
    CHECK(f.settings.lastUsedModel() == Model::PC1600);

    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")) && f.part().ce1600pAttached());
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")) && !f.part().ce1600pAttached());
    CHECK(f.notices().empty());

    // The default preset of another model is refused, touching nothing.
    sessions::ReplyMessage r = f.run(c::kLoadDefaultPreset, Value::object({{"path", "tests/fixtures/presets/startup-pc1500a.pc1500a"},
                                                                           {"model", "PC-1600"}}));
    CHECK(!r.done);
    CHECK(f.part().currentModel() == Model::PC1600);
    // A preset that doesn't parse: refused, and a notice.
    r = f.run(c::kLoadPreset, preset("tests/fixtures/presets/no-such.pc1600"));
    CHECK(!r.done && !r.reason.empty());
    const auto n = f.notices();
    CHECK(!n.empty() && n.back() == r.reason);
}

// The tape topic is what the tape bay saw at the last call -- and an
// operation ends with one, so it is current right after.
TEST(test_tape_status_follows_calls) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-150")));
    CHECK(f.part().tapeInterfaceAttached());
    auto tapes = [&] { return f.state<shared::TapesPayload>(shared::topics::kTapes)->status.mode; };
    CHECK(tapes() == TapeDeck::Mode::Empty);
    CHECK(f.part().tapes().recordNew("side a", nullptr));
    CHECK(tapes() == TapeDeck::Mode::Empty);   // not seen yet
    f.call();
    CHECK(tapes() == TapeDeck::Mode::Record);
    std::string error;
    CHECK(f.part().tapes().selectForPlay({}, &error));
    CHECK(f.ok(c::kReset, Value::object({{"all", false}})));
    CHECK(tapes() == TapeDeck::Mode::Empty);
}

// The control state topic: each model's own peripherals on the bar, the
// CE-1600P excluding the CE-150 / CE-158, the floppy and tape rows
// following the attached boxes.
TEST(test_control_state) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    auto state = [&] { return f.state<shared::ControlStatePayload>(shared::topics::kControlState)->state; };
    shared::ControlState s = state();
    CHECK(s.model == Model::PC1500 && s.slotCount == 1);
    CHECK(s.ce150.onControlBar && s.ce158.onControlBar && !s.ce1600p.offered);
    CHECK(s.ce150.enabled && s.ce158.enabled);
    CHECK(s.pc1500RomMenu && !s.pc1600RomMenu && !s.ce1600pRomMenu);
    CHECK(!s.floppyShown && !s.tapeShown && !s.hostDriveOffered);
    CHECK(!s.paperOwner && !s.printerShown);

    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-150")));
    s = state();
    CHECK(s.ce150.attached && s.tapeShown && s.paperOwner && s.paperOwner->value == "CE-150");

    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1600"}})));
    s = state();
    CHECK(s.slotCount == 2 && s.floppyShown && !s.floppyEnabled);
    CHECK(s.ce1600p.offered && s.ce1600p.onControlBar && s.ce1600p.enabled);
    CHECK(!s.ce150.onControlBar && s.ce150.offered && !s.pc1500RomMenu && s.pc1600RomMenu);
    CHECK(s.hostDriveOffered && s.hostDriveDir.empty());

    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")));
    s = state();
    CHECK(s.ce1600p.attached && s.floppyEnabled && s.tapeShown && s.paperOwner && s.paperOwner->value == "CE-1600P");
    CHECK(!s.ce150.enabled && !s.ce158.enabled);
}

// A frontend without host directories never offers the host drive.
TEST(test_control_state_without_host_directories) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.hostDirectories = false;
    CHECK(f.start("PC-1600"));
    CHECK(!f.state<shared::ControlStatePayload>(shared::topics::kControlState)->state.hostDriveOffered);
}

std::vector<std::uint8_t> readBytes(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

const shared::MediaItem* findItem(const std::vector<shared::MediaItem>& items, const std::string& name) {
    for (const auto& item : items)
        if (item.name == name) return &item;
    return nullptr;
}

// MediaLibrary: the save folder's cards, disks and tapes -- listed with
// their in-use state, exported, deleted (not while in use), imported back.
TEST(test_media_library) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    shared::MediaLibrary library(f.part());
    CHECK(library.items().empty());

    CHECK(f.ok(c::kSelectModule, module(1, "CE-1600M")));
    std::string error;
    CHECK(f.part().modules().nameAndSave(1, "Work", &error));
    std::vector<shared::MediaItem> items = library.items();
    const shared::MediaItem* work = findItem(items, "Work");
    CHECK(work && work->type == shared::MediaType::Card && work->inUse && !work->isTemplate);
    CHECK(work && work->fileName == "Work.card.yaml");
    if (!work) return;
    const shared::MediaItem workItem = *work;
    std::vector<std::uint8_t> bytes;
    CHECK(library.exportBytes(workItem.path, &bytes) && !bytes.empty());
    CHECK(!library.remove(workItem.path).ok);   // in slot 1

    CHECK(f.ok(c::kSelectModule, module(1, "")));
    CHECK(library.remove(workItem.path).ok);
    CHECK(library.items().empty());
    CHECK(library.importFile("Work.card.yaml", bytes).ok);
    CHECK(findItem(library.items(), "Work") != nullptr);
    CHECK(!library.importFile("Work.card.yaml", bytes).ok);   // exists
    shared::OpResult r = library.importFile("x.card.yaml", {'n', 'o', 'p', 'e'});
    CHECK(!r.ok && r.error.find("isn't a card file") != std::string::npos);
    std::vector<std::uint8_t> bundled = readBytes("frontends/qt6/resources/cards/ce161.card.yaml");
    CHECK(!bundled.empty() && !library.importFile("ce161.card.yaml", bundled).ok);   // a bundled card's name

    const std::vector<std::uint8_t> wav = readBytes("tests/fixtures/tape/pc1500_tape.wav");
    CHECK(library.importFile("Side A.wav", wav).ok);
    CHECK(!library.importFile("side a.wav", wav).ok);
    CHECK(!library.importFile("Broken.wav", {'R', 'I', 'F', 'F'}).ok);
    CHECK(findItem(library.items(), "Broken") == nullptr);
    items = library.items();
    const shared::MediaItem* tape = findItem(items, "Side A");
    CHECK(tape && tape->type == shared::MediaType::Tape && !tape->inUse);
    CHECK(!library.importFile("notes.txt", {'x'}).ok);
}

// Name & Save with "write-protect": the file records the switch, the live
// card stops taking writes at once, the saved instance comes back protected.
TEST(test_card_write_protect) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    CHECK(f.ok(c::kSelectModule, module(1, "CE-161")));
    CHECK(f.part().modules().canNameAndSave(1));
    CHECK(f.part().modules().canWriteProtect(1));
    sharp::LoaderMemory* m = sharp::firstOf<sharp::LoaderMemory>(*f.part().machine());
    CHECK(m != nullptr);
    if (!m) return;
    const std::uint8_t b55 = 0x55, bAA = 0xAA;
    CHECK(m->writeBuiltIn(0x0000, &b55, 1).stored);
    CHECK(m->peek(0x0000) == 0x55);

    std::string error;
    CHECK(f.part().modules().nameAndSave(1, "Rom", &error, /*writeProtect=*/true));
    CHECK(!f.part().modules().canWriteProtect(1));
    const ExpansionCard* card = f.part().slotCard(1);
    CHECK(card && card->writeProtected());
    CHECK(!m->writeBuiltIn(0x0000, &bAA, 1).stored);   // the switch blocks it
    CHECK(m->peek(0x0000) == 0x55);
    std::ifstream in(fs::path(f.paths->instanceDir()) / "Rom.card.yaml");
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(text.find("write-protect: protected") != std::string::npos);

    CHECK(f.ok(c::kSelectModule, module(1, "")));
    CHECK(f.ok(c::kSelectModule, module(1, "Rom")));
    m = sharp::firstOf<sharp::LoaderMemory>(*f.part().machine());
    card = f.part().slotCard(1);
    CHECK(m && card && card->writeProtected());
    if (m) CHECK(m->peek(0x0000) == 0x55);

    CHECK(f.ok(c::kSelectModule, module(1, "CE-1638")));   // battery-backed, no switch
    CHECK(!f.part().modules().canWriteProtect(1));
    CHECK(!f.part().modules().nameAndSave(1, "Locked", &error, true));
    CHECK(error == "\"CE-1638\" has no write-protect switch.");
}

TEST(test_card_yaml_dump) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    std::vector<std::string> dump = shared::cardYamlDump(f.part());
    CHECK(dump.size() == 1 && dump[0].find("no card attached") != std::string::npos);
    CHECK(f.ok(c::kSelectModule, module(2, "CE-1600M")));
    dump = shared::cardYamlDump(f.part());
    CHECK(dump.size() == 2 && dump[0].find("CE-1600M (Slot 2), unbanked") != std::string::npos);
}

// pick-model: a fresh machine of the model, then its default preset;
// default-preset is done with none set.
TEST(test_pick_model) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.ok(c::kDefaultPreset));   // none set
    f.settings.setDefaultPresetPath(Model::PC1600, "tests/fixtures/presets/startup-pc1600.pc1600");
    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1600"}})));
    CHECK(f.part().currentModel() == Model::PC1600);
    CHECK(f.part().ce1600pAttached());   // the preset's plotter
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")));
    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1600"}})));   // re-picking does nothing
    CHECK(!f.part().ce1600pAttached());
}

TEST(test_example_ref_default_preset) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    f.settings.setDefaultPresetPath(Model::PC1500A, "examples:startup/default-pc1500a.pc1500a");
    CHECK(f.ok(c::kDefaultPreset));
    CHECK(f.part().ce150Attached());
    // Another model's default is refused by its name, not its path.
    const sessions::ReplyMessage wrong = f.run(
        c::kLoadDefaultPreset, Value::object({{"path", "examples:startup/default-pc1600.pc1600"}, {"model", "PC-1500A"}}));
    CHECK(!wrong.done && wrong.reason.find("\"CE-1600P + modules\"") != std::string::npos);
}

// LIB-2 AC-34 (design §6 P11): the real-time clocks are set from the
// session's host-time source when the machine is built and at the end of
// every operation.
TEST(lib2_ac34_set_at_build_and_after_operation) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    std::tm t{};
    t.tm_year = 2031 - 1900;
    t.tm_mon = 2;
    t.tm_mday = 14;
    t.tm_hour = 15;
    t.tm_min = 9;
    t.tm_sec = 26;
    t.tm_isdst = -1;
    const auto fixed = std::chrono::system_clock::from_time_t(std::mktime(&t));
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        SessionFixture f;
        f.fixedHostTime = true;
        f.fixedTime = fixed;
        CHECK(f.start(model));
        auto read = [&] {
            auto* clock = sharp::firstOf<calcu::peripherals::RealTimeClock>(*f.part().machine());
            return clock ? clock->read() : calcu::peripherals::ClockFields{};
        };
        auto isFixed = [](const calcu::peripherals::ClockFields& cf) {
            return cf.month == 3 && cf.day == 14 && cf.hour == 15 && cf.minute == 9 && cf.second == 26;
        };
        CHECK(isFixed(read()));
        f.runFor(3.0);   // emulated time: the clock runs on it
        CHECK(!isFixed(read()));
        CHECK(f.ok(c::kReset, Value::object({{"all", false}})));   // an operation: set again at its end
        CHECK(isFixed(read()));
    }
}

} // namespace
