#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "media/Names.hpp"

// Names of media and their files.

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {
const KindDecl& diskKind() {
    static const std::vector<KindDecl> k = testKinds();
    return k[1];
}
FixtureOptions onDot() {
    FixtureOptions o;
    o.model = "test-dot";
    return o;
}
} // namespace

TEST(inner_name_wins) {
    MediumInfo info;
    info.name = "Inner";
    CHECK_EQ(nameOf(info, "outer.testdisk.yaml", diskKind()), std::string("Inner"));
    MediaFixture f(onDot());
    f.saveFile("outer.testdisk.yaml", disk("Inner", "x"));
    CHECK(f.ok("media.refresh"));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.size() == 1 && k->instances[0].name == "Inner");
}

TEST(file_name_counts_only_without_inner_name) {
    MediumInfo info;
    CHECK_EQ(nameOf(info, "dir/Plain.TESTDISK.yaml", diskKind()), std::string("Plain"));
    MediaFixture f(onDot());
    f.saveFile("Plain.testdisk.yaml", disk("", "x"));
    CHECK(f.ok("media.refresh"));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.size() == 1 && k->instances[0].name == "Plain");
}

TEST(medium_name_spaces_trimmed_and_runs_collapsed) {
    CHECK_EQ(checkName("  My   own  disk ").name, std::string("My own disk"));
    CHECK(checkName("  My   own  disk ").refusal.empty());
}

TEST(medium_name_refuses_characters_and_control_characters) {
    for (const char* bad : {"a/b", "a\\b", "a:b", "a*b", "a?b", "a\"b", "a<b", "a>b", "a|b"}) CHECK(!checkName(bad).refusal.empty());
    CHECK(!checkName(std::string("a\x01" "b")).refusal.empty());
    CHECK(!checkName(std::string("a\x7f")).refusal.empty());
    CHECK(!checkName("tab\there").refusal.empty());
    CHECK(checkName("\xC3\x84-ok_(1).2").refusal.empty());
}

TEST(empty_medium_name_refused) {
    CHECK_EQ(checkName("").refusal, std::string("A name cannot be empty."));
    CHECK_EQ(checkName("    ").refusal, std::string("A name cannot be empty."));
}

TEST(names_differing_in_case_are_one) {
    CHECK(sameName("Work Disk", "work disk"));
    CHECK(!sameName("Work", "Work2"));
    CHECK(!sameName("\xC3\x84", "\xC3\xA4"));   // ASCII letters only
    // A save under the other case finds the same medium.
    MediaFixture f(onDot());
    f.saveFile("Work.testdisk.yaml", disk("Work", "x"));
    CHECK(f.ok("media.refresh"));
    CHECK(f.media->shelf().find(diskKind(), "WORK") != nullptr);
}

TEST(file_is_name_and_suffix_in_save_folder) {
    CHECK_EQ(fileNameFor("Work", diskKind()), std::string("Work.testdisk.yaml"));
    MediaFixture f(onDot());
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "implicit:none"}})) == false);
    f.saveFile("Blank.testdisk.yaml", disk("Blank", "", true));
    CHECK(f.ok("media.refresh"));
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Blank.testdisk.yaml"}})));
    CHECK(f.ok("media.name-and-save", Value::object({{"place", "disk"}, {"name", "Work"}})));
    CHECK(f.exists("Work.testdisk.yaml"));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "name"), std::string("Work"));
}

TEST(numbered_file_when_file_holds_another_name) {
    const auto taken = [](const std::string& file) { return file == "foo-1.testdisk.yaml"; };
    CHECK_EQ(numberedFileName("foo", diskKind(), taken), std::string("foo-2.testdisk.yaml"));
    MediaFixture f(onDot());
    f.saveFile("Work.testdisk.yaml", disk("Other", "x"));   // the file the name gives holds another medium
    f.saveFile("Blank.testdisk.yaml", disk("Blank", "", true));
    CHECK(f.ok("media.refresh"));
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Blank.testdisk.yaml"}})));
    CHECK(f.ok("media.name-and-save", Value::object({{"place", "disk"}, {"name", "Work"}})));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "name"), std::string("Other"));
    CHECK_EQ(field(f.saved("Work-1.testdisk.yaml"), "name"), std::string("Work"));
}
