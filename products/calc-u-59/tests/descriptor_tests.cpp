// The catalogue's three descriptors, its peripherals and a running machine of
// each model against its descriptor, without peripherals and with the
// PC-100C and a module (media places included).

#include "TestHarness.hpp"
#include "harness/MachineState.hpp"
#include "typing/TypingSupport.hpp"
#include "TestRoms.hpp"
#include "../core/TIKeys.hpp"
#include "machine/Conformance.hpp"
#include "media/Conformance.hpp"
#include "media/MediaPlace.hpp"
#include "paper/PaperOutput.hpp"
#include "../core/TraceRecords.hpp"
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

TEST(descriptor_models_and_tick_rates) {
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

TEST(descriptor_segment_display_and_no_sound) {
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

TEST(descriptor_rom_entries_per_model) {
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

TEST(descriptor_keys_connectors_places_debug_target) {
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

TEST(descriptor_card_reader_on_ti59_only) {
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

TEST(descriptor_components_and_depth) {
    for (const char* m : kModels) {
        const auto& d = model(m);
        const auto* cpu = component(d, "TMC0501");
        CHECK(cpu && cpu->depth == Depth::Separate);
        const auto* scom = component(d, "SCOM");
        CHECK(scom && scom->depth == Depth::Folded && scom->foldedInto && scom->foldedInto->value == "TMC0501");
        // The module and the printer are peripherals behind functional
        // connectors (module-slot, printer-connector), not folded into TMC0501.
        for (const char* gone : {"CROM", "printer-interface"}) CHECK(component(d, gone) == nullptr);
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

// With the card reader's media place capability.
TEST(machine_conforms_to_descriptor_without_peripherals) {
    for (const char* m : kModels) {
        auto machine = cu59::buildMachine(*cu59::modelFrom(ModelId{m}), *testroms::romSet(),
                                          cu59::MachinePeripherals{std::nullopt, false, false});
        const auto diffs = checkAgainstDescriptor(*machine, tiCatalogue());
        for (const auto& d : diffs) std::printf("  %s: %s\n", m, d.c_str());
        CHECK(diffs.empty());
        CHECK(machine->model().value == m);
        CHECK(machine->descriptor().tickRate == model(m).tickRate);
        CHECK(machine->plugged().byPlace.empty());
        // Typing support, the trace records, the program memory, the library
        // program, on the TI-58C the constant memory, the debug panel's four
        // kinds, on the TI-59 the card reader's media place, the machine state.
        const std::size_t own = std::string(m) == "TI-58" ? 9 : 10;
        CHECK_EQ(machine->capabilities().size(), own);
        const auto media = calcu::media::checkMediaPlaces(*machine, tiCatalogue());
        for (const auto& d : media) std::printf("  %s: %s\n", m, d.c_str());
        CHECK(media.empty());
        CHECK_EQ(machine->capabilities(calcu::media::MediaPlace::kKind).size(), std::size_t{std::string(m) == "TI-59" ? 1u : 0u});
        CHECK_EQ(machine->capabilities(TraceRecords::kKind).size(), std::size_t{1});
        CHECK_EQ(machine->capabilities(calcu::typing::TypingSupport::kKind).size(), std::size_t{1});
    }
}

TEST(descriptor_pc100c_and_14_modules) {
    const auto& c = tiCatalogue();
    CHECK_EQ(c.peripherals().size(), size_t{15});
    const auto* p = c.peripheral(kPC100C);
    CHECK(p != nullptr);
    if (p) {
        CHECK(p->fits == (std::vector<PinOutId>{PinOutId{"printer-connector"}}));
        CHECK(p->roms.empty());
        std::vector<std::string> ids;
        for (const auto& k : p->controls) ids.push_back(k.id.value + (k.form == ControlForm::Key ? ":key" : ":switch"));
        CHECK(ids == (std::vector<std::string>{"PRINT:key", "ADV:key", "TRACE:switch"}));
        CHECK(!p->components.empty() && p->components[0].depth == Depth::Functional);
    }
    // Its paper: the capability of the kind paper-output while it is plugged in.
    auto with = cu59::buildMachine(cu59::Model::TI59, *testroms::romSet(), cu59::MachinePeripherals{std::nullopt, true, false});
    CHECK_EQ(with->capabilities(calcu::paper::PaperOutput::kKind).size(), size_t{1});
    for (const auto& m : tiModules()) {
        const auto* d = c.peripheral(PeripheralId{m.id});
        CHECK(d != nullptr);
        if (!d) continue;
        CHECK_EQ(d->name, std::string(m.name));
        CHECK(d->fits == (std::vector<PinOutId>{PinOutId{"module-slot"}}));
        CHECK_EQ(d->roms.size(), size_t{1});
        if (d->roms.empty()) continue;
        CHECK_EQ(d->roms[0].fileName, std::string(m.id) + ".rom.yaml");
        CHECK_EQ(d->roms[0].md5.size(), size_t{32});
        CHECK(d->roms[0].unit == ContentUnit::Bytes && d->roms[0].decodedAmount == 5000);
        CHECK(!d->components.empty() && d->components[0].id.value == m.chip);
        CHECK(d->controls.empty());
    }
}

TEST(machine_conforms_to_descriptor_with_printer_and_module) {
    for (const char* m : kModels) {
        for (bool with : {true, false}) {
            const cu59::MachinePeripherals places{with ? std::optional<std::string>("ML") : std::nullopt, with, false};
            auto machine = cu59::buildMachine(*cu59::modelFrom(ModelId{m}), *testroms::romSet(), places);
            const auto diffs = checkAgainstDescriptor(*machine, tiCatalogue());
            for (const auto& d : diffs) std::printf("  %s: %s\n", m, d.c_str());
            CHECK(diffs.empty());
            CHECK_EQ(machine->plugged().byPlace.size(), size_t{with ? 2u : 0u});
            // + typing support, trace records, program memory, library program,
            // constant memory (TI-58C), card reader (TI-59), the debug panel's
            // asm-overlay, rom-heatmap, calculator-state, machine-log, the machine state
            const std::size_t own = std::string(m) == "TI-58" ? 9 : 10;
            CHECK_EQ(machine->capabilities().size(), (with ? 2u : 0u) + own);
            CHECK(calcu::media::checkMediaPlaces(*machine, tiCatalogue()).empty());
            for (const auto& e : machine->capabilities()) {
                CHECK(machine->capability(e.id) != nullptr && machine->capability(e.id)->kind() == e.kind);
                if (e.kind == calcu::typing::TypingSupport::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "TMC0501" && e.id.value == "TMC0501/typing-support");
                    continue;
                }
                if (e.kind == cu59::AsmOverlay::kKind || e.kind == cu59::RomHeatmapCounter::kKind ||
                    e.kind == cu59::CalculatorState::kKind || e.kind == cu59::MachineLog::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "TMC0501" && e.id.value == "TMC0501/" + std::string(e.kind.value));
                    continue;
                }
                if (e.kind == calcu::harness::MachineState::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "TMC0501" && e.id.value == "TMC0501/machine-state");
                    continue;
                }
                if (e.kind == TraceRecords::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "TMC0501" && e.id.value == "TMC0501/c59.trace-records");
                    continue;
                }
                if (e.kind == cu59::LibraryProgram::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "TMC0501" && e.id.value == "TMC0501/c59.library-program");
                    continue;
                }
                if (e.kind == calcu::media::MediaPlace::kKind) {   // the card reader's
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "card-reader" && std::string(m) == "TI-59");
                    continue;
                }
                if (e.kind == cu59::ProgramMemory::kKind || e.kind == cu59::ConstantMemory::kKind) {
                    const auto* c = std::get_if<ComponentOwner>(&e.owner);
                    CHECK(c && c->component.value == "RAM" && e.id.value == "RAM/" + std::string(e.kind.value));
                    continue;
                }
                const auto* owner = std::get_if<PeripheralOwner>(&e.owner);
                CHECK(owner && owner->peripheral == kPC100C);
            }
        }
    }
}
