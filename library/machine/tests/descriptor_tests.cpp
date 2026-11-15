#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "machine/Catalogue.hpp"
#include <algorithm>
#include <variant>

// LIB-1 AC-1, AC-2, AC-3, AC-4, AC-14, EC-6 (design §6, T3).

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
bool hasId(const std::vector<PeripheralId>& v, const PeripheralId& id) {
    return std::find(v.begin(), v.end(), id) != v.end();
}
} // namespace

TEST(ac1_catalogue_gives_descriptors_without_machine_or_rom) {
    // No machine is built and no ROM file exists for the test models.
    const Catalogue cat = makeTestCatalogue();
    CHECK_EQ(cat.models().size(), std::size_t{2});
    const MachineDescriptor* seg = cat.model(kSeg);
    const MachineDescriptor* dot = cat.model(kDot);
    CHECK(seg && seg->model == kSeg);
    CHECK(dot && dot->model == kDot);
    CHECK(cat.model(ModelId{"no-such-model"}) == nullptr);
    // The variants are inside the model's descriptor.
    CHECK(dot && dot->variants.size() == 2 && dot->variants[0].id == VariantId{"V1"});
}

TEST(ac2_machine_descriptor_states_every_part) {
    const Catalogue& cat = testCatalogue();
    const MachineDescriptor& seg = *cat.model(kSeg);
    CHECK(seg.family == ModelFamilyId{"test-family"});
    CHECK(seg.variants.empty() && !seg.defaultVariant);
    const auto& roms = seg.romsFor(std::nullopt);
    CHECK_EQ(roms.size(), std::size_t{1});
    CHECK_EQ(roms[0].fileName, std::string("test-seg.rom"));
    CHECK_EQ(roms[0].decodedAmount, std::uint64_t{2048});
    CHECK(roms[0].unit == ContentUnit::Bytes && roms[0].encoding == RomEncoding::Binary);
    CHECK_EQ(roms[0].md5.size(), std::size_t{32});
    CHECK_EQ(seg.tickRate, kSegRate);
    // Display kind, geometry, segment layout, indicators by name.
    const auto* g = std::get_if<SegmentGeometry>(&seg.display.geometry);
    CHECK(g != nullptr);
    if (g) {
        CHECK_EQ(g->positions, 3);
        CHECK_EQ(g->layout.segments.size(), std::size_t{8});
        CHECK_EQ(g->layout.segments.back().name, std::string("DP"));
        CHECK(g->layout.segments.back().ownBrightness && !g->layout.segments[0].ownBrightness);
        CHECK(g->positionBrightness);
    }
    CHECK(seg.display.indicators.size() == 1 && seg.display.indicators[0].name == "C" &&
          seg.display.indicators[0].hasBrightness);
    CHECK(!seg.hasSound);
    CHECK(seg.hasKey(KeyId{"clr"}) && !seg.hasKey(KeyId{"a"}));
    // Connectors with their form and pin-outs.
    const ConnectorDescriptor* bus = seg.connector(ConnectorId{"bus"});
    CHECK(bus && bus->form == "11-pad" && bus->pinOuts.size() == 1 &&
          bus->pinOuts[0].id == PinOutId{"test.bus"} && bus->pinOuts[0].takes == Takes::Chain &&
          bus->pinOuts[0].level == PinOutLevel::Functional);
    // Places with the connector behind them and how many they take.
    const PlaceDescriptor* slot = seg.place(PlaceName{"slot-1"});
    CHECK(slot && slot->connector == ConnectorId{"slot"} && slot->takesAtOnce == 1);
    CHECK_EQ(seg.places.size(), std::size_t{3});
    // Components with what they stand for and their depth.
    CHECK_EQ(seg.components.size(), std::size_t{3});
    CHECK(seg.components[0].depth == Depth::Separate && seg.components[0].standsFor == "test CPU");
    CHECK(seg.components[1].depth == Depth::Folded && seg.components[1].foldedInto == ComponentId{"cpu"});
    CHECK(seg.components[2].depth == Depth::Functional);
    // Media places with kind, passage and sides.
    CHECK(seg.mediaPlaces.size() == 1 && seg.mediaPlaces[0].mediumKind == "test-card" &&
          seg.mediaPlaces[0].passage == MediaPassage::PassThrough &&
          seg.mediaPlaces[0].sides == std::vector<int>({0, 1}));
    CHECK(seg.debugTargets == std::vector<DebugTargetId>{DebugTargetId{"test-cpu"}});

    // The other display kind, and ROM entries per variant.
    const MachineDescriptor& dot = *cat.model(kDot);
    const auto* d = std::get_if<DotMatrixGeometry>(&dot.display.geometry);
    CHECK(d && d->width == 12 && d->height == 8 && d->cells && d->cells->cellWidth == 6 &&
          d->cells->cellHeight == 8 && d->cells->columns == 2 && d->cells->rows == 1);
    CHECK(dot.hasSound && dot.tickRate == kDotRate && dot.defaultVariant == VariantId{"V1"});
    CHECK_EQ(dot.romsFor(VariantId{"V2"}).size(), std::size_t{2});
    CHECK_EQ(dot.romsFor(VariantId{"V2"})[1].unit == ContentUnit::Rows, true);
    CHECK_EQ(dot.romsFor(std::nullopt)[0].fileName, std::string("test-dot-v1.rom"));
    CHECK(dot.romsFor(VariantId{"V9"}).empty());
    CHECK(dot.mediaPlaces.size() == 1 && dot.mediaPlaces[0].passage == MediaPassage::Holding);
}

