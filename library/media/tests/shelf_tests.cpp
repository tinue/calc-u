#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "media/Names.hpp"

// Kinds and the shelf.

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions onDot(std::function<void(calcu::testing::TempFolder&, calcu::testing::TempFolder&)> prepare = {}) {
    FixtureOptions o;
    o.model = "test-dot";
    o.prepare = std::move(prepare);
    return o;
}

Value choose(const std::string& place, const std::string& entry) {
    return Value::object({{"place", place}, {"entry", entry}});
}

const KindDecl& kindOf(MediaFixture& f, const char* id) { return *f.media->kind(id); }

} // namespace

TEST(bundled_medium_is_template_and_never_written) {
    MediaFixture f(onDot([](auto&, auto& b) { b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "empty").text()); }));
    const std::string before = calcu::testing::readText(f.bundled.path() / "disks/Formatted.testdisk.yaml");
    const ShelfEntry* e = f.media->shelf().find(kindOf(f, "test-disk"), "Formatted");
    CHECK(e && e->isTemplate && e->origin == Origin::Bundled);   // its file does not say so: bundled decides
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    f.drive()->write("written");
    f.ok("media.refresh");
    CHECK_EQ(calcu::testing::readText(f.bundled.path() / "disks/Formatted.testdisk.yaml"), before);
    CHECK(f.place("disk")->holds && f.place("disk")->holds->unsaved);
}

TEST(user_template_by_its_file) {
    MediaFixture f(onDot([](auto& s, auto&) { s.write("Mine.testdisk.yaml", disk("Mine", "t", true).text()); }));
    const std::string before = f.saved("Mine.testdisk.yaml");
    const ShelfEntry* e = f.media->shelf().find(kindOf(f, "test-disk"), "Mine");
    CHECK(e && e->isTemplate && e->origin == Origin::Saved);
    CHECK(f.ok("media.choose", choose("disk", "saved:Mine.testdisk.yaml")));
    f.drive()->write("changed");
    f.ok("media.refresh");
    CHECK_EQ(f.saved("Mine.testdisk.yaml"), before);   // only a save as a template writes it
}

TEST(instance_in_save_folder) {
    MediaFixture f(onDot([](auto& s, auto&) { s.write("Work.testdisk.yaml", disk("Work", "a").text()); }));
    const ShelfEntry* e = f.media->shelf().find(kindOf(f, "test-disk"), "Work");
    CHECK(e && !e->isTemplate);
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("b");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("b"));
}

TEST(shelf_bundled_first_then_save_folder) {
    MediaFixture f(onDot([](auto& s, auto& b) {
        b.write("disks/Shared.testdisk.yaml", disk("Shared", "bundled").text());
        s.write("Own.testdisk.yaml", disk("Own", "saved").text());
    }));
    const auto entries = f.media->shelf().entries(kindOf(f, "test-disk"));
    CHECK(entries.size() == 2 && entries[0]->origin == Origin::Bundled && entries[1]->origin == Origin::Saved);
    const ShelfEntry* e = f.media->shelf().find(kindOf(f, "test-disk"), "shared");
    CHECK(e && e->origin == Origin::Bundled);
    CHECK(f.done(f.load(header("test-dot") + "keys:\n  - test-disk: { name: Shared }\n")));
    CHECK_EQ(f.drive()->bytes(), std::string("bundled"));
}

TEST(two_files_one_name_both_listed_and_found_by_rule) {
    MediaFixture f(onDot([](auto& s, auto&) {
        s.write("a.testdisk.yaml", disk("Twin", "a").text());
        s.write("Twin.testdisk.yaml", disk("Twin", "own").text());
        s.write("c.testdisk.yaml", disk("Pair", "c").text());
        s.write("b.testdisk.yaml", disk("Pair", "b").text());
    }));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.size() == 4);   // both are listed
    CHECK_EQ(f.media->shelf().find(kindOf(f, "test-disk"), "Twin")->fileName, std::string("Twin.testdisk.yaml"));
    CHECK_EQ(f.media->shelf().find(kindOf(f, "test-disk"), "Pair")->fileName, std::string("b.testdisk.yaml"));
    // A save by that name replaces the one the rule finds.
    CHECK(f.done(f.load(header("test-dot") + "keys:\n  - test-disk: { name: Pair }\n"
                        "  - saveas: { place: disk, as: live, name: Twin }\n")));
    CHECK_EQ(field(f.saved("Twin.testdisk.yaml"), "content"), std::string("b"));
    CHECK_EQ(field(f.saved("a.testdisk.yaml"), "content"), std::string("a"));
}

TEST(saved_file_with_bundled_name_not_listed_and_logged) {
    MediaFixture f(onDot([](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "bundled").text());
        s.write("copy.testdisk.yaml", disk("formatted", "copy").text());
    }));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.empty() && k->templates.size() == 1);
    CHECK(hasLine(f.logLines(), "copy.testdisk.yaml carries the name of a bundled test disk"));
}

