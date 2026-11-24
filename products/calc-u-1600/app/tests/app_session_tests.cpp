// Calc-U-1600's app layer (SharpAppPart) on the library's app session:
// the Sharp operations as
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
#include "app/ModuleYamlDump.hpp"
#include "app/SharpMediaKinds.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include <variant>
#include "TestHarness.hpp"
#include "presets/PresetsPart.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = cu1600::commands;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;
using Model = calcu::machine::ModelId;

// The module in slot 1 has its write-protect switch on (the controls of
// the peripheral there).
bool slot1Protected(cu1600::SharpAppPart& part) {
    calcu::machine::Machine* m = part.machine();
    if (!m) return false;
    for (const auto& e : m->capabilities(calcu::peripherals::Controls::kKind)) {
        const auto* o = std::get_if<calcu::machine::PeripheralOwner>(&e.owner);
        auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(*m, e.id);
        if (!o || !c || o->place.steps.empty() || o->place.steps[0].value != "slot-1") continue;
        for (const auto& i : c->list())
            if (i.id.value == "write-protect") return i.on;
    }
    return false;
}

Value toggle(const char* p) { return Value::object({{"peripheral", p}}); }
Value preset(const char* path) { return Value::object({{"ref", path}}); }

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
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1500A"});
    f.send(calcu::sessions::kLogLevelCommand, calcu::sessions::Value::object({{"level", "info"}}));   // the step log
    f.messages.clear();
    f.readCost = std::chrono::milliseconds(1);   // the host's work: the allowance of a call runs out

    // A PC-1600 preset (two slot cards, typed commands, a typed program),
    // a host call at a time (`load-preset`).
    const sessions::CallId id = f.send(calcu::presets::kLoadPresetCommand, preset("tests/fixtures/presets/maxed-out-mem.pc1600"));
    CHECK(f.busy());
    // A second operation is refused while one runs.
    const sessions::CallId second = f.send(calcu::presets::kLoadPresetCommand, preset("tests/fixtures/presets/hanoi.pc1600"));
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
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1600"});
    CHECK(f.held("slot-1") && f.held("slot-1")->name == "CE-1600M");
    CHECK(f.held("slot-2") && f.held("slot-2")->name == "CE-1601M");
    // The last item (FILES) is not waited for: until idle.
    CHECK(f.ok(calcu::presets::kWaitIdleCommand));
    CHECK(f.screen().find("LOOP") != std::string::npos);
}

TEST(test_operations_and_selection) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start());
    CHECK(f.model("PC-1600"));
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1600"});
    CHECK(cu1600::sharpModel(f.settings.file().text("startup", "last-model")) == calcu::machine::ModelId{"PC-1600"});

    CHECK(f.toggle("CE-1600P") && f.part().ce1600pAttached());
    CHECK(f.toggle("CE-1600P") && !f.part().ce1600pAttached());
    CHECK(f.notices().empty());

    // A preset that cannot be read: refused, and a notice.
    sessions::ReplyMessage r = f.run(calcu::presets::kLoadPresetCommand, preset("tests/fixtures/presets/no-such.pc1600"));
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1600"});
    CHECK(!r.done && !r.reason.empty());
    const auto n = f.notices();
    CHECK(!n.empty() && n.back() == r.reason);
}

// The CE-150 on the PC-1600: its cassette is offered there (MODE 1).
TEST(test_cassette_for_ce150_on_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-150"));
    CHECK(f.part().tapeInterfaceAttached());
    CHECK(f.mediaPlace("cassette") != nullptr);
}

// The tape counter in the machine topic is what the deck had at the last
// call -- and an operation ends with one, so it is current right after.
// `media.start-new` on the blank tape: a new tape under a name.
TEST(test_tape_status_follows_calls) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    CHECK(f.toggle("CE-150"));
    CHECK(f.part().tapeInterfaceAttached());
    auto tapes = [&] { return f.state<cu1600::LampsPayload>(cu1600::topics::kLamps)->tape.mode; };
    CHECK(tapes() == TapeDeck::Mode::Empty);
    const sessions::CallId id = f.send(calcu::media::kStartNewCommand,
                                       Value::object({{"place", "cassette"}, {"template", f.entry("cassette", "Blank")}, {"name", "side a"}}));
    const auto r = f.takeReply(id);
    CHECK(r && r->done);
    CHECK(tapes() == TapeDeck::Mode::Empty);   // not seen yet
    f.call();
    CHECK(tapes() == TapeDeck::Mode::Record);
    CHECK(f.held("cassette") && f.held("cassette")->name == "side a");
    CHECK(f.run(calcu::media::kEjectCommand, Value::object({{"place", "cassette"}})).done);
    CHECK(f.reset());
    CHECK(tapes() == TapeDeck::Mode::Empty);
    // Nothing was recorded: no file (the file comes at the first write).
    CHECK(!fs::exists(fs::path(f.cassetteFolder()) / "side a.cassette.yaml"));
}

