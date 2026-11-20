#include "MediaFixture.hpp"
#include "TestHarness.hpp"

// The saved media and import, and a medium opened from elsewhere (the command
// the media opener uses).

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions dot(sessions::HostServices services = {}) {
    FixtureOptions o;
    o.model = "test-dot";
    o.services = std::move(services);
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "f").text());
        s.write("Work.testdisk.yaml", disk("Work", "w").text());
        s.write("Mine.testdisk.yaml", disk("Mine", "m", true).text());
    };
    return o;
}

Value paths(std::initializer_list<std::string> ps) {
    Value::Array a;
    for (const auto& p : ps) a.push_back(Value(p));
    return Value::object({{"paths", Value(std::move(a))}});
}

std::vector<sessions::ServiceAsk> asks(const sessions::test::TestFrontend& fe) {
    std::vector<sessions::ServiceAsk> out;
    for (const auto& m : fe.log)
        if (const auto* a = std::get_if<sessions::ServiceAsk>(&m)) out.push_back(*a);
    return out;
}

} // namespace

TEST(import_current_medium_under_inner_name) {
    MediaFixture f(dot());
    calcu::testing::TempFolder outside("calcu-media-outside");
    const std::string text = disk("Imported", "i").text();
    outside.write("upload.testdisk.yaml", text);
    CHECK(f.ok("media.import", paths({outside / "upload.testdisk.yaml"})));
    CHECK_EQ(f.saved("Imported.testdisk.yaml"), text);   // byte for byte, named by its inner name
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(std::any_of(k->instances.begin(), k->instances.end(), [](const KindState::Item& i) { return i.name == "Imported"; }));
    outside.write("t.testdisk.yaml", disk("Imported Template", "t", true).text());
    CHECK(f.ok("media.import", paths({outside / "t.testdisk.yaml"})));
    const ShelfEntry* e = f.media->shelf().find(*f.media->kind("test-disk"), "Imported Template");
    CHECK(e && e->isTemplate);   // as the template it says it is
}

TEST(import_refused_name_taken_or_unreadable) {
    MediaFixture f(dot());
    calcu::testing::TempFolder outside("calcu-media-outside");
    outside.write("a.testdisk.yaml", disk("formatted", "x").text());
    outside.write("b.testdisk.yaml", disk("WORK", "x").text());
    TestFile old = disk("Old", "o");
    old.version = 1;
    outside.write("c.testdisk.yaml", old.text());
    outside.write("d.testdisk.yaml", "nothing\n");
    outside.write("e.txt", "text\n");
    CHECK_EQ(f.refusal("media.import", paths({outside / "a.testdisk.yaml"})), std::string("A test disk named “Formatted” exists already."));
    CHECK_EQ(f.refusal("media.import", paths({outside / "b.testdisk.yaml"})), std::string("A test disk named “Work” exists already."));
    CHECK_EQ(f.refusal("media.import", paths({outside / "c.testdisk.yaml"})),
             std::string("The test disk file c.testdisk.yaml cannot be read: Its version is not known."));
    CHECK(f.refusal("media.import", paths({outside / "d.testdisk.yaml"})).find("cannot be read") != std::string::npos);
    CHECK_EQ(f.refusal("media.import", paths({outside / "e.txt"})), std::string("The file e.txt is not a medium of this product."));
    CHECK(!f.exists("a.testdisk.yaml") && f.saved("Work.testdisk.yaml").find("WORK") == std::string::npos);
}

TEST(import_with_companion_file) {
    MediaFixture f(dot());
    calcu::testing::TempFolder outside("calcu-media-outside");
    TestFile t = disk("Tape", "head");
    t.companion = "sound.dat";
    outside.write("in.testdisk.yaml", t.text());
    outside.write("sound.dat", "wave");
    CHECK(f.ok("media.import", paths({outside / "in.testdisk.yaml"})));
    CHECK(f.exists("Tape.testdisk.yaml"));
    CHECK_EQ(f.saved("sound.dat"), std::string("wave"));
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Tape.testdisk.yaml"}})));
    CHECK_EQ(f.drive()->bytes(), std::string("head+wave"));
}

