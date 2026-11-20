// The preset harness commands, on the harness's test product
// (its preset rules: library/presets/tests/TestPresetRules.hpp).
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestPresetRules.hpp"
#include "TestProduct.hpp"
#include "harness/Run.hpp"
#include "peripherals/Controls.hpp"
#include <filesystem>
#include <string>

using namespace calcu::harness;
using namespace calcu::harness::test;
namespace mt = calcu::machine::test;
namespace machine = calcu::machine;

namespace {
bool startsWith(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }
const std::string kHeader = "format-version: 2\nmodel: test-seg\n";
bool controlOn(mt::TestMachine& m, const char* control) {
    const machine::PeripheralOwner owner{mt::kPrinter, machine::PlacePath{{machine::PlaceName{"printer"}}}};
    for (const auto& c : calcu::peripherals::controlsOf(m, owner))
        if (c.id == machine::ControlId{control}) return c.on;
    return false;
}
} // namespace

TEST(preset_done_or_refused_with_line) {
    calcu::testing::TempFolder t("calcu-harness-presets");
    t.write("good.test-seg", kHeader + "printer: test-printer\nkeys:\n  - key: 2 3\n");
    t.write("bad.test-seg", kHeader + "keys:\n  - key: 9\n");
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("preset " + (t / "good.test-seg")), std::string("done\n"));
    CHECK_EQ(run.machine().presses(), std::uint64_t(2));
    CHECK_EQ(run.line("preset " + (t / "bad.test-seg")),
             "refused: " + (t / "bad.test-seg") + ", line 4: 9 is not a key of the test-seg.\n");
    CHECK_EQ(run.line("preset examples:basic/two.test-seg"), std::string("done\n"));   // a bundle path
    CHECK(startsWith(run.line("preset"), "refused: usage: preset"));
}

TEST(check_preset_one_line_per_file_run_fails) {
    calcu::testing::TempFolder t("calcu-harness-presets");
    t.write("a.test-seg", kHeader);
    t.write("b.test-seg", kHeader + "colour: red\n");
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const auto ok = run.server->line("check-preset " + (t / "a.test-seg"));
    CHECK(ok.done);
    CHECK_EQ(ok.text, "done 1\nok " + (t / "a.test-seg") + "\n");
    const auto both = run.server->line("check-preset " + (t / "a.test-seg") + " " + (t / "b.test-seg"));
    CHECK(!both.done);   // the run ends as failed
    CHECK_EQ(both.text, "done 2\nok " + (t / "a.test-seg") + "\nFAIL " + (t / "b.test-seg") +
                            ", line 3: Unknown key colour.\n");
    CHECK_EQ(run.machine().presses(), std::uint64_t(0));   // nothing built, nothing carried out
}

TEST(preset_list) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("preset-list"), std::string("done 2\n"
                                                  "examples:startup/seg.test-seg | test-seg | startup | Seg\n"
                                                  "examples:basic/two.test-seg | test-seg | example (Basic) | Two | Presses 2\n"));
}

TEST(wait_until_idle_reports_time_and_hour) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const std::string r = run.line("wait");
    CHECK(startsWith(r, "done 3\nticks: "));
    CHECK(r.find("hour reached: no\n") != std::string::npos);
    // A machine that never becomes idle: the hour is reached.
    auto* rules = static_cast<calcu::presets::test::TestPresetRules*>(run.product.presetRules());
    run.machine().busy = {"a test's computation"};
    rules->seg.rule.sample = 100'000;
    const std::string h = run.line("wait");
    CHECK(startsWith(h, "done 3\nticks: 3600"));
    CHECK(h.find("hour reached: yes\n") != std::string::npos);
}

TEST(full_and_regular_speed) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    auto& ctx = run.server->host().session().context();
    CHECK_EQ(run.line("full-speed"), std::string("done\n"));
    CHECK(ctx.commandedSpeed() == calcu::pacing::Speed::Full);
    CHECK_EQ(run.line("regular-speed"), std::string("done\n"));
    CHECK(ctx.commandedSpeed() == calcu::pacing::Speed::Regular);
}

TEST(elapsed_time) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("elapsed-time start"), std::string("done\n"));
    run.line("wait 1250ms");
    CHECK_EQ(run.line("elapsed-time report"), std::string("done 1\nElapsed: 0:01.250\n"));
    CHECK(startsWith(run.line("elapsed-time later"), "refused: elapsed-time: takes start or report."));
}

TEST(expect_and_not_expect_refused_with_display) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // The test product's display after the start: presses 0, (ticks / 1000) % 10 = 1, no key down.
    CHECK_EQ(run.line("expect 010"), std::string("done\n"));
    CHECK_EQ(run.line("expect /^0\\d0$/"), std::string("done\n"));
    CHECK_EQ(run.line("expect 7"), std::string("refused: Expected 7; the display shows 010.\n"));
    CHECK_EQ(run.line("not-expect 7"), std::string("done\n"));
    CHECK_EQ(run.line("not-expect 01"), std::string("refused: Not expected: 01; the display shows 010.\n"));
}

TEST(screenshot_into_the_run_folder) {
    calcu::testing::TempFolder out("calcu-harness-shots");
    RunOptions o;
    o.outputFolder = out.str();
    TestRun run(o);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("screenshot a.png"), std::string("done\n"));
    CHECK(std::filesystem::exists(out.path() / "a.png"));
    CHECK(startsWith(run.line("screenshot ../a.png"), "refused: screenshot: takes a file name without a folder."));
}

TEST(printer_trace_refused_without_printer) {
    TestRun run;   // the start's machine: test-seg without a printer
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("printer-trace on"), std::string("refused: printer-trace: needs printer: test-printer.\n"));
    CHECK_EQ(run.line("attach printer test-printer"), std::string("done\n"));
    CHECK_EQ(run.line("printer-trace on"), std::string("done\n"));
    CHECK(controlOn(run.machine(), "trace"));
    CHECK_EQ(run.line("printer-trace off"), std::string("done\n"));
    CHECK(!controlOn(run.machine(), "trace"));
}

TEST(set_control_sets_a_control) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("attach printer test-printer"), std::string("done\n"));
    CHECK_EQ(run.line("set-control test-printer online on"), std::string("done\n"));
    CHECK(controlOn(run.machine(), "online"));
    CHECK_EQ(run.line("set-control TEST-PRINTER feed on"), std::string("done\n"));   // a key: pressed
    CHECK(controlOn(run.machine(), "feed"));
    CHECK_EQ(run.line("set-control test-printer feed off"), std::string("done\n"));
    CHECK(!controlOn(run.machine(), "feed"));
    CHECK_EQ(run.line("set-control test-printer lid on"), std::string("refused: The test-printer has no control lid.\n"));
    CHECK(startsWith(run.line("set-control test-printer online"), "refused: usage: set-control"));
}

TEST(syncclock_takes_the_run_clock) {
    RunOptions o;
    o.clock = ClockMode::Given;
    o.given = LocalDateTime{2024, 5, 6, 7, 8, 9};
    TestRun run(o);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("model test-dot"), std::string("done\n"));
    run.line("wait 2s");
    CHECK_EQ(run.line("sync-clock"), std::string("done\n"));
    const auto f = run.machine().clock()->read();
    CHECK(f.month == 5 && f.day == 6 && f.hour == 7 && f.minute == 8 && f.second == 9);
    TestRun seg;
    CHECK_EQ(seg.line("sync-clock"), std::string("refused: sync-clock is not available on the test-seg.\n"));
}
