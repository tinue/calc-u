// The catalogue of Calc-U-1600 and the running machines against it.

#include <algorithm>
#include <set>

#include "machine/Conformance.hpp"
#include "media/Conformance.hpp"
#include "media/MediaPlace.hpp"
#include "../app/SharpMediaKinds.hpp"

#include "SharpTestSupport.hpp"

using namespace calcu::machine;
using namespace sharptest;

namespace {

const MachineDescriptor& model(const char* id) { return *sharp::sharpCatalogue().model(ModelId{id}); }

std::vector<std::string> variantIds(const std::vector<VariantDescriptor>& v) {
    std::vector<std::string> out;
    for (const auto& x : v) out.push_back(x.id.value);
    return out;
}

const PinOutDescriptor* pinOutOf(const MachineDescriptor& d, const char* placeName) {
    const PlaceDescriptor* p = d.place(PlaceName{placeName});
    if (!p) return nullptr;
    const ConnectorDescriptor* c = d.connector(p->connector);
    if (!c) return nullptr;
    for (const auto& po : c->pinOuts)
        if (po.id == p->pinOut) return &po;
    return nullptr;
}

bool hasComponent(const MachineDescriptor& d, const char* id, Depth depth) {
    return std::any_of(d.components.begin(), d.components.end(),
                       [&](const ComponentDescriptor& c) { return c.id.value == id && c.depth == depth; });
}

} // namespace

TEST(models_variants_rates) {
    const Catalogue& c = sharp::sharpCatalogue();
    CHECK_EQ(c.models().size(), std::size_t{3});
    const MachineDescriptor& p15 = model("PC-1500");
    CHECK(variantIds(p15.variants) == (std::vector<std::string>{"A01", "A03", "A04"}));
    CHECK(p15.defaultVariant && p15.defaultVariant->value == "A04");
    CHECK_EQ(p15.tickRate, TickRate{1'300'000});
    const MachineDescriptor& p15a = model("PC-1500A");
    CHECK(variantIds(p15a.variants) == (std::vector<std::string>{"A04"}));
    CHECK(p15a.defaultVariant && p15a.defaultVariant->value == "A04");
    CHECK_EQ(p15a.tickRate, TickRate{1'300'000});
    const MachineDescriptor& p16 = model("PC-1600");
    CHECK(variantIds(p16.variants) == (std::vector<std::string>{"New", "Old"}));
    CHECK(p16.defaultVariant && p16.defaultVariant->value == "New");
    CHECK_EQ(p16.tickRate, TickRate{3'580'000});
    CHECK(p15.family == p16.family);
}

TEST(dot_matrix_indicators_sound) {
    for (const char* id : {"PC-1500", "PC-1500A", "PC-1600"}) {
        const MachineDescriptor& d = model(id);
        const bool p16 = std::string(id) == "PC-1600";
        const auto* g = std::get_if<DotMatrixGeometry>(&d.display.geometry);
        CHECK(g != nullptr);
        if (!g) continue;
        CHECK_EQ(g->width, 156);
        CHECK_EQ(g->height, p16 ? 32 : 7);
        CHECK_EQ(d.display.indicators.size(), std::size_t(p16 ? 18 : 14));
        CHECK(d.hasSound);
    }
}

