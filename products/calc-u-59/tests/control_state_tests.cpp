// The control state of the app session as the frontends draw it, on the
// real ROMs.
#include "../app/HarnessProduct.hpp"
#include "AppRig.hpp"
#include "TestHarness.hpp"
#include "control/ControlState.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <string>
#include <vector>

using apprig::Rig;
namespace c = calcu::control;
namespace m = calcu::machine;
using calcu::sessions::Value;

namespace {

bool roms() { return std::filesystem::exists("roms/TMC0571B.txt"); }
const char* const kSkip = "the ROMs are not fetched (roms/fetch_roms.sh)";

const c::ControlState* state(Rig& r) { return r.fe.state<c::ControlState>(c::kControlStateTopic); }

const c::PlaceEntry* place(Rig& r, const char* name) {
    const auto* cs = state(r);
    return cs ? cs->place(m::PlacePath{{m::PlaceName{name}}}) : nullptr;
}

std::vector<std::string> fittingIds(const c::PlaceEntry& p) {
    std::vector<std::string> out;
    for (const auto& f : p.fitting) out.push_back(f.id.value);
    return out;
}

} // namespace

TEST(models_ti59_ti58_ti58c_without_variants) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r;
    const auto* cs = state(r);
    CHECK(cs != nullptr);
    if (!cs) return;
    std::vector<std::string> models;
    for (const auto& e : cs->models) models.push_back(e.id.value);
    CHECK(models == (std::vector<std::string>{"TI-59", "TI-58", "TI-58C"}));
    CHECK_EQ(cs->model.value, std::string("TI-59"));
    CHECK(cs->variants.empty());
    CHECK(!cs->variant.has_value());
    for (const auto& p : cs->places)
        for (const auto& f : p.fitting) CHECK(f.variants.empty());
}

TEST(slot_1_lists_14_modules_and_the_empty_slot) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(model);
        const auto* slot = place(r, "slot-1");
        CHECK(slot != nullptr);
        if (!slot) continue;
        CHECK_EQ(slot->pinOut.value, std::string("module-slot"));
        CHECK_EQ(slot->fitting.size(), std::size_t{14});
        CHECK(slot->empty.now);   // the empty slot can be chosen
        const auto* ml = c::choice(*slot, m::PeripheralId{"ML"});
        CHECK(ml && ml->plugged);   // the start: the Master Library
    }
}

TEST(printer_lists_pc100c_and_the_empty_place) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r;
    const auto* printer = place(r, "printer");
    CHECK(printer != nullptr);
    if (!printer) return;
    CHECK(fittingIds(*printer) == (std::vector<std::string>{"PC-100C"}));
    CHECK(!printer->fitting[0].plugged);   // no printer at the start
    CHECK(r.send(calcu::startup::kAttachCommand, apprig::attach("printer", "PC-100C")).done);
    CHECK(c::choice(*place(r, "printer"), m::PeripheralId{"PC-100C"})->plugged);
    CHECK(r.send(calcu::startup::kDetachCommand, apprig::detach("printer")).done);
    CHECK(!c::choice(*place(r, "printer"), m::PeripheralId{"PC-100C"})->plugged);
}

TEST(choices_refused_while_a_preset_loads) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r;
    // A preset that keeps the session busy: a program that runs for a while.
    const auto id = r.session().send(
        r.me, calcu::sessions::Command{calcu::presets::kLoadPresetCommand,
                                       Value::object({{"content", "format-version: 2\nmodel: TI-59\nkeys:\n  - wait: 3s\n"}, {"name", "busy.ti59"}})});
    CHECK(r.until([&] { return r.session().context().busy(); }, std::chrono::seconds(5)));
    r.runFor(std::chrono::milliseconds(100));
    const auto* slot = place(r, "slot-1");
    CHECK(slot && !slot->empty.now);   // the empty slot as a choice
    const auto* other = slot ? c::choice(*slot, m::PeripheralId{"RE"}) : nullptr;
    CHECK(other && !other->available.now);
    CHECK(other && !other->available.reason.empty());
    const auto* printer = place(r, "printer");
    CHECK(printer && !printer->fitting[0].available.now);
    r.until([&] { return r.fe.reply(id) != nullptr; });
    r.settle();
    CHECK(place(r, "slot-1")->empty.now);
    CHECK(c::choice(*place(r, "slot-1"), m::PeripheralId{"RE"})->available.now);
}

TEST(card_reader_on_the_ti59_only) {
    SKIP_UNLESS(roms(), kSkip);
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        Rig r(model);
        const auto* cs = state(r);
        CHECK(cs != nullptr);
        if (!cs) continue;
        if (std::string(model) == "TI-59") {
            CHECK_EQ(cs->mediaPlaces.size(), std::size_t{1});
            if (cs->mediaPlaces.empty()) continue;
            const auto& reader = cs->mediaPlaces[0];
            CHECK_EQ(reader.id.value, std::string("card-reader"));
            CHECK_EQ(reader.mediumKind, std::string("magnetic-card"));
            CHECK(reader.passage == m::MediaPassage::PassThrough);
            CHECK(reader.sides == (std::vector<int>{0, 1}));
            CHECK(reader.ownerPlace.steps.empty());   // the machine's own
        } else {
            CHECK(cs->mediaPlaces.empty());
        }
    }
}

TEST(paper_panel_owned_by_pc100c_or_none) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r;
    auto paper = [&]() -> const c::PanelEntry* {
        const auto* cs = state(r);
        if (!cs) return nullptr;
        for (const auto& p : cs->panels)
            if (p.kind == c::kPaperPanel) return &p;
        return nullptr;
    };
    CHECK(paper() != nullptr);
    CHECK(paper() && !paper()->owner);
    r.send(calcu::startup::kAttachCommand, apprig::attach("printer", "PC-100C"));
    CHECK(paper() && paper()->owner && paper()->owner->value == "PC-100C");
    r.send(calcu::startup::kModelCommand, Value::object({{"name", "TI-58"}}));
    CHECK(paper() != nullptr);   // always listed
    r.send(calcu::startup::kDetachCommand, apprig::detach("printer"));
    CHECK(paper() && !paper()->owner);
}

TEST(debug_panel_entries_carried) {
    SKIP_UNLESS(roms(), kSkip);
    Rig r;
    const auto* cs = state(r);
    CHECK(cs != nullptr);
    if (!cs) return;
    for (const char* name : {"c59.asm-run", "c59.calculator-tab"}) {
        const auto* e = cs->productEntry(name);
        CHECK(e != nullptr);
        CHECK(e && e->value && e->value->toValue().find("available") != nullptr);
    }
    CHECK(cs->productEntry("c59.trace") == nullptr);   // TRACE can always be pressed
    // In the payload's Value form under `product`, as the frontends read it.
    const Value v = cs->toValue();
    CHECK(v.find("product") && v.find("product")->find("c59.asm-run"));
}

// The harness reads the control state with the paper panel and the card
// reader.
TEST(harness_state_control_state_has_paper_panel_and_card_reader) {
    SKIP_UNLESS(roms(), kSkip);
    auto product = cu59::makeHarnessProduct();
    std::string error;
    auto server = calcu::harness::Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    const std::string s = server->line("state control-state").text;
    CHECK(s.rfind("done", 0) == 0);
    CHECK(s.find("\"panels\":[{\"kind\":\"paper\"") != std::string::npos);
    CHECK(s.find("\"card-reader\"") != std::string::npos);
    CHECK(s.find("\"slot-1\"") != std::string::npos);
    server->finish();
}
