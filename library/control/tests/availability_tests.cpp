// Availability in the control state: its order, while an operation runs, a
// product's rules.
#include "TestControlRules.hpp"
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "TestParts.hpp"
#include "control/Compute.hpp"
#include "control/ControlPart.hpp"
#include "control/ControlState.hpp"
#include "sessions/LibraryTopics.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace calcu::control;
using namespace calcu::control::test;
using calcu::sessions::kAnotherOperationRuns;

namespace {

calcu::machine::PeripheralRequest plug(const char* place, const PeripheralId& id,
                                       std::optional<VariantId> v = std::nullopt) {
    return mt::peripheralAt(mt::testCatalogue(), place, id, std::move(v));
}
const PeripheralChoice& choiceAt(const ControlState& s, const char* place, const PeripheralId& id) {
    static const PeripheralChoice none{};
    const PlaceEntry* p = s.place(at({place}));
    const PeripheralChoice* c = p ? choice(*p, id) : nullptr;
    CHECK(c != nullptr);
    return c ? *c : none;
}
const Availability kBusy = no(kAnotherOperationRuns);

} // namespace

// ── Availability and its order ──────────────────────────────────────────

TEST(availability_machine_reason_for_peripheral) {
    // The machine's rule: the test printer and the test UART are not used together.
    auto m = mt::buildTestMachine(mt::kSeg, std::nullopt, {plug("printer", mt::kPrinter)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    CHECK(choiceAt(s, "aux", mt::kUart).available == no(mt::kTogether));
    CHECK(choiceAt(s, "aux", mt::kModule).available == kYes);
    // The same answer the machine gives an attach.
    const auto refusal = m->canAttach(plug("aux", mt::kUart), mt::testCatalogue());
    CHECK(refusal && refusal->reason == choiceAt(s, "aux", mt::kUart).available.reason);
    // A place on a Takes::One connector: the other place holds the module.
    auto d = mt::buildTestMachine(mt::kDot, std::nullopt, {plug("card-1", mt::kModule)});
    const ControlState ds = computeControlState(*d, testSetup(), false);
    CHECK(choiceAt(ds, "card-2", mt::kModule).available == no("card-1 already holds the Test module."));
}

TEST(availability_full_place_asked_as_replace) {
    auto m = mt::buildTestMachine(mt::kSeg, std::nullopt, {plug("printer", mt::kPrinter)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    // canAttach would say "printer already holds the Test printer."; a replace is asked.
    CHECK(choiceAt(s, "printer", mt::kModule).available == kYes);
    CHECK(choiceAt(s, "printer", mt::kUart).available == kYes);   // the printer goes out with it
    CHECK(m->canAttach(plug("printer", mt::kModule), mt::testCatalogue()).has_value());
}

TEST(availability_variant_of_plugged_peripheral) {
    auto m = mt::buildTestMachine(mt::kSeg, std::nullopt, {plug("slot-1", mt::kModule)});
    const ControlState s = computeControlState(*m, testSetup(), false);
    const PeripheralChoice& mod = choiceAt(s, "slot-1", mt::kModule);
    CHECK(mod.plugged && mod.variant == std::optional<VariantId>(VariantId{"4k"}));
    CHECK(mod.available == kYes);   // its detach
    // The second module variant in a full slot: a replace, available.
    CHECK(variantOf(mod, "8k") && variantOf(mod, "8k")->available == kYes);
    CHECK(variantOf(mod, "4k") && variantOf(mod, "4k")->available == kYes);   // the one in force
    // A variant entry has its own key: a product's rule refuses 8k alone.
    TestControlRules rules;
    rules.refuse(ChoiceKey{at({"slot-1"}), mt::kModule, VariantId{"8k"}});
    const ControlState r = computeControlState(*m, testSetup(&rules), false);
    const PeripheralChoice& mod2 = choiceAt(r, "slot-1", mt::kModule);
    CHECK(variantOf(mod2, "8k")->available == no(kProductReason));
    CHECK(variantOf(mod2, "4k")->available == kYes);
    CHECK(mod2.available == kYes);
}

TEST(availability_no_entry_refused_for_rom) {
    // The check request carries the ROM file names, no bytes: no entry
    // with ROM entries is refused for a ROM.
    for (const auto& model : {mt::kSeg, mt::kDot}) {
        auto m = mt::buildTestMachine(model);
        const ControlState s = computeControlState(*m, testSetup(), false);
        for (const auto& p : s.places)
            for (const auto& c : p.fitting) {
                CHECK(c.available.reason.find("missing") == std::string::npos);
                for (const auto& v : c.variants) CHECK(v.available.reason.find("missing") == std::string::npos);
            }
    }
    auto m = mt::buildTestMachine(mt::kSeg);
    const ControlState s = computeControlState(*m, testSetup(), false);
    const PeripheralChoice& mod = choiceAt(s, "slot-1", mt::kModule);
    CHECK(mod.available == kYes && variantOf(mod, "8k")->available == kYes);
}

TEST(availability_operation_before_product_before_machine) {
    auto m = mt::buildTestMachine(mt::kSeg, std::nullopt, {plug("printer", mt::kPrinter)});
    TestControlRules rules;
    rules.refuse(ChoiceKey{at({"aux"}), mt::kUart, std::nullopt});
    // The machine alone: its reason.
    CHECK(choiceAt(computeControlState(*m, testSetup(), false), "aux", mt::kUart).available == no(mt::kTogether));
    // The product too: the product's.
    CHECK(choiceAt(computeControlState(*m, testSetup(&rules), false), "aux", mt::kUart).available ==
          no(kProductReason));
    // An operation runs as well: that one.
    CHECK(choiceAt(computeControlState(*m, testSetup(&rules), true), "aux", mt::kUart).available == kBusy);
}

// ── While an operation runs ─────────────────────────────────────────────

TEST(availability_choices_and_actions_refused_while_operation_runs) {
    auto m = mt::buildTestMachine(mt::kDot, std::nullopt, {plug("a", mt::kHub)});
    TestControlRules rules;
    const ControlState s = computeControlState(*m, testSetup(&rules), true);
    for (const auto& v : s.variants) CHECK(v.available == kBusy);
    for (const auto& p : s.places) {
        CHECK(p.available == kYes);   // the place as a whole: only a product rule refuses it
        CHECK(p.empty == kBusy);
        for (const auto& c : p.fitting) {
            CHECK(c.available == kBusy);
            for (const auto& v : c.variants) CHECK(v.available == kBusy);
        }
    }
    for (const auto& mp : s.mediaPlaces) CHECK(mp.insert == kBusy);
    for (const auto& a : s.actions)
        if (a.command == kLoadPresetAction || a.command == kOpenAction) CHECK(a.available == kBusy);
    // The models, reset and cold-boot end the operation: not refused for it.
    for (const auto& e : s.models) CHECK(e.available == kYes);
}

TEST(availability_models_reset_cold_boot_stay_available) {
    auto m = mt::buildTestMachine(mt::kSeg);
    const ControlState s = computeControlState(*m, testSetup(), true);
    for (const auto& e : s.models) CHECK(e.available == kYes);
    CHECK(s.action(kResetAction) && s.action(kResetAction)->available == kYes);
    CHECK(s.action(kColdBootAction) && s.action(kColdBootAction)->available == kYes);
    CHECK(s.action(kOpenAction) && s.action(kOpenAction)->available == kBusy);
}

namespace {
calcu::sessions::Task<calcu::sessions::OperationResult> attachPrinterThenRun(calcu::sessions::OperationContext& c,
                                                                             calcu::machine::Ticks n) {
    if (calcu::machine::Machine* m = c.session().machine()) m->attach(plug("printer", mt::kPrinter), mt::testCatalogue());
    co_await c.run(n);
    co_return calcu::sessions::OperationResult{};
}
} // namespace

TEST(availability_recomputed_from_machine_after_operation) {
    ControlPart control(testSetup());
    calcu::sessions::test::RigOptions o;
    o.extraParts = {&control};
    calcu::sessions::test::Rig r(o);
    r.build();
    r.call();
    r.part.operation = [](calcu::sessions::OperationContext& c) { return attachPrinterThenRun(c, 100'000); };
    r.send("tp.op");
    CHECK(r.session->busy());
    const auto* during = r.fe.state<ControlState>(kControlStateTopic);
    CHECK(during != nullptr);
    if (!during) return;
    CHECK(choiceAt(*during, "printer", mt::kPrinter).available == kBusy);
    CHECK(choiceAt(*during, "printer", mt::kPrinter).plugged);   // attached at once, what is plugged in
    CHECK(r.callWhileBusy() > 0);
    const auto* after = r.fe.state<ControlState>(kControlStateTopic);
    CHECK(after && !r.session->busy());
    if (!after) return;
    CHECK(choiceAt(*after, "printer", mt::kPrinter).plugged);
    CHECK(choiceAt(*after, "printer", mt::kPrinter).available == kYes);
    CHECK(choiceAt(*after, "aux", mt::kUart).available == no(mt::kTogether));   // the machine as it now is
}

// ── A product's rules ───────────────────────────────────────────────────

TEST(availability_product_entry_carried_unread) {
    auto m = mt::buildTestMachine(mt::kSeg);
    TestControlRules rules;
    const ControlState s = computeControlState(*m, testSetup(&rules), false);
    const ProductEntry* e = s.productEntry(kTestEntry);
    CHECK(e && e->value == rules.last);   // the very payload the product gave
    CHECK(calcu::sessions::toText(s.toValue()).find("\"product\":{\"test.entry\":{\"model\":\"test-seg\"}}") !=
          std::string::npos);
    rules.entry = false;
    CHECK(computeControlState(*m, testSetup(&rules), false).productEntries.empty());
}

TEST(availability_product_refuses_generic_entry_with_its_reason) {
    auto m = mt::buildTestMachine(mt::kDot);
    TestControlRules rules;
    rules.refuse(ModelKey{mt::kSeg});
    rules.refuse(VariantKey{VariantId{"V2"}});
    rules.refuse(ActionKey{kResetAction}, "Not now, says the product.");
    rules.refuse(InsertKey{MediaPlaceId{"drive"}, {}});
    rules.refuse(ChoiceKey{at({"b"}), std::nullopt, std::nullopt});   // the empty choice of b
    const ControlState s = computeControlState(*m, testSetup(&rules), false);
    CHECK(s.models[0].available == no(kProductReason) && s.models[1].available == kYes);
    CHECK(s.variants[0].available == kYes && s.variants[1].available == no(kProductReason));
    CHECK(s.action(kResetAction)->available == no("Not now, says the product."));
    CHECK(s.action(kColdBootAction)->available == kYes);
    CHECK(s.mediaPlace(MediaPlaceId{"drive"})->insert == no(kProductReason));
    CHECK(s.place(at({"b"}))->empty == no(kProductReason));
    CHECK(s.place(at({"b"}))->available == kYes);
    CHECK(s.place(at({"a"}))->empty == kYes);
}

TEST(availability_refused_place_passes_reason_to_its_choices) {
    auto m = mt::buildTestMachine(mt::kSeg, std::nullopt, {plug("slot-1", mt::kModule)});
    TestControlRules rules;
    rules.refuse(PlaceKey{at({"slot-1"})}, "This frontend cannot use the slot.");
    const ControlState s = computeControlState(*m, testSetup(&rules), false);
    const Availability why = no("This frontend cannot use the slot.");
    const PlaceEntry* slot = s.place(at({"slot-1"}));
    CHECK(slot && slot->available == why && slot->empty == why);
    if (!slot) return;
    for (const auto& c : slot->fitting) {
        CHECK(c.available == why);
        for (const auto& v : c.variants) CHECK(v.available == why);
    }
    CHECK(s.place(at({"printer"}))->available == kYes);
    // While an operation runs, that comes first for the choices.
    const ControlState busy = computeControlState(*m, testSetup(&rules), true);
    CHECK(busy.place(at({"slot-1"}))->empty == kBusy);
    CHECK(busy.place(at({"slot-1"}))->available == why);
}
