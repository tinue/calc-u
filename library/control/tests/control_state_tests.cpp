// The control state: the payload, the lists, what a model lacks and the
// controls of what is plugged in.
#include "TestControlRules.hpp"
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "control/Compute.hpp"
#include "control/ControlState.hpp"
#include "peripherals/Controls.hpp"
#include "startup/StartupPart.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace calcu::control;
using namespace calcu::control::test;
using calcu::sessions::Value;
using calcu::sessions::toText;
using calcu::sessions::valuePayload;

namespace {

std::unique_ptr<mt::TestMachine> seg(std::vector<calcu::machine::PeripheralRequest> plugs = {}) {
    return mt::buildTestMachine(mt::kSeg, std::nullopt, std::move(plugs));
}
std::unique_ptr<mt::TestMachine> dot(std::optional<VariantId> v = std::nullopt,
                                     std::vector<calcu::machine::PeripheralRequest> plugs = {}) {
    return mt::buildTestMachine(mt::kDot, std::move(v), std::move(plugs));
}
calcu::machine::PeripheralRequest plug(const char* place, const PeripheralId& id,
                                       std::optional<VariantId> v = std::nullopt) {
    return mt::peripheralAt(mt::testCatalogue(), place, id, std::move(v));
}
// test-dot with the hub in `a` and the printer in the hub's `jack`.
std::unique_ptr<mt::TestMachine> dotWithHubAndPrinter() {
    auto m = dot(std::nullopt, {plug("a", mt::kHub)});
    calcu::machine::PeripheralRequest r = plug("a", mt::kPrinter);
    r.place = at({"a", "jack"});
    const auto res = m->attach(r, mt::testCatalogue());
    CHECK(!res.refused);
    return m;
}
bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

} // namespace

// ── The payload ─────────────────────────────────────────────────────────

TEST(control_state_value_round_trip) {
    auto m = seg({plug("printer", mt::kPrinter)});
    TestControlRules rules;
    rules.refuse(ActionKey{kOpenAction});
    const ControlState s = computeControlState(*m, testSetup(&rules), false);
    const std::string t = toText(s.toValue());
    // One object, keys in the order of the fields.
    const char* keys[] = {"\"models\":", "\"model\":", "\"variants\":", "\"variant\":", "\"places\":",
                          "\"mediaPlaces\":", "\"panels\":", "\"controls\":", "\"actions\":", "\"product\":"};
    std::size_t last = 0;
    for (const char* k : keys) {
        const std::size_t at = t.find(k);
        CHECK(at != std::string::npos && at >= last);
        if (at != std::string::npos) last = at;
    }
    CHECK(contains(t, "\"model\":\"test-seg\""));
    CHECK(contains(t, "{\"place\":\"slot-1\",\"connector\":\"slot\",\"pinOut\":\"test.slot\",\"available\":true,"
                      "\"empty\":{\"available\":true},\"fitting\":[{\"id\":\"test-module\",\"name\":\"Test module\","
                      "\"plugged\":false,\"variant\":\"4k\",\"variants\":[{\"id\":\"4k\",\"available\":true},"
                      "{\"id\":\"8k\",\"available\":true}],\"available\":true}]}"));
    CHECK(contains(t, "{\"id\":\"reader\",\"kind\":\"test-card\",\"passage\":\"pass-through\",\"sides\":[0,1],"
                      "\"insert\":{\"available\":true}}"));
    CHECK(contains(t, "\"panels\":[{\"kind\":\"paper\",\"owner\":{\"place\":\"printer\",\"peripheral\":\"test-printer\"}}]"));
    // `reason` only where `available` is false.
    CHECK(contains(t, "{\"command\":\"reset\",\"available\":true}"));
    CHECK(contains(t, std::string("{\"command\":\"open\",\"available\":false,\"reason\":\"") + kProductReason + "\"}"));
    CHECK(contains(t, "\"product\":{\"test.entry\":{\"model\":\"test-seg\"}}"));
    // Computed twice from the same machine: the same value, and equal payloads.
    const ControlState again = computeControlState(*m, testSetup(&rules), false);
    CHECK(again.toValue() == s.toValue());
    CHECK(again.equals(s));
}