std::vector<std::uint8_t> readBytes(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

const calcu::media::KindState::Item* findItem(const calcu::media::MediaState& s, const std::string& name) {
    for (const auto& k : s.kinds)
        for (const auto* list : {&k.templates, &k.instances})
            for (const auto& item : *list)
                if (item.name == name) return &item;
    return nullptr;
}

const calcu::media::KindDecl& kindOf(SessionFixture& f, const char* id) { return *f.part().mediaPart().kind(id); }

// The saved media: the save folder's modules, disks and cassettes -- listed
// with the place they are in, exported, deleted after asking (not while in
// a place), imported back.
TEST(test_media_library) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    calcu::media::MediaPart& media = f.part().mediaPart();
    sessions::SessionContext& ctx = f.session().context();
    CHECK(findItem(*f.media(), "Work") == nullptr);

    CHECK(f.choose("slot-1", "CE-1600M").done);
    CHECK(f.nameAndSave("slot-1", "Work").done);
    auto state = f.media();
    const calcu::media::KindState::Item* work = findItem(*state, "Work");
    CHECK(work && !work->isTemplate && work->inPlace && *work->inPlace == "slot-1");
    CHECK(work && work->fileName == "Work.module.yaml");
    std::string reason;
    const auto files = media.exportFiles(kindOf(f, cu1600::kModuleKind), "Work", &reason);
    CHECK(files && files->size() == 1 && !files->front().bytes.empty());
    if (!files) return;
    const std::string bytes = files->front().bytes;
    CHECK(!media.remove(ctx, kindOf(f, cu1600::kModuleKind), "Work", true).ok);   // in slot 1

    CHECK(f.choose("slot-1", "").done);
    CHECK(media.remove(ctx, kindOf(f, cu1600::kModuleKind), "Work", false).reason.rfind("asks:", 0) == 0);   // asks first
    CHECK(media.remove(ctx, kindOf(f, cu1600::kModuleKind), "Work", true).ok);
    CHECK(findItem(*f.media(), "Work") == nullptr);
    CHECK(media.importBytes(ctx, "Work.module.yaml", bytes, {}).ok);
    CHECK(findItem(*f.media(), "Work") != nullptr);
    CHECK(!media.importBytes(ctx, "Work.module.yaml", bytes, {}).ok);   // the name is taken
    CHECK(!media.importBytes(ctx, "x.module.yaml", "nope", {}).ok);
    const std::vector<std::uint8_t> bundled = readBytes("frontends/qt6/resources/cards/ce161.module.yaml");
    CHECK(!media.importBytes(ctx, "ce161.module.yaml", std::string(bundled.begin(), bundled.end()), {}).ok);   // a bundled name

    // A tape: its definition with its WAV; a WAV alone is no medium.
    const std::vector<std::uint8_t> wav = readBytes("tests/fixtures/tape/pc1500_tape.wav");
    const std::string sound(wav.begin(), wav.end());
    const auto companion = [&](const std::string& rel) -> std::optional<std::string> {
        return rel == "Side A.wav" ? std::optional<std::string>(sound) : std::nullopt;
    };
    const std::string def = "format: cassette\nformat-version: 1\ncassette-name: \"Side A\"\naudio: { file: \"Side A.wav\" }\n";
    CHECK(media.importBytes(ctx, "Side A.cassette.yaml", def, companion).ok);
    CHECK(!media.importBytes(ctx, "side a.cassette.yaml", def, companion).ok);   // one name, any case
    CHECK(!media.importBytes(ctx, "Broken.cassette.yaml", def, {}).ok);          // without its WAV
    state = f.media();
    const calcu::media::KindState::Item* tape = findItem(*state, "Side A");
    CHECK(tape && !tape->inPlace && fs::exists(fs::path(f.cassetteFolder()) / "Side A.wav"));
    CHECK(!media.importBytes(ctx, "Side A.wav", sound, {}).ok);
    CHECK(!media.importBytes(ctx, "notes.txt", "x", {}).ok);
}