TEST(connectors_pinouts_places_debug_targets) {
    for (const char* id : {"PC-1500", "PC-1500A"}) {
        const MachineDescriptor& d = model(id);
        const PinOutDescriptor* slot = pinOutOf(d, "slot-1");
        CHECK(slot && slot->takes == Takes::One && slot->level == PinOutLevel::ContactLevel);
        CHECK(slot && slot->id.value == "PC-1500 memory slot");
        CHECK(d.place(PlaceName{"slot-1"}) && d.place(PlaceName{"slot-1"})->takesAtOnce == 1);
        CHECK(d.connector(ConnectorId{"40-pin"}) != nullptr);
        // One pin-out per 60-pin place, each a chain: the places share
        // the one bus connector, one peripheral each (takesAtOnce 1).
        for (const char* p : {"printer", "interface"}) {
            const PinOutDescriptor* bus = pinOutOf(d, p);
            CHECK(bus && bus->takes == Takes::Chain && bus->level == PinOutLevel::ContactLevel);
            CHECK(bus && bus->id.value == std::string("pc1500-60-pin-") + p);
            CHECK(d.place(PlaceName{p})->connector.value == "60-pin");
        }
        CHECK(pinOutOf(d, "bus-rom") && pinOutOf(d, "bus-rom")->takes == Takes::Chain);
        CHECK(d.place(PlaceName{"slot-2"}) == nullptr);
        CHECK(d.place(PlaceName{"host-drive"}) == nullptr);
        CHECK_EQ(d.places.size(), std::size_t{4});   // and bus-rom
        CHECK(d.place(PlaceName{"bus-rom"}) && d.place(PlaceName{"bus-rom"})->connector.value == "60-pin");
        CHECK(d.debugTargets == (std::vector<DebugTargetId>{DebugTargetId{"LH5801"}}));
    }
    const MachineDescriptor& d = model("PC-1600");
    CHECK(pinOutOf(d, "slot-1") && pinOutOf(d, "slot-1")->id.value == "PC-1600 Slot 1");
    CHECK(pinOutOf(d, "slot-2") && pinOutOf(d, "slot-2")->id.value == "PC-1600 Slot 2");
    CHECK(d.place(PlaceName{"slot-1"})->connector != d.place(PlaceName{"slot-2"})->connector);
    for (const char* p : {"slot-1", "slot-2"})
        CHECK(pinOutOf(d, p)->takes == Takes::One && pinOutOf(d, p)->level == PinOutLevel::ContactLevel);
    for (const char* p : {"printer", "interface", "host-drive"}) {
        CHECK(pinOutOf(d, p) && pinOutOf(d, p)->takes == Takes::Chain && pinOutOf(d, p)->level == PinOutLevel::ContactLevel);
        CHECK(pinOutOf(d, p) && pinOutOf(d, p)->id.value == std::string("pc1600-60-pin-") + p);
        CHECK(d.place(PlaceName{p})->connector.value == "60-pin");
    }
    CHECK(pinOutOf(d, "bus-rom") && pinOutOf(d, "bus-rom")->takes == Takes::Chain);
    CHECK_EQ(d.places.size(), std::size_t{6});   // and bus-rom
    CHECK(d.place(PlaceName{"bus-rom"}) && d.place(PlaceName{"bus-rom"})->connector.value == "60-pin");
    CHECK(d.connector(ConnectorId{"rs232c"}) != nullptr);
    CHECK(d.debugTargets == (std::vector<DebugTargetId>{DebugTargetId{"SC7852"}, DebugTargetId{"LH5803"}}));
}

TEST(module_slots_and_components) {
    for (const char* id : {"PC-1500", "PC-1500A", "PC-1600"}) {
        const MachineDescriptor& d = model(id);
        // Each module slot takes battery-backed modules, a kind of medium
        // held by a module slot: the module is a
        // peripheral there, so the slot is no media place of the descriptor.
        bool slots = false;
        for (const auto& p : d.places) {
            if (p.name.value.rfind("slot-", 0) != 0) continue;
            slots = true;
            CHECK(std::none_of(d.mediaPlaces.begin(), d.mediaPlaces.end(),
                               [&](const MediaPlaceDescriptor& m) { return m.id.value == p.name.value; }));
        }
        CHECK(slots);
        CHECK(hasComponent(d, "rtc", std::string(id) == "PC-1600" ? Depth::Folded : Depth::Separate));
        for (const auto& c : d.components)
            if (c.depth == Depth::Folded) CHECK(c.foldedInto.has_value());
    }
    // The battery-backed modules are media of that kind.
    const Catalogue c = catalogueWithModules();
    calcu::machine::Catalogue copy = c;
    const auto kinds = cu1600::mediaKinds("frontends/qt6/resources/cards", copy);
    const auto module = std::find_if(kinds.begin(), kinds.end(), [](const auto& k) { return k.id == cu1600::kModuleKind; });
    CHECK(module != kinds.end() && module->heldBy == calcu::media::HeldBy::ModuleSlot);
    if (module != kinds.end()) {
        CHECK(module->isMedium(*c.peripheral(PeripheralId{"CE-1600M"})));
        CHECK(!module->isMedium(*c.peripheral(PeripheralId{"CE-151"})));
    }
    // The components of each model and how deeply each is emulated.
    const MachineDescriptor& p15 = model("PC-1500");
    CHECK(hasComponent(p15, "LH5801", Depth::Separate) && hasComponent(p15, "keyboard", Depth::Separate));
    CHECK(hasComponent(p15, "LH5811", Depth::Folded) && hasComponent(p15, "lcd", Depth::Functional));
    const MachineDescriptor& p16 = model("PC-1600");
    CHECK(hasComponent(p16, "SC7852", Depth::Separate) && hasComponent(p16, "LH5803", Depth::Separate));
    CHECK(hasComponent(p16, "bus-arbiter", Depth::Separate) && hasComponent(p16, "sub-cpu", Depth::Functional));
    CHECK(hasComponent(p16, "LR38041", Depth::Folded) && hasComponent(p16, "rs232c", Depth::Separate));
}

