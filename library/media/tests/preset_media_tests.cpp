#include "MediaFixture.hpp"
#include "TestHarness.hpp"
#include "presets/BundleFile.hpp"

// LIB-7 L8 (design §6): media in presets (AC-21 to AC-27, EC-5, EC-6), with
// the clauses of earlier designs that pass here: LIB-6 AC-3 (a module by
// file:, LIB-6 D-29), LIB-18 AC-2 and AC-7 (the media clauses, LIB-18 D-19)
// and the bundle check of bundled media (bundle.md §3).

using namespace calcu;
using namespace calcu::media;
using namespace calcu::media::test;

namespace {

FixtureOptions seg() {
    FixtureOptions o;
    o.prepare = [](auto& s, auto& b) {
        b.write("modules/Blank.testmodule.yaml", moduleFile("Blank", "").text());
        s.write("Bank.testmodule.yaml", moduleFile("Bank", "bank").text());
        s.write("Prog.testcard.yaml", card("Prog", "p").text());
    };
    return o;
}

FixtureOptions dot() {
    FixtureOptions o;
    o.model = "test-dot";
    o.prepare = [](auto& s, auto& b) {
        b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "f").text());
        s.write("Work.testdisk.yaml", disk("Work", "w").text());
        s.write("Two.testdisk.yaml", disk("Two", "2").text());
        TestFile old = disk("Old", "o");
        old.version = 1;
        s.write("Old.testdisk.yaml", old.text());
        s.write("Broken.testdisk.yaml", "broken\n");
    };
    return o;
}

std::string keys(const std::string& model, const std::string& steps) { return header(model) + "keys:\n" + steps; }

} // namespace

TEST(ac21_reference_mapping_name_file_mem_side) {
    MediaFixture f(seg());
    CHECK(f.readError(keys("test-seg", "  - test-card: { name: Prog, side: 1 }\n")).empty());
    CHECK(f.readError(keys("test-seg", "  - test-card: { file: " + (f.saveRoot / "Prog.testcard.yaml") + " }\n")).empty());
    CHECK(f.readError(keys("test-seg", "  - test-card:\n      mem: X\n      side: 0\n")).empty());   // block form
    CHECK(f.readError(header() + "slot-1: { name: Bank }\n").empty());
    CHECK(hasLine({f.readError(keys("test-seg", "  - test-card: { name: Prog, file: x.testcard.yaml }\n"))}, "exactly one of"));
    CHECK(hasLine({f.readError(keys("test-seg", "  - test-card: { name: Prog, side: 2 }\n"))}, "side: is 0 or 1"));
    CHECK(hasLine({f.readError(keys("test-seg", "  - test-arm: { name: Prog, side: 1 }\n"))}, "does not take side:"));
    MediaFixture d(dot());
    CHECK(hasLine({d.readError(keys("test-dot", "  - test-disk: { name: Work, mem: X }\n"))}, "does not take mem:"));
    CHECK(hasLine({d.readError(keys("test-dot", "  - test-disk: { name: Work, side: 0 }\n"))}, "does not take side:"));   // one side only
}

TEST(ac21_bare_value_refused) {
    MediaFixture f(dot());
    CHECK(hasLine({f.readError(keys("test-dot", "  - test-disk: Work\n"))}, "test-disk: takes a reference"));
    MediaFixture s(seg());
    CHECK(hasLine({s.readError(keys("test-seg", "  - test-card: Prog\n"))}, "test-card: takes a reference"));
}

TEST(ac21_old_file_keys_refused) {
    MediaFixture f(seg());
    CHECK(hasLine({f.readError(header() + "slot-1-file: Bank.testmodule.yaml\n")}, "Unknown key slot-1-file"));
    MediaFixture d(dot());
    CHECK(hasLine({d.readError(header("test-dot") + "floppy-file: Work.testdisk.yaml\n")}, "Unknown key floppy-file"));
}

