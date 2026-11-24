// The syntax of docs/formats/README.md §1 for the whole file, the files a
// preset names, unreadable files and empty step lists.
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
namespace machine = calcu::machine;

namespace {
const LoadPlan* planOf(const std::variant<LoadPlan, ReadError>& r) { return std::get_if<LoadPlan>(&r); }
std::string messageOf(const std::variant<LoadPlan, ReadError>& r) {
    const auto* e = std::get_if<ReadError>(&r);
    return e ? e->message : std::string();
}
const TestHeader* testHeader(const LoadPlan& p) {
    const auto it = p.productHeader.find("test-header");
    return it == p.productHeader.end() ? nullptr : static_cast<const TestHeader*>(it->second.get());
}
} // namespace

TEST(block_and_flow_forms_of_readme_section_1) {
    Reading rd;
    const auto r = rd.read(header() +
                           "test-header:\n"
                           "  level: 3\n"
                           "  note: block form\n"
                           "printer: { name: test-printer }\n"
                           "test-memory: { at: 1, bytes: 0102 }\n"
                           "test-memory:\n"
                           "  at: 4\n"
                           "  bytes: |\n"
                           "    0A 0B\n"
                           "keys:\n"
                           "  - key: 1 2\n"
                           "  - test-panel: [a, b]\n"   // a flow sequence is no scalar: refused below
                           "  - full-speed:\n");
    CHECK(hasLine({messageOf(r)}, "line 14: test-panel: needs a value."));
    const auto ok = rd.read(header() +
                            "test-header: { level: 3, note: flow form }\n"
                            "printer: { name: test-printer }\n"
                            "test-memory:\n"
                            "  at: 4\n"
                            "  bytes: |\n"
                            "    0A 0B\n"
                            "keys:\n"
                            "  - key: 1 2\n"
                            "  - full-speed:\n");
    const LoadPlan* p = planOf(ok);
    CHECK(p != nullptr);
    if (!p) return;
    CHECK(testHeader(*p) && testHeader(*p)->level == 3 && testHeader(*p)->note == "flow form");
    CHECK_EQ(p->places.size(), std::size_t(1));
    CHECK_EQ(p->items.size(), std::size_t(3));
    CHECK_EQ(p->items[0].item->logText(), std::string("test-memory: 2 bytes"));
    CHECK_EQ(p->items[1].item->logText(), std::string("key: 1 2"));
    CHECK_EQ(p->items[2].item->logText(), std::string("full-speed"));
}

TEST(quotes_without_escapes) {
    Reading rd;
    const auto a = rd.read(header() + "preset-name: \"a \\ b\"\n");
    CHECK(planOf(a) && planOf(a)->header.presetName == "a \\ b");   // no escape processing
    const auto b = rd.read(header() + "preset-name: 'say \"hi\"'\n");
    CHECK(planOf(b) && planOf(b)->header.presetName == "say \"hi\"");
    const auto c = rd.read(header() + "preset-name: \"x # y\"\n");   // # inside quotes is text
    CHECK(planOf(c) && planOf(c)->header.presetName == "x # y");
}

TEST(block_scalar_skips_blank_and_hash_lines) {
    Reading rd;
    const auto r = rd.read(header() +
                           "test-memory:\n"
                           "  at: 0\n"
                           "  bytes: |\n"
                           "    01 02\n"
                           "\n"
                           "    # a comment in the block\n"
                           "    03\n");
    CHECK(planOf(r) && planOf(r)->items[0].item->logText() == "test-memory: 3 bytes");
}

TEST(hash_without_space_is_text) {
    Reading rd;
    const auto a = rd.read(header() + "preset-name: a#b\n");
    CHECK(planOf(a) && planOf(a)->header.presetName == "a#b");
    const auto b = rd.read(header() + "preset-name: a #b\n");
    CHECK(planOf(b) && planOf(b)->header.presetName == "a");
}

TEST(raw_values_of_type_expect_not_expect) {
    Reading rd;
    const auto r = rd.read(header() +
                           "keys:\n"
                           "  - expect: \"1\" # not a comment\n"
                           "  - not-expect: {x} [y]\n");
    const LoadPlan* p = planOf(r);
    CHECK(p != nullptr);
    if (!p) return;
    CHECK_EQ(p->items[0].item->logText(), std::string("expect: \"1\" # not a comment"));
    CHECK_EQ(p->items[1].item->logText(), std::string("not-expect: {x} [y]"));
    // type: the `#` and the space are typed, and test-seg has no key for them.
    CHECK(hasLine({rd.error(header() + "keys:\n  - type: 12 # 3\n")}, "No key types the character \" \""));
    const auto t = rd.read(header() + "keys:\n  - type: 12#3\n");
    CHECK(hasLine({messageOf(t)}, "No key types the character \"#\""));
}

TEST(tab_anywhere_refused) {
    Reading rd;
    CHECK(hasLine({rd.error(header() + "# a comment\twith a tab\n")}, "t.test-seg, line 3: Tab character"));
    CHECK(hasLine({rd.error(header() + "keys:\n  - key:\t1\n")}, "line 4: Tab character"));
}

