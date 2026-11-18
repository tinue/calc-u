// The ROM set builds every model, variant and peripheral as far as ROMs go,
// and no file is read after the lookup.
#include "TestHarness.hpp"
#include "TestRomFiles.hpp"

using namespace calcu::roms;
using namespace calcu::roms::test;

namespace {
std::shared_ptr<const RomSet> lookedUp(const TempFolder& folder, const FullSet& set) {
    writeFullSet(folder, set);
    TestTextReader reader;
    auto r = lookUp(set, DirectoryBundle(folder.path()), &reader);
    CHECK(r.ok());
    return r.set;
}
} // namespace

TEST(rom_set_images_for_every_model_variant_peripheral) {
    const Catalogue& cat = testCatalogue();
    const FullSet full = fullSet(cat);
    TempFolder folder;
    auto set = lookedUp(folder, full);
    if (!set) return;
    CHECK_EQ(set->fileNames().size(), full.files.size());
    auto names = [](const std::vector<RomImage>& images) {
        std::vector<std::string> out;
        for (const auto& i : images) out.push_back(i.fileName);
        return out;
    };
    for (const auto& m : cat.models()) {
        std::vector<std::optional<VariantId>> variants{std::nullopt};
        for (const auto& v : m.variants) variants.push_back(v.id);
        for (const auto& v : variants) {
            const auto images = set->imagesFor(m, v);
            CHECK_EQ(images.size(), m.romsFor(v).size());
            for (const auto& i : images) CHECK_EQ(i.bytes, *set->bytes(i.fileName));
        }
    }
    CHECK_EQ(names(set->imagesFor(*cat.model(kDot), VariantId{"V2"})),
             (std::vector<std::string>{"test-dot-v2.rom", "test-dot-font.txt"}));
    CHECK_EQ(names(set->imagesFor(*cat.peripheral(kModule), VariantId{"8k"})),
             std::vector<std::string>{"test-module-8k.rom"});
    CHECK(set->imagesFor(*cat.peripheral(kPrinter), std::nullopt).empty());
    CHECK(set->bytes("not-in-the-set.rom") == nullptr);
}

TEST(rom_set_build_never_misses_a_rom) {
    const Catalogue& cat = testCatalogue();
    TempFolder folder;
    auto set = lookedUp(folder, fullSet(cat));
    if (!set) return;
    TestBuilder builder;
    for (const auto& m : cat.models()) {
        std::vector<std::optional<VariantId>> variants{std::nullopt};
        for (const auto& v : m.variants) variants.push_back(v.id);
        for (const auto& v : variants) {
            BuildRequest request;
            request.model = m.model;
            request.variant = v;
            request.roms = set->imagesFor(m, v);
            if (m.model == kSeg) {
                for (const char* variant : {"4k", "8k"}) {
                    PeripheralRequest p;
                    p.place = PlacePath{{PlaceName{"slot-1"}}};
                    p.peripheral = kModule;
                    p.variant = VariantId{variant};
                    p.roms = set->imagesFor(*cat.peripheral(kModule), p.variant);
                    request.peripherals = {p};
                    BuildResult r = build(cat, builder, request);
                    CHECK(r.missingRoms.empty());
                    CHECK(r.machine != nullptr);
                }
                request.peripherals.clear();
            }
            BuildResult r = build(cat, builder, request);
            CHECK(r.missingRoms.empty());
            CHECK(r.machine != nullptr);
        }
    }
}

TEST(rom_set_no_file_read_after_lookup) {
    const Catalogue& cat = testCatalogue();
    std::shared_ptr<const RomSet> set;
    {
        TempFolder folder;
        set = lookedUp(folder, fullSet(cat));
    }   // the folder and every file in it are gone
    if (!set) return;
    TestBuilder builder;
    BuildRequest request;
    request.model = kDot;
    request.variant = VariantId{"V2"};
    request.roms = set->imagesFor(*cat.model(kDot), request.variant);
    BuildResult r = build(cat, builder, request);
    CHECK(r.machine != nullptr);
    CHECK_EQ(set->bytes("test-seg.rom")->size(), std::size_t{2048});
}
