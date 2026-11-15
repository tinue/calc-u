#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <algorithm>
#include <set>

// LIB-1 AC-20, AC-21, AC-22, AC-23, AC-25, EC-5 (design §6, T12).

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
PlacePath at(const char* place) { return PlacePath{{PlaceName{place}}}; }

std::unique_ptr<TestMachine> fullSeg() {
    const Catalogue& cat = testCatalogue();
    return buildTestMachine(kSeg, std::nullopt,
                            {peripheralAt(cat, "slot-1", kModule), peripheralAt(cat, "printer", kPrinter),
                             peripheralAt(cat, "aux", kModule, VariantId{"8k"})});
}
} // namespace

TEST(ac20_plugged_per_place_and_per_connector) {
    auto m = fullSeg();
    CHECK(m != nullptr);
    if (!m) return;
    const Plugged p = m->plugged();
    CHECK_EQ(p.byPlace.size(), std::size_t{3});
    auto inPlace = [&](const char* place, const PeripheralId& id) {
        return std::any_of(p.byPlace.begin(), p.byPlace.end(),
                           [&](const PlugEntry& e) { return e.place == at(place) && e.peripheral == id; });
    };
    CHECK(inPlace("slot-1", kModule) && inPlace("printer", kPrinter) && inPlace("aux", kModule));
    // Per connector; the chain on `bus` gives each peripheral under its own place.
    CHECK_EQ(p.byConnector.size(), std::size_t{2});
    const ConnectorPlugs* bus = nullptr;
    const ConnectorPlugs* slot = nullptr;
    for (const auto& c : p.byConnector) {
        if (c.connector == ConnectorId{"bus"}) bus = &c;
        if (c.connector == ConnectorId{"slot"}) slot = &c;
    }
    CHECK(slot && slot->owner.steps.empty() && slot->plugged.size() == 1);
    CHECK(bus && bus->plugged.size() == 2);
    if (bus && bus->plugged.size() == 2) {
        CHECK(bus->plugged[0].place == at("printer") && bus->plugged[0].peripheral == kPrinter);
        CHECK(bus->plugged[1].place == at("aux") && bus->plugged[1].variant == VariantId{"8k"});
    }
    // The descriptor is not changed by what is plugged in.
    const MachineDescriptor& d = m->descriptor();
    const MachineDescriptor& c = *testCatalogue().model(kSeg);
    CHECK(d.places.size() == c.places.size() && d.connectors.size() == c.connectors.size());
    // Nothing plugged: every connector, none on it.
    auto bare = buildTestMachine(kSeg);
    CHECK(bare && bare->plugged().byPlace.empty() && bare->plugged().byConnector.size() == 2);
}

TEST(ac21_capabilities_listed_with_kind_id_owner) {
    auto m = fullSeg();
    auto twin = fullSeg();
    CHECK(m && twin);
    if (!m || !twin) return;
    const auto caps = m->capabilities();
    CHECK_EQ(caps.size(), std::size_t{3});   // with LIB-2's controls of the printer
    std::set<std::string> ids;
    for (const auto& e : caps) ids.insert(e.id.value);
    CHECK_EQ(ids.size(), caps.size());   // unique within the machine
    // Kind, ID and owner.
    const Owner cpu = ComponentOwner{ComponentId{"cpu"}};
    const Owner printer = PeripheralOwner{kPrinter, at("printer")};
    CHECK(caps[0].kind == TestGeneric::kKind && caps[0].id == CapabilityId{"cpu/test-generic"} && caps[0].owner == cpu);
    CHECK(caps[1].kind == TestCounter::kKind && caps[1].id == CapabilityId{"printer/test.counter"} &&
          caps[1].owner == printer);
    // The same for a machine built alike.
    const auto twinCaps = twin->capabilities();
    CHECK(twinCaps.size() == caps.size());
    for (std::size_t i = 0; i < std::min(caps.size(), twinCaps.size()); ++i)
        CHECK(twinCaps[i].id == caps[i].id && twinCaps[i].kind == caps[i].kind && twinCaps[i].owner == caps[i].owner);
    // The typed interface on request; nothing for a wrong kind or an absent ID.
    CHECK(capabilityAs<TestGeneric>(*m, caps[0].id) != nullptr);
    CHECK(capabilityAs<TestCounter>(*m, caps[0].id) == nullptr);
    CHECK(m->capability(CapabilityId{"no/such"}) == nullptr);
}

TEST(ac22_peripheral_capabilities_only_with_peripheral) {
    auto with = buildTestMachine(kSeg, std::nullopt, {peripheralAt(testCatalogue(), "printer", kPrinter)});
    auto without = buildTestMachine(kSeg);
    CHECK(with && without);
    if (!with || !without) return;
    CHECK_EQ(capabilitiesOf<TestCounter>(*with).size(), std::size_t{1});
    CHECK_EQ(capabilitiesOf<TestCounter>(*without).size(), std::size_t{0});
    CHECK(capabilityAs<TestCounter>(*without, CapabilityId{"printer/test.counter"}) == nullptr);
    CHECK_EQ(with->capabilities().size(), without->capabilities().size() + 2);   // the counter and LIB-2's controls
}

TEST(ac23_product_kind_handed_out_like_generic) {
    auto m = buildTestMachine(kSeg, std::nullopt, {peripheralAt(testCatalogue(), "printer", kPrinter)});
    CHECK(m != nullptr);
    if (!m) return;
    Machine& base = *m;   // through the interface only
    // Listed the same way...
    const auto product = capabilitiesOf<TestCounter>(base);
    const auto generic = capabilitiesOf<TestGeneric>(base);
    CHECK(product.size() == 1 && generic.size() == 1);
    CHECK(base.capabilities(KindId{"test.counter"}).size() == 1);
    // ...and handed out the same way.
    auto* counter = capabilityAs<TestCounter>(base, product[0].id);
    auto* instr = capabilityAs<TestGeneric>(base, generic[0].id);
    CHECK(counter && instr);
    if (!counter || !instr) return;
    base.advance(80);
    CHECK(counter->count > 0 && counter->count == instr->instructions);
}

TEST(ac25_debug_targets_from_descriptor) {
    for (const ModelId& id : {kSeg, kDot}) {
        auto m = buildTestMachine(id);
        CHECK(m && m->debugTargets() == testCatalogue().model(id)->debugTargets && !m->debugTargets().empty());
    }
}

TEST(ec5_absent_kind_gives_empty_list) {
    auto m = buildTestMachine(kDot);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(m->capabilities(KindId{"no-such-kind"}).empty());
    CHECK(capabilitiesOf<TestCounter>(*m).empty());
}