TEST(ac3_peripheral_descriptor_and_one_added_later) {
    Catalogue cat = makeTestCatalogue();
    CHECK_EQ(cat.peripherals().size(), std::size_t{4});   // with LIB-2's hub and UART
    const PeripheralDescriptor* printer = cat.peripheral(kPrinter);
    const PeripheralDescriptor* module = cat.peripheral(kModule);
    CHECK(printer && printer->name == "Test printer" && printer->variants.empty() && printer->roms.empty());
    CHECK(printer && printer->fits.size() == 3 && printer->fits[0] == PinOutId{"test.bus"});
    CHECK(printer && printer->controls.size() == 3 && printer->controls[0].form == ControlForm::Switch);
    CHECK(printer && printer->components.size() == 1 && printer->components[0].depth == Depth::Functional);
    CHECK(printer && printer->connectors.empty() && printer->places.empty() && printer->mediaPlaces.empty());
    CHECK(module && module->defaultVariant == VariantId{"4k"} && module->variants.size() == 2);
    CHECK(module && module->romsFor(VariantId{"8k"})[0].fileName == "test-module-8k.rom");
    CHECK(module && module->romsFor(std::nullopt)[0].fileName == "test-module-4k.rom");

    // A module definition read after the catalogue was built.
    auto def = std::make_shared<TestModuleDefinition>("name: later\nfits: test.slot\n");
    PeripheralDescriptor later{PeripheralId{"test-module-later"}, "Later module", {}, std::nullopt,
                               {RomEntry{"later.rom", RomEncoding::Binary, 1024, ContentUnit::Bytes,
                                         "00000000000000000000000000000000"}},
                               {PinOutId{"test.slot"}}, {}, {}, {}, {}, {}, def};
    cat.addPeripheral(later);
    const PeripheralDescriptor* got = cat.peripheral(PeripheralId{"test-module-later"});
    CHECK(got && got->name == "Later module" && got->roms.size() == 1);
    CHECK(got && static_cast<const TestModuleDefinition*>(got->productData.get())->text.starts_with("name: later"));
    auto info = cat.place(kSeg, PlaceName{"slot-1"});
    CHECK(info && hasId(info->fitting, PeripheralId{"test-module-later"}));
}

TEST(ac4_place_gives_connector_and_fitting_peripherals) {
    const Catalogue& cat = testCatalogue();
    auto slot = cat.place(kSeg, PlaceName{"slot-1"});
    CHECK(slot.has_value());
    if (slot) {
        CHECK(slot->place->name == PlaceName{"slot-1"});
        CHECK(slot->connector->id == ConnectorId{"slot"});
        CHECK(slot->pinOut->id == PinOutId{"test.slot"});
        CHECK(slot->fitting == std::vector<PeripheralId>{kModule});
    }
    auto printer = cat.place(kSeg, PlaceName{"printer"});
    CHECK(printer && printer->connector->id == ConnectorId{"bus"});
    CHECK(printer && hasId(printer->fitting, kPrinter) && hasId(printer->fitting, kModule));
    CHECK(!cat.place(kSeg, PlaceName{"slot-2"}).has_value());
    CHECK(!cat.place(kDot, PlaceName{"slot-1"}).has_value());
    CHECK(!cat.place(ModelId{"no-such-model"}, PlaceName{"slot-1"}).has_value());
}

TEST(ac14_peripheral_controls_are_not_machine_keys) {
    const Catalogue& cat = testCatalogue();
    const PeripheralDescriptor* printer = cat.peripheral(kPrinter);
    CHECK(printer && printer->controls[0].id == ControlId{"online"});
    int clashes = 0;
    for (const auto& p : cat.peripherals())
        for (const auto& c : p.controls)
            for (const auto& m : cat.models())
                if (m.hasKey(KeyId{c.id.value})) ++clashes;
    CHECK_EQ(clashes, 0);
    // The machine refuses a control's name as a key.
    auto m = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "printer", kPrinter)});
    CHECK(m && m->keyDown(KeyId{"online"}) == KeyCall::UnknownKey);
}

TEST(ec6_no_variant_and_single_variant) {
    const Catalogue& cat = testCatalogue();
    const MachineDescriptor& seg = *cat.model(kSeg);
    CHECK(seg.variants.empty() && !seg.defaultVariant);
    auto m = buildTestMachine(kSeg);
    CHECK(m && !m->variant().has_value());
    // A variant named for a model without variants is refused.
    TestBuilder builder;
    auto bad = build(cat, builder, requestFor(cat, kSeg, VariantId{"V1"}));
    CHECK(!bad.machine && !bad.refusal.empty());

    // A model with a single variant: that variant is the default.
    MachineDescriptor one = *cat.model(kDot);
    one.model = ModelId{"test-dot-1"};
    one.variants.resize(1);
    one.defaultVariant = one.variants[0].id;
    Catalogue single({one}, {});
    CHECK(single.model(one.model)->defaultVariant == VariantId{"V1"});
    auto r = build(single, builder, requestFor(single, one.model));
    CHECK(r.machine && r.machine->variant() == VariantId{"V1"});
}
