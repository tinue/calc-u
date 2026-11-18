#include "MediaFixture.hpp"
#include "TestHarness.hpp"

// Name & Save and the save rules.

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions dot() {
    FixtureOptions o;
    o.model = "test-dot";
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "f").text());
        s.write("Tpl.testdisk.yaml", disk("Tpl", "t", true).text());
        s.write("Free.testdisk.yaml", disk("Free", "free").text());
        s.write("Busy.testdisk.yaml", disk("Busy", "busy").text());
        s.write("Broken.testdisk.yaml", "broken\n");
        TestFile old = disk("Old", "o");
        old.version = 1;
        s.write("Old.testdisk.yaml", old.text());
    };
    return o;
}

FixtureOptions seg() {
    FixtureOptions o;
    o.prepare = [](auto& s, auto& b) {
        b.write("modules/Blank.testmodule.yaml", moduleFile("Blank", "").text());
        s.write("Bank.testmodule.yaml", moduleFile("Bank", "bank").text());
    };
    return o;
}

Value choose(const std::string& place, const std::string& entry) { return Value::object({{"place", place}, {"entry", entry}}); }
Value nameAndSave(const std::string& place, const std::string& name, bool confirmed = false, bool protect = false) {
    return Value::object({{"place", place}, {"name", name}, {"confirmed", confirmed}, {"protect", protect}});
}

} // namespace

TEST(name_and_save_offered_for_template_that_can_change) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    CHECK(f.place("disk")->nameAndSave);
    CHECK(f.ok("media.choose", choose("disk", "saved:Tpl.testdisk.yaml")));
    CHECK(f.place("disk")->nameAndSave);
    CHECK(f.ok("media.choose", choose("disk", "saved:Free.testdisk.yaml")));
    CHECK(!f.place("disk")->nameAndSave);   // an instance
    MediaFixture m(seg());
    m.saveFile("Locked.testmodule.yaml", moduleFile("Locked", "l", true, true));
    CHECK(m.done(m.load(header() + "slot-1: { name: Locked }\n")));
    CHECK(!m.place("slot-1")->nameAndSave);   // its switch is on, its content cannot change
}

TEST(name_and_save_asks_write_protect_only_when_switch_off) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(header() + "slot-1: { name: Blank }\n")));
    CHECK(f.place("slot-1")->nameAndSave && f.place("slot-1")->askWriteProtect);
    MediaFixture d(dot());
    CHECK(d.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    CHECK(d.place("disk")->nameAndSave && !d.place("disk")->askWriteProtect);   // no switch
}

TEST(save_rules_table) {
    MediaFixture f(dot());
    CHECK(f.ok("attach", Value::object({{"place", "a"}, {"peripheral", "test-hub"}})));
    CHECK(f.ok("media.choose", choose("hub-disk", "saved:Busy.testdisk.yaml")));
    const KindDecl& k = *f.media->kind("test-disk");
    const InPlace inPlace = [&](const std::string& path) -> std::optional<std::string> {
        return path == f.saveRoot / "Busy.testdisk.yaml" ? std::optional<std::string>("hub-disk") : std::nullopt;
    };
    auto plan = [&](SaveBy by, const std::string& name, bool confirmed = false) {
        SaveRequest r;
        r.kind = &k;
        r.by = by;
        r.target = name;
        r.replaceConfirmed = confirmed;
        r.from = "drive";
        return planSave(r, f.media->shelf(), inPlace);
    };
    using V = Plan::Verdict;
    const SaveBy all[] = {SaveBy::NameAndSave, SaveBy::SaveasLive, SaveBy::SaveasTemplate};
    for (SaveBy by : all) {
        CHECK(plan(by, "a/b").verdict == V::Refused);                                            // a refused name
        CHECK(plan(by, "formatted").verdict == V::Refused);                                      // a bundled name
        CHECK(plan(by, "Busy").verdict == V::Refused);                                           // in another place
        CHECK(plan(by, "Broken").verdict == V::Write && plan(by, "Broken").path == f.saveRoot / "Broken-1.testdisk.yaml");   // numbered
        CHECK(plan(by, "Old").verdict == V::Write && plan(by, "Old").path == f.saveRoot / "Old-1.testdisk.yaml");
    }
    CHECK(plan(SaveBy::NameAndSave, "Tpl").verdict == V::Refused);   // a user template's name
    CHECK(plan(SaveBy::SaveasLive, "Tpl").verdict == V::Refused);
    CHECK(plan(SaveBy::SaveasTemplate, "Tpl").verdict == V::Write && plan(SaveBy::SaveasTemplate, "Tpl").path == f.saveRoot / "Tpl.testdisk.yaml");
    CHECK(plan(SaveBy::NameAndSave, "free").verdict == V::Ask);       // an instance's name
    CHECK(plan(SaveBy::NameAndSave, "free", true).verdict == V::Write);
    CHECK(plan(SaveBy::SaveasLive, "free").verdict == V::Write && plan(SaveBy::SaveasLive, "free").path == f.saveRoot / "Free.testdisk.yaml");
    CHECK(plan(SaveBy::SaveasTemplate, "free").verdict == V::Refused);
    CHECK(plan(SaveBy::NameAndSave, "New").verdict == V::Write && plan(SaveBy::NameAndSave, "New").path == f.saveRoot / "New.testdisk.yaml");
    CHECK_EQ(plan(SaveBy::NameAndSave, "formatted").refusal, std::string("“formatted” is the name of a bundled test disk."));
    CHECK_EQ(plan(SaveBy::NameAndSave, "Busy").refusal, std::string("The test disk “Busy” is in hub-disk now."));
}

