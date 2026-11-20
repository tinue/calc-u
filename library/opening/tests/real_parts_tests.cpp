// Opening through the real presets and media parts, on the
// media tests' fixture (the media test catalogue, the test kinds, a save
// folder of the test's own), a pass-through place among them; the folder of
// a file dialog, on a session with a settings file.
#include "MediaFixture.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "opening/Classify.hpp"
#include "opening/OpenPart.hpp"

using namespace calcu::opening;
using namespace calcu::media::test;
using calcu::sessions::Value;

namespace {

// The media fixture with an open part over its PresetsPart and MediaPart.
struct RealOpen {
    explicit RealOpen(std::string model = "test-dot", std::string settingsIni = {})
        : fixture(options(std::move(model), std::move(settingsIni))) {}

    FixtureOptions options(std::string model, std::string ini) {
        FixtureOptions o;
        o.model = std::move(model);
        o.settingsIni = std::move(ini);
        o.moreParts = [this](MediaFixture& f) -> std::vector<calcu::sessions::SessionPart*> {
            OpenSetup s;
            s.productName = "Test-Product";
            s.presets = f.presetsPart.get();
            s.presetExtensions = {".TEST-SEG", ".test-dot"};
            s.media = f.media.get();
            part = std::make_unique<OpenPart>(std::move(s));
            return {part.get()};
        };
        return o;
    }

    OpenedFile onDisk(const calcu::testing::TempFolder& dir, const std::string& name) {
        const std::string path = dir / name;
        const auto bytes = calcu::testing::readText(dir.path() / name);
        return OpenedFile{name, path, std::vector<std::uint8_t>(bytes.begin(), bytes.end())};
    }
    CallId open(OpenedFile f, Way w = Way::Drop, bool upload = false) {
        return fixture.send("open", OpenRequest{{std::move(f)}, w, upload, false}.toValue());
    }
    const calcu::sessions::ReplyMessage* opened(OpenedFile f, Way w = Way::Drop, bool upload = false) {
        const CallId c = open(std::move(f), w, upload);
        fixture.callWhileBusy();
        return fixture.fe.reply(c);
    }

    std::unique_ptr<OpenPart> part;
    MediaFixture fixture;   // last: built after the part's holder
};

const OpenReply* replyOf(const calcu::sessions::ReplyMessage* r) {
    return r ? calcu::sessions::payloadAs<OpenReply>(r->result) : nullptr;
}

} // namespace

TEST(open_preset_through_presets_part) {
    RealOpen r("test-seg");
    calcu::testing::TempFolder dir("calcu-open-presets");
    dir.write("Game.Test-Dot", header("test-dot"));
    const auto* a = r.opened(r.onDisk(dir, "Game.Test-Dot"));   // the extension in any case
    CHECK(a && a->done && replyOf(a) && replyOf(a)->as == OpenedAs::Preset);
    CHECK(r.fixture.machine() && r.fixture.machine()->descriptor().model.value == "test-dot");
    // A preset by its model line, without a preset extension (test 5), as an upload: its content.
    const std::string text = header("test-seg");
    const auto* b = r.opened(OpenedFile{"notes.txt", "/uploads/notes.txt", std::vector<std::uint8_t>(text.begin(), text.end())},
                             Way::Drop, true);
    CHECK(b && b->done && r.fixture.machine()->descriptor().model.value == "test-seg");
    // Through the presets part: an unreadable preset is refused, one notice.
    r.fixture.notices();
    dir.write("bad.test-seg", "format-version: 1\nmodel: test-seg\n");
    const auto* c = r.opened(r.onDisk(dir, "bad.test-seg"));
    CHECK(c && !c->done && !c->reason.empty());
    const auto n = r.fixture.notices();
    CHECK_EQ(n.size(), std::size_t(1));
    CHECK(!n.empty() && n[0] == (c ? c->reason : std::string()));
}