TEST(saved_media_list_by_kind_and_name_marks_template_and_in_place) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Work.testdisk.yaml"}})));
    const KindState* k = f.state()->kindState("test-disk");
    std::vector<std::string> saved;
    for (const auto* list : {&k->templates, &k->instances})
        for (const auto& i : *list)
            if (i.origin == Origin::Saved)
                saved.push_back(i.name + (i.isTemplate ? " [template]" : "") + (i.inPlace ? " [in " + *i.inPlace + "]" : ""));
    CHECK((saved == std::vector<std::string>{"Mine [template]", "Work [in disk]"}));
    CHECK_EQ(k->label, std::string("test disk"));
}

TEST(export_hands_files_to_keep_file) {
    sessions::HostServices s;
    s.offered = {sessions::HostService::KeepFile};
    MediaFixture f(dot(s));
    TestFile t = disk("Tape", "head");
    t.companion = "Tape.dat";
    f.saveFile("Tape.testdisk.yaml", t);
    f.writeSaved("Tape.dat", "wave");
    CHECK(f.ok("media.export", Value::object({{"kind", "test-disk"}, {"name", "tape"}})));
    const auto a = asks(f.fe);
    CHECK(a.size() == 2);
    if (a.size() != 2) return;
    CHECK(a[0].service == sessions::HostService::KeepFile && *a[0].args.find("name")->string() == "Tape.testdisk.yaml");
    CHECK(*a[1].args.find("name")->string() == "Tape.dat");   // its own file first
    const auto* bytes = a[1].args.find("bytes")->bytes();
    CHECK(bytes && std::string(bytes->begin(), bytes->end()) == "wave");
    MediaFixture none(dot());
    CHECK_EQ(none.refusal("media.export", Value::object({{"kind", "test-disk"}, {"name", "Work"}})), std::string("Nobody keeps a file here."));
}

TEST(delete_asks_then_deletes_not_in_place) {
    MediaFixture f(dot());
    const Value del = Value::object({{"kind", "test-disk"}, {"name", "Mine"}});
    CHECK_EQ(f.refusal("media.delete", del), std::string("asks: Delete the test disk “Mine”?"));
    CHECK(f.exists("Mine.testdisk.yaml"));
    CHECK(f.ok("media.delete", Value::object({{"kind", "test-disk"}, {"name", "Mine"}, {"confirmed", true}})));
    CHECK(!f.exists("Mine.testdisk.yaml"));
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Work.testdisk.yaml"}})));
    CHECK_EQ(f.refusal("media.delete", Value::object({{"kind", "test-disk"}, {"name", "Work"}, {"confirmed", true}})),
             std::string("The test disk “Work” is in disk now."));
    CHECK(f.exists("Work.testdisk.yaml"));
    CHECK(f.refusal("media.delete", Value::object({{"kind", "test-disk"}, {"name", "Formatted"}, {"confirmed", true}}))
              .find("is no longer there") != std::string::npos);   // a bundled medium is not a saved one
}

TEST(second_delete_no_longer_there) {
    MediaFixture f(dot());
    const Value del = Value::object({{"kind", "test-disk"}, {"name", "Mine"}, {"confirmed", true}});
    CHECK(f.ok("media.delete", del));
    CHECK_EQ(f.refusal("media.delete", del), std::string("The test disk “Mine” is no longer there."));
}

TEST(import_through_the_file_picker) {
    sessions::HostServices s;
    s.offered = {sessions::HostService::FilePicker};
    MediaFixture f(dot(s));
    calcu::testing::TempFolder outside("calcu-media-outside");
    outside.write("p.testdisk.yaml", disk("Picked", "p").text());
    CHECK(f.ok("media.import"));
    const auto a = asks(f.fe);
    CHECK(a.size() == 1 && a[0].service == sessions::HostService::FilePicker);
    if (a.size() != 1) return;
    CHECK(*a[0].args.find("accept")->string() == ".testcard.yaml,.testdisk.yaml,.dat,.testmodule.yaml");
    sessions::ServiceAnswer ans;
    ans.given = true;
    ans.result = Value(Value::Array{Value(outside / "p.testdisk.yaml")});
    f.session->answer(a[0].ask, ans);
    CHECK(f.exists("Picked.testdisk.yaml"));
}

