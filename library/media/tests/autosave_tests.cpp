#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "settings/AtomicFile.hpp"

// Autosave and the writes that do not wait.

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions dot(sessions::Hosting h = sessions::Hosting::Paced) {
    FixtureOptions o;
    o.model = "test-dot";
    o.hosting = h;
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "").text());
        s.write("Work.testdisk.yaml", disk("Work", "w").text());
        s.write("Mine.testdisk.yaml", disk("Mine", "t", true).text());
    };
    return o;
}

Value choose(const std::string& place, const std::string& entry) { return Value::object({{"place", place}, {"entry", entry}}); }

std::string content(MediaFixture& f, const std::string& file = "Work.testdisk.yaml") { return field(f.saved(file), "content"); }

int savedLines(const std::vector<std::string>& lines) {
    return static_cast<int>(std::count_if(lines.begin(), lines.end(), [](const std::string& l) { return l.rfind("Saved ", 0) == 0; }));
}

} // namespace

TEST(autosave_within_half_a_second_at_60_hz) {
    for (int at = 0; at < 30; at += 7) {   // the change at different points of the 60 Hz rhythm
        MediaFixture f(dot());
        CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
        for (int i = 0; i < at; ++i) f.call(16'667us);
        const pacing::WallTime before = f.clock.now;   // the change comes in the next call's span
        f.drive()->write("changed");
        pacing::WallTime written{0};
        for (int i = 0; i < 60 && written == pacing::WallTime{0}; ++i) {
            f.call(16'667us);
            if (content(f) == "changed") written = f.clock.now;
        }
        CHECK(written != pacing::WallTime{0});
        CHECK(written - before <= 500ms + 16'667us);   // at the latest half a second after the change
        CHECK(written - before >= 400ms);              // and not at once
    }
}

TEST(autosave_not_while_nothing_changes) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.appLog.clear();
    for (int i = 0; i < 120; ++i) f.call(16'667us);
    CHECK_EQ(savedLines(f.appLog.take()), 0);
    f.drive()->write("once");
    for (int i = 0; i < 120; ++i) f.call(16'667us);
    CHECK_EQ(savedLines(f.appLog.take()), 1);
    // What the app did for the user: at info, and not in the calculator's log.
    f.drive()->write("twice");
    for (int i = 0; i < 120; ++i) f.call(16'667us);
    CHECK(f.appLog.has(calcu::applog::AppLogLevel::Info, "Saved test disk “Work”."));
    CHECK_EQ(savedLines(f.logLines()), 0);
}

// A place whose writes wait for its own moments (a tape: the motor stopped)
// is not written half a second after a change, but when it asks, and before
// it leaves its place.
TEST(autosave_writes_when_asked_wait_for_the_moment) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->onlyWhenAsked = true;
    f.drive()->write("recording");
    for (int i = 0; i < 120; ++i) f.call(16'667us);
    CHECK_EQ(content(f), std::string("w"));   // two seconds: not written
    f.drive()->requestWriteNow();
    f.call(16'667us);
    CHECK_EQ(content(f), std::string("recording"));
    f.drive()->write("more");
    for (int i = 0; i < 60; ++i) f.call(16'667us);
    CHECK_EQ(content(f), std::string("recording"));
    CHECK(f.ok("media.eject", Value::object({{"place", "disk"}})));
    CHECK_EQ(content(f), std::string("more"));   // before it leaves its place
}

TEST(autosave_harness_at_end_of_command) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("by the ROM");
    CHECK_EQ(content(f), std::string("w"));
    f.session->advanceUnpaced(10);   // the harness's `wait`: its command ends here
    CHECK_EQ(content(f), std::string("by the ROM"));
    f.drive()->write("again");
    CHECK(f.ok("media.refresh"));
    CHECK_EQ(content(f), std::string("again"));
}

TEST(pass_through_written_when_given_back_if_written) {
    FixtureOptions o;
    o.prepare = [](auto& s, auto&) { s.write("Prog.testcard.yaml", card("Prog", "p").text()); };
    MediaFixture f(o);
    f.appLog.clear();
    f.reader()->writeOnPass = "written";
    CHECK(f.done(f.step("test-card", "{ name: Prog, side: 1 }")));
    CHECK_EQ(field(f.saved("Prog.testcard.yaml"), "content"), std::string("written"));
    CHECK_EQ(savedLines(f.appLog.take()), 1);
    CHECK(f.done(f.step("test-card", "{ name: Prog }")));   // read only: not written
    CHECK_EQ(savedLines(f.appLog.take()), 0);
    CHECK(f.place("reader")->holds && f.place("reader")->holds->name == "Prog");
}

TEST(fed_again_inserts_what_the_last_pass_wrote) {
    FixtureOptions o;
    o.prepare = [](auto& s, auto&) {
        s.write("Prog.testcard.yaml", card("Prog", "p").text());
        s.write("Blank.testcard.yaml", card("Blank", "", true).text());
    };
    MediaFixture f(o);
    const Value side0 = Value::object({{"place", "reader"}, {"side", 0}});
    // An instance: written at each give-back, and the fitted card is what was written.
    CHECK(f.ok("media.arm", Value::object({{"place", "reader"}, {"entry", "saved:Prog.testcard.yaml"}})));
    CHECK(f.ok("media.feed", side0));
    f.reader()->writeOnPass = "first";
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    CHECK_EQ(field(f.saved("Prog.testcard.yaml"), "content"), std::string("first"));
    CHECK(f.ok("media.feed", side0));
    CHECK_EQ(f.reader()->bytes(), std::string("first"));
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    // A template: never written; what was written stays with the fitted card.
    CHECK(f.ok("media.arm", Value::object({{"place", "reader"}, {"entry", "saved:Blank.testcard.yaml"}})));
    CHECK(f.ok("media.feed", side0));
    f.reader()->writeOnPass = "on the template";
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    CHECK(f.ok("media.refresh"));
    CHECK(f.place("reader")->holds && f.place("reader")->holds->unsaved);
    CHECK_EQ(field(f.saved("Blank.testcard.yaml"), "content"), std::string(""));
    CHECK(f.ok("media.feed", Value::object({{"place", "reader"}, {"side", 1}})));
    CHECK_EQ(f.reader()->bytes(), std::string("on the template"));
}

TEST(medium_written_before_leaving_place) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("leaving");
    CHECK(f.ok("media.eject", Value::object({{"place", "disk"}})));
    CHECK_EQ(content(f), std::string("leaving"));
    CHECK(!f.place("disk")->holds);
}

TEST(medium_written_before_operation) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("before reset");
    const CallId c = f.send("reset");
    CHECK(f.session->busy());   // the operation has started, not ended
    CHECK_EQ(content(f), std::string("before reset"));
    f.callWhileBusy();
    CHECK(f.done(c));
}

TEST(medium_written_before_machine_change) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("before the build");
    auto old = f.session->machineSession().takeMachine();   // MachineSession: before the machine goes
    CHECK_EQ(content(f), std::string("before the build"));
    MediaFixture g(dot());
    CHECK(g.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    g.drive()->write("before the attach");
    const CallId c = g.send("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}}));
    g.callWhileBusy();
    CHECK(g.done(c));
    CHECK_EQ(content(g), std::string("before the attach"));
}

TEST(medium_written_on_suspend) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("hidden tab");
    CHECK(f.ok("suspend"));
    CHECK_EQ(content(f), std::string("hidden tab"));
}

TEST(medium_written_on_close) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.drive()->write("quit");
    f.session->close();   // the window closes (the host, before it destroys the session)
    CHECK_EQ(content(f), std::string("quit"));
}

TEST(failed_medium_write_one_notice_until_success) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    f.notices();
    settings::testing::failBeforeRename(true);
    f.drive()->write("one");
    f.ok("media.refresh");
    f.drive()->write("two");
    f.ok("media.refresh");
    auto n = f.notices();
    CHECK(n.size() == 1 && n[0].rfind("The test disk “Work” could not be saved: ", 0) == 0);
    settings::testing::failBeforeRename(false);
    f.drive()->write("three");
    f.ok("media.refresh");
    CHECK_EQ(content(f), std::string("three"));
    settings::testing::failBeforeRename(true);
    f.drive()->write("four");
    f.ok("media.refresh");
    settings::testing::failBeforeRename(false);
    CHECK_EQ(f.notices().size(), std::size_t(1));   // again, after a write succeeded
}

TEST(failed_medium_write_next_change_tries_again) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    settings::testing::failBeforeRename(true);
    f.drive()->write("lost for now");
    f.ok("media.refresh");
    settings::testing::failBeforeRename(false);
    // The app log has the cause, with the file.
    CHECK(f.appLog.has(calcu::applog::AppLogLevel::Warn, "could not be saved to "));
    CHECK(f.appLog.has(calcu::applog::AppLogLevel::Warn, "the write was interrupted before the rename"));
    f.appLog.clear();
    f.ok("media.refresh");
    CHECK_EQ(savedLines(f.appLog.take()), 0);   // no change, no retry
    f.drive()->write("next");
    f.ok("media.refresh");
    CHECK_EQ(content(f), std::string("next"));
}

TEST(picker_template_not_written_lives_in_place) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    const std::string before = f.saved("Mine.testdisk.yaml");
    CHECK(f.ok("media.choose", choose("disk", "saved:Mine.testdisk.yaml")));
    f.drive()->write("lives here");
    f.ok("media.refresh");
    CHECK_EQ(f.saved("Mine.testdisk.yaml"), before);
    CHECK(f.place("disk")->holds->unsaved && f.place("disk")->nameAndSave);
    CHECK_EQ(f.drive()->bytes(), std::string("lives here"));
    CHECK(f.ok("media.eject", Value::object({{"place", "disk"}})));   // it leaves: what it held is gone
    CHECK(f.ok("media.choose", choose("disk", "saved:Mine.testdisk.yaml")));
    CHECK_EQ(f.drive()->bytes(), std::string("t"));
}

