// ROM entries and the full set.
#include "TestHarness.hpp"
#include "TestRomFiles.hpp"
#include <algorithm>

using namespace calcu::roms;
using namespace calcu::roms::test;

namespace {
const FullSetEntry* entryOf(const FullSet& s, const std::string& name) {
    for (const auto& f : s.files)
        if (f.entry.fileName == name) return &f;
    return nullptr;
}
bool isHex32(const std::string& s) {
    return s.size() == 32 && std::all_of(s.begin(), s.end(), [](char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}
} // namespace

TEST(rom_entry_states_bundle_name_encoding_amount_unit_md5) {
    const FullSet set = fullSet(testCatalogue());
    CHECK(!set.files.empty());
    bool sawText = false, sawBinary = false;
    for (const auto& f : set.files) {
        const RomEntry& e = f.entry;
        CHECK(e.fileName.find('.') != std::string::npos);   // the bundled name, with its extension
        CHECK(e.fileName.find('/') == std::string::npos);
        CHECK(e.decodedAmount > 0);
        CHECK(isHex32(e.md5));
        if (e.encoding == RomEncoding::Text) {
            sawText = true;
            CHECK(e.unit == ContentUnit::Rows);
        } else {
            sawBinary = true;
            CHECK(e.unit == ContentUnit::Bytes);
        }
    }
    CHECK(sawText);
    CHECK(sawBinary);
}

TEST(full_set_has_every_model_variant_and_peripheral_file) {
    const FullSet set = fullSet(testCatalogue());
    std::vector<std::string> names;
    for (const auto& f : set.files) names.push_back(f.entry.fileName);
    // Catalogue order: the models (each variant), then the peripherals (each variant).
    CHECK_EQ(names, (std::vector<std::string>{"test-seg.rom", "test-dot-v1.rom", "test-dot-v2.rom", "test-dot-font.txt",
                                              "test-module-4k.rom", "test-module-8k.rom"}));
    CHECK(set.catalogueErrors.empty());
    CHECK(entryOf(set, "test-dot-v2.rom")->neededBy == (std::vector<NeededBy>{{kDot, VariantId{"V2"}}}));
    CHECK(entryOf(set, "test-seg.rom")->neededBy == (std::vector<NeededBy>{{kSeg, std::nullopt}}));
    CHECK(entryOf(set, "test-module-4k.rom")->neededBy == (std::vector<NeededBy>{{kModule, VariantId{"4k"}}}));
    CHECK_EQ(describe(entryOf(set, "test-dot-v2.rom")->neededBy.front()), std::string("test-dot (variant V2)"));
    // Entries outside the catalogue (as Calc-U-1600's CE-502B) join the set.
    const FullSetEntry extra{RomEntry{"extra.rom", RomEncoding::Binary, 16, ContentUnit::Bytes,
                                      "0123456789abcdef0123456789abcdef"},
                             {NeededBy{PeripheralId{"x"}, std::nullopt}}};
    const FullSet withExtra = fullSet(testCatalogue(), std::span<const FullSetEntry>(&extra, 1));
    CHECK_EQ(withExtra.files.size(), set.files.size() + 1);
    CHECK_EQ(withExtra.files.back().entry.fileName, std::string("extra.rom"));
}

TEST(fullset_file_of_two_entries_is_in_the_set_once_with_both) {
    const FullSet set = fullSet(catalogueWithSharedFile());
    CHECK_EQ(std::count_if(set.files.begin(), set.files.end(),
                           [](const FullSetEntry& f) { return f.entry.fileName == "test-module-8k.rom"; }),
             std::ptrdiff_t{1});
    CHECK(set.catalogueErrors.empty());
    const auto* shared = entryOf(set, "test-module-8k.rom");
    CHECK(shared->neededBy == (std::vector<NeededBy>{{kModule, VariantId{"8k"}}, {PeripheralId{"test-twin"}, std::nullopt}}));
}

TEST(fullset_conflicting_entries_for_one_name_are_a_catalogue_error) {
    const FullSet set = fullSet(catalogueWithConflict());
    CHECK_EQ(set.catalogueErrors, std::vector<std::string>{"test-seg.rom"});
    // Looked up, it fails like a missing file, naming that file.
    TempFolder folder;
    writeFullSet(folder, set);
    TestTextReader reader;
    auto r = lookUp(set, DirectoryBundle(folder.path()), &reader);
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    CHECK(problemOf(r.missing, "test-seg.rom") && problemOf(r.missing, "test-seg.rom")->problem == Problem::Catalogue);
}