TEST(saveas_file_form_writes_that_file) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    const std::string target = f.presetsDir / "out/Work Copy.testdisk.yaml";
    CHECK(f.done(f.step("saveas", "{ place: disk, as: live, file: " + target + " }")));
    const std::string text = calcu::testing::readText(f.presetsDir.path() / "out/Work Copy.testdisk.yaml");
    CHECK_EQ(field(text, "name"), std::string("Work Copy"));   // the file name without its suffix
    CHECK_EQ(field(text, "content"), std::string("f"));
    CHECK(!f.done(f.step("saveas", "{ place: disk, as: live, file: " + (f.presetsDir / "x.txt") + " }")));
}

TEST(saveas_file_form_refused_for_bundled_and_user_template) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    const CallId b = f.step("saveas", "{ place: disk, as: template, file: " + (f.bundled / "disks/Formatted.testdisk.yaml") + " }");
    CHECK(!f.done(b) && f.reason(b).find("is never written") != std::string::npos);
    const CallId t = f.step("saveas", "{ place: disk, as: live, file: " + (f.saveRoot / "Tpl.testdisk.yaml") + " }");
    CHECK(!f.done(t) && f.reason(t).find("is a template") != std::string::npos);
    CHECK(f.done(f.step("saveas", "{ place: disk, as: template, file: " + (f.saveRoot / "Tpl.testdisk.yaml") + " }")));
    CHECK_EQ(field(f.saved("Tpl.testdisk.yaml"), "content"), std::string("f"));
    CHECK_EQ(field(f.saved("Tpl.testdisk.yaml"), "template"), std::string("true"));
}

TEST(name_and_save_asks_before_replacing) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    const CallId c = f.send("media.name-and-save", nameAndSave("disk", "Free"));
    CHECK(!f.done(c) && f.reason(c) == "asks: A test disk named “Free” exists. Replace it?");
    CHECK(f.notices().empty());   // no notice, nothing changed
    CHECK_EQ(field(f.saved("Free.testdisk.yaml"), "content"), std::string("free"));
    CHECK(f.ok("media.name-and-save", nameAndSave("disk", "FREE", true)));
    CHECK_EQ(field(f.saved("Free.testdisk.yaml"), "content"), std::string("f"));
    CHECK_EQ(field(f.saved("Free.testdisk.yaml"), "name"), std::string("FREE"));   // its file, the new spelling
}

