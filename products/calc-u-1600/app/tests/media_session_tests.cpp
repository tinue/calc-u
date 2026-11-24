// The modules, the
// floppy disk and the tapes as media of the library's media part on the
// product's app session -- the pickers of the topic `media`, `media.choose`,
// `media.turn`, Name & Save, autosave -- against the real ROMs and the
// bundled cards and disks, in a throwaway bundle and save folder. SKIP
// without ROMs.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "Connector/CE1600FCard.hpp"
#include "Connector/ExpansionCard.hpp"
#include "Machine/SharpMediaPlaces.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "TestSettings.hpp"
#include "media/MediaPlace.hpp"

namespace {

namespace fs = std::filesystem;
namespace media = calcu::media;
using calcu::sessions::Value;
using sessionfixture::SessionFixture;

// The labels of a place's picker entries of one group.
std::vector<std::string> labels(SessionFixture& f, const std::string& place, media::PickerEntry::Group g) {
    std::vector<std::string> out;
    if (const media::PlaceState* p = f.mediaPlace(place))
        for (const auto& e : p->picker)
            if (e.group == g) out.push_back(e.label);
    return out;
}
bool has(const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); }

std::optional<std::string> tooltip(SessionFixture& f, const std::string& place, const std::string& label) {
    if (const media::PlaceState* p = f.mediaPlace(place))
        for (const auto& e : p->picker)
            if (e.label == label) return e.tooltip;
    return std::nullopt;
}

media::MediaPlace* placeCap(SessionFixture& f, const char* id) {
    calcu::machine::Machine* m = f.part().machine();
    if (!m) return nullptr;
    for (const auto& e : m->capabilities(media::MediaPlace::kKind))
        if (auto* p = calcu::machine::capabilityAs<media::MediaPlace>(*m, e.id); p && p->id().value == id) return p;
    return nullptr;
}

std::vector<std::uint8_t> diskImage(SessionFixture& f) {
    media::MediaPlace* p = placeCap(f, "floppy");
    const auto c = p ? std::dynamic_pointer_cast<const sharp::FloppyContent>(p->content()) : nullptr;
    return c ? c->image : std::vector<std::uint8_t>{};
}

// The drive writes: the disk's content changes in the drive.
void driveWrites(SessionFixture& f, std::size_t at) {
    media::MediaPlace* p = placeCap(f, "floppy");
    std::vector<std::uint8_t> image = diskImage(f);
    if (!p || image.empty()) return;
    image[at] ^= 0xFF;
    const int side = p->side().value_or(0);
    p->insert(std::make_shared<sharp::FloppyContent>(image), side);
}

std::string fileText(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

using G = media::PickerEntry::Group;

// ── The module bay ──────────────────────────────────────────────────────

TEST(module_bay_on_the_app_session) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(has(labels(f, "slot-1", G::BundledTemplate), "CE-1600M"));
    CHECK(tooltip(f, "slot-1", "CE-1600M") == std::optional<std::string>("32 KB battery-backed RAM"));
    CHECK(!has(labels(f, "slot-1", G::BundledTemplate), "CE-1601M"));   // Slot 2 only
    CHECK(labels(f, "slot-1", G::Instance).empty());

    CHECK(f.choose("slot-1", "CE-1600M").done);
    CHECK(f.notices().empty());
    CHECK(f.part().slotModuleName(1) == "CE-1600M");
    CHECK(f.mediaPlace("slot-1") && f.mediaPlace("slot-1")->nameAndSave);
    CHECK(f.held("slot-1") && !f.held("slot-1")->instance());

    CHECK(!f.nameAndSave("slot-1", "CE-1600M").done);   // a bundled template's name
    CHECK(f.nameAndSave("slot-1", "My RAM").done);
    CHECK(fs::exists(fs::path(f.saveFolder()) / "My RAM.module.yaml"));
    CHECK(f.held("slot-1") && f.held("slot-1")->name == "My RAM" && f.held("slot-1")->instance());
    CHECK(f.mediaPlace("slot-1") && !f.mediaPlace("slot-1")->nameAndSave);   // autosaved now
    CHECK(has(labels(f, "slot-1", G::Instance), "My RAM"));
    CHECK(!f.nameAndSave("slot-1", "Other").done);

    // A variant switch carries the instance over, with its content.
    CHECK(f.variant("Old"));
    CHECK(f.part().slotModuleName(1) == "CE-1600M");
    CHECK(f.held("slot-1") && f.held("slot-1")->name == "My RAM");
    // A model switch clears the slots.
    CHECK(f.model("PC-1500A"));
    CHECK(f.held("slot-1") == nullptr);
}