TEST(rom_entries_per_variant) {
    // The fetch list of roms/fetch_roms.sh, file names and md5.
    const MachineDescriptor& p15 = model("PC-1500");
    CHECK(p15.romsFor(VariantId{"A01"}).at(0).fileName == "PC-1500_A01.ROM");
    CHECK(p15.romsFor(VariantId{"A01"}).at(0).md5 == "fbc55a9a8743e619b7709721ff5bcbff");
    CHECK(p15.romsFor(VariantId{"A03"}).at(0).md5 == "4bcf78a6d3d32e2a0349eb2d28987b8d");
    CHECK(p15.romsFor(std::nullopt).at(0).fileName == "PC-1500_A04.ROM");
    CHECK(model("PC-1500A").romsFor(std::nullopt).at(0).md5 == "8ebec8b0ef358645df14807c31df7d06");
    const MachineDescriptor& p16 = model("PC-1600");
    for (const char* v : {"New", "Old"}) {
        const auto& roms = p16.romsFor(VariantId{v});
        CHECK_EQ(roms.size(), std::size_t{6});
        const std::string suffix = std::string("-") + (v[0] == 'N' ? "new" : "old") + ".bin";
        for (const auto& r : roms) {
            CHECK(r.fileName.size() > suffix.size() && r.fileName.compare(r.fileName.size() - suffix.size(), suffix.size(), suffix) == 0);
            CHECK_EQ(r.md5.size(), std::size_t{32});
            CHECK_EQ(r.decodedAmount, std::uint64_t{16384});
        }
    }
    CHECK(p16.romsFor(VariantId{"New"}).at(0).md5 == "404bf6f2df489e09649167078acd9a25");
    CHECK(p16.romsFor(VariantId{"Old"}).at(5).md5 == "6005b6420bd5e191e81a1562f3242ec9");
}

TEST(peripheral_descriptors) {
    SKIP_UNLESS(std::filesystem::exists(std::string(cardsDir()) + "/ce161.module.yaml"), "module definitions missing");
    const Catalogue c = catalogueWithModules();
    auto p = [&](const char* id) { return c.peripheral(PeripheralId{id}); };
    for (const char* m : {"CE-151", "CE-155", "CE-161", "CE-1638", "CE-163F", "CE-1600M", "CE-1601M", "256K custom", "superRAM r4"})
        CHECK(p(m) != nullptr);
    const bool ce502b = testRomSet() != nullptr;   // from the ROM set
    if (ce502b) CHECK(p("CE-502B") != nullptr);
    CHECK_EQ(c.peripherals().size(), std::size_t(ce502b ? 15 : 14));   // 4, the bus ROM + the ten modules

    const PeripheralDescriptor* ce150 = p("CE-150");
    CHECK(ce150 && ce150->roms.at(0).fileName == "CE-150.ROM");
    CHECK(ce150 && ce150->mediaPlaces.size() == 1 && ce150->mediaPlaces[0].mediumKind == "cassette");
    CHECK(ce150 && ce150->fitsPinOut(PinOutId{"pc1500-60-pin-printer"}) && ce150->fitsPinOut(PinOutId{"pc1600-60-pin-printer"}));
    CHECK(ce150 && !ce150->fitsPinOut(PinOutId{"pc1600-60-pin-interface"}));
    const PeripheralDescriptor* ce158 = p("CE-158");
    CHECK(ce158 && ce158->roms.at(0).fileName == "CE-158.ROM");
    CHECK(ce158 && ce158->fitsPinOut(PinOutId{"pc1500-60-pin-interface"}) && ce158->fitsPinOut(PinOutId{"pc1600-60-pin-interface"}));
    const PeripheralDescriptor* ce1600p = p("CE-1600P");
    CHECK(ce1600p && variantIds(ce1600p->variants) == (std::vector<std::string>{"New", "Old"}));
    CHECK(ce1600p && ce1600p->defaultVariant && ce1600p->defaultVariant->value == "New");
    CHECK(ce1600p && ce1600p->romsFor(VariantId{"Old"}).size() == 2);
    CHECK(ce1600p && !ce1600p->fitsPinOut(PinOutId{"pc1500-60-pin-printer"}) && ce1600p->fitsPinOut(PinOutId{"pc1600-60-pin-printer"}));
    if (ce1600p) {
        const auto floppy = std::find_if(ce1600p->mediaPlaces.begin(), ce1600p->mediaPlaces.end(),
                                         [](const MediaPlaceDescriptor& m) { return m.id.value == "floppy"; });
        CHECK(floppy != ce1600p->mediaPlaces.end());
        if (floppy != ce1600p->mediaPlaces.end()) {
            CHECK(floppy->passage == MediaPassage::Holding);
            CHECK(floppy->sides == (std::vector<int>{0, 1}));
        }
        CHECK(std::any_of(ce1600p->mediaPlaces.begin(), ce1600p->mediaPlaces.end(),
                          [](const MediaPlaceDescriptor& m) { return m.mediumKind == "cassette"; }));
    }
    const PeripheralDescriptor* ce161 = p("CE-161");
    CHECK(ce161 && ce161->controls.size() == 1 && ce161->controls[0].id.value == "write-protect" &&
          ce161->controls[0].form == ControlForm::Switch);
    for (const char* m : {"CE-151", "CE-1600M"}) CHECK(p(m)->controls.empty());
    const PeripheralDescriptor* hd = p("host-drive");
    CHECK(hd && hd->roms.at(0).fileName == "PC1600-P1-B7-HOSTDRIVE.bin");
    CHECK(hd && hd->fitsPinOut(PinOutId{"pc1600-60-pin-host-drive"}) && !hd->fitsPinOut(PinOutId{"pc1600-60-pin-printer"}));
    CHECK(c.place(ModelId{"PC-1600"}, PlaceName{"host-drive"}).has_value());
}

