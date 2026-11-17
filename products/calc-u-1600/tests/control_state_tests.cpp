// LIB-4 Calc-U-1600 (M-49; design Part D, D.6): the control state of the
// product's session, from the library's ControlPart with app/SharpControlRules
// and the catalogue's one pin-out per 60-pin place (D-11). The drawing of it
// is LIB-14 1600's (CalcU1600QtTests, shell.spec.js).

#include <algorithm>
#include <string>
#include <vector>

#include "../app/tests/SessionFixture.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "TestHarness.hpp"
#include "app/SharpControlRules.hpp"
#include "control/ControlState.hpp"
#include "machine/Catalogue.hpp"
#include "sessions/Value.hpp"

namespace {

using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
namespace control = calcu::control;
namespace machine = calcu::machine;
namespace sessions = calcu::sessions;
using sessions::Value;

const control::ControlState* stateOf(SessionFixture& f) {
    return f.state<control::ControlState>(control::kControlStateTopic);
}

machine::PlacePath path(const char* place) { return machine::PlacePath{{machine::PlaceName{place}}}; }

std::vector<std::string> fitting(const control::ControlState& s, const char* place) {
    std::vector<std::string> out;
    if (const control::PlaceEntry* p = s.place(path(place)))
        for (const auto& c : p->fitting) out.push_back(c.id.value);
    return out;
}

std::vector<std::string> ids(const std::vector<control::VariantChoice>& v) {
    std::vector<std::string> out;
    for (const auto& c : v) out.push_back(c.id.value);
    return out;
}


// The product entry's Value (kept alive by the holder).
struct EntryValue {
    Value v;
    bool ok = false;
};
EntryValue product(const control::ControlState& s, const char* name) {
    const control::ProductEntry* e = s.productEntry(name);
    if (!e || !e->value) return {};
    return {e->value->toValue(), true};
}
bool flag(const EntryValue& e, const char* key) {
    const Value* v = e.ok ? e.v.find(key) : nullptr;
    return v && v->boolean() && *v->boolean();
}
std::string text(const EntryValue& e, const char* key) {
    const Value* v = e.ok ? e.v.find(key) : nullptr;
    return v && v->string() ? *v->string() : std::string();
}

bool plugged(const control::ControlState& s, const char* place, const char* peripheral) {
    const control::PlaceEntry* p = s.place(path(place));
    const control::PeripheralChoice* c = p ? control::choice(*p, machine::PeripheralId{peripheral}) : nullptr;
    return c && c->plugged;
}

} // namespace

// T16-1 (O-2, D-11): each 60-pin place lists exactly what fits it, from the
// catalogue alone; the bus-rom place keeps the chain.
TEST(lib4_ac17_each_60_pin_place_lists_only_what_fits) {
    const machine::Catalogue& c = sharp::sharpCatalogue();
    auto fits = [&](const char* model, const char* place) {
        std::vector<std::string> out;
        const auto pl = c.place(machine::ModelId{model}, machine::PlaceName{place});
        if (!pl) return out;
        for (const auto& p : c.peripherals())
            if (p.fitsPinOut(pl->pinOut->id)) out.push_back(p.id.value);
        return out;
    };
    using V = std::vector<std::string>;
    for (const char* m : {"PC-1500", "PC-1500A"}) {
        CHECK(fits(m, "printer") == (V{"CE-150"}));
        CHECK(fits(m, "interface") == (V{"CE-158"}));
        CHECK(fits(m, "bus-rom") == (V{"bus-rom"}));
    }
    CHECK(fits("PC-1600", "printer") == (V{"CE-150", "CE-1600P"}));
    CHECK(fits("PC-1600", "interface") == (V{"CE-158"}));
    CHECK(fits("PC-1600", "host-drive") == (V{"host-drive"}));
    CHECK(fits("PC-1600", "bus-rom") == (V{"bus-rom"}));
}

