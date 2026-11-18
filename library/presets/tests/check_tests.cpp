// The check before anything is built, and the bad fixtures, one per class
// of error.
#include "PresetRig.hpp"
#include "TestHarness.hpp"
#include <filesystem>

using namespace calcu::presets;
using namespace calcu::presets::test;

TEST(first_error_in_file_order_with_line) {
    Reading rd;
    CHECK_EQ(rd.error(header() +
                      "printer: test-printer\n"
                      "slot-1: test-printer\n"            // line 4
                      "keys:\n"
                      "  - key: 1\n"
                      "  - key: 9\n"),                    // line 7
             std::string("t.test-seg, line 4: The Test printer does not fit slot-1 of the test-seg."));
    // A header key after the steps: still the lowest line first.
    CHECK_EQ(rd.error(header() + "keys:\n  - key: 9\nslot-1: test-printer\n"),
             std::string("t.test-seg, line 4: 9 is not a key of the test-seg."));
}

TEST(syntax_error_after_semantic_error_reports_the_earlier) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "colour: red\nkeys:\n  - key: 1\nprinter: { name: test-printer\n"),
             std::string("t.test-seg, line 3: Unknown key colour."));
    CHECK(hasLine({rd.error(header() + "printer: { name: test-printer\ncolour: red\n")},
                  "t.test-seg, line 3: Unterminated flow mapping."));
}

TEST(refused_load_leaves_machine_untouched_reply_and_notice) {
    Rig r;
    CHECK(r.done(r.load(header() + "printer: test-printer\nkeys:\n  - key: 2\n")));
    r.notices();
    mt::TestMachine* before = r.machine();
    const auto ticks = r.ticks();
    const int builds = r.machines.builder_.inner.builds;
    const std::size_t mark = r.fe.log.size();
    const CallId c = r.load(header("test-dot") + "a: test-hub\nkeys:\n  - key: z\n");
    const std::string want = "t.test-seg, line 5: z is not a key of the test-dot.";
    CHECK(!r.done(c));
    CHECK(hasLine({r.reason(c)}, want));
    const auto n = r.notices();
    CHECK(n.size() == 1 && n[0] == r.reason(c));
    CHECK(r.machine() == before);
    CHECK_EQ(r.ticks(), ticks);
    CHECK_EQ(r.machines.builder_.inner.builds, builds);
    // Never busy (read before the operation starts): no busy state but the one there was.
    CHECK_EQ(r.fe.stateMessages(calcu::sessions::kBusyTopic, mark), 0);
}

TEST(keys_text_places_and_files_checked_before_build) {
    Rig r;
    const int builds = r.machines.builder_.inner.builds;
    struct Case {
        std::string text, want;
    };
    const Case cases[] = {
        {header() + "keys:\n  - key: 1 7\n", "line 4: 7 is not a key of the test-seg."},
        {header() + "keys:\n  - type: 12x\n", "line 4: No key types the character \"x\" on the test-seg."},
        {header() + "aux: test-module:2k\n", "line 3: The Test module has no variant 2k"},
        {header() + "test-memory: { at: 0, file: missing.bin }\n", "line 3: The file "},
        {header() + "keys:\n  - key: 1\n  - printer-trace: on\n", "line 5: printer-trace: needs printer: test-printer."},
        {header("test-dot") + "keys:\n  - sync-clock:\n  - type: abd\n", "line 5: No key types the character \"d\""},
    };
    for (const Case& k : cases) {
        const CallId c = r.load(k.text);
        CHECK(!r.done(c));
        CHECK(hasLine({r.reason(c)}, k.want));
    }
    CHECK_EQ(r.machines.builder_.inner.builds, builds);
}

TEST(unknown_section_or_step_refused) {
    Reading rd;
    // A section of another product is unknown here.
    CHECK_EQ(rd.error(header() + "partition: 479\n"), std::string("t.test-seg, line 3: Unknown key partition."));
    CHECK_EQ(rd.error(header() + "keys:\n  - card: { mem: A }\n"), std::string("t.test-seg, line 4: Unknown step card."));
    CHECK_EQ(rd.error(header() + "key: 1\n"), std::string("t.test-seg, line 3: Unknown key key."));   // a step outside keys:
}

TEST(keyword_not_for_model_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n  - sync-clock:\n"),
             std::string("t.test-seg, line 4: sync-clock is not available on the test-seg."));
    CHECK_EQ(rd.error(header("test-dot") + "a: test-printer\nkeys:\n  - printer-trace: on\n"),
             std::string("t.test-seg, line 5: printer-trace is not available on the test-dot."));
    CHECK_EQ(rd.error(header("test-dot") + "printer: test-printer\n"),
             std::string("t.test-seg, line 3: printer is not available on the test-dot."));
}

TEST(undefined_keyword_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n  - test-undefined:\n"),
             std::string("t.test-seg, line 4: test-undefined is not defined yet for the test-seg by the preset format."));
}

TEST(content_without_folder_naming_relative_file_refused) {
    Rig r;
    r.folder.write("data.bin", "\x01");
    const std::string text = header() + "test-memory: { at: 0, file: data.bin }\n";
    const CallId c = r.send("load-preset", Value::object({{"content", text}, {"name", "dropped.test-seg"}}));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("dropped.test-seg, line 3: The preset dropped.test-seg was handed over without "
                                      "its folder, so the file data.bin it names cannot be read."));
    // An absolute path is read.
    const CallId ok = r.send("load-preset", Value::object({{"content", header() + "test-memory: { at: 0, file: " +
                                                                          (r.folder / "data.bin") + " }\n"},
                                                         {"name", "dropped.test-seg"}}));
    CHECK(r.done(ok));
}

// The bad fixtures: each names the error it gives in its first line,
// `# error: <part of the message>`.
TEST(bad_fixtures_each_refused_with_its_error) {
    namespace fs = std::filesystem;
    Reading rd;
    int n = 0;
    for (const auto& e : fs::directory_iterator(fs::path(CALCU_PRESET_FIXTURES) / "bad")) {
        const std::string text = calcu::testing::readText(e.path());
        const std::string first = text.substr(0, text.find('\n'));
        const std::string want = first.substr(first.find(": ") + 2);
        const auto read = rd.readFile(e.path().generic_string());
        const std::string got = std::holds_alternative<ReadError>(read) ? std::get<ReadError>(read).message : "(read)";
        if (got.find(want) == std::string::npos)
            std::fprintf(stderr, "  %s: %s\n", e.path().filename().generic_string().c_str(), got.c_str());
        CHECK(got.find(want) != std::string::npos);
        ++n;
    }
    CHECK(n >= 10);
}