TEST(place_holds_saved_medium_no_power_cycle) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    f.drive()->write("work");
    const auto ticks = f.machine()->tickCount();
    const CallId c = f.send("media.name-and-save", nameAndSave("disk", "Saved"));
    CHECK(f.done(c) && !f.session->busy() && f.machine()->tickCount() == ticks);
    const PlaceState* p = f.place("disk");
    CHECK(p->holds && p->holds->name == "Saved" && !p->holds->isTemplate && !p->holds->unsaved && !p->nameAndSave);
    CHECK_EQ(f.drive()->bytes(), std::string("work"));
}

TEST(saved_live_autosaved_template_not) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    CHECK(f.done(f.step("saveas", "{ place: disk, as: live, name: Live }")));
    f.drive()->write("after live");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Live.testdisk.yaml"), "content"), std::string("after live"));
    CHECK(f.done(f.step("saveas", "{ place: disk, as: template, name: AsTemplate }")));
    f.drive()->write("after template");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("AsTemplate.testdisk.yaml"), "content"), std::string("after live"));
    CHECK(f.place("disk")->holds->isTemplate);
}

TEST(protect_in_file_at_once_and_power_cycle) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(header() + "slot-1: { name: Blank }\n")));
    CHECK(f.module()->write("rom data"));
    const auto told = f.machine()->operationsTold(startup::test::at({"slot-1"}));
    CHECK(f.ok("media.name-and-save", nameAndSave("slot-1", "My ROM", false, true)));
    CHECK_EQ(field(f.saved("My ROM.testmodule.yaml"), "write-protect"), std::string("true"));
    CHECK_EQ(field(f.saved("My ROM.testmodule.yaml"), "content"), std::string("rom data"));
    CHECK(!f.module()->write("refused"));   // protected at once
    CHECK(f.machine()->operationsTold(startup::test::at({"slot-1"})) == told + 1);   // the power cycle
    CHECK(f.machine()->isKeyReady());
    CHECK(f.place("slot-1")->holds->writeProtected);
}

TEST(no_command_switches_write_protect_off) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(header() + "slot-1: { name: Blank }\n")));
    CHECK(f.ok("media.name-and-save", nameAndSave("slot-1", "Locked", false, true)));
    CHECK(!f.ok("media.name-and-save", nameAndSave("slot-1", "Unlocked")));
    CHECK(!f.done(f.step("saveas", "{ place: slot-1, as: live, name: Locked }")) || f.place("slot-1")->holds->writeProtected);
    CHECK_EQ(field(f.saved("Locked.testmodule.yaml"), "write-protect"), std::string("true"));
    CHECK(f.place("slot-1")->holds->writeProtected && !f.place("slot-1")->nameAndSave);
}

TEST(started_name_asked_first_file_at_first_write) {
    MediaFixture f(dot());
    const Value start = Value::object({{"place", "disk"}, {"template", "bundled:Formatted.testdisk.yaml"}, {"name", "Free"}});
    const CallId c = f.send("media.start-new", start);
    CHECK(!f.done(c) && f.reason(c).rfind("asks: ", 0) == 0);
    CHECK(f.ok("media.start-new", Value::object({{"place", "disk"}, {"template", "bundled:Formatted.testdisk.yaml"},
                                                 {"name", "Free"}, {"confirmed", true}})));
    CHECK_EQ(field(f.saved("Free.testdisk.yaml"), "content"), std::string("free"));   // not yet: no write so far
    CHECK(f.place("disk")->holds->name == "Free" && f.place("disk")->holds->unsaved);
    f.drive()->write("first write");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Free.testdisk.yaml"), "content"), std::string("first write"));
    CHECK_EQ(field(calcu::testing::readText(f.bundled.path() / "disks/Formatted.testdisk.yaml"), "content"), std::string("f"));
    // On a pass-through place the started medium is fitted: the card a user
    // feeds (Fit Fresh Card), side 0 then side 1, and it stays fitted.
    MediaFixture s;
    CHECK(s.ok("media.start-new", Value::object({{"place", "reader"}, {"template", "implicit:Fresh"}, {"name", "Recorded"}})));
    CHECK(s.place("reader")->holds && s.place("reader")->holds->name == "Recorded" && !s.reader()->side());
    CHECK(!s.exists("Recorded.testcard.yaml"));
    CHECK(s.ok("media.feed", Value::object({{"place", "reader"}, {"side", 0}})));
    s.reader()->writeOnPass = "recorded";
    s.session->advanceUnpaced(mt::kPassTicks + 10);
    CHECK_EQ(field(s.saved("Recorded.testcard.yaml"), "content"), std::string("recorded"));
    CHECK(s.place("reader")->holds && s.place("reader")->holds->name == "Recorded");
    CHECK(s.ok("media.feed", Value::object({{"place", "reader"}, {"side", 1}})));
    CHECK_EQ(s.reader()->bytes(), std::string("recorded"));   // what the first pass wrote
    s.reader()->writeOnPass = "both sides";
    s.session->advanceUnpaced(mt::kPassTicks + 10);
    CHECK_EQ(field(s.saved("Recorded.testcard.yaml"), "content"), std::string("both sides"));
}