TEST(ac22_in_file_media_from_header_key_saved_only_by_saveas) {
    MediaFixture f(seg());
    f.logLines();
    CHECK(f.done(f.load(header() + "test-cards:\n  - { name: Defined, content: def }\n"
                                   "keys:\n  - test-card: { mem: Defined }\n")));
    CHECK_EQ(contentOf(f.media->bindingOf("reader")->content), std::string("def"));
    CHECK(f.place("reader")->holds->name == "Defined" && f.place("reader")->holds->isTemplate);
    CHECK(!f.exists("Defined.testcard.yaml"));
    CHECK(f.done(f.load(header() + "test-cards:\n  - { name: Defined, content: def }\n"
                                   "keys:\n  - test-card: { mem: Defined }\n"
                                   "  - saveas: { test-card: Defined, as: template, name: Kept }\n")));
    CHECK_EQ(field(f.saved("Kept.testcard.yaml"), "content"), std::string("def"));
    CHECK_EQ(field(f.saved("Kept.testcard.yaml"), "template"), std::string("true"));
    // The store goes with the next load (D-10).
    CHECK(f.done(f.load(keys("test-seg", "  - test-card: { mem: Defined }\n"))));
    CHECK_EQ(contentOf(f.media->bindingOf("reader")->content), std::string(""));
}

TEST(ac23_missing_outdated_unreadable_refused_at_read) {
    MediaFixture f(dot());
    CHECK(hasLine({f.readError(keys("test-dot", "  - test-disk: { name: Nowhere }\n"))}, "there is no test disk “Nowhere”"));
    CHECK(hasLine({f.readError(keys("test-dot", "  - test-disk: { name: Old }\n"))},
                  "is outdated: The test disk format version 1 is outdated: the current version is 2"));
    CHECK(hasLine({f.readError(keys("test-dot", "  - test-disk: { file: " + (f.saveRoot / "Broken.testdisk.yaml") + " }\n"))},
                  "cannot be read"));
    CHECK(hasLine({f.readError(keys("test-dot", "  - test-disk: { file: " + (f.saveRoot / "Gone.testdisk.yaml") + " }\n"))},
                  "Gone.testdisk.yaml"));
    MediaFixture s(seg());
    CHECK(hasLine({s.readError(header() + "slot-1: { file: " + (s.saveRoot / "Missing.testmodule.yaml") + " }\n")},
                  "Missing.testmodule.yaml"));
    CHECK(s.machine()->model() == mt::kSeg);   // nothing was built
}

TEST(ac23_undefined_mem_is_fresh) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(keys("test-seg", "  - test-card: { mem: Undefined }\n"))));
    CHECK_EQ(contentOf(f.media->bindingOf("reader")->content), std::string(""));
    CHECK(f.place("reader")->holds->name == "Undefined");
}

TEST(ac24_slot_instance_fitted_before_start_and_autosaved) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(header() + "slot-1: { name: Bank }\n")));
    CHECK(f.module() && f.module()->bytes() == "bank");   // fitted at the build: no power cycle for it
    CHECK(f.machine()->operationsTold(startup::test::at({"slot-1"})) == 0);
    const PlaceState* p = f.place("slot-1");
    CHECK(p->holds && p->holds->name == "Bank" && !p->holds->isTemplate);
    CHECK(f.module()->write("bank 2"));
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Bank.testmodule.yaml"), "content"), std::string("bank 2"));
}

TEST(ac24_step_inserts_instance) {
    MediaFixture f(dot());
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { name: Work }\n"))));
    CHECK_EQ(f.drive()->bytes(), std::string("w"));
    f.drive()->write("w2");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("w2"));
}

TEST(ac25_template_file_never_written_wherever) {
    MediaFixture f(dot());
    f.presetsDir.write("disks/Here.testdisk.yaml", disk("Here", "h", true).text());
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { file: disks/Here.testdisk.yaml }\n"), "here.test-dot")));
    f.drive()->write("changed");
    f.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(f.presetsDir.path() / "disks/Here.testdisk.yaml"), "content"), std::string("h"));
    CHECK(f.place("disk")->nameAndSave);   // Name & Save is offered
}