TEST(same_compares_product_entries_by_equals) {
    ControlState a, b;
    a.model = b.model = mt::kSeg;
    a.productEntries.push_back(ProductEntry{"test.x", valuePayload("test.x", Value(1))});
    b.productEntries.push_back(ProductEntry{"test.x", valuePayload("test.x", Value(1))});   // another object
    CHECK(a.same(b));
    b.productEntries[0].value = valuePayload("test.x", Value(2));
    CHECK(!a.same(b));
    b.productEntries[0] = ProductEntry{"test.y", valuePayload("test.x", Value(1))};
    CHECK(!a.same(b));
    b.productEntries.clear();
    CHECK(!a.same(b));
    a.productEntries.clear();
    b.model = mt::kDot;
    CHECK(!a.same(b));
}

// ── The lists ───────────────────────────────────────────────────────────

TEST(control_state_models_and_running_model) {
    const ControlState s = computeControlState(*dot(), testSetup(), false);
    CHECK_EQ(s.models.size(), std::size_t(2));
    CHECK(s.models[0].id == mt::kSeg && s.models[1].id == mt::kDot);   // the catalogue's order
    CHECK(s.model == mt::kDot);
    for (const auto& e : s.models) CHECK(e.available == kYes);
}

TEST(control_state_variants_of_running_model) {
    const ControlState d = computeControlState(*dot(VariantId{"V2"}), testSetup(), false);
    CHECK_EQ(d.variants.size(), std::size_t(2));
    CHECK(d.variants[0].id == VariantId{"V1"} && d.variants[1].id == VariantId{"V2"});
    CHECK(d.variant == std::optional<VariantId>(VariantId{"V2"}));
    CHECK(computeControlState(*dot(), testSetup(), false).variant == std::optional<VariantId>(VariantId{"V1"}));   // the default
    const ControlState s = computeControlState(*seg(), testSetup(), false);
    CHECK(s.variants.empty());   // a model without variants lists none
    CHECK(!s.variant);
}

