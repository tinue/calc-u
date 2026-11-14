#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "machine/Build.hpp"
#include <algorithm>

// LIB-1 AC-5, EC-1 (design §6, T5).

using namespace calcu::machine;
using namespace calcu::machine::test;

namespace {
// Counts the calls that reach the product's builder.
struct CountingBuilder : MachineBuilder {
    TestBuilder inner;
    int calls = 0;
    BuildResult build(const BuildRequest& r, const Catalogue& c) override { ++calls; return inner.build(r, c); }
};

void dropRom(std::vector<RomImage>& roms, const std::string& name) {
    roms.erase(std::remove_if(roms.begin(), roms.end(), [&](const RomImage& i) { return i.fileName == name; }),
               roms.end());
}
} // namespace

TEST(ac5_machine_reports_model_and_variant_for_its_life) {
    auto dot = buildTestMachine(kDot, VariantId{"V2"});
    CHECK(dot != nullptr);
    if (!dot) return;
    auto same = [&] { return dot->model() == kDot && dot->variant() == VariantId{"V2"}; };
    CHECK(same());
    dot->advance(1'000);
    dot->reset();
    CHECK(same());
    dot->advance(1'000);
    dot->powerCycle();
    CHECK(same());
    dot->advance(1'000);
    dot->coldBoot();
    CHECK(same());
    runUntilKeyReady(*dot);
    CHECK(same());

    auto seg = buildTestMachine(kSeg);
    CHECK(seg && seg->model() == kSeg && !seg->variant());
    // No variant named: the default.
    auto def = buildTestMachine(kDot);
    CHECK(def && def->variant() == VariantId{"V1"});
}

TEST(ec1_missing_rom_names_the_file) {
    const Catalogue& cat = testCatalogue();
    CountingBuilder builder;
    BuildRequest req = requestFor(cat, kDot, VariantId{"V2"});
    CHECK_EQ(req.roms.size(), std::size_t{2});
    dropRom(req.roms, "test-dot-font.txt");
    BuildResult r = build(cat, builder, req);
    CHECK(!r.machine);
    CHECK(r.missingRoms == std::vector<std::string>{"test-dot-font.txt"});
    CHECK_EQ(builder.calls, 0);
    // Handed in, it builds.
    CHECK(build(cat, builder, requestFor(cat, kDot, VariantId{"V2"})).machine != nullptr);
    CHECK_EQ(builder.calls, 1);
}

TEST(ec1_missing_peripheral_rom_names_the_file) {
    const Catalogue& cat = testCatalogue();
    CountingBuilder builder;
    PeripheralRequest module = peripheralAt(cat, "slot-1", kModule, VariantId{"8k"});
    dropRom(module.roms, "test-module-8k.rom");
    BuildResult r = build(cat, builder, requestFor(cat, kSeg, std::nullopt, {module}));
    CHECK(!r.machine);
    CHECK(r.missingRoms == std::vector<std::string>{"test-module-8k.rom"});
    CHECK_EQ(builder.calls, 0);
}
