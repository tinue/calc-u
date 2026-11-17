// LIB-14 L2 (design §6): the bar and panel models, rules B1 to B7 (spec AC-8
// to AC-10, AC-23, AC-42, EC-5). Mirrored in shell_model.test.js.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/BarModel.hpp"
#include <string>
#include <vector>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;
using sessions::Value;

namespace {

std::string describe(const std::vector<BarGroupModel>& groups) {
    std::string s;
    for (const auto& g : groups) {
        s += "[";
        bool first = true;
        for (const auto& c : g.controls) {
            s += (first ? "" : " ") + c.id;
            first = false;
        }
        s += "]";
    }
    return s;
}

const BarControl* controlOf(const std::vector<BarGroupModel>& groups, const std::string& id) {
    for (const auto& g : groups)
        for (const auto& c : g.controls)
            if (c.id == id) return &c;
    return nullptr;
}

} // namespace

TEST(ac8_groups_and_order) {
    const auto b = bar(st::inputs(), st::product());
    // The model picker; the module slots; the other places; the media places; the product's end.
    CHECK_EQ(describe(b), std::string("[model][slot-1 slot-2][printer aux][card][]"));
    CHECK(b[0].group == BarGroup::AfterModel);
    CHECK(b[1].group == BarGroup::Slots);
    CHECK(b[2].group == BarGroup::Places);
    CHECK(b[3].group == BarGroup::Media);
    CHECK(b[4].group == BarGroup::End);
    const BarControl* model = controlOf(b, "model");
    CHECK(model->kind == BarControl::Kind::ModelPicker);
    CHECK_EQ(model->choices.size(), std::size_t{2});
    CHECK(model->choices[0].checked);
    CHECK_EQ(model->choices[1].command.value, std::string("model"));
    const BarControl* card = controlOf(b, "card");
    CHECK(card->kind == BarControl::Kind::Media);
    CHECK_EQ(card->mediumKind, std::string("test-card"));   // the factory of its owner is chosen by kind
    CHECK_EQ(read::text(card->entry, "id"), std::string("card"));
}

TEST(ac8_slot_numbers_with_two_slots) {
    const auto b = bar(st::inputs(), st::product());
    CHECK_EQ(controlOf(b, "slot-1")->label, std::string("1:"));
    CHECK_EQ(controlOf(b, "slot-2")->label, std::string("2:"));
    CHECK(controlOf(b, "slot-1")->kind == BarControl::Kind::SlotPicker);
    CHECK_EQ(read::text(controlOf(b, "slot-1")->entry, "place"), std::string("slot-1"));   // LIB-7's picker reads it
    // One slot: no number.
    auto cs = st::controlState();
    cs.places.erase(cs.places.begin() + 1);
    const auto one = bar(st::inputs(cs.toValue()), st::product());
    CHECK_EQ(controlOf(one, "slot-1")->label, std::string(""));
}

TEST(ac9_button_for_one_picker_for_several) {
    const auto b = bar(st::inputs(), st::product());
    const BarControl* printer = controlOf(b, "printer");
    CHECK(printer->kind == BarControl::Kind::PlaceButton);
    CHECK_EQ(printer->label, std::string("TD-20"));
    CHECK(!printer->plugged);
    const BarControl* aux = controlOf(b, "aux");
    CHECK(aux->kind == BarControl::Kind::PlacePicker);
    std::string labels;
    for (const auto& c : aux->choices) labels += c.label + ";";
    CHECK_EQ(labels, std::string("None;TP-4;TD-21;"));   // D-11: the empty place first
    CHECK(aux->choices[0].checked);
    CHECK_EQ(aux->choices[1].command.value, std::string("attach"));
}