// A companion picked with its medium is read beside it,
// not imported on its own; picked alone it is refused as before.
TEST(import_file_picker_takes_a_companion_along) {
    sessions::HostServices s;
    s.offered = {sessions::HostService::FilePicker};
    MediaFixture f(dot(s));
    calcu::testing::TempFolder outside("calcu-media-outside");
    TestFile t = disk("Tape", "head");
    t.companion = "sound.dat";
    outside.write("in.testdisk.yaml", t.text());
    outside.write("sound.dat", "wave");
    auto pick = [&](Value::Array files) {
        CHECK(f.ok("media.import"));
        const auto a = asks(f.fe);
        if (a.empty()) return;
        sessions::ServiceAnswer ans;
        ans.given = true;
        ans.result = Value(std::move(files));
        f.session->answer(a.back().ask, ans);
    };
    const auto before = f.notices().size();
    pick(Value::Array{Value(outside / "in.testdisk.yaml"), Value(outside / "sound.dat")});
    CHECK(f.exists("Tape.testdisk.yaml"));
    CHECK_EQ(f.saved("sound.dat"), std::string("wave"));
    CHECK_EQ(f.notices().size(), before);
    pick(Value::Array{Value(outside / "sound.dat")});
    const auto n = f.notices();
    CHECK(n.size() == before + 1 && n.back() == "The file sound.dat is not a medium of this product.");
}

TEST(opened_medium_written_where_it_lies) {
    MediaFixture f(dot());
    calcu::testing::TempFolder outside("calcu-media-outside");
    outside.write("Away.testdisk.yaml", disk("Away", "a").text());
    CHECK(f.ok("media.open-file", Value::object({{"name", "Away.testdisk.yaml"}, {"path", outside / "Away.testdisk.yaml"}, {"place", "disk"}})));
    CHECK_EQ(f.drive()->bytes(), std::string("a"));
    const KindState* k = f.state()->kindState("test-disk");
    CHECK(std::any_of(k->instances.begin(), k->instances.end(),
                      [](const KindState::Item& i) { return i.name == "Away" && i.origin == Origin::Opened && i.inPlace; }));
    f.drive()->write("written back");
    f.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(outside.path() / "Away.testdisk.yaml"), "content"), std::string("written back"));
    CHECK(!f.exists("Away.testdisk.yaml"));   // not copied into the save folder
    // A Web frontend cannot write it back: it is imported first, and asked before a name is replaced.
    const std::string bytes = disk("Work", "uploaded").text();
    const Value up = Value::object({{"name", "Work.testdisk.yaml"}, {"bytes", Value::Bytes(bytes.begin(), bytes.end())}, {"upload", true}, {"place", "disk"}});
    CHECK(f.refusal("media.open-file", up).rfind("asks: ", 0) == 0);
    CHECK(f.ok("media.open-file", Value::object({{"name", "Work.testdisk.yaml"}, {"bytes", Value::Bytes(bytes.begin(), bytes.end())},
                                                 {"upload", true}, {"place", "disk"}, {"confirmed", true}})));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("uploaded"));
    CHECK(f.place("disk")->holds->name == "Work");
    // The reads the opening decides the place with.
    const auto o = f.media->readOpened("x.testdisk.yaml", bytes);
    CHECK(!o.refusal && o.kind && o.kind->id == "test-disk" && !o.isModule);
    CHECK((f.media->placesFor(o, *f.machine()) == std::vector<std::string>{"disk"}));
    CHECK(!f.media->placesInCatalogue(o).empty());
    CHECK(f.media->readOpened("x.txt", bytes).refusal.has_value());
}