TEST(module_slot_one_list_of_names) {
    MediaFixture f(FixtureOptions{});
    f.saveFile("Bank.testmodule.yaml", moduleFile("Bank", "m"));
    CHECK(f.ok("media.refresh"));
    const PlaceState* slot = f.place("slot-1");
    CHECK(slot && slot->slot);
    std::vector<std::string> labels;
    for (const auto& e : slot->picker) labels.push_back(e.label);
    // The module medium's template, the saved instance and the module that is not a medium: one list.
    CHECK(std::find(labels.begin(), labels.end(), "Bank") != labels.end());
    CHECK(std::find(labels.begin(), labels.end(), "Test module") != labels.end());
    CHECK(std::find(labels.begin(), labels.end(), "Test memory module") != labels.end());
    CHECK(f.done(f.load(header() + "slot-1: { name: Bank }\n")));
    CHECK(f.module() && f.module()->bytes() == "m");
    CHECK(f.done(f.load(header() + "slot-1: { name: test-module }\n")));
    CHECK(!f.module() && f.machine()->plugged().byPlace.size() == 1);
}

TEST(codec_gives_all_the_library_knows) {
    MediaFixture f(FixtureOptions{});
    TestFile t = moduleFile("Info", "abc+def", true, true);
    t.description = "Described";
    t.companion = "Info.dat";
    t.content = "abc";
    f.writeSaved("Info.testmodule.yaml", t.text());
    f.writeSaved("Info.dat", "def");
    CHECK(f.ok("media.refresh"));
    const ShelfEntry* e = f.media->shelf().find(kindOf(f, "test-module"), "Info");
    CHECK(e != nullptr);
    if (!e) return;
    CHECK_EQ(e->info.name, std::string("Info"));
    CHECK_EQ(e->info.description, std::string("Described"));
    CHECK(e->info.isTemplate && e->info.hasWriteProtect && e->info.writeProtected);
    CHECK_EQ(e->info.formatVersion, 2);
    CHECK(e->info.companions == std::vector<std::string>{"Info.dat"});
    CHECK_EQ(contentOf(e->info.content), std::string("abc+def"));
}

TEST(content_handed_through_unread) {
    MediaFixture f(onDot([](auto& s, auto&) { s.write("Work.testdisk.yaml", disk("Work", "payload").text()); }));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    const auto* c = dynamic_cast<const mt::TestMediumContent*>(f.drive()->content().get());
    CHECK(c && c->bytes == "payload");   // the codec's object, as the product's core gets it
    f.drive()->write("new");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("new"));
}

// A file of the kind's suffix that is no medium of it (Calc-U-1600:
// a module definition without a battery) is neither listed nor logged.
TEST(not_a_medium_neither_listed_nor_logged) {
    MediaFixture f(onDot([](auto& s, auto&) {
        s.write("Plain.testdisk.yaml", "format: test-disk\nformat-version: 2\nname: Plain\nmedium: no\ncontent: x\n");
        s.write("Good.testdisk.yaml", disk("Good", "g").text());
    }));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.size() == 1 && k->instances[0].name == "Good");
    CHECK(!hasLine(f.logLines(), "Plain.testdisk.yaml"));
}

TEST(unreadable_medium_not_listed_one_log_line_per_start) {
    MediaFixture f(onDot([](auto& s, auto&) {
        s.write("Broken.testdisk.yaml", "this is no medium\n");
        s.write("Good.testdisk.yaml", disk("Good", "g").text());
    }));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(k && k->instances.size() == 1 && k->instances[0].name == "Good");
    const auto lines = f.logLines();
    CHECK(hasLine(lines, "The test disk file Broken.testdisk.yaml cannot be read: It is not a test-disk file."));
    CHECK(f.ok("media.refresh"));
    CHECK(!hasLine(f.logLines(), "Broken.testdisk.yaml"));   // once per start
}

namespace {
// A host's file access that records what goes through it.
struct RecordingAccess final : FileAccess {
    mutable std::vector<std::string> calls;
    std::vector<std::string> list(const std::string& folder) const override {
        calls.push_back("list");
        return FileAccess::list(folder);
    }
    Read read(const std::string& path) const override {
        calls.push_back("read " + baseName(path));
        return FileAccess::read(path);
    }
    Written write(const std::string& path, const std::string& bytes) const override {
        calls.push_back("write " + baseName(path));
        return FileAccess::write(path, bytes);
    }
};
} // namespace

// The shelf and the writer go through the file
// access the host hands the part (Calc-U-59's coordinated iCloud access).
TEST(shelf_and_writer_through_the_hosts_file_access) {
    auto access = std::make_shared<RecordingAccess>();
    FixtureOptions o = onDot([](auto& s, auto&) { s.write("Work.testdisk.yaml", disk("Work", "a").text()); });
    o.files = access;
    MediaFixture f(o);
    auto has = [&](const std::string& c) {
        return std::find(access->calls.begin(), access->calls.end(), c) != access->calls.end();
    };
    CHECK(has("list"));
    CHECK(has("read Work.testdisk.yaml"));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("b");
    f.ok("media.refresh");
    CHECK(has("write Work.testdisk.yaml"));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("b"));
}