// ── Module slots ────────────────────────────────────────────────────────

TEST(picker_lists_what_fits_the_slot_in_order) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    const media::PlaceState* p = f.mediaPlace("slot-2");
    CHECK(p != nullptr);
    if (!p) return;
    // The empty slot; the bundled templates by name; the instances; the
    // modules that are not media (no battery), each only where it fits.
    CHECK(!p->picker.empty() && p->picker.front().group == G::Empty);
    std::vector<std::string> bundled = labels(f, "slot-2", G::BundledTemplate);
    CHECK(has(bundled, "CE-1601M") && has(bundled, "superRAM r4") && has(bundled, "256K custom") && has(bundled, "CE-1600M"));
    CHECK(std::is_sorted(bundled.begin(), bundled.end(), [](const std::string& a, const std::string& b) {
        std::string x = a, y = b;
        for (auto& c : x) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (auto& c : y) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return x < y;
    }));
    CHECK(!has(labels(f, "slot-2", G::Module), "CE-151"));   // slot 1 only
    const std::vector<std::string> slot1Modules = labels(f, "slot-1", G::Module);
    CHECK(has(slot1Modules, "CE-151") && has(slot1Modules, "CE-155"));
    CHECK(!has(labels(f, "slot-1", G::BundledTemplate), "superRAM r4"));
    // Each label once (a module of the catalogue is its bundled template).
    for (const std::string& l : bundled) CHECK(std::count(bundled.begin(), bundled.end(), l) == 1);
    // Groups in order.
    int last = -1;
    for (const auto& e : p->picker) {
        CHECK(static_cast<int>(e.group) >= last);
        last = static_cast<int>(e.group);
    }
}

// The bundled superRAM r4 says, once it is plugged, that it needs its
// initialisation program.
TEST(superram_r4_template_shows_its_insert_notice) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    f.notices();
    CHECK(f.choose("slot-2", "superRAM r4").done);
    const std::vector<std::string> n = f.notices();
    CHECK(n.size() == 1 && n[0].find("SRAMINIT.BIN") != std::string::npos);
}

TEST(choose_is_replace_power_cycle_memory_kept) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.choose("slot-1", "CE-1600M").done);
    f.runFor(0.5);
    const auto ticks = f.part().machine()->tickCount();
    CHECK(f.choose("slot-1", "CE-161").done);   // a replace
    CHECK(f.part().slotModuleName(1) == "CE-161");
    CHECK(f.part().machine()->tickCount() > ticks);   // its power cycle ran
    f.runFor(0.5);
    // The memory is kept through the power cycle: the ROM finds it changed
    // and asks NEW0?, as on the real device.
    CHECK(f.screen().find("NEW0?") != std::string::npos);
}

TEST(other_slot_and_disk_untouched) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("floppy", "Formatted").done);
    CHECK(f.choose("slot-2", "CE-1601M").done);
    CHECK(f.choose("slot-1", "CE-1600M").done);
    driveWrites(f, 5000);
    const std::vector<std::uint8_t> disk = diskImage(f);
    int banks = 0;
    const std::vector<std::uint8_t> before = f.part().debugSlotCardImage(2, &banks);
    CHECK(f.choose("slot-1", "CE-161").done);
    // The same card, not read again: its RAM as it was.
    CHECK(f.part().slotModuleName(2) == "CE-1601M");
    CHECK(!before.empty() && f.part().debugSlotCardImage(2, &banks) == before);
    CHECK(diskImage(f) == disk && f.held("floppy") && f.held("floppy")->name == "Formatted");
    CHECK(f.held("floppy") && f.held("floppy")->unsaved);   // what the template held is not lost
}

