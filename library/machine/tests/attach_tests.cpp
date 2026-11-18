// Attach, detach and replace, with a product rule on the test catalogue.
// The machines and peripherals of
// TestMachines.hpp.
#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "machine/Attach.hpp"
#include "peripherals/SerialPort.hpp"
#include <algorithm>

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
PlacePath at(std::initializer_list<const char*> steps) {
    PlacePath p;
    for (const char* s : steps) p.steps.push_back(PlaceName{s});
    return p;
}

PeripheralRequest request(const Catalogue& cat, PlacePath place, const PeripheralId& id,
                          std::optional<VariantId> variant = std::nullopt) {
    PeripheralRequest r = peripheralAt(cat, "x", id, variant);
    r.place = std::move(place);
    return r;
}

bool holds(const Machine& m, const PlacePath& place, const PeripheralId& id) {
    const auto p = m.plugged().byPlace;
    return std::any_of(p.begin(), p.end(), [&](const PlugEntry& e) { return e.place == place && e.peripheral == id; });
}

std::size_t ownedBy(const Machine& m, const PeripheralId& id, const PlacePath& place) {
    std::size_t n = 0;
    for (const auto& e : m.capabilities())
        if (const auto* o = std::get_if<PeripheralOwner>(&e.owner); o && o->peripheral == id && o->place == place) ++n;
    return n;
}

// What a machine shows from outside: plugged in and capabilities.
bool samePlugged(const Machine& a, const Plugged& b) {
    const auto p = a.plugged().byPlace;
    if (p.size() != b.byPlace.size()) return false;
    for (std::size_t i = 0; i < p.size(); ++i)
        if (p[i].place != b.byPlace[i].place || p[i].peripheral != b.byPlace[i].peripheral ||
            p[i].variant != b.byPlace[i].variant)
            return false;
    return true;
}
std::vector<std::string> capIds(const Machine& m) {
    std::vector<std::string> out;
    for (const auto& e : m.capabilities()) out.push_back(e.id.value);
    return out;
}
} // namespace

// ── Pin-outs and fit ─────────────────────────────────────────────────────

TEST(pinout_states_level_takes_and_after) {
    const Catalogue& cat = testCatalogue();
    const auto chain = cat.place(kDot, PlaceName{"a"});
    const auto card = cat.place(kDot, PlaceName{"card-1"});
    const auto bus = cat.place(kSeg, PlaceName{"printer"});
    const auto jack = cat.place(kHub, PlaceName{"jack"});
    CHECK(chain && card && bus && jack);
    if (!chain || !card || !bus || !jack) return;
    CHECK(chain->pinOut->level == PinOutLevel::ContactLevel && chain->pinOut->takes == Takes::Chain &&
          chain->pinOut->after == AfterChange::PowerCycle);
    CHECK(card->pinOut->level == PinOutLevel::Functional && card->pinOut->takes == Takes::One &&
          card->pinOut->after == AfterChange::PowerCycle);
    CHECK(bus->pinOut->takes == Takes::Chain && bus->pinOut->after == AfterChange::Nothing);
    CHECK(jack->pinOut->after == AfterChange::Nothing);
    // What follows is read from the pin-out: the same peripheral, two answers.
    auto dot = buildTestMachine(kDot);
    auto seg = buildTestMachine(kSeg);
    CHECK(dot && seg);
    if (!dot || !seg) return;
    CHECK(dot->attach(request(cat, at({"a"}), kUart), cat).follows == AfterChange::PowerCycle);
    CHECK(seg->attach(request(cat, at({"aux"}), kUart), cat).follows == AfterChange::Nothing);
}

TEST(fit_follows_pinout_also_for_added_peripheral) {
    Catalogue cat = makeTestCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    // A place presents exactly one pin-out; fit is "the pin-out is in fits".
    for (const auto& p : cat.model(kDot)->places) {
        const auto info = cat.place(kDot, p.name);
        CHECK(info && info->pinOut->id == p.pinOut);
        for (const auto& per : cat.peripherals()) {
            const bool fits = per.fitsPinOut(p.pinOut);
            CHECK_EQ(std::find(info->fitting.begin(), info->fitting.end(), per.id) != info->fitting.end(), fits);
            // On the empty machine, a fitting peripheral is taken, any other refused.
            CHECK_EQ(!m->canAttach(request(cat, at({p.name.value.c_str()}), per.id), cat).has_value(), fits);
        }
    }
    // A module definition read after the catalogue was built.
    PeripheralDescriptor later;
    later.id = PeripheralId{"test-module-later"};
    later.name = "Later module";
    later.fits = {PinOutId{"test.card"}};
    cat.addPeripheral(later);
    const auto info = cat.place(kDot, PlaceName{"card-1"});
    CHECK(info && std::find(info->fitting.begin(), info->fitting.end(), later.id) != info->fitting.end());
    const ChangeResult r = m->attach(request(cat, at({"card-1"}), later.id), cat);
    CHECK(!r.refused && holds(*m, at({"card-1"}), later.id));
    CHECK(m->canAttach(request(cat, at({"a"}), later.id), cat).has_value());   // it fits only test.card
}