TEST(preset_named_template_not_written) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    const std::string before = f.saved("Mine.testdisk.yaml");
    CHECK(f.done(f.load(header("test-dot") + "keys:\n  - test-disk: { file: " + (f.saveRoot / "Mine.testdisk.yaml") + " }\n")));
    f.drive()->write("by the preset's machine");
    f.ok("media.refresh");
    CHECK_EQ(f.saved("Mine.testdisk.yaml"), before);
    CHECK(f.done(f.load(header("test-dot") + "keys:\n  - test-disk: { name: Formatted }\n")));
    f.drive()->write("on a bundled template");
    f.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(f.bundled.path() / "disks/Formatted.testdisk.yaml"), "content"), std::string(""));
}

TEST(in_file_medium_not_written_unless_saveas) {
    MediaFixture f;
    CHECK(f.done(f.load(header() + "test-cards:\n  - { name: A, content: abc }\nkeys:\n  - test-card: { mem: A }\n  - wait: 10ms\n")));   // not last: waited for
    CHECK_EQ(contentOf(f.media->bindingOf("reader")->content), std::string("abc"));
    f.reader()->writeOnPass = "first";
    CHECK(f.done(f.step("test-card", "{ mem: A }")));
    CHECK(!f.exists("A.testcard.yaml"));
    // The store kept what was written: the next pass reads it.
    CHECK(f.done(f.step("test-card", "{ mem: A }")));
    CHECK_EQ(contentOf(f.media->bindingOf("reader")->content), std::string("first"));
    CHECK(f.done(f.step("saveas", "{ test-card: A, as: live, name: Card A }")));
    CHECK_EQ(field(f.saved("Card A.testcard.yaml"), "content"), std::string("first"));
    f.reader()->writeOnPass = "second";
    CHECK(f.done(f.step("test-card", "{ mem: A }")));   // that instance from then on
    CHECK_EQ(field(f.saved("Card A.testcard.yaml"), "content"), std::string("second"));
}