// ── The floppy bay ──────────────────────────────────────────────────────

TEST(floppy_bay_on_the_app_session) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.mediaPlace("floppy") == nullptr);   // no drive: no place
    CHECK(f.toggle("CE-1600P"));
    CHECK(has(labels(f, "floppy", G::BundledTemplate), "Formatted"));
    CHECK(tooltip(f, "floppy", "Formatted") == std::optional<std::string>("Formatted, both sides empty"));
    CHECK(f.choose("floppy", "Formatted").done);
    CHECK(!diskImage(f).empty());
    CHECK(f.mediaPlace("floppy") && f.mediaPlace("floppy")->nameAndSave);

    CHECK(f.nameAndSave("floppy", "Work").done);
    const fs::path saved = fs::path(f.saveFolder()) / "Work.floppy.yaml";
    CHECK(fs::exists(saved));
    CHECK(f.mediaPlace("floppy") && !f.mediaPlace("floppy")->nameAndSave);
    CHECK(fileText(saved).find("description: \"Formatted, both sides empty\"") != std::string::npos);   // kept

    // The drive writes: autosaved within half a second, not before.
    fs::remove(saved);
    driveWrites(f, 100);
    for (int i = 0; i < 15; ++i) f.call(1000.0 / 60);   // a quarter of a second at 60 Hz
    CHECK(!fs::exists(saved));
    for (int i = 0; i < 20 && !fs::exists(saved); ++i) f.call(1000.0 / 60);
    CHECK(fs::exists(saved));
    CHECK(fileText(saved).find("description: \"Formatted, both sides empty\"") != std::string::npos);   // and by the autosave

    // A detach of the CE-1600P saves the disk first, without waiting.
    fs::remove(saved);
    driveWrites(f, 100);
    CHECK(f.toggle("CE-1600P"));
    CHECK(!f.part().ce1600pAttached());
    CHECK(fs::exists(saved));
}

TEST(turn_is_eject_and_insert_other_side) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("floppy", "Formatted").done);
    const std::vector<std::uint8_t> disk = diskImage(f);
    CHECK(f.mediaPlace("floppy") && f.mediaPlace("floppy")->holds && f.mediaPlace("floppy")->holds->side == 0);
    CHECK(f.run(media::kTurnCommand, Value::object({{"place", "floppy"}})).done);
    CHECK(f.mediaPlace("floppy") && f.mediaPlace("floppy")->holds && f.mediaPlace("floppy")->holds->side == 1);
    CHECK(diskImage(f) == disk);   // the same disk, the other side up
    media::MediaPlace* p = placeCap(f, "floppy");
    CHECK(p && p->side() == 1);
    CHECK(f.run(media::kTurnCommand, Value::object({{"place", "floppy"}})).done);
    CHECK(p && p->side() == 0);
}

// The lamp: green while the drive's motor runs -- `DSKF"X:"` spins it.
TEST(lamp_while_motor_runs) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("floppy", "Formatted").done);
    auto lamp = [&] { return f.state<cu1600::LampsPayload>(cu1600::topics::kLamps)->floppyMotor; };
    f.call();
    CHECK(!lamp());
    f.typeLine("A=DSKF\"X:\"");
    bool seen = false;
    for (int i = 0; i < 60 * 20 && !seen; ++i) {
        f.call();
        seen = lamp();
    }
    CHECK(seen);
    CHECK(f.part().floppyMotorOn() == lamp());
}

TEST(name_and_save_for_a_template_disk) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    CHECK(f.choose("floppy", "Blank").done);
    driveWrites(f, 7);
    f.call();
    CHECK(f.held("floppy") && f.held("floppy")->unsaved);
    CHECK(!fs::exists(fs::path(f.saveFolder()) / "Blank.floppy.yaml"));   // a template is never written
    CHECK(f.nameAndSave("floppy", "Mine").done);
    CHECK(f.held("floppy") && f.held("floppy")->instance() && !f.held("floppy")->unsaved);
    CHECK(has(labels(f, "floppy", G::Instance), "Mine"));
}

// ── Name & Save ─────────────────────────────────────────────────────────

