// The bundled files, read by exact name.
#include "TestHarness.hpp"
#include "TestRomFiles.hpp"

using namespace calcu::roms;
using namespace calcu::roms::test;

TEST(bundle_file_of_exactly_that_name_is_read) {
    TempFolder folder;
    folder.write("CE-150.ROM", std::string("\x01\x02\x03", 3));
    folder.write("other.bin", "zz");
    DirectoryBundle bundle(folder.path());
    auto bytes = bundle.read("CE-150.ROM");
    CHECK(bytes.has_value());
    CHECK_EQ(bytes->size(), std::size_t{3});
    CHECK_EQ(int((*bytes)[2]), 3);
    CHECK(!bundle.read("CE-150").has_value());
    CHECK(!bundle.read("CE-150.ROM.tmp").has_value());
    CHECK_EQ(bundle.where(), folder.path().string());
}

TEST(bundle_name_differing_in_case_is_missing) {
    TempFolder folder;
    folder.write("ce-150.rom", "abc");
    DirectoryBundle bundle(folder.path());
    // On a case-insensitive file system open() would find it; the lookup does not.
    CHECK(!bundle.read("CE-150.ROM").has_value());
    CHECK(bundle.read("ce-150.rom").has_value());
}

TEST(bundle_subfolders_are_not_searched) {
    TempFolder folder;
    fs::create_directories(folder.path() / "calculator");
    folder.write("calculator/TMC0582.txt", "rom");
    DirectoryBundle bundle(folder.path());
    CHECK(!bundle.read("TMC0582.txt").has_value());
    CHECK(!bundle.read("calculator/TMC0582.txt").has_value());
    CHECK(!bundle.read("calculator").has_value());   // a folder is not a file
    DirectoryBundle missing(folder.path() / "no-such-folder");
    CHECK(!missing.read("TMC0582.txt").has_value());
}