TEST(module_fit_table) {
    SKIP_UNLESS(testRomSet() != nullptr, "no ROM set in roms/ (the CE-502B is fetched)");
    const Catalogue c = catalogueWithModules();
    struct Row { const char* module; bool p15, s1, s2; };
    const Row rows[] = {{"CE-151", true, true, false},   {"CE-155", true, true, false},  {"CE-161", true, true, true},
                        {"CE-1638", true, true, true},   {"CE-163F", true, true, true},  {"CE-502B", true, false, false},
                        {"CE-1600M", false, true, true}, {"CE-1601M", false, false, true}, {"256K custom", false, false, true}, {"superRAM r4", false, false, true}};
    auto fits = [&](const char* model, const char* place, const char* module) {
        const auto info = c.place(ModelId{model}, PlaceName{place});
        return info && std::find(info->fitting.begin(), info->fitting.end(), PeripheralId{module}) != info->fitting.end();
    };
    for (const Row& r : rows) {
        CHECK(fits("PC-1500", "slot-1", r.module) == r.p15);
        CHECK(fits("PC-1500A", "slot-1", r.module) == r.p15);
        CHECK(fits("PC-1600", "slot-1", r.module) == r.s1);
        CHECK(fits("PC-1600", "slot-2", r.module) == r.s2);
    }
}