TEST(place_takes_its_count_chain_holds_several) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    // a and b take one each; the chain connector holds both at once.
    CHECK(!m->attach(request(cat, at({"a"}), kHub), cat).refused);
    CHECK(!m->attach(request(cat, at({"b"}), kUart), cat).refused);
    CHECK(m->canAttach(request(cat, at({"b"}), kHub), cat).has_value());
    // The hub's jack takes two (the UART out first: the product rule).
    CHECK(!m->detach(at({"b"}), kUart, cat).refused);
    CHECK(!m->attach(request(cat, at({"a", "jack"}), kPrinter), cat).refused);
    CHECK(!m->attach(request(cat, at({"a", "jack"}), kPrinter), cat).refused);
    const auto third = m->attach(request(cat, at({"a", "jack"}), kPrinter), cat);
    CHECK(third.refused && third.refused->reason == "a/jack already holds the Test printer.");
    std::size_t inJack = 0;
    for (const auto& e : m->plugged().byPlace) inJack += e.place == at({"a", "jack"}) ? 1 : 0;
    CHECK_EQ(inJack, std::size_t{2});
    // A Takes::One connector holds one over all its places.
    CHECK(!m->attach(request(cat, at({"card-1"}), kModule), cat).refused);
    const auto other = m->canAttach(request(cat, at({"card-2"}), kModule), cat);
    CHECK(other && other->reason == "card-1 already holds the Test module.");
}

// ── Attach and detach ────────────────────────────────────────────────────

TEST(attach_lists_peripheral_and_capabilities_and_says_what_follows) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    m->advance(1000);   // between two advances
    CHECK(!holds(*m, at({"b"}), kPrinter) && ownedBy(*m, kPrinter, at({"b"})) == 0);
    const ChangeResult r = m->attach(request(cat, at({"b"}), kPrinter), cat);
    CHECK(!r.refused);
    CHECK(r.follows == AfterChange::PowerCycle);   // test.chain says so, the machine is on
    CHECK(holds(*m, at({"b"}), kPrinter));
    // Its capabilities, owned by the peripheral at its place: the product kind and controls.
    CHECK_EQ(ownedBy(*m, kPrinter, at({"b"})), std::size_t{2});
    for (const auto& e : m->capabilities())
        if (const auto* o = std::get_if<PeripheralOwner>(&e.owner)) CHECK(m->capability(e.id) != nullptr && o->place == at({"b"}));
    // By connector as well.
    bool onChain = false;
    for (const auto& c : m->plugged().byConnector)
        if (c.connector == ConnectorId{"chain"} && c.owner.steps.empty())
            for (const auto& e : c.plugged) onChain = onChain || (e.peripheral == kPrinter && e.place == at({"b"}));
    CHECK(onChain);
    // The variant to use is the one asked for (or the default).
    CHECK(!m->attach(request(cat, at({"card-2"}), kModule, VariantId{"8k"}), cat).refused);
    for (const auto& e : m->plugged().byPlace)
        if (e.peripheral == kModule) CHECK(e.variant == std::optional<VariantId>(VariantId{"8k"}));
}

TEST(switched_off_machine_says_nothing_follows) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    m->keyDown(KeyId{"off"});
    m->keyUp(KeyId{"off"});
    const ChangeResult a = m->attach(request(cat, at({"a"}), kUart), cat);
    CHECK(!a.refused && a.follows == AfterChange::Nothing && holds(*m, at({"a"}), kUart));
    const ChangeResult d = m->detach(at({"a"}), kUart, cat);
    CHECK(!d.refused && d.follows == AfterChange::Nothing && !holds(*m, at({"a"}), kUart));
    // Switched on again, the pin-out's answer.
    m->keyDown(KeyId{"on"});
    CHECK(m->attach(request(cat, at({"a"}), kUart), cat).follows == AfterChange::PowerCycle);
}