TEST(open_medium_through_media_part) {
    RealOpen r("test-dot");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("Away.testdisk.yaml", disk("Away", "a").text());
    const auto* a = r.opened(r.onDisk(outside, "Away.testdisk.yaml"));
    CHECK(a && a->done && replyOf(a) && replyOf(a)->as == OpenedAs::Medium && replyOf(a)->detail == "disk");
    CHECK_EQ(r.fixture.drive()->bytes(), std::string("a"));
    r.fixture.drive()->write("written back");
    r.fixture.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(outside.path() / "Away.testdisk.yaml"), "content"), std::string("written back"));
    CHECK(!r.fixture.exists("Away.testdisk.yaml"));   // not copied
    // An upload: imported into the save folder, then inserted; the name taken asks.
    const std::string bytes = disk("Work", "up").text();
    const OpenedFile up{"Work.testdisk.yaml", "/uploads/Work.testdisk.yaml", std::vector<std::uint8_t>(bytes.begin(), bytes.end())};
    const auto* b = r.opened(up, Way::Drop, true);
    CHECK(b && b->done && r.fixture.exists("Work.testdisk.yaml"));
    const auto* again = r.opened(up, Way::Drop, true);
    CHECK(again && !again->done && again->reason.rfind("asks: ", 0) == 0);
    // A card has no place on the test-dot: refused, saying where it goes.
    outside.write("C.testcard.yaml", card("C", "c").text());
    const auto* c = r.opened(r.onDisk(outside, "C.testcard.yaml"));
    CHECK(c && !c->done && c->reason.find("The test-dot has no place for C.testcard.yaml now; a test card goes into") == 0);
    // A file of another version: media's refusal.
    TestFile old = disk("Old", "o");
    old.version = 1;
    outside.write("Old.testdisk.yaml", old.text());
    const auto* d = r.opened(r.onDisk(outside, "Old.testdisk.yaml"));
    CHECK(d && !d->done && d->reason == "The test disk file Old.testdisk.yaml cannot be read: Its version is not known.");
}

TEST(open_pass_through_follows_product) {
    // A medium for a pass-through place is fitted (as a TI magnetic card is),
    // used where it lies, fed by a side and written back there.
    RealOpen r("test-seg");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("Away.testcard.yaml", card("Away", "a").text());
    outside.write("Next.testcard.yaml", card("Next", "n").text());
    const auto* a = r.opened(r.onDisk(outside, "Away.testcard.yaml"));
    CHECK(a && a->done && replyOf(a) && replyOf(a)->as == OpenedAs::Medium && replyOf(a)->detail == "reader");
    CHECK(r.fixture.place("reader")->holds && r.fixture.place("reader")->holds->name == "Away");
    CHECK(!r.fixture.reader()->side() && r.fixture.reader()->inserts == 0);   // fitted, not fed
    for (int side : {0, 1}) {
        CHECK(r.fixture.ok("media.feed", Value::object({{"place", "reader"}, {"side", side}})));
        r.fixture.reader()->writeOnPass = "pass " + std::to_string(side);
        r.fixture.session->advanceUnpaced(calcu::machine::test::kPassTicks + 10);
        CHECK_EQ(field(calcu::testing::readText(outside.path() / "Away.testcard.yaml"), "content"),
                 "pass " + std::to_string(side));
    }
    CHECK(!r.fixture.exists("Away.testcard.yaml"));   // not copied
    // While a card passes, nothing else is opened into the reader.
    CHECK(r.fixture.ok("media.feed", Value::object({{"place", "reader"}, {"side", 0}})));
    const auto* b = r.opened(r.onDisk(outside, "Next.testcard.yaml"));
    CHECK(b && !b->done && b->reason == "A test card is passing through reader now.");
    r.fixture.session->advanceUnpaced(calcu::machine::test::kPassTicks + 10);
    const auto* c = r.opened(r.onDisk(outside, "Next.testcard.yaml"));
    CHECK(c && c->done);
    r.fixture.ok("media.refresh");
    CHECK(r.fixture.place("reader")->holds && r.fixture.place("reader")->holds->name == "Next");
}

TEST(no_way_in_changes_open_folders) {
    calcu::testing::TempFolder home("calcu-open-settings");
    const std::string ini = "[folders]\npreset = /some/where\ntest-disk = fixed:/disks\n";
    home.write("settings.ini", ini);
    RealOpen r("test-dot", home / "settings.ini");
    calcu::testing::TempFolder dir("calcu-open-presets");
    dir.write("g.test-dot", header("test-dot"));
    dir.write("W.testdisk.yaml", disk("W", "w").text());
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness}) {
        const auto* p = r.opened(r.onDisk(dir, "g.test-dot"), w);
        CHECK(p && p->done);
        const auto* m = r.opened(r.onDisk(dir, "W.testdisk.yaml"), w);
        CHECK(m && m->done);
    }
    r.fixture.session->close();
    CHECK_EQ(calcu::testing::readText(home.path() / "settings.ini"), ini);   // [folders] as it was
}