TEST(ac25_instance_by_file_written_in_place_rerun_starts_from_it) {
    MediaFixture f(dot());
    f.presetsDir.write("disks/Run.testdisk.yaml", disk("Run", "start").text());
    const std::string preset = keys("test-dot", "  - test-disk: { file: disks/Run.testdisk.yaml }\n");
    CHECK(f.done(f.load(preset, "run.test-dot")));
    f.drive()->write("after run 1");
    f.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(f.presetsDir.path() / "disks/Run.testdisk.yaml"), "content"), std::string("after run 1"));
    CHECK(f.done(f.load(preset, "run.test-dot")));
    CHECK_EQ(f.drive()->bytes(), std::string("after run 1"));   // a preset run again starts from what the last run left
}

TEST(ac25_log_names_kind_and_name_tag) {
    MediaFixture f(dot());
    f.logLines();
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { file: " + (f.saveRoot / "Work.testdisk.yaml") + " }\n"))));
    f.drive()->write("logged");
    f.ok("media.refresh");
    const auto lines = f.logLines();
    CHECK(hasLine(lines, "test-disk: test disk “Work”: done"));
    CHECK(hasLine(lines, "Saved test disk “Work”."));
    for (const auto& l : lines) CHECK(l.find(f.saveRoot.str()) == std::string::npos);   // never the path
}

TEST(ac26_steps_table) {
    MediaFixture f(dot());
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { name: Work }\n  - eject: disk\n"))));
    CHECK(!f.place("disk")->holds && !f.drive()->side());   // eject: the place shows as empty
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { name: Formatted }\n"
                                         "  - saveas: { place: disk, as: live, name: Result }\n"))));
    CHECK(f.exists("Result.testdisk.yaml") && f.place("disk")->holds->name == "Result");
    CHECK(hasLine({f.readError(keys("test-dot", "  - eject: card\n"))}, "eject: takes disk"));
    CHECK(hasLine({f.readError(keys("test-dot", "  - saveas: { place: disk, name: X }\n"))}, "as: is live or template"));
    CHECK(hasLine({f.readError(keys("test-dot", "  - saveas: { place: disk, as: live }\n"))}, "exactly one of name: and file:"));
    MediaFixture s(seg());
    CHECK(s.done(s.load(keys("test-seg", "  - test-arm: { name: Prog }\n"))));
    CHECK(s.place("reader")->armed == std::optional<std::string>("Prog") && !s.reader()->side());   // armed, not inserted
    CHECK(s.ok("media.feed", Value::object({{"place", "reader"}, {"side", 1}})));
    CHECK(s.reader()->side() == std::optional<int>(1));
}

TEST(ac26_pass_through_step_finishes_when_passed_within_timeout) {
    MediaFixture f(seg());
    const machine::Ticks before = f.machine()->tickCount();
    CHECK(f.done(f.load(keys("test-seg", "  - test-card: { name: Prog }\n"))));
    CHECK(!f.reader()->side());                                  // it has passed when the step finished
    CHECK(f.machine()->tickCount() >= mt::kPassTicks);           // in emulated time (LIB-6 AC-19)
    (void)before;
    // A card the machine does not take is waited for within timeout: (LIB-6 AC-57).
    f.machines.builder_.onBuild = [](mt::TestMachine& m) { m.mediaPlace("reader")->holdsBack = true; };
    const CallId c = f.load(header() + "timeout: 1\nkeys:\n  - test-card: { name: Prog }\n");
    CHECK(!f.done(c) && f.reason(c).find("has not passed after the timeout") != std::string::npos);
    CHECK(f.machine()->tickCount() >= mt::kSegRate);   // one second of emulated time
}

TEST(ac27_armed_never_fed_logged_at_next_build) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(keys("test-seg", "  - test-arm: { name: Prog }\n"))));
    f.logLines();
    CHECK(f.done(f.load(header())));
    CHECK(hasLine(f.logLines(), "The test card “Prog” was armed and never fed."));
    CHECK(!f.place("reader")->armed);
}

TEST(ec5_insert_ejects_held_first) {
    MediaFixture f(dot());
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { name: Work }\n"))));
    f.drive()->write("w-pending");
    CHECK(f.done(f.step("test-disk", "{ name: Two }")));
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("w-pending"));   // AC-13 first
    CHECK_EQ(f.drive()->bytes(), std::string("2"));
    CHECK(f.place("disk")->holds->name == "Two");
}

