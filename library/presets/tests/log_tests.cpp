// The log of a load. The wording is not part of the
// contract but the line of `elapsed-time: report`.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;

TEST(log_lines_file_machine_peripherals_items_end) {
    Rig r;
    r.logLines();
    const CallId c = r.load("format-version: 2\nmodel: test-seg\nprinter: test-printer\nslot-1: test-module:8k\n"
                            "test-memory: { at: 0, bytes: 01 }\n"
                            "keys:\n  - key: 2\n  - elapsed-time: report\n");
    CHECK(r.done(c));
    const auto log = r.logLines();
    const std::string path = r.folder / "t.test-seg";
    CHECK(log.size() >= 7);
    if (log.size() < 7) return;
    CHECK_EQ(log[0], "Preset " + path + ": test-seg");          // the file and the machine
    CHECK_EQ(log[1], std::string("  printer: test-printer"));    // one per peripheral
    CHECK_EQ(log[2], std::string("  slot-1: test-module:8k"));
    CHECK_EQ(log[3], std::string("  line 5 test-memory: 1 bytes: done"));   // each section and step
    CHECK_EQ(log[4], std::string("  line 7 key: 2: done"));
    CHECK(log[5].rfind("Elapsed: 0:00.0", 0) == 0);
    CHECK_EQ(log[6], std::string("  line 8 elapsed-time: report: done"));
    CHECK_EQ(log.back(), "Preset " + path + " carried out to its end.");
}

TEST(failure_line_holds_the_error) {
    Rig r;
    r.logLines();
    const CallId c = r.load(header() + "keys:\n  - key: 2\n  - expect: 7\n  - key: 3\n");
    CHECK(!r.done(c));
    const auto log = r.logLines();
    CHECK(hasLine(log, "  line 5 expect: 7: failed: Expected 7; the display shows "));
    CHECK(hasLine(log, "t.test-seg stopped: "));
    CHECK(hasLine(log, "t.test-seg, line 5: Expected 7"));
    CHECK(!hasLine(log, "carried out to its end"));
}