TEST(detach_removes_peripheral_and_capabilities) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    const auto before = capIds(*m);
    CHECK(!m->attach(request(cat, at({"a"}), kUart), cat).refused);
    CHECK(capIds(*m).size() == before.size() + 1);
    m->advance(50'000);
    const ChangeResult r = m->detach(at({"a"}), kUart, cat);
    CHECK(!r.refused && r.follows == AfterChange::PowerCycle);
    CHECK(!holds(*m, at({"a"}), kUart));
    CHECK(capIds(*m) == before);
    CHECK(m->capabilities(calcu::peripherals::SerialPort::kKind).empty());
    // On a pin-out that says nothing follows.
    auto seg = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "printer", kPrinter)});
    CHECK(seg && seg->detach(at({"printer"}), kPrinter, cat).follows == AfterChange::Nothing);
    CHECK(seg && seg->plugged().byPlace.empty());
}

TEST(can_attach_agrees_with_attach_refusal_changes_nothing) {
    Catalogue cat = makeTestCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart), peripheralAt(cat, "card-1", kModule)});
    CHECK(m != nullptr);
    if (!m) return;
    PeripheralRequest noRom = request(cat, at({"card-2"}), kModule);
    noRom.roms.clear();
    struct Case {
        PeripheralRequest req;
        std::string reason;
    };
    std::vector<Case> cases{
        {request(cat, at({"slot-1"}), kModule), "The test-dot has no place slot-1."},
        {request(cat, at({"a", "jack"}), kPrinter), "The test-dot has no place a/jack."},   // no hub at a
        {request(cat, at({"b"}), PeripheralId{"no-such"}), "no-such is not a known peripheral."},
        {request(cat, at({"b"}), kModule), "The Test module does not fit b."},
        {request(cat, at({"card-2"}), kModule, VariantId{"16k"}), "The Test module has no variant 16k."},
        {noRom, "The Test module could not be attached: test-module-4k.rom is missing."},
        {request(cat, at({"a"}), kHub), "a already holds the Test UART."},
        {request(cat, at({"card-2"}), kModule), "card-1 already holds the Test module."},
        {request(cat, at({"b"}), kPrinter), kTogether},
    };
    for (const auto& c : cases) {
        const Plugged before = m->plugged();
        const auto capsBefore = capIds(*m);
        const int plugsBefore = m->plugCalls;
        const auto answer = m->canAttach(c.req, cat);
        const ChangeResult r = m->attach(c.req, cat);
        CHECK(answer.has_value() && r.refused.has_value());
        if (!answer || !r.refused) continue;
        CHECK_EQ(answer->reason, c.reason);
        CHECK_EQ(r.refused->reason, answer->reason);   // the same reason
        CHECK(r.follows == AfterChange::Nothing);
        CHECK(samePlugged(*m, before) && capIds(*m) == capsBefore && m->plugCalls == plugsBefore);
    }
    // Yes, and then the attach goes through.
    const PeripheralRequest ok = request(cat, at({"b"}), kHub);
    CHECK(!m->canAttach(ok, cat).has_value());
    CHECK(!m->attach(ok, cat).refused && holds(*m, at({"b"}), kHub));
}

TEST(full_place_refuses) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "slot-1", kModule)});
    CHECK(m != nullptr);
    if (!m) return;
    const ChangeResult r = m->attach(request(cat, at({"slot-1"}), kModule, VariantId{"8k"}), cat);
    CHECK(r.refused && r.refused->reason == "slot-1 already holds the Test module.");
    CHECK_EQ(m->plugged().byPlace.size(), std::size_t{1});
}