// T16-2, T16-3, T16-4: one session per model; the fields of today's control
// state as AC-17's table has them.
TEST(lib4_ac16_ac17_pc1500_models_variants_and_fields) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500"));
    const control::ControlState* s = stateOf(f);
    CHECK(s != nullptr);
    if (!s) return;
    // AC-16: the models, the running one's variants and which runs.
    std::vector<std::string> models;
    for (const auto& m : s->models) models.push_back(m.id.value);
    CHECK(models == (std::vector<std::string>{"PC-1500", "PC-1500A", "PC-1600"}));
    CHECK_EQ(s->model.value, std::string("PC-1500"));
    CHECK(ids(s->variants) == (std::vector<std::string>{"A01", "A03", "A04"}));
    CHECK(s->variant && s->variant->value == "A04");
    // slotCount: slot-1 only; CE-150 and CE-158 in their own places.
    CHECK(s->place(path("slot-1")) && !s->place(path("slot-2")) && !s->place(path("host-drive")));
    CHECK(fitting(*s, "printer") == (std::vector<std::string>{"CE-150"}));
    CHECK(fitting(*s, "interface") == (std::vector<std::string>{"CE-158"}));
    // No paper owner, no media places, no product panel, no disk controls.
    CHECK(s->panels.size() == 1 && s->panels[0].kind == "paper" && !s->panels[0].owner);
    CHECK(s->mediaPlaces.empty());
    CHECK(!flag(product(*s, cu1600::kCe158PrinterPanelEntry), "shown"));
    CHECK(!flag(product(*s, cu1600::kDiskControlsEntry), "shown"));
    CHECK(!product(*s, cu1600::kCe1600pNextVariantEntry).ok);
    // The CE-150: plugged, the paper panel's owner, its cassette listed.
    CHECK(f.attach("CE-150"));
    CHECK(f.attach("CE-158"));
    s = stateOf(f);
    CHECK(plugged(*s, "printer", "CE-150") && plugged(*s, "interface", "CE-158"));   // EC-1
    CHECK(s->panels.size() == 1 && s->panels[0].owner && s->panels[0].owner->value == "CE-150");
    CHECK(s->mediaPlaces.size() == 1 && s->mediaPlaces[0].id.value == "cassette-recorder" && s->mediaPlaces[0].owner &&
          s->mediaPlaces[0].owner->value == "CE-150");
    CHECK(flag(product(*s, cu1600::kCe158PrinterPanelEntry), "shown"));
    // AC-16 on the PC-1500A: its one variant.
    CHECK(f.model("PC-1500A"));
    s = stateOf(f);
    CHECK(ids(s->variants) == (std::vector<std::string>{"A04"}));
}

TEST(lib4_ac16_ac17_pc1600_fields_ce1600p_variants_and_next_variant) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const control::ControlState* s = stateOf(f);
    CHECK(s != nullptr);
    if (!s) return;
    CHECK(ids(s->variants) == (std::vector<std::string>{"New", "Old"}));
    CHECK(s->variant && s->variant->value == "New");
    CHECK(s->place(path("slot-1")) && s->place(path("slot-2")));
    CHECK(fitting(*s, "printer") == (std::vector<std::string>{"CE-150", "CE-1600P"}));
    CHECK(fitting(*s, "host-drive") == (std::vector<std::string>{"host-drive"}));
    // floppyShown on every PC-1600, usable only with the CE-1600P.
    auto disk = product(*s, cu1600::kDiskControlsEntry);
    CHECK(flag(disk, "shown") && !flag(disk, "available") && text(disk, "reason") == cu1600::kDiskInCe1600p);
    // The CE-1600P's variants in `printer`, and the one an attach uses.
    const control::PlaceEntry* printer = s->place(path("printer"));
    const control::PeripheralChoice* p = printer ? control::choice(*printer, machine::PeripheralId{"CE-1600P"}) : nullptr;
    CHECK(p && ids(p->variants) == (std::vector<std::string>{"New", "Old"}));
    CHECK(text(product(*s, cu1600::kCe1600pNextVariantEntry), "variant") == "New");
    CHECK(f.ok(cu1600::commands::kCE1600PRom, Value::object({{"version", "old"}})));
    s = stateOf(f);
    CHECK(text(product(*s, cu1600::kCe1600pNextVariantEntry), "variant") == "Old");
    // Plugged: the running variant, the disk controls usable, the floppy
    // and the cassette listed with their owner, the paper panel's owner.
    CHECK(f.attach("CE-1600P", "Old"));
    s = stateOf(f);
    printer = s->place(path("printer"));
    p = printer ? control::choice(*printer, machine::PeripheralId{"CE-1600P"}) : nullptr;
    CHECK(p && p->plugged && p->variant && p->variant->value == "Old");
    CHECK(!product(*s, cu1600::kCe1600pNextVariantEntry).ok);
    CHECK(flag(product(*s, cu1600::kDiskControlsEntry), "available"));
    std::vector<std::string> media;
    for (const auto& m : s->mediaPlaces) media.push_back(m.id.value + "@" + (m.owner ? m.owner->value : ""));
    CHECK(media == (std::vector<std::string>{"cassette-recorder@CE-1600P", "floppy@CE-1600P"}));
    CHECK(s->panels.size() == 1 && s->panels[0].owner && s->panels[0].owner->value == "CE-1600P");
    // The CE-158 beside the CE-1600P is refused by the machine (ledger 1600-22).
    const control::PlaceEntry* iface = s->place(path("interface"));
    const control::PeripheralChoice* ce158 = iface ? control::choice(*iface, machine::PeripheralId{"CE-158"}) : nullptr;
    CHECK(ce158 && !ce158->available.now && !ce158->available.reason.empty());
    // The host drive's folder (hostDriveDir) and File ▸ Unmount.
    CHECK(text(product(*s, cu1600::kHostDriveFolderEntry), "folder").empty());
    CHECK(!flag(product(*s, cu1600::kHostDriveFolderEntry), "available"));
    CHECK(flag(product(*s, cu1600::kMountDirectoryEntry), "available"));
    const std::string dir = (f.root / "drive").generic_string();
    std::filesystem::create_directories(f.root / "drive");
    CHECK(f.ok(cu1600::commands::kMountHostDrive, Value::object({{"dir", dir}})));
    s = stateOf(f);
    CHECK(plugged(*s, "host-drive", "host-drive"));
    CHECK_EQ(text(product(*s, cu1600::kHostDriveFolderEntry), "folder"), dir);
    CHECK(flag(product(*s, cu1600::kHostDriveFolderEntry), "available"));
}