TEST(control_state_places_fitting_and_plugged_with_variant) {
    auto m = seg({plug("slot-1", mt::kModule, VariantId{"8k"}), plug("printer", mt::kPrinter)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    CHECK(placeTexts(s) == (std::vector<std::string>{"slot-1", "printer", "aux"}));
    const PlaceEntry* slot = s.place(at({"slot-1"}));
    CHECK(slot && slot->connector.value == "slot" && slot->pinOut.value == "test.slot");
    if (!slot) return;
    CHECK(fittingIds(*slot) == (std::vector<std::string>{"test-module"}));
    const PeripheralChoice* mod = choice(*slot, mt::kModule);
    CHECK(mod && mod->plugged && mod->name == "Test module");
    CHECK(mod && mod->variant == std::optional<VariantId>(VariantId{"8k"}));   // the one that runs
    CHECK(mod && mod->variants.size() == 2 && variantOf(*mod, "4k") && variantOf(*mod, "8k"));
    const PlaceEntry* printer = s.place(at({"printer"}));
    CHECK(printer && fittingIds(*printer) == (std::vector<std::string>{"test-printer", "test-module", "test-uart"}));
    if (!printer) return;
    const PeripheralChoice* p = choice(*printer, mt::kPrinter);
    CHECK(p && p->plugged && !p->variant && p->variants.empty());   // a peripheral without variants
    const PeripheralChoice* free = choice(*printer, mt::kModule);
    CHECK(free && !free->plugged && free->variant == std::optional<VariantId>(VariantId{"4k"}));   // an attach's default
}

TEST(control_state_nested_place_after_its_peripheral) {
    auto m = dotWithHubAndPrinter();
    const ControlState s = computeControlState(*m, testSetup(), false);
    CHECK(placeTexts(s) == (std::vector<std::string>{"a", "a/jack", "b", "card-1", "card-2"}));
    const PlaceEntry* jack = s.place(at({"a", "jack"}));
    CHECK(jack && jack->connector.value == "jack" && jack->pinOut.value == "test.jack");
    if (!jack) return;
    CHECK(fittingIds(*jack) == (std::vector<std::string>{"test-printer"}));
    CHECK(choice(*jack, mt::kPrinter) && choice(*jack, mt::kPrinter)->plugged);
    // The hub out: its place goes with it.
    m->detach(at({"a"}), mt::kHub, mt::testCatalogue());
    CHECK(placeTexts(computeControlState(*m, testSetup(), false)) ==
          (std::vector<std::string>{"a", "b", "card-1", "card-2"}));
}

TEST(control_state_empty_place_is_a_choice) {
    auto m = seg({plug("printer", mt::kPrinter)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    for (const auto& p : s.places) CHECK(p.empty == kYes);   // empty and full places alike
}

TEST(control_state_media_places_owner_kind_passage_sides) {
    const ControlState s = computeControlState(*seg(), testSetup(), false);
    CHECK_EQ(s.mediaPlaces.size(), std::size_t(1));
    const MediaPlaceEntry* r = s.mediaPlace(MediaPlaceId{"reader"});
    CHECK(r && r->ownerPlace.steps.empty() && !r->owner);   // the machine's
    CHECK(r && r->mediumKind == "test-card" && r->passage == calcu::machine::MediaPassage::PassThrough);
    CHECK(r && r->sides == (std::vector<int>{0, 1}) && r->insert == kYes);
    const ControlState d = computeControlState(*dot(), testSetup(), false);
    const MediaPlaceEntry* drive = d.mediaPlace(MediaPlaceId{"drive"});
    CHECK(drive && drive->passage == calcu::machine::MediaPassage::Holding && drive->sides == (std::vector<int>{0}));
}

TEST(control_state_paper_panel_owned_by_plugged_printer_or_none) {
    const ControlState bare = computeControlState(*seg(), testSetup(), false);
    CHECK_EQ(bare.panels.size(), std::size_t(1));   // always, in a product with printers
    CHECK(bare.panels[0].kind == kPaperPanel && !bare.panels[0].owner && !bare.panels[0].ownerPlace);
    const ControlState with = computeControlState(*seg({plug("aux", mt::kPrinter)}), testSetup(), false);
    CHECK_EQ(with.panels.size(), std::size_t(1));
    CHECK(with.panels[0].owner == std::optional<PeripheralId>(mt::kPrinter));
    CHECK(with.panels[0].ownerPlace == std::optional<PlacePath>(at({"aux"})));
    // A printer on a peripheral's place owns it too.
    const ControlState nested = computeControlState(*dotWithHubAndPrinter(), testSetup(), false);
    CHECK(nested.panels.size() == 1 && nested.panels[0].ownerPlace == std::optional<PlacePath>(at({"a", "jack"})));
}

TEST(control_state_actions_in_order) {
    const ControlState s = computeControlState(*seg(), testSetup(), false);
    CHECK_EQ(s.actions.size(), std::size_t(4));
    if (s.actions.size() != 4) return;
    CHECK(s.actions[0].command == kResetAction && s.actions[1].command == kColdBootAction);
    CHECK(s.actions[2].command == kLoadPresetAction && s.actions[3].command == kOpenAction);
    // Named as the commands are.
    CHECK(kResetAction == calcu::startup::kResetCommand && kColdBootAction == calcu::startup::kColdBootCommand);
    for (const auto& a : s.actions) CHECK(a.available == kYes);
}

TEST(control_state_entries_in_descriptor_order) {
    auto m = dot(std::nullopt, {plug("b", mt::kUart), plug("a", mt::kHub)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    // Places by the descriptor, not by the order of plugging.
    CHECK(placeTexts(s) == (std::vector<std::string>{"a", "a/jack", "b", "card-1", "card-2"}));
    const PlaceEntry* a = s.place(at({"a"}));
    CHECK(a && fittingIds(*a) == (std::vector<std::string>{"test-printer", "test-hub", "test-uart"}));
    // Media places: the machine's, then the peripherals' in the order of the place walk.
    CHECK(s.mediaPlaces.size() == 2 && s.mediaPlaces[0].id.value == "drive" && s.mediaPlaces[1].id.value == "hub-disk");
}

// A peripheral's media place that exists only at other pin-outs is not
// listed where it is plugged.
TEST(control_state_media_place_only_at_its_pin_outs) {
    const calcu::machine::Catalogue& base = mt::testCatalogue();
    std::vector<calcu::machine::PeripheralDescriptor> peripherals = base.peripherals();
    for (auto& p : peripherals)
        if (p.id == mt::kHub) p.mediaPlaces[0].onlyAt = {calcu::machine::PinOutId{"test.elsewhere"}};
    const calcu::machine::Catalogue cat(base.models(), std::move(peripherals));
    auto m = dot(std::nullopt, {plug("a", mt::kHub)});
    const ControlState s = computeControlState(*m, testSetup({}, &cat), false);
    CHECK(s.mediaPlaces.size() == 1 && s.mediaPlaces[0].id.value == "drive");
    CHECK(computeControlState(*m, testSetup(), false).mediaPlaces.size() == 2);   // at every pin-out it fits
}

TEST(control_state_plugged_peripheral_says_plugged) {
    auto m = dot(std::nullopt, {plug("card-2", mt::kModule)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    const PlaceEntry* c1 = s.place(at({"card-1"}));
    const PlaceEntry* c2 = s.place(at({"card-2"}));
    CHECK(c1 && c2);
    if (!c1 || !c2) return;
    CHECK(!choice(*c1, mt::kModule)->plugged);
    CHECK(choice(*c2, mt::kModule)->plugged);
    for (const auto& p : s.places)
        for (const auto& c : p.fitting) CHECK_EQ(c.plugged, p.path == at({"card-2"}));
}

// ── What a model lacks ──────────────────────────────────────────────────

TEST(control_state_place_media_place_panel_absent_where_model_lacks_them) {
    const ControlState s = computeControlState(*seg(), testSetup(), false);
    const ControlState d = computeControlState(*dot(), testSetup(), false);
    for (const char* p : {"a", "b", "card-1", "card-2"}) CHECK(!s.place(at({p})));
    for (const char* p : {"slot-1", "printer", "aux"}) CHECK(!d.place(at({p})));
    CHECK(!s.mediaPlace(MediaPlaceId{"drive"}) && !d.mediaPlace(MediaPlaceId{"reader"}));
    CHECK(!s.mediaPlace(MediaPlaceId{"hub-disk"}));   // its peripheral is not plugged in
    // A product without printers has no panel; a model on which none of its
    // printers fits has none either (the hub fits test-dot's chain only).
    ControlSetup noPrinters = testSetup();
    noPrinters.printers.clear();
    CHECK(computeControlState(*seg(), noPrinters, false).panels.empty());
    ControlSetup hubPrinter = testSetup();
    hubPrinter.printers = {mt::kHub};
    CHECK(computeControlState(*seg(), hubPrinter, false).panels.empty());
    CHECK_EQ(computeControlState(*dot(), hubPrinter, false).panels.size(), std::size_t(1));
    // Nothing says "not on this model".
    CHECK(!contains(toText(s.toValue()), "card-1") && !contains(toText(s.toValue()), "drive"));
}

// ── Later definitions, a peripheral's media ─────────────────────────────

TEST(control_state_module_added_to_catalogue_listed_next_time) {
    calcu::machine::Catalogue cat = mt::makeTestCatalogue();
    auto m = seg();
    const ControlSetup setup = testSetup(nullptr, &cat);
    CHECK(fittingIds(*computeControlState(*m, setup, false).place(at({"slot-1"}))) ==
          (std::vector<std::string>{"test-module"}));
    calcu::machine::PeripheralDescriptor later;
    later.id = PeripheralId{"test-module-2"};
    later.name = "Second test module";
    later.fits = {calcu::machine::PinOutId{"test.slot"}};
    cat.addPeripheral(later);
    const ControlState s = computeControlState(*m, setup, false);
    const PlaceEntry* slot = s.place(at({"slot-1"}));
    CHECK(slot && fittingIds(*slot) == (std::vector<std::string>{"test-module", "test-module-2"}));
    CHECK(slot && choice(*slot, PeripheralId{"test-module-2"})->available == kYes);
}

TEST(control_state_peripheral_media_place_owned_by_peripheral_no_place_for_it) {
    auto m = dot(std::nullopt, {plug("a", mt::kHub)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    const MediaPlaceEntry* disk = s.mediaPlace(MediaPlaceId{"hub-disk"}, at({"a"}));
    CHECK(disk && disk->owner == std::optional<PeripheralId>(mt::kHub));
    CHECK(disk && disk->mediumKind == "test-disk" && disk->passage == calcu::machine::MediaPassage::Holding &&
          disk->sides == (std::vector<int>{0}));
    // No place stands for the medium's drive: only the descriptor's places.
    CHECK(placeTexts(s) == (std::vector<std::string>{"a", "a/jack", "b", "card-1", "card-2"}));
    const std::string t = toText(s.toValue());
    CHECK(contains(t, "{\"id\":\"hub-disk\",\"owner\":{\"place\":\"a\",\"peripheral\":\"test-hub\"},\"kind\":\"test-disk\","
                      "\"passage\":\"holding\""));
}

// ── The controls of what is plugged in ──────────────────────────────────

TEST(control_state_controls_of_plugged_peripherals_with_place_form_and_position) {
    using calcu::machine::ControlForm;
    using calcu::machine::ControlId;
    // Nothing with controls plugged in: none listed (a module has none).
    CHECK(computeControlState(*seg(), testSetup(), false).controls.empty());
    CHECK(computeControlState(*seg({plug("slot-1", mt::kModule)}), testSetup(), false).controls.empty());
    // The printer's three, in the descriptor's order, with their owner.
    auto m = seg({plug("slot-1", mt::kModule), plug("printer", mt::kPrinter)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    CHECK_EQ(s.controls.size(), std::size_t(3));
    if (s.controls.size() != 3) return;
    const char* ids[] = {"online", "feed", "trace"};
    const ControlForm forms[] = {ControlForm::Switch, ControlForm::Key, ControlForm::Switch};
    for (std::size_t i = 0; i < 3; ++i) {
        CHECK(s.controls[i].place == at({"printer"}) && s.controls[i].peripheral == mt::kPrinter);
        CHECK(s.controls[i].control.id == ControlId{ids[i]} && s.controls[i].control.form == forms[i]);
        CHECK(!s.controls[i].control.on);
    }
    // A switch moved and a key held: their positions, and the state differs.
    const calcu::machine::Owner owner = calcu::machine::PeripheralOwner{mt::kPrinter, at({"printer"})};
    calcu::peripherals::Controls* c = calcu::peripherals::controlsCapability(*m, owner);
    CHECK(c != nullptr);
    if (!c) return;
    c->set(ControlId{"trace"}, true);
    c->press(ControlId{"feed"});
    const ControlState moved = computeControlState(*m, testSetup(), false);
    CHECK(moved.controls.size() == 3 && moved.controls[1].control.on && moved.controls[2].control.on);
    CHECK(!moved.same(s));
    // The value form: {place, peripheral, id, form, on}, after the panels.
    const std::string t = toText(moved.toValue());
    CHECK(contains(t, "\"controls\":[{\"place\":\"printer\",\"peripheral\":\"test-printer\",\"id\":\"online\","
                      "\"form\":\"switch\",\"on\":false},{\"place\":\"printer\",\"peripheral\":\"test-printer\","
                      "\"id\":\"feed\",\"form\":\"key\",\"on\":true},{\"place\":\"printer\",\"peripheral\":\"test-printer\","
                      "\"id\":\"trace\",\"form\":\"switch\",\"on\":true}]"));
    // A peripheral on a peripheral's place: the place path names it.
    const ControlState nested = computeControlState(*dotWithHubAndPrinter(), testSetup(), false);
    CHECK(nested.controls.size() == 3 && nested.controls[0].place == at({"a", "jack"}));
    // Unplugged: gone.
    CHECK(!m->detach(at({"printer"}), mt::kPrinter, mt::testCatalogue()).refused);
    CHECK(computeControlState(*m, testSetup(), false).controls.empty());
}

// The controls of one place, as the frontends draw a peripheral's
// pane: the plugged peripheral's, in their order; none elsewhere.
TEST(control_state_controls_at_a_place) {
    using calcu::machine::ControlId;
    auto m = seg({plug("slot-1", mt::kModule), plug("printer", mt::kPrinter)});
    const calcu::machine::Owner owner = calcu::machine::PeripheralOwner{mt::kPrinter, at({"printer"})};
    calcu::peripherals::Controls* c = calcu::peripherals::controlsCapability(*m, owner);
    CHECK(c != nullptr);
    if (!c) return;
    c->set(ControlId{"trace"}, true);
    const ControlState s = computeControlState(*m, testSetup(), false);
    const auto printer = s.controlsAt(at({"printer"}));
    CHECK_EQ(printer.size(), std::size_t(3));
    if (printer.size() != 3) return;
    CHECK(printer[0].id == ControlId{"online"} && printer[1].id == ControlId{"feed"} && printer[2].id == ControlId{"trace"});
    CHECK(!printer[0].on && printer[2].on);
    CHECK(s.controlsAt(at({"slot-1"})).empty());   // a module has none
    CHECK(s.controlsAt(at({"nowhere"})).empty());
    const ControlState nested = computeControlState(*dotWithHubAndPrinter(), testSetup(), false);
    CHECK_EQ(nested.controlsAt(at({"a", "jack"})).size(), std::size_t(3));
    CHECK(nested.controlsAt(at({"a"})).empty());   // the hub has none
}