TEST(replace_detaches_attaches_asks_once) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "card-1", kModule)});
    CHECK(m != nullptr);
    if (!m) return;
    const int plugs = m->plugCalls, unplugs = m->unplugCalls;
    // Another module in the same place: one step, although the place is full.
    PeripheralRequest other = request(cat, at({"card-1"}), kModule, VariantId{"8k"});
    const ChangeResult r = m->replace(kModule, other, cat);
    CHECK(!r.refused && r.follows == AfterChange::PowerCycle);
    CHECK_EQ(m->unplugCalls, unplugs + 1);
    CHECK_EQ(m->plugCalls, plugs + 1);
    const auto p = m->plugged().byPlace;
    CHECK(p.size() == 1 && p[0].variant == std::optional<VariantId>(VariantId{"8k"}));
    // A replace of what is not there is refused and changes nothing.
    const ChangeResult wrong = m->replace(kHub, request(cat, at({"card-1"}), kModule), cat);
    CHECK(wrong.refused && wrong.refused->reason == "card-1 does not hold the Test hub.");
    CHECK_EQ(m->plugCalls, plugs + 1);
    // The new one is checked as an attach: a refusal keeps the old one in.
    const ChangeResult bad = m->replace(kModule, request(cat, at({"card-1"}), kHub), cat);
    CHECK(bad.refused && bad.refused->reason == "The Test hub does not fit card-1.");
    CHECK(holds(*m, at({"card-1"}), kModule));
    // The product rule sees what is taken out.
    auto u = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart)});
    CHECK(u && !u->replace(kUart, request(cat, at({"a"}), kPrinter), cat).refused && holds(*u, at({"a"}), kPrinter));
}

TEST(chain_holds_peripherals_in_their_places) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->attach(request(cat, at({"a"}), kHub), cat).refused);
    CHECK(!m->attach(request(cat, at({"b"}), kUart), cat).refused);
    CHECK(holds(*m, at({"a"}), kHub) && holds(*m, at({"b"}), kUart));
    for (const auto& c : m->plugged().byConnector)
        if (c.connector == ConnectorId{"chain"} && c.owner.steps.empty()) {
            CHECK_EQ(c.plugged.size(), std::size_t{2});
        }
}

TEST(peripheral_place_makes_a_tree_and_its_pinout_decides) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(m->attach(request(cat, at({"a"}), kHub), cat).follows == AfterChange::PowerCycle);   // test.chain
    const ChangeResult r = m->attach(request(cat, at({"a", "jack"}), kPrinter), cat);
    CHECK(!r.refused && r.follows == AfterChange::Nothing);   // test.jack, on the hub
    // machine, place a, the hub, place jack, the printer.
    CHECK(holds(*m, at({"a"}), kHub) && holds(*m, at({"a", "jack"}), kPrinter));
    bool jack = false;
    for (const auto& c : m->plugged().byConnector)
        if (c.owner == at({"a"}) && c.connector == ConnectorId{"jack"})
            jack = c.plugged.size() == 1 && c.plugged[0].peripheral == kPrinter;
    CHECK(jack);
    CHECK(m->capability(CapabilityId{"a/jack/controls"}) != nullptr);
    CHECK(m->detach(at({"a", "jack"}), kPrinter, cat).follows == AfterChange::Nothing);
}

TEST(operations_keep_peripherals_and_tell_each) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->attach(request(cat, at({"a"}), kHub), cat).refused);
    CHECK(!m->attach(request(cat, at({"a", "jack"}), kPrinter), cat).refused);
    CHECK(!m->attach(request(cat, at({"card-1"}), kModule), cat).refused);
    const Plugged before = m->plugged();
    std::uint64_t told = 0;
    for (auto op : {&Machine::reset, &Machine::powerCycle, &Machine::coldBoot}) {
        (m.get()->*op)();
        runUntilKeyReady(*m);
        ++told;
        CHECK(samePlugged(*m, before));
        for (const auto& e : before.byPlace) CHECK_EQ(m->operationsTold(e.place), told);
    }
}

TEST(attach_reads_no_file) {
    // Every byte in memory, file names that exist nowhere: the peripheral gets
    // exactly these bytes (and Attach.cpp has no file call: review).
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    PeripheralRequest r = request(cat, at({"slot-1"}), kModule, VariantId{"8k"});
    r.roms = {RomImage{"test-module-8k.rom", {0xDE, 0xAD, 0xBE, 0xEF}}};
    CHECK(!m->attach(r, cat).refused);
    const auto* roms = m->romsAt(at({"slot-1"}));
    CHECK(roms && roms->size() == 1 && (*roms)[0].bytes == std::vector<std::uint8_t>({0xDE, 0xAD, 0xBE, 0xEF}));
}

TEST(product_refusal_reaches_caller) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kPrinter)});
    CHECK(m != nullptr);
    if (!m) return;
    const ChangeResult r = m->attach(request(cat, at({"b"}), kUart), cat);
    CHECK(r.refused && r.refused->reason == kTogether);
    CHECK(m->canAttach(request(cat, at({"b"}), kUart), cat)->reason == kTogether);
    CHECK(!holds(*m, at({"b"}), kUart));
}

