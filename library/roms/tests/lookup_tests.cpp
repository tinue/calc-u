// LIB-12 AC-3, AC-5, AC-7 (the text), EC-1, EC-2 (library halves): the
// lookup's checks and its message (design §6, T5, T6).
#include "TestHarness.hpp"
#include "TestRomFiles.hpp"

using namespace calcu::roms;
using namespace calcu::roms::test;

namespace {
struct Fixture {
    FullSet set = fullSet(testCatalogue());
    TempFolder folder;
    TestTextReader reader;
    Fixture() { writeFullSet(folder, set); }
    LookupResult look() const { return lookUp(set, DirectoryBundle(folder.path()), &reader); }
};
} // namespace

TEST(ac3_binary_checked_by_byte_size) {
    Fixture f;
    CHECK(f.look().ok());
    f.folder.write("test-seg.rom", std::string(2047, 'x'));
    auto r = f.look();
    CHECK(!r.ok());
    const FileProblem* p = problemOf(r.missing, "test-seg.rom");
    CHECK(p && p->problem == Problem::WrongAmount);
    CHECK(p && p->detail.find("2047 bytes") != std::string::npos);
    f.folder.write("test-seg.rom", std::string(2049, 'x'));
    CHECK(!f.look().ok());
}

TEST(ac3_text_checked_by_reader_amount) {
    Fixture f;
    f.folder.write("test-dot-font.txt", fontText("\n", 63));
    auto r = f.look();
    CHECK(!r.ok());
    const FileProblem* p = problemOf(r.missing, "test-dot-font.txt");
    CHECK(p && p->problem == Problem::WrongAmount);
    CHECK(p && p->detail.find("63 rows") != std::string::npos);
    // The size in bytes does not count for a text file: more blank lines, same rows.
    f.folder.write("test-dot-font.txt", fontText("\n\n\n"));
    CHECK(f.look().ok());
    // What the reader cannot read is bad, too.
    f.folder.write("test-dot-font.txt", fontText() + "bad\n");
    r = f.look();
    CHECK(problemOf(r.missing, "test-dot-font.txt") &&
          problemOf(r.missing, "test-dot-font.txt")->problem == Problem::Unreadable);
    // A text entry without a reader is a problem of that file, not a crash.
    f.folder.write("test-dot-font.txt", fontText());
    r = lookUp(f.set, DirectoryBundle(f.folder.path()), nullptr);
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    CHECK(problemOf(r.missing, "test-dot-font.txt"));
}

TEST(ac3_product_set_checks_run_after_file_checks) {
    Fixture f;
    f.reader.rejectSet = true;
    f.folder.remove("test-seg.rom");
    auto r = f.look();
    CHECK(!r.ok());
    CHECK_EQ(f.reader.checkSetCalls, 0);   // a file failed: no set check
    f.folder.write("test-seg.rom", std::string(2048, 'x'));
    r = f.look();
    CHECK_EQ(f.reader.checkSetCalls, 1);
    CHECK(!r.ok());
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    CHECK(r.missing.files[0].problem == Problem::ProductCheck);
    CHECK(!r.missing.files[0].neededBy.empty());   // filled from the full set
    f.reader.rejectSet = false;
    CHECK(f.look().ok());
}

TEST(ac3_md5_is_not_checked_at_start) {
    Fixture f;
    // Right amount, other bytes: the md5 differs, and the start passes (fetch_roms.sh checks it).
    f.folder.write("test-seg.rom", std::string(2048, 'y'));
    f.folder.write("test-dot-font.txt", fontText("\n") + "\n");
    auto r = f.look();
    CHECK(r.ok());
    CHECK_EQ(int(r.set->bytes("test-seg.rom")->front()), int('y'));
}

TEST(ac5_every_missing_or_bad_file_named_and_no_set) {
    Fixture f;
    f.folder.remove("test-seg.rom");
    f.folder.remove("test-module-8k.rom");
    f.folder.write("test-dot-v1.rom", "short");
    auto r = f.look();
    CHECK(!r.ok());
    CHECK(r.set == nullptr);
    CHECK_EQ(r.missing.files.size(), std::size_t{3});   // it does not stop at the first
    CHECK(problemOf(r.missing, "test-seg.rom") && problemOf(r.missing, "test-seg.rom")->problem == Problem::Missing);
    CHECK(problemOf(r.missing, "test-module-8k.rom") &&
          problemOf(r.missing, "test-module-8k.rom")->problem == Problem::Missing);
    CHECK(problemOf(r.missing, "test-dot-v1.rom") &&
          problemOf(r.missing, "test-dot-v1.rom")->problem == Problem::WrongAmount);
    CHECK_EQ(r.missing.where, f.folder.path().string());
    const std::string text = describe(r.missing);
    for (const char* name : {"test-seg.rom", "test-module-8k.rom", "test-dot-v1.rom"})
        CHECK(text.find(name) != std::string::npos);
    CHECK(text.find("test-dot-v2.rom") == std::string::npos);   // a good file is not named
}

TEST(ac5_message_says_full_set_needed_and_offers_nothing_else) {
    Fixture f;
    f.folder.remove("test-dot-font.txt");
    const std::string text = describe(f.look().missing);
    CHECK(text.find("1 ROM file is missing or bad") != std::string::npos);
    CHECK(text.find("test-dot-font.txt: missing; needed by test-dot (variant V2)") != std::string::npos);
    CHECK(text.find("The full set of ROM files is needed") != std::string::npos);
    CHECK(text.find("fetch_roms.sh") != std::string::npos);
    CHECK(text.find(f.folder.path().string()) != std::string::npos);
    // Nothing else is offered: no other folder, no own dumps, no start without them.
    for (const char* other : {"Choose", "folder of your own", "Continue", "Check again", "Start without", "Continue without"})
        CHECK(text.find(other) == std::string::npos);
}

TEST(ac7_harness_text_names_the_files) {
    // The text the harness ends with (LIB-11 EC-1, M-28): describe() names every file.
    Fixture f;
    f.folder.remove("test-module-4k.rom");
    f.folder.remove("test-module-8k.rom");
    const std::string text = describe(f.look().missing);
    CHECK(text.find("2 ROM files are missing or bad") != std::string::npos);
    CHECK(text.find("test-module-4k.rom: missing; needed by test-module (variant 4k)") != std::string::npos);
    CHECK(text.find("test-module-8k.rom: missing; needed by test-module (variant 8k)") != std::string::npos);
}

TEST(ec1_file_of_two_entries_is_named_once_in_the_message) {
    const FullSet set = fullSet(catalogueWithSharedFile());
    TempFolder folder;
    writeFullSet(folder, set);
    folder.remove("test-module-8k.rom");
    TestTextReader reader;
    auto r = lookUp(set, DirectoryBundle(folder.path()), &reader);
    CHECK_EQ(r.missing.files.size(), std::size_t{1});
    const std::string text = describe(r.missing);
    CHECK_EQ(text.find("test-module-8k.rom"), text.rfind("test-module-8k.rom"));
    CHECK(text.find("needed by test-module (variant 8k), test-twin") != std::string::npos);
}

TEST(ec2_text_with_other_line_endings_checked_alike) {
    Fixture f;
    for (const char* eol : {"\n", "\r\n", "\r"}) {
        f.folder.write("test-dot-font.txt", fontText(eol));
        auto r = f.look();
        CHECK(r.ok());
        // The bytes reach the set unchanged.
        CHECK_EQ(r.set->bytes("test-dot-font.txt")->size(), fontText(eol).size());
    }
}