TEST(machines_conform) {
    const Catalogue c = catalogueWithModules();
    struct Case { const char* model; std::vector<std::string> variants; };
    const Case cases[] = {{"PC-1500", {"A01", "A03", "A04"}}, {"PC-1500A", {"A04"}}, {"PC-1600", {"New", "Old"}}};
    int built = 0;
    for (const Case& k : cases)
        for (const auto& v : k.variants) {
            auto m = buildSharp(c, k.model, VariantId{v});
            if (!m) { std::fprintf(stderr, "  SKIP %s %s: ROM missing\n", k.model, v.c_str()); continue; }
            ++built;
            CHECK(m->variant() && m->variant()->value == v);
            const auto diff = checkAgainstDescriptor(*m, c);
            for (const auto& d : diff) std::fprintf(stderr, "  %s %s: %s\n", k.model, v.c_str(), d.c_str());
            CHECK(diff.empty());
        }
    SKIP_UNLESS(built > 0, "no Sharp ROM");
    // With each peripheral above plugged in where it fits.
    struct Plug { const char* model; std::vector<PeripheralRequest> peripherals; };
    std::vector<Plug> plugs;
    plugs.push_back({"PC-1500A", {request(c, "CE-150", "printer"), request(c, "CE-158", "interface"), moduleRequest("CE-161", "slot-1")}});
    plugs.push_back({"PC-1500", {moduleRequest("CE-151", "slot-1")}});
    plugs.push_back({"PC-1500", {moduleRequest("CE-155", "slot-1")}});
    plugs.push_back({"PC-1500", {moduleRequest("CE-163F", "slot-1")}});
    plugs.push_back({"PC-1500", {moduleRequest("CE-1638", "slot-1")}});
    if (c.peripheral(PeripheralId{"CE-502B"})) plugs.push_back({"PC-1500A", {moduleRequest("CE-502B", "slot-1")}});
    plugs.push_back({"PC-1600", {request(c, "CE-1600P", "printer", VariantId{"Old"}), request(c, "host-drive", "host-drive"),
                                 moduleRequest("CE-1600M", "slot-1"), moduleRequest("CE-1601M", "slot-2")}});
    plugs.push_back({"PC-1600", {request(c, "CE-150", "printer"), request(c, "CE-158", "interface"),
                                 moduleRequest("CE-151", "slot-1"), moduleRequest("superRAM r4", "slot-2")}});
    plugs.push_back({"PC-1600", {moduleRequest("CE-155", "slot-1"), moduleRequest("CE-161", "slot-2")}});
    for (auto& pl : plugs) {
        const std::size_t n = pl.peripherals.size();
        std::string refusal;
        auto m = buildSharp(c, pl.model, std::nullopt, std::move(pl.peripherals), &refusal);
        if (!refusal.empty()) std::fprintf(stderr, "  %s: %s\n", pl.model, refusal.c_str());
        CHECK(m != nullptr);
        if (!m) continue;
        CHECK_EQ(m->plugged().byPlace.size(), n);
        const auto diff = checkAgainstDescriptor(*m, c);
        for (const auto& d : diff) std::fprintf(stderr, "  %s: %s\n", pl.model, d.c_str());
        CHECK(diff.empty());
    }
    // The media places: one MediaPlace per media place
    // of the descriptors, checked in one_media_place_capability_per_media_place.
}

// The media places:
// media::checkMediaPlaces on each model, bare and with each peripheral
// that has media places (the CE-150's cassette; the CE-1600P's cassette
// and floppy drive); a battery-backed module in a slot is a module medium.
TEST(one_media_place_capability_per_media_place) {
    const Catalogue c = catalogueWithModules();
    struct Plug { const char* model; std::vector<PeripheralRequest> peripherals; std::size_t places; };
    std::vector<Plug> plugs;
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) plugs.push_back({model, {}, 0});
    plugs.push_back({"PC-1500", {request(c, "CE-150", "printer"), moduleRequest("CE-161", "slot-1")}, 1});
    plugs.push_back({"PC-1500A", {request(c, "CE-150", "printer"), request(c, "CE-158", "interface")}, 1});
    plugs.push_back({"PC-1600", {request(c, "CE-1600P", "printer", VariantId{"Old"}), moduleRequest("CE-1600M", "slot-1"),
                                 moduleRequest("CE-1601M", "slot-2")}, 2});
    plugs.push_back({"PC-1600", {request(c, "CE-1600P", "printer", VariantId{"New"})}, 2});
    // The CE-150 on the PC-1600: its cassette too (MODE 1).
    plugs.push_back({"PC-1600", {request(c, "CE-150", "printer"), moduleRequest("CE-151", "slot-1")}, 1});
    int built = 0;
    for (auto& pl : plugs) {
        std::vector<PeripheralRequest> requests = pl.peripherals;
        std::size_t modules = 0;
        for (const auto& r : requests)
            if (r.place.steps.front().value.rfind("slot-", 0) == 0 && r.peripheral.value != "CE-151") ++modules;
        std::string refusal;
        auto m = buildSharp(c, pl.model, std::nullopt, std::move(requests), &refusal);
        if (!m) { std::fprintf(stderr, "  SKIP %s: %s\n", pl.model, refusal.c_str()); continue; }
        ++built;
        const auto diff = calcu::media::checkMediaPlaces(*m, c);
        for (const auto& d : diff) std::fprintf(stderr, "  %s: %s\n", pl.model, d.c_str());
        CHECK(diff.empty());
        CHECK_EQ(m->capabilities(calcu::media::MediaPlace::kKind).size(), pl.places);
        CHECK_EQ(m->capabilities(calcu::media::ModuleMedium::kKind).size(), modules);
    }
    SKIP_UNLESS(built > 0, "no Sharp ROM");
}