TEST(ac9_button_attaches_and_detaches) {
    auto cs = st::controlState();
    const auto before = bar(st::inputs(cs.toValue()), st::product());
    const BarControl* off = controlOf(before, "printer");
    CHECK_EQ(off->choices.size(), std::size_t{1});
    CHECK_EQ(off->choices[0].command.value, std::string("attach"));
    CHECK(off->choices[0].args == Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    st::plug(cs, "printer", "test-dotter");
    const auto b = bar(st::inputs(cs.toValue()), st::product());
    const BarControl* on = controlOf(b, "printer");
    CHECK(on->plugged);
    CHECK_EQ(on->choices[0].command.value, std::string("detach"));
    CHECK(on->choices[0].args == Value::object({{"place", "printer"}}));
    // A picker: "None" detaches what is plugged in.
    st::plug(cs, "aux", "test-plotter");
    const auto picked = bar(st::inputs(cs.toValue()), st::product());
    const BarControl* aux = controlOf(picked, "aux");
    CHECK_EQ(aux->choices[0].command.value, std::string("detach"));
    CHECK(aux->choices[1].checked);
}

TEST(ac9_place_needing_more_has_no_control) {
    const auto b = bar(st::inputs(), st::product());
    CHECK(controlOf(b, "host-drive") == nullptr);
    auto p = st::product();
    p.placesWithoutControl.clear();   // without the product's mark it is a button
    CHECK(controlOf(bar(st::inputs(), p), "host-drive") != nullptr);
}

TEST(ac10_no_entry_no_gap) {
    const auto b = bar(st::inputs(st::controlValue("Model-B")), st::product());
    CHECK_EQ(describe(b), std::string("[model][][printer][][]"));   // empty groups: a component shows nothing
    CHECK(controlOf(b, "slot-1") == nullptr);
    CHECK(controlOf(b, "card") == nullptr);
}

TEST(ac23_paper_panel_always_with_or_without_owner) {
    auto cs = st::controlState();
    auto ps = panels(st::inputs(cs.toValue()));
    CHECK_EQ(ps.size(), std::size_t{1});
    CHECK_EQ(ps[0].kind, std::string("paper"));
    CHECK_EQ(ps[0].owner, std::string(""));
    st::plug(cs, "printer", "test-dotter");
    ps = panels(st::inputs(cs.toValue()));
    CHECK_EQ(ps.size(), std::size_t{1});
    CHECK_EQ(ps[0].owner, std::string("test-dotter"));
    CHECK_EQ(ps[0].ownerPlace, std::string("printer"));
}

TEST(ac42_shell_reads_no_product_entry) {
    auto cs = st::controlState();
    const auto before = describe(bar(st::inputs(cs.toValue()), st::product()));
    cs.productEntries.push_back(control::ProductEntry{
        "tp.extra", sessions::valuePayload("tp.extra", Value::object({{"show", true}, {"available", false}}))});
    cs.productEntries.front().value = sessions::valuePayload("tp.disk", Value::object({{"available", true}}));
    const auto b = bar(st::inputs(cs.toValue()), st::product());
    CHECK_EQ(describe(b), before);
    for (const auto& g : b)
        for (const auto& c : g.controls) CHECK(c.id.rfind("tp.", 0) != 0);
}

TEST(ec5_printer_change_moves_paper_owner) {
    auto cs = st::controlState();
    st::plug(cs, "printer", "test-dotter");
    const auto plugged = panels(st::inputs(cs.toValue()));
    CHECK_EQ(plugged[0].owner, std::string("test-dotter"));
    cs = st::controlState();   // removed: an empty paper under none
    const auto removed = panels(st::inputs(cs.toValue()));
    CHECK_EQ(removed[0].owner, std::string(""));
}

TEST(b7_no_bar_without_control_state) {
    CHECK(bar(ShellInputs{}, st::product()).empty());
    CHECK(panels(ShellInputs{}).empty());
}

// Part E (design §10.3.1), B8: a window without a menu bar.
TEST(bar_without_menu_bar_keeps_reset_preset_settings_and_more) {
    Traits t{false, false, true};
    t.menuBar = false;
    const auto p = st::product();
    const auto in = st::inputs();
    const auto b = bar(in, p, t);
    CHECK_EQ(describe(b), std::string("[reset][model][slot-1 slot-2][printer aux][card][load-preset settings more]"));
    CHECK(b[0].group == BarGroup::Start);
    CHECK(b[1].group == BarGroup::AfterModel);
    CHECK(b[5].group == BarGroup::End);
    const BarControl* reset = controlOf(b, "reset");
    CHECK(reset->kind == BarControl::Kind::Action);
    CHECK_EQ(reset->label, std::string("Reset"));
    CHECK_EQ(reset->item.command.value, std::string("reset"));
    const BarControl* preset = controlOf(b, "load-preset");
    CHECK_EQ(preset->label, std::string("Preset"));
    CHECK(preset->item.command.value.empty());   // the shell's file dialog
    CHECK_EQ(controlOf(b, "settings")->label, std::string("Settings"));
    const BarControl* more = controlOf(b, "more");
    CHECK(more->kind == BarControl::Kind::More);
    CHECK_EQ(more->choices.size(), moreItems(menus(in, p, t), p).size());
    // Each action follows its menu item (M9).
    auto cs = st::controlState();
    for (auto& a : cs.actions)
        if (a.command == control::kResetAction) a.available = st::unavailable("An operation runs.");
    const auto b2 = bar(st::inputs(cs.toValue()), p, t);
    CHECK(!controlOf(b2, "reset")->item.enabled);
    CHECK_EQ(controlOf(b2, "reset")->item.reason, std::string("An operation runs."));
    // M18: no Settings button without settings.
    t.settings = false;
    CHECK(controlOf(bar(in, p, t), "settings") == nullptr);
    // With a menu bar: no Start group, no action, no More (B1 to B7).
    CHECK_EQ(describe(bar(in, p, Traits{})), std::string("[model][slot-1 slot-2][printer aux][card][]"));
}