// ── Edge cases ──────────────────────────────────────────────────────────

TEST(unfitting_peripheral_refused_with_reason) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    const ChangeResult r = m->attach(request(cat, at({"slot-1"}), kPrinter), cat);
    CHECK(r.refused && r.refused->reason == "The Test printer does not fit slot-1.");
    CHECK(m->plugged().byPlace.empty());
}

TEST(detach_of_empty_place_changes_nothing) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "a", kUart)});
    CHECK(m != nullptr);
    if (!m) return;
    const Plugged before = m->plugged();
    const int unplugs = m->unplugCalls;
    for (const ChangeResult& r : {m->detach(at({"b"}), kUart, cat), m->detach(at({"a"}), kHub, cat),
                                  m->detach(at({"nowhere"}), kUart, cat)}) {
        CHECK(!r.refused && r.follows == AfterChange::Nothing);
    }
    CHECK(samePlugged(*m, before) && m->unplugCalls == unplugs);
}

TEST(detach_takes_children_asks_once) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(!m->attach(request(cat, at({"a"}), kHub), cat).refused);
    CHECK(!m->attach(request(cat, at({"a", "jack"}), kPrinter), cat).refused);
    CHECK(!m->attach(request(cat, at({"a", "jack"}), kPrinter), cat).refused);
    const int unplugs = m->unplugCalls;
    const ChangeResult r = m->detach(at({"a"}), kHub, cat);
    CHECK(!r.refused && r.follows == AfterChange::PowerCycle);   // at the hub's place: test.chain
    CHECK_EQ(m->unplugCalls, unplugs + 1);
    CHECK(m->plugged().byPlace.empty());
    CHECK(m->capabilities(calcu::peripherals::Controls::kKind).empty());
}

TEST(missing_rom_named) {
    const Catalogue& cat = testCatalogue();
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    PeripheralRequest r = request(cat, at({"slot-1"}), kModule, VariantId{"8k"});
    r.roms = {RomImage{"test-module-4k.rom", {1}}};   // another variant's file
    const ChangeResult res = m->attach(r, cat);
    CHECK(res.refused.has_value());
    if (!res.refused) return;
    CHECK_EQ(res.refused->reason, std::string("The Test module could not be attached: test-module-8k.rom is missing."));
    CHECK(res.refused->missingRoms == std::vector<std::string>{"test-module-8k.rom"});
    CHECK(m->plugged().byPlace.empty());
}

// canReplace gives the refusal replace()
// would give, and changes nothing.
TEST(can_replace_answers_as_replace_does) {
    const Catalogue& cat = testCatalogue();
    auto make = [&] {
        return buildTestMachine(kDot, std::nullopt, {peripheralAt(cat, "card-1", kModule), peripheralAt(cat, "b", kUart)});
    };
    auto m = make();
    CHECK(m != nullptr);
    if (!m) return;
    const std::vector<std::pair<PeripheralId, PeripheralRequest>> cases = {
        {kModule, request(cat, at({"card-1"}), kModule, VariantId{"8k"})},   // allowed
        {kHub, request(cat, at({"card-1"}), kModule)},                        // not there
        {kModule, request(cat, at({"card-1"}), kHub)},                        // does not fit
        {kModule, request(cat, at({"card-1"}), kModule, VariantId{"2k"})},   // no such variant
        {kUart, request(cat, at({"b"}), kPrinter)},                           // allowed: the UART goes out
    };
    for (const auto& [old, req] : cases) {
        const int plugs = m->plugCalls, unplugs = m->unplugCalls;
        const Plugged before = m->plugged();
        const auto asked = m->canReplace(old, req, cat);
        CHECK(samePlugged(*m, before));
        CHECK_EQ(m->plugCalls, plugs);
        CHECK_EQ(m->unplugCalls, unplugs);
        auto other = make();
        const ChangeResult done = other->replace(old, req, cat);
        CHECK_EQ(asked.has_value(), done.refused.has_value());
        if (asked && done.refused) CHECK_EQ(asked->reason, done.refused->reason);
    }
    CHECK(!m->canReplace(kModule, cases[0].second, cat));
    const auto notThere = m->canReplace(kHub, cases[1].second, cat);
    CHECK(notThere && notThere->reason == "card-1 does not hold the Test hub.");
}