TEST(ec6_eject_empty_place_nothing) {
    MediaFixture f(dot());
    CHECK(f.done(f.step("eject", "disk")));
    CHECK(!f.place("disk")->holds);
    CHECK(f.ok("media.eject", Value::object({{"place", "disk"}})));
}

TEST(lib6_ac3_module_by_file_reference) {
    MediaFixture f(seg());
    CHECK(f.done(f.load(header() + "slot-1: { file: " + (f.saveRoot / "Bank.testmodule.yaml") + " }\n")));
    CHECK(f.module() && f.module()->bytes() == "bank");
    CHECK(f.place("slot-1")->holds->name == "Bank");
    CHECK(hasLine({f.readError(header() + "slot-1: { file: other.txt }\n")}, "is not a module medium"));
}

TEST(lib6_check_bundle_accepts_bundled_media) {
    MediaFixture f(dot());
    calcu::testing::TempFolder b("calcu-media-bundle");
    b.write("bundle.yaml", "format: example-bundle\nformat-version: 1\nstartup: []\ngroups:\n  - group: G\n    examples:\n"
                           "      - preset: { file: p.test-dot }\n");
    b.write("disks/Formatted.testdisk.yaml", disk("Formatted", "f").text());   // the product's bundled medium
    b.write("p.test-dot", keys("test-dot", "  - test-disk: { file: disks/Formatted.testdisk.yaml }\n"));
    presets::DirectoryPresetBundle bundle(b.path());
    const auto problems = presets::checkBundle(bundle, f.presetsPart->readSetup());
    CHECK(problems.empty());
    b.write("disks/Own.testdisk.yaml", disk("Own", "o").text());
    b.write("q.test-dot", keys("test-dot", "  - test-disk: { file: disks/Own.testdisk.yaml }\n"));
    b.write("bundle.yaml", "format: example-bundle\nformat-version: 1\nstartup: []\ngroups:\n  - group: G\n    examples:\n"
                           "      - preset: { file: q.test-dot }\n");
    CHECK(presets::checkBundle(bundle, f.presetsPart->readSetup()).size() == 1);   // not listed, not a bundled medium
}

TEST(lib18_ac2_what_was_saved_as_a_medium_stays_saved) {
    MediaFixture f(dot());
    CHECK(f.done(f.load(keys("test-dot", "  - test-disk: { name: Work }\n"))));
    f.drive()->write("kept");
    CHECK(f.ok("model", Value::object({{"name", "test-seg"}})));   // a new machine, the old one gone
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("kept"));
    CHECK(f.ok("model", Value::object({{"name", "test-dot"}})));
    CHECK(!f.drive()->side());   // the new machine's memory is its own; the medium is in the save folder
    CHECK(f.ok("media.choose", Value::object({{"place", "disk"}, {"entry", "saved:Work.testdisk.yaml"}})));
    CHECK_EQ(f.drive()->bytes(), std::string("kept"));
}

TEST(lib18_ac7_variant_switch_keeps_the_media) {
    MediaFixture f(dot());
    f.saveFile("Card.testmodule.yaml", moduleFile("Card", "c"));
    CHECK(f.done(f.load(header("test-dot") + "card-1: { name: Card }\nkeys:\n  - test-disk: { name: Work }\n")));
    f.drive()->write("unsaved before switch");
    CHECK(f.module("card-1")->write("module before switch"));
    CHECK(f.ok("variant", Value::object({{"name", "V2"}})));
    CHECK(f.machine()->variant() == std::optional<machine::VariantId>(machine::VariantId{"V2"}));
    CHECK_EQ(f.drive()->bytes(), std::string("unsaved before switch"));   // the same media in them
    CHECK(f.module("card-1") && f.module("card-1")->bytes() == "module before switch");
    CHECK(f.place("disk")->holds->name == "Work" && f.place("card-1")->holds->name == "Card");
    f.drive()->write("after switch");
    f.ok("media.refresh");
    CHECK_EQ(field(f.saved("Work.testdisk.yaml"), "content"), std::string("after switch"));   // still that instance
    CHECK_EQ(field(f.saved("Card.testmodule.yaml"), "content"), std::string("module before switch"));
}