TEST(instance_name_replaced_after_asking) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.choose("slot-1", "CE-1600M").done);
    CHECK(f.nameAndSave("slot-1", "Work").done);
    CHECK(f.choose("slot-2", "CE-1601M").done);
    // "Work" is a module of the save folder: asked before it is replaced.
    CHECK(f.choose("slot-1", "").done);
    const auto asked = f.nameAndSave("slot-2", "work");
    CHECK(!asked.done && asked.reason.rfind("asks:", 0) == 0);
    CHECK(f.nameAndSave("slot-2", "work", false, /*confirmed=*/true).done);
    CHECK(f.held("slot-2") && f.held("slot-2")->name == "work");
    CHECK(fs::exists(fs::path(f.saveFolder()) / "Work.module.yaml"));   // its file kept
    CHECK(!fs::exists(fs::path(f.saveFolder()) / "work.module.yaml") || fs::equivalent(fs::path(f.saveFolder()) / "work.module.yaml",
                                                                             fs::path(f.saveFolder()) / "Work.module.yaml"));
    // An instance in another place now: refused.
    CHECK(f.choose("slot-1", "CE-1600M").done);
    CHECK(!f.nameAndSave("slot-1", "work", false, true).done);
}

TEST(name_with_slash_or_colon_refused) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.choose("slot-1", "CE-1600M").done);
    for (const char* name : {"a/b", "a:b", "  ", "a\\b"}) CHECK(!f.nameAndSave("slot-1", name).done);
    CHECK(f.nameAndSave("slot-1", "  My   card ").done);   // trimmed, runs of spaces as one
    CHECK(f.held("slot-1") && f.held("slot-1")->name == "My card");
}

} // namespace

// The cassette's counter as every GUI shows it.
TEST(tape_counter_text) {
    cu1600::LampsPayload l;
    CHECK(l.tapeEmpty());
    CHECK_EQ(l.tapeCounter(), std::string("0:00"));
    CHECK_EQ(l.tapeTip(), std::string("No cassette"));
    l.tapeName = "hanoi";
    l.tape.mode = TapeDeck::Mode::Play;
    l.tape.position = 75;
    CHECK_EQ(l.tapeCounter(), std::string("▶ 1:15"));
    CHECK_EQ(l.tapeTip(), std::string("Playing hanoi"));
    l.tape.mode = TapeDeck::Mode::Record;
    l.tape.length = 9;
    CHECK_EQ(l.tapeCounter(), std::string("● 0:09"));
    CHECK_EQ(l.tapeTip(), std::string("Recording onto hanoi"));
}

// CLOAD with no cassette in: the topic `media` shows the cassette place
// waiting for a medium (the machine's tape loader waits for an edge), as
// Calc-U-59's shows its card reader; BREAK ends the wait.
namespace {

bool cassetteWaits(SessionFixture& f) {
    const auto* m = f.state<media::MediaState>(media::kMediaTopic);
    const media::PlaceState* p = m ? m->place("cassette") : nullptr;
    return p && p->waits;
}

void cloadWaitsUntilBreak(SessionFixture& f) {
    f.runFor(1);
    CHECK(!cassetteWaits(f));
    f.typeLine("CLOAD");
    CHECK(f.runUntilTyped(10));
    f.runFor(1);
    CHECK(cassetteWaits(f));
    CHECK(f.mediaPlace("cassette") && f.mediaPlace("cassette")->waits);
    f.tap("on");   // BREAK
    f.runFor(1);
    CHECK(!cassetteWaits(f));
}

} // namespace

TEST(cassette_waits_while_cload_awaits_the_tape_pc1500) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1500A"));
    CHECK(f.attach("CE-150"));
    f.runFor(2);
    f.tap("cl");   // the memory changed: NEW0? :CHECK
    f.runFor(0.5);
    f.typeLine("NEW0");
    CHECK(f.runUntilTyped(10));
    cloadWaitsUntilBreak(f);
}

TEST(cassette_waits_while_cload_awaits_the_tape_pc1600) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    SessionFixture f("calcu-media-session-test");
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    f.runFor(2);
    cloadWaitsUntilBreak(f);
}