TEST(saveas_protected_without_switch_fails) {
    MediaFixture f(dot());
    const CallId c = f.load(header("test-dot") + "keys:\n  - test-disk: { name: Formatted }\n"
                            "  - saveas: { place: disk, as: live, protected: true, name: P }\n");
    CHECK(!f.done(c) && f.reason(c).find("The test disk has no write-protect switch.") != std::string::npos);
    CHECK(!f.exists("P.testdisk.yaml"));
}

TEST(name_and_save_not_for_instance) {
    MediaFixture f(dot());
    CHECK(f.ok("media.choose", choose("disk", "saved:Free.testdisk.yaml")));
    const CallId c = f.send("media.name-and-save", nameAndSave("disk", "Other"));
    CHECK(!f.done(c) && f.reason(c).find("is saved already") != std::string::npos);
    CHECK(!f.exists("Other.testdisk.yaml"));
}

// A kind words the replace question itself (the magnetic card).
TEST(kind_words_replace_question) {
    FixtureOptions o;
    o.prepare = [](auto& s, auto&) { s.write("Taken.testcard.yaml", card("Taken", "t").text()); };
    o.kinds = [](std::vector<KindDecl>& ks) {
        for (KindDecl& k : ks)
            if (k.id == "test-card") k.replaceQuestion = "Replace card “{name}”? Its first write replaces both sides.";
    };
    MediaFixture f(o);
    const CallId c = f.send("media.start-new", Value::object({{"place", "reader"}, {"template", "implicit:Fresh"}, {"name", "taken"}}));
    CHECK(!f.done(c));
    CHECK_EQ(f.reason(c), std::string("asks: Replace card “Taken”? Its first write replaces both sides."));
    // The other kinds keep the library's sentence.
    MediaFixture d(dot());
    CHECK(d.ok("media.choose", choose("disk", "bundled:Formatted.testdisk.yaml")));
    const CallId e = d.send("media.name-and-save", nameAndSave("disk", "Free"));
    CHECK_EQ(d.reason(e), std::string("asks: A test disk named “Free” exists. Replace it?"));
}

// A kind without Name & Save never offers it (the magnetic card).
TEST(kind_without_name_and_save_never_offers_it) {
    FixtureOptions o;
    o.kinds = [](std::vector<KindDecl>& ks) {
        for (KindDecl& k : ks)
            if (k.id == "test-card") k.offersNameAndSave = false;
    };
    MediaFixture f(o);
    CHECK(f.ok("media.arm", Value::object({{"place", "reader"}, {"entry", "implicit:Fresh"}})));
    CHECK(f.place("reader")->holds && f.place("reader")->holds->isTemplate);
    CHECK(!f.place("reader")->nameAndSave);
    const CallId c = f.send("media.name-and-save", nameAndSave("reader", "Mine"));
    CHECK(!f.done(c) && f.reason(c).find("not saved by Name & Save") != std::string::npos);
    CHECK(!f.exists("Mine.testcard.yaml"));
    MediaFixture g;   // the default: offered
    CHECK(g.ok("media.arm", Value::object({{"place", "reader"}, {"entry", "implicit:Fresh"}})));
    CHECK(g.place("reader")->nameAndSave);
}