// T16-4 (AC-18, D-12): two sessions on the same machine, one that cannot
// mount a folder: the place host-drive and its choices differ, nothing else.
TEST(lib4_ac18_host_drive_refused_where_no_folder_can_be_mounted) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture qt("calcu-control-qt"), web("calcu-control-web");
    web.hostDirectories = false;
    CHECK(qt.start("PC-1600"));
    CHECK(web.start("PC-1600"));
    const control::ControlState* a = stateOf(qt);
    const control::ControlState* b = stateOf(web);
    CHECK(a && b);
    if (!a || !b) return;
    const control::PlaceEntry* pa = a->place(path("host-drive"));
    const control::PlaceEntry* pb = b->place(path("host-drive"));
    CHECK(pa && pa->available.now);
    CHECK(pb && !pb->available.now && pb->available.reason == cu1600::kNoHostFolderHere);
    CHECK(pb && !pb->empty.now && pb->fitting.size() == 1 && !pb->fitting[0].available.now);
    // Everything else the same; the product entry of Mount Directory… follows.
    CHECK(a->models == b->models && a->variants == b->variants && a->mediaPlaces == b->mediaPlaces &&
          a->panels == b->panels && a->actions == b->actions);
    for (std::size_t i = 0; i < a->places.size() && i < b->places.size(); ++i)
        if (a->places[i].path != path("host-drive")) CHECK(a->places[i] == b->places[i]);
    CHECK(!flag(product(*b, cu1600::kMountDirectoryEntry), "available"));
}

// T16-6 (AC-20, AC-3): while a preset loads, the variants, the places'
// choices and the inserts are refused; the models, Reset and Cold boot not.
TEST(lib4_ac20_operation_refuses_model_variant_and_place_choices) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    CHECK(f.attach("CE-1600P"));
    const sessions::CallId id =
        f.send(calcu::presets::kLoadPresetCommand, Value::object({{"ref", "examples:basic/hanoi.pc1600"}}));
    f.call();
    CHECK(f.busy());
    const control::ControlState* s = stateOf(f);
    CHECK(s != nullptr);
    if (!s) return;
    const std::string busy = sessions::kAnotherOperationRuns;
    for (const auto& m : s->models) CHECK(m.available.now);
    for (const auto& v : s->variants) CHECK(!v.available.now && v.available.reason == busy);
    for (const auto& p : s->places) {
        CHECK(!p.empty.now);
        for (const auto& c : p.fitting) CHECK(!c.available.now && c.available.reason == busy);
    }
    for (const auto& m : s->mediaPlaces) CHECK(!m.insert.now);
    for (const char* a : {"reset", "cold-boot"}) CHECK(s->action(sessions::CommandName{a}) && s->action(sessions::CommandName{a})->available.now);
    CHECK(s->action(sessions::CommandName{"load-preset"}) && !s->action(sessions::CommandName{"load-preset"})->available.now);
    while (!f.takeReply(id)) f.call();
}