// Name & Save with "write-protect": the file records the switch, the
// machine is power-cycled as when a module is plugged in, the live card
// stops taking writes at once, the
// saved instance comes back protected; a module without the switch refuses.
TEST(write_protect_power_cycle) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    CHECK(f.choose("slot-1", "CE-161").done);
    const calcu::media::PlaceState* p = f.mediaPlace("slot-1");
    CHECK(p && p->nameAndSave && p->askWriteProtect);
    sharp::LoaderMemory* m = calcu::machine::first<sharp::LoaderMemory>(*f.part().machine());
    CHECK(m != nullptr);
    if (!m) return;
    const std::uint8_t b55 = 0x55, bAA = 0xAA;
    CHECK(m->writeBuiltIn(0x0000, &b55, 1).stored);
    CHECK(m->peek(0x0000) == 0x55);

    CHECK(f.nameAndSave("slot-1", "Rom", /*protect=*/true).done);
    p = f.mediaPlace("slot-1");
    CHECK(p && !p->askWriteProtect && !p->nameAndSave);
    m = calcu::machine::first<sharp::LoaderMemory>(*f.part().machine());
    CHECK(m && slot1Protected(f.part()));
    if (!m) return;
    CHECK(!m->writeBuiltIn(0x0000, &bAA, 1).stored);   // the switch blocks it
    CHECK(m->peek(0x0000) == 0x55);
    std::ifstream in(fs::path(f.saveFolder()) / "Rom.module.yaml");
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(text.find("write-protect: protected") != std::string::npos);

    CHECK(f.choose("slot-1", "").done);
    CHECK(f.choose("slot-1", "Rom").done);
    m = calcu::machine::first<sharp::LoaderMemory>(*f.part().machine());
    CHECK(m && slot1Protected(f.part()));
    if (m) CHECK(m->peek(0x0000) == 0x55);

    CHECK(f.choose("slot-1", "CE-1638").done);   // battery-backed, no switch
    p = f.mediaPlace("slot-1");
    CHECK(p && !p->askWriteProtect);
    const sessions::ReplyMessage r = f.nameAndSave("slot-1", "Locked", true);
    CHECK(!r.done && r.reason.find("no write-protect switch") != std::string::npos);
}
TEST(test_module_yaml_dump) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    std::vector<std::string> dump = cu1600::moduleYamlDump(f.part());
    CHECK(dump.size() == 1 && dump[0] == "\xE2\x94\x80\xE2\x94\x80 Module dump: no module plugged in, or it has no dumpable RAM "
                                         "\xE2\x94\x80\xE2\x94\x80");
    CHECK(f.choose("slot-2", "CE-1600M").done);
    dump = cu1600::moduleYamlDump(f.part());
    CHECK(dump.size() == 2 && dump[0].find("Module dump: CE-1600M (Slot 2), unbanked") != std::string::npos);
}

// `model`: a fresh machine of the model, then its startup preset; the same
// model again does nothing.
TEST(test_pick_model) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    f.settings.file().set("preset", "PC-1500", "examples:startup/default-pc1500.pc1500");
    CHECK(f.model("PC-1500"));
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1500"});
    CHECK(f.part().ce150Attached());   // the startup preset's plotter
    CHECK(f.toggle("CE-150"));
    CHECK(f.model("PC-1500"));   // re-picking does nothing
    CHECK(!f.part().ce150Attached());
}

// A startup preset by its bundle path; one of another model gives the
// built-in default and the notice.
TEST(test_example_ref_default_preset) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    f.settings.file().set("preset", "PC-1500A", "examples:startup/default-pc1500a.pc1500a");
    f.settings.file().set("preset", "PC-1500", "examples:basic/hanoi.pc1600");
    CHECK(f.start("PC-1500A"));
    CHECK(f.part().ce150Attached());
    CHECK(f.model("PC-1500"));
    CHECK(f.part().model() == calcu::machine::ModelId{"PC-1500"} && !f.part().ce150Attached());
    const auto n = f.notices();
    CHECK(!n.empty() && n.back().find("could not be loaded") != std::string::npos);
}

// The real-time clocks are set from the
// session's host-time source when the machine is built and at the end of
// every operation.
TEST(set_at_build_and_after_operation) {
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
            auto* clock = calcu::machine::first<calcu::peripherals::RealTimeClock>(*f.part().machine());
            return clock ? clock->read() : calcu::peripherals::ClockFields{};
        };
        auto isFixed = [](const calcu::peripherals::ClockFields& cf) {
            return cf.month == 3 && cf.day == 14 && cf.hour == 15 && cf.minute == 9 && cf.second == 26;
        };
        CHECK(isFixed(read()));
        f.runFor(3.0);   // emulated time: the clock runs on it
        CHECK(!isFixed(read()));
        CHECK(f.reset());   // an operation: set again at its end
        CHECK(isFixed(read()));
    }
}

} // namespace

// What fails while the part is made goes to the app log once the session
// is there: a module definition that does not read, the host drive's ROM
// where the host offers the drive. The session starts all the same.
TEST(start_problems_in_the_app_log) {
    SessionFixture f;
    const fs::path bundle = f.root / "bundle";
    std::ofstream(bundle / "broken.module.yaml") << "format-version: 1\nmodule-name: BROKEN\nbogus-key: 1\n";
    fs::remove(bundle / "PC1600-P1-B7-HOSTDRIVE.bin");
    f.open();
    CHECK(f.appLog.has(calcu::applog::AppLogLevel::Warn, "broken.module.yaml: "));
    CHECK(f.appLog.has(calcu::applog::AppLogLevel::Warn, "The host drive is off: its ROM cannot be read: "));
}

// A browser offers no host drive: its missing ROM is no problem there.
TEST(no_host_drive_no_rom_warning) {
    SessionFixture f;
    f.hostDirectories = false;
    fs::remove(f.root / "bundle" / "PC1600-P1-B7-HOSTDRIVE.bin");
    f.open();
    CHECK(!f.appLog.has(calcu::applog::AppLogLevel::Warn, "host drive"));
}
