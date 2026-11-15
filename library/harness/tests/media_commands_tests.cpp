// LIB-7 L9 (design §6): the harness's media commands (AC-26, AC-28; LIB-11
// AC-8, AC-22) on its test product with the test kinds (TestProduct::media).
#include "Files.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestKinds.hpp"
#include "TestProduct.hpp"

using namespace calcu;
using namespace calcu::harness;
using namespace calcu::harness::test;

namespace {

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

struct MediaRun : TestRun {
    explicit MediaRun(RunOptions o = {}) : TestRun(std::move(o), machine::ModelId{"test-dot"}, true) {}
    machine::test::TestMediaPlace* drive() { return machine().mediaPlace("drive"); }
};

} // namespace

TEST(ac28_media_reads_topic) {
    MediaRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const std::string r = run.line("media");
    CHECK(has(r, "\"kinds\"") && has(r, "\"test-disk\"") && has(r, "\"place\":\"disk\""));
    TestRun plain;   // a product without kinds of medium
    CHECK_EQ(plain.line("media"), std::string("refused: Calc-U-Test has no media.\n"));
}

TEST(ac28_run_has_empty_save_folder_and_bundled_media) {
    MediaRun run;
    if (!run.server) return;
    const std::string r = run.line("media");
    CHECK(has(r, "\"name\":\"Formatted\"") && has(r, "\"origin\":\"bundled\""));   // the bundled media are always there
    CHECK(has(r, "\"instances\":[]"));                                            // an empty save folder of its own
    calcu::testing::TempFolder save("calcu-media-run-save");
    save.write("Mine.testdisk.yaml", "format: test-disk\nformat-version: 2\nname: Mine\ncontent: m\n");
    RunOptions o;
    o.saveFolder = save.str();
    MediaRun given(o);
    CHECK(has(given.line("media"), "\"name\":\"Mine\""));
}

TEST(ac26_steps_as_commands) {
    MediaRun run;
    if (!run.server) return;
    CHECK_EQ(run.line("test-disk { name: Formatted }"), std::string("done\n"));
    CHECK_EQ(run.drive()->bytes(), std::string(""));
    run.drive()->write("work");
    CHECK_EQ(run.line("saveas { place: disk, as: live, name: Work }"), std::string("done\n"));
    run.drive()->write("work 2");
    run.line("wait 10ms");   // AC-11: written at the end of the command
    CHECK_EQ(calcu::testing::readText(run.server->setup().saveFolder() + "/Work.testdisk.yaml").find("content: work 2") != std::string::npos,
             true);
    CHECK_EQ(run.line("eject disk"), std::string("done\n"));
    CHECK(!run.drive()->side());
    CHECK(has(run.line("test-disk { name: Nowhere }"), "refused: "));
    CHECK(has(run.line("test-disk Formatted"), "takes a reference"));   // a bare value
}

TEST(ac28_import_export_delete) {
    MediaRun run;
    if (!run.server) return;
    calcu::testing::TempFolder outside("calcu-media-run-outside");
    outside.write("in.testdisk.yaml", "format: test-disk\nformat-version: 2\nname: Imported\ncompanion: snd.dat\ncontent: i\n");
    outside.write("snd.dat", "wave");
    CHECK_EQ(run.line("media import " + (outside / "in.testdisk.yaml")), std::string("done\n"));
    CHECK(has(run.line("media"), "\"name\":\"Imported\""));
    CHECK(has(run.line("media import " + (outside / "in.testdisk.yaml")), "refused: A test disk named “Imported” exists already."));
    const std::string out = outside / "out/Exported.testdisk.yaml";
    std::filesystem::create_directories(outside.path() / "out");
    CHECK(has(run.line("media export test-disk Imported " + out), "done"));
    CHECK(has(calcu::testing::readText(outside.path() / "out/Exported.testdisk.yaml"), "name: Imported"));
    CHECK_EQ(calcu::testing::readText(outside.path() / "out/snd.dat"), std::string("wave"));   // side by side
    CHECK_EQ(run.line("media delete test-disk Imported"), std::string("done\n"));   // no question
    CHECK_EQ(run.line("media delete test-disk Imported"), std::string("refused: The test disk “Imported” is no longer there.\n"));
    CHECK(!std::filesystem::exists(run.server->setup().saveFolder() + "/Imported.testdisk.yaml"));
    CHECK(has(run.line("media delete test-disk Formatted"), "is no longer there"));   // a bundled one is not deleted
}

TEST(ac26_mapping_line_as_step) {
    MediaRun run;
    if (!run.server) return;
    calcu::testing::TempFolder outside("calcu-media-run-outside");
    outside.write("Away.testdisk.yaml", "format: test-disk\nformat-version: 2\nname: Away\ncontent: a\n");
    CHECK_EQ(run.line("test-disk { file: " + (outside / "Away.testdisk.yaml") + " }"), std::string("done\n"));
    CHECK_EQ(run.drive()->bytes(), std::string("a"));
    CHECK_EQ(run.line("saveas {place: disk, as: template, name: Mapped}"), std::string("done\n"));
    CHECK(has(run.line("media"), "\"name\":\"Mapped\""));
}