TEST(save_folder_made_at_first_write) {
    FixtureOptions o = dot(sessions::Hosting::VirtualTime);
    o.saveFolderMissing = true;
    o.prepare = [](auto&, auto& b) { b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "").text()); };
    MediaFixture f(o);
    CHECK(!std::filesystem::exists(f.saveRoot.path() / "not-yet"));
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    CHECK(f.ok("media.name-and-save", Value::object({{"place", "disk"}, {"name", "First"}})));
    CHECK(std::filesystem::exists(f.saveRoot.path() / "not-yet" / "First.testdisk.yaml"));
}

TEST(outside_change_overwritten_not_merged) {
    MediaFixture f(dot(sessions::Hosting::VirtualTime));
    CHECK(f.ok("media.choose", choose("disk", "saved:Work.testdisk.yaml")));
    TestFile outside = disk("Work", "from outside");
    outside.description = "edited by hand";
    f.saveFile("Work.testdisk.yaml", outside);
    f.drive()->write("product");
    f.ok("media.refresh");
    CHECK_EQ(content(f), std::string("product"));
    CHECK(f.saved("Work.testdisk.yaml").find("from outside") == std::string::npos);
}

TEST(suspend_writes_every_pending_change) {
    MediaFixture f(dot());
    CHECK(f.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    f.saveFile("Two.testdisk.yaml", disk("Two", "2"));
    CHECK(f.ok("media.refresh"));
    CHECK(f.ok("media.choose", choose("drive", "saved:Work.testdisk.yaml")));
    CHECK(f.ok("media.choose", choose("hub-disk", "saved:Two.testdisk.yaml")));
    f.drive()->write("one pending");
    f.machine()->mediaPlace("hub-disk", startup::test::at({"a"}))->write("two pending");
    f.call(16'667us);
    CHECK(f.ok("suspend"));
    CHECK_EQ(content(f), std::string("one pending"));
    CHECK_EQ(content(f, "Two.testdisk.yaml"), std::string("two pending"));
}

namespace {
// The test card's codec, recording the source each write is handed.
std::vector<std::optional<std::string>> g_sources;
struct RecordingCodec final : MediumCodec {
    const MediumCodec* inner = nullptr;
    ReadOutcome read(const std::string& b, const Companion& c) const override { return inner->read(b, c); }
    std::vector<FileOut> write(const Write& w, std::string& problem) const override {
        g_sources.push_back(w.source);
        return inner->write(w, problem);
    }
};
RecordingCodec g_recording;
// A host's file access whose reads fail while `unreadable` is set.
struct FlakyAccess final : FileAccess {
    bool unreadable = false;
    Read read(const std::string& path) const override {
        if (unreadable) return {std::nullopt, true};
        return FileAccess::read(path);
    }
};
} // namespace

// A kind that asks for it gets the instance's file as it is just before each
// write, not the bytes last read (the magnetic card keeps the other side).
TEST(reread_before_write_hands_current_file_as_source) {
    g_sources.clear();
    auto access = std::make_shared<FlakyAccess>();
    FixtureOptions o;
    o.files = access;
    o.prepare = [](auto& s, auto&) { s.write("Prog.testcard.yaml", card("Prog", "p").text()); };
    o.kinds = [](std::vector<KindDecl>& ks) {
        for (KindDecl& k : ks)
            if (k.id == "test-card") {
                g_recording.inner = k.codec;
                k.codec = &g_recording;
                k.rereadBeforeWrite = true;
            }
    };
    MediaFixture f(o);
    CHECK(f.ok("media.arm", Value::object({{"place", "reader"}, {"entry", "saved:Prog.testcard.yaml"}})));
    TestFile outside = card("Prog", "changed elsewhere");
    outside.description = "by another device";
    f.saveFile("Prog.testcard.yaml", outside);   // after it was fitted
    CHECK(f.ok("media.feed", Value::object({{"place", "reader"}, {"side", 0}})));
    f.reader()->writeOnPass = "written";
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    f.ok("media.refresh");
    CHECK_EQ(g_sources.size(), std::size_t{1});
    CHECK(!g_sources.empty() && g_sources.front() && g_sources.front()->find("changed elsewhere") != std::string::npos);
    CHECK_EQ(field(f.saved("Prog.testcard.yaml"), "content"), std::string("written"));
    // A file that cannot be read then: the write fails with one notice.
    access->unreadable = true;
    CHECK(f.ok("media.feed", Value::object({{"place", "reader"}, {"side", 0}})));
    f.reader()->writeOnPass = "again";
    f.session->advanceUnpaced(mt::kPassTicks + 10);
    f.ok("media.refresh");
    CHECK(hasLine(f.notices(), "could not be saved"));
    CHECK_EQ(g_sources.size(), std::size_t{1});   // the codec was not asked
    CHECK_EQ(field(f.saved("Prog.testcard.yaml"), "content"), std::string("written"));
}