TEST(unknown_key_refused_at_every_level) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "colour: red\n"), std::string("t.test-seg, line 3: Unknown key colour."));
    CHECK_EQ(rd.error(header() + "test-header:\n  level: 1\n  size: 2\n"),
             std::string("t.test-seg, line 5: Unknown key size in test-header:."));
    CHECK_EQ(rd.error(header() + "keys:\n  - jump: 3\n"), std::string("t.test-seg, line 4: Unknown step jump."));
    CHECK(hasLine({rd.error(header() + "printer: { name: test-printer, side: 1 }\n")}, "exactly one of name: and file:"));
}

TEST(hex_forms_and_booleans) {
    for (const char* at : {"&10", "0x10", "$10", "16"}) {
        Rig r;
        const CallId c = r.load(header() + "test-memory: { at: " + at + ", bytes: AB }\n");
        CHECK(r.done(c));
        CHECK(r.machine()->memory() && r.machine()->memory()->bytes[16] == 0xAB);
    }
    Reading rd;
    const auto t = rd.read(header() + "test-header: { level: &FF, flag: true }\n");
    CHECK(planOf(t) && testHeader(*planOf(t))->flag && testHeader(*planOf(t))->level == 255);
    CHECK(hasLine({rd.error(header() + "test-header: { flag: yes }\n")}, "flag: is true or false."));
    CHECK(hasLine({rd.error(header() + "test-memory: { at: 1G, bytes: AB }\n")}, "at: is a number."));
}

TEST(paths_from_preset_folder_and_home) {
    Rig r;
    r.folder.write("sub/data.bin", std::string("\x11\x22", 2));
    r.folder.write("home/mine.bin", std::string("\x33", 1));
    const std::string path = r.write(header() +
                                         "test-memory: { at: 0, file: data.bin }\n"
                                         "test-memory: { at: 2, file: ~/mine.bin }\n",
                                     "sub/p.test-seg");
    const CallId c = r.send("load-preset", Value::object({{"ref", path}}));
    CHECK(r.done(c));
    const auto& m = r.machine()->memory()->bytes;
    CHECK(m[0] == 0x11 && m[1] == 0x22 && m[2] == 0x33);
}

TEST(names_any_case_hyphen_required) {
    Reading rd;
    const auto a = rd.read("format-version: 2\nmodel: TEST-SEG\nprinter: TEST-Printer\nslot-1: Test-Module:8K\n");
    CHECK(planOf(a) && planOf(a)->model == mt::kSeg);
    CHECK(planOf(a) && planOf(a)->places[1].request.variant == machine::VariantId{"8k"});
    const auto b = rd.read("format-version: 2\nmodel: test-dot:v2\n");
    CHECK(planOf(b) && planOf(b)->variant == machine::VariantId{"V2"});
    CHECK(hasLine({rd.error("format-version: 2\nmodel: testseg\n")}, "There is no model testseg"));
    CHECK(hasLine({rd.error(header() + "printer: testprinter\n")}, "There is no peripheral testprinter."));
}

TEST(header_key_once_sections_repeat) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "preset-name: A\ndescription: one\npreset-name: B\n"),
             std::string("t.test-seg, line 5: preset-name: is given twice; a header key appears once."));
    CHECK(hasLine({rd.error(header() + "model: test-seg\n")}, "line 3: model: is given twice"));
    const auto r = rd.read(header() + "keys:\n  - key: 1\ntest-memory: { at: 0, bytes: 01 }\nkeys:\n  - key: 2\n");
    const LoadPlan* p = planOf(r);
    CHECK(p && p->items.size() == 3);
    if (p) {
        CHECK_EQ(p->items[0].item->logText(), std::string("key: 1"));
        CHECK_EQ(p->items[1].item->logText(), std::string("test-memory: 1 bytes"));
        CHECK_EQ(p->items[2].item->logText(), std::string("key: 2"));
    }
}

TEST(unreadable_or_not_utf8_names_the_file) {
    Reading rd;
    const auto gone = rd.readFile("/nonexistent-folder/gone.test-seg");
    // The system's reason goes with it.
    CHECK_EQ(messageOf(gone),
             std::string("The file /nonexistent-folder/gone.test-seg cannot be read: No such file or directory."));
    const auto bad = rd.read(header() + "preset-name: \xC3\x28\n", "latin.test-seg");
    CHECK_EQ(messageOf(bad), std::string("The file latin.test-seg is not UTF-8 text."));
    // A byte-order mark at the start is dropped (editors on Windows write one).
    CHECK(planOf(rd.read("\xEF\xBB\xBF" + header())) != nullptr);
    // In the session: refused with a notice that names the file.
    Rig r;
    const CallId c = r.send("load-preset", Value::object({{"ref", r.folder / "none.test-seg"}}));
    CHECK(!r.done(c) && hasLine({r.reason(c)}, "none.test-seg cannot be read"));
    CHECK(hasLine(r.notices(), "none.test-seg cannot be read"));
    // The app log names the preset and the cause.
    CHECK(r.appLog.has(calcu::applog::AppLogLevel::Warn, "none.test-seg was not loaded: The file "));
    CHECK(r.appLog.has(calcu::applog::AppLogLevel::Warn, "none.test-seg cannot be read: No such file or directory."));
}

TEST(empty_step_list_refused) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "keys:\n"), std::string("t.test-seg, line 3: keys: has no steps."));
    CHECK_EQ(rd.error(header() + "keys: []\n"), std::string("t.test-seg, line 3: keys: has no steps."));
}
