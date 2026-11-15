// LIB-1, Calc-U-59 column, the M-19 part (LIB-1 design §1; C59-1 design T2,
// T13): the catalogue's three descriptors (AC-26 to AC-31) and a running
// machine of each model against its descriptor, without peripherals (AC-33;
// the media place clause passes in M-55, LIB-1 D-1; AC-32 and AC-33 with
// peripherals in M-25).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../core/TIKeys.hpp"
#include "machine/Conformance.hpp"
#include <algorithm>

using namespace calcu::machine;

namespace {

const char* const kModels[] = {"TI-59", "TI-58", "TI-58C"};

const MachineDescriptor& model(const char* id) { return *tiCatalogue().model(ModelId{id}); }

const ComponentDescriptor* component(const MachineDescriptor& d, const char* id) {
    for (const auto& c : d.components)
        if (c.id.value == id) return &c;
    return nullptr;
}

} // namespace

TEST(lib1_ac26_models_and_tick_rates) {
    const auto& models = tiCatalogue().models();
    CHECK_EQ(models.size(), size_t{3});
    for (const char* m : kModels) {
        CHECK(tiCatalogue().model(ModelId{m}) != nullptr);
        CHECK(model(m).variants.empty());
        CHECK(!model(m).defaultVariant.has_value());
        CHECK(model(m).family == kTIFamily);
    }
    CHECK_EQ(model("TI-59").tickRate, TickRate{455'000});
    CHECK_EQ(model("TI-58").tickRate, TickRate{384'000});
    CHECK_EQ(model("TI-58C").tickRate, TickRate{394'000});
}

TEST(lib1_ac27_segment_display_and_no_sound) {
    for (const char* m : kModels) {
        const auto& d = model(m);
        const auto* g = std::get_if<SegmentGeometry>(&d.display.geometry);
        CHECK(g != nullptr);
        if (!g) continue;
        CHECK_EQ(g->positions, 12);
        CHECK(g->positionBrightness);
        std::vector<std::string> names;
        for (const auto& s : g->layout.segments) {
            names.push_back(s.name);
            CHECK_EQ(s.ownBrightness, s.name == "DP");   // a separate brightness for the decimal point only
        }
        CHECK(names == (std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "DP"}));
        CHECK_EQ(d.display.indicators.size(), size_t{1});
        CHECK(d.display.indicators[0].name == "C" && d.display.indicators[0].hasBrightness);
        CHECK(!d.hasSound);
    }
}

TEST(lib1_ac28_rom_entries_per_model) {
    for (const char* m : kModels) {
        const auto& d = model(m);
        std::vector<std::string> files;
        for (const auto& e : d.romsFor(std::nullopt)) {
            files.push_back(e.fileName);
            CHECK(e.encoding == RomEncoding::Text);
            CHECK_EQ(e.md5.size(), size_t{32});
        }
        const bool c = std::string(m) == "TI-58C";
        const std::vector<std::string> want = c
            ? std::vector<std::string>{"CD2400.txt", "CD2401.txt", "TMC0573.txt", "CD2400-CONST-K.txt", "CD2401-CONST-K.txt"}
            : std::vector<std::string>{"TMC0582.txt", "TMC0583.txt", "TMC0571B.txt", "TMC0582-CONST-K.txt", "TMC0583-CONST-K.txt"};
        CHECK(files == want);
    }
}

TEST(lib1_ac29_keys_connectors_places_debug_target) {
    for (const char* m : kModels) {
        const auto& d = model(m);
        CHECK_EQ(d.keys.size(), size_t{45});
        for (size_t i = 0; i < d.keys.size() && i < tiKeys().size(); ++i) CHECK(d.keys[i].id.value == tiKeys()[i].id);
        for (const char* printerKey : {"PRINT", "ADV", "TRACE"}) CHECK(!d.hasKey(KeyId{printerKey}));
        CHECK_EQ(d.connectors.size(), size_t{2});
        const auto* slot = d.connector(ConnectorId{"module-slot"});
        const auto* prn = d.connector(ConnectorId{"printer-connector"});
        CHECK(slot && slot->form == "8 contacts");
        CHECK(prn && prn->form == "11 pads");
        for (const auto* c : {slot, prn}) {
            if (!c) continue;
            CHECK_EQ(c->pinOuts.size(), size_t{1});
            CHECK(c->pinOuts[0].level == PinOutLevel::Functional);
            CHECK(c->pinOuts[0].takes == Takes::One);
        }
        CHECK_EQ(d.places.size(), size_t{2});
        const auto* s1 = d.place(PlaceName{"slot-1"});
        const auto* pp = d.place(PlaceName{"printer"});
        CHECK(s1 && s1->connector.value == "module-slot" && s1->takesAtOnce == 1);
        CHECK(pp && pp->connector.value == "printer-connector" && pp->takesAtOnce == 1);
        CHECK(tiCatalogue().place(d.model, PlaceName{"slot-1"}).has_value());
        CHECK(tiCatalogue().place(d.model, PlaceName{"printer"}).has_value());
        CHECK(d.debugTargets == (std::vector<DebugTargetId>{DebugTargetId{"TMC0501"}}));
    }
}

TEST(lib1_ac30_card_reader_on_ti59_only) {
    const auto& ti59 = model("TI-59");
    CHECK_EQ(ti59.mediaPlaces.size(), size_t{1});
    if (!ti59.mediaPlaces.empty()) {
        const auto& r = ti59.mediaPlaces[0];
        CHECK(r.id.value == "card-reader");
        CHECK(r.passage == MediaPassage::PassThrough);
        CHECK(r.sides == (std::vector<int>{0, 1}));
    }
    CHECK(model("TI-58").mediaPlaces.empty());
    CHECK(model("TI-58C").mediaPlaces.empty());
}

TEST(lib1_ac31_components_and_depth) {
    for (const char* m : kModels) {
        const auto& d = model(m);
        const auto* cpu = component(d, "TMC0501");
        CHECK(cpu && cpu->depth == Depth::Separate);
        for (const char* folded : {"SCOM", "CROM", "printer-interface"}) {
            const auto* c = component(d, folded);
            CHECK(c && c->depth == Depth::Folded && c->foldedInto && c->foldedInto->value == "TMC0501");
        }
        for (const char* separate : {"ROM", "RAM"}) {
            const auto* c = component(d, separate);
            CHECK(c && c->depth == Depth::Separate && !c->foldedInto);
        }
        for (const char* functional : {"module-slot", "printer-connector"}) {
            const auto* c = component(d, functional);
            CHECK(c && c->depth == Depth::Functional);
        }
        const auto* reader = component(d, "card-reader");
        CHECK_EQ(reader != nullptr, std::string(m) == "TI-59");
        if (reader) CHECK(reader->depth == Depth::Folded && reader->foldedInto->value == "TMC0501");
    }
}

TEST(lib1_ac33_machine_conforms_without_peripherals) {
    for (const char* m : kModels) {
        auto machine = cu59::buildMachine(*cu59::modelFrom(ModelId{m}), *testroms::romSet());
        const auto diffs = checkAgainstDescriptor(*machine, tiCatalogue());
        for (const auto& d : diffs) std::printf("  %s: %s\n", m, d.c_str());
        CHECK(diffs.empty());
        CHECK(machine->model().value == m);
        CHECK(machine->descriptor().tickRate == model(m).tickRate);
        CHECK(machine->plugged().byPlace.empty());
        CHECK(machine->capabilities().empty());   // the card reader's media place: M-55 (LIB-1 D-1)
    }
}
