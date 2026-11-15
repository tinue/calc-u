// LIB-6 L12: the preset list from the bundle file (AC-31 to AC-33, EC-9).
#include "PresetRig.hpp"
#include "TestHarness.hpp"
#include <filesystem>

using namespace calcu::presets;
using namespace calcu::presets::test;

namespace {
const std::filesystem::path kBundle = std::filesystem::path(CALCU_PRESET_FIXTURES) / "bundle";

std::vector<PresetListEntry> listOf(Rig& r) {
    const auto reply = r.ask("preset-list");
    const auto* l = calcu::sessions::payloadAs<PresetListReply>(reply.result);
    return l ? l->entries : std::vector<PresetListEntry>{};
}

const char* const kHead = "format: example-bundle\nformat-version: 1\n";
} // namespace

TEST(ac31_bundle_order_groups_startup_files) {
    const DirectoryPresetBundle b(kBundle);
    const auto read = readBundleFile(b);
    const auto* entries = std::get_if<std::vector<BundleEntry>>(&read);
    CHECK(entries && entries->size() == 5);
    if (!entries || entries->size() != 5) return;
    CHECK((*entries)[0].preset == "startup/seg.test-seg" && (*entries)[0].startup);
    CHECK((*entries)[1].preset == "startup/dot.test-dot" && (*entries)[1].startup);
    CHECK((*entries)[2].preset == "basic/count.test-seg" && (*entries)[2].group == "Basic");
    CHECK((*entries)[2].files == std::vector<std::string>{"basic/count.bin"});
    CHECK((*entries)[3].preset == "basic/plain.test-seg" && (*entries)[3].group == "Basic");
    CHECK((*entries)[4].preset == "letters/abc.test-dot" && (*entries)[4].group == "Letters");
    // Unknown keys are refused, as in every format.
    calcu::testing::TempFolder t("calcu-bundle");
    t.write("bundle.yaml", std::string(kHead) + "startup: []\ngroups: []\nextra: 1\n");
    CHECK(hasLine({std::get<ReadError>(readBundleFile(DirectoryPresetBundle(t.path()))).message}, "Unknown key extra."));
}

TEST(ac31_check_bundle_finds_missing_and_unlisted_files) {
    Reading rd;
    CHECK(checkBundle(DirectoryPresetBundle(kBundle), rd.setup()).empty());
    calcu::testing::TempFolder t("calcu-bundle");
    t.write("a.test-seg", header() + "test-memory: { at: 0, file: data/a.bin }\n");
    t.write("data/a.bin", "\x01");
    t.write("b.test-seg", header());
    t.write("bundle.yaml", std::string(kHead) +
                               "startup:\n  - preset: { file: b.test-seg }\n"
                               "groups:\n  - group: G\n    examples:\n"
                               "      - preset: { file: a.test-seg }\n"            // names data/a.bin, not listed
                               "      - preset: { file: b.test-seg }\n"            // listed twice
                               "      - preset: { file: c.test-seg }\n"            // missing
                               "        files:\n          - { file: c.bin }\n");   // missing
    const auto problems = checkBundle(DirectoryPresetBundle(t.path()), rd.setup());
    CHECK(hasLine(problems, "examples:a.test-seg names examples:data/a.bin, which is not listed with it."));
    CHECK(hasLine(problems, "examples:b.test-seg is listed more than once."));
    CHECK(hasLine(problems, "The listed preset examples:c.test-seg is missing."));
    CHECK(hasLine(problems, "The listed file examples:c.bin is missing."));
    t.write("bundle.yaml", std::string(kHead) + "startup:\n  - preset: { file: ../outside.test-seg }\ngroups: []\n");
    CHECK(hasLine(checkBundle(DirectoryPresetBundle(t.path()), rd.setup()), "leaves the bundle"));
}

TEST(ac32_entries_with_bundle_path_model_name_description_kind_group) {
    const DirectoryPresetBundle b(kBundle);
    RigOptions o;
    o.bundle = &b;
    Rig r(o);
    const auto l = listOf(r);
    CHECK_EQ(l.size(), std::size_t(5));
    if (l.size() != 5) return;
    CHECK_EQ(l[0].bundlePath, std::string("examples:startup/seg.test-seg"));
    CHECK(l[0].model == mt::kSeg && l[0].name == "Seg default" && l[0].description == "test-seg with the printer");
    CHECK(l[0].kind == PresetListEntry::Kind::Startup && l[0].group.empty());
    CHECK(l[1].model == mt::kDot && l[1].name == "dot");   // the file stem without preset-name
    CHECK(l[2].kind == PresetListEntry::Kind::Example && l[2].group == "Basic" && l[2].name == "Count");
    CHECK(l[2].files == std::vector<std::string>{"basic/count.bin"});
    CHECK(l[4].group == "Letters" && l[4].name == "ABC" && l[4].description == "Types abc");
    // The reply as a value (translating hosts).
    const auto v = r.ask("preset-list").result->toValue();
    CHECK(v.find("entries") && v.find("entries")->elements()->size() == 5);
}

TEST(ac32_header_only_entry_naming_missing_file_listed) {
    calcu::testing::TempFolder t("calcu-bundle");
    t.write("a.test-seg", header() + "preset-name: Needs a file\ntest-memory: { at: 0, file: gone.bin }\n");
    t.write("bundle.yaml", std::string(kHead) + "startup: []\ngroups:\n  - group: G\n    examples:\n"
                                                "      - preset: { file: a.test-seg }\n");
    const DirectoryPresetBundle b(t.path());
    RigOptions o;
    o.bundle = &b;
    Rig r(o);
    const auto l = listOf(r);
    CHECK(l.size() == 1 && l[0].name == "Needs a file");
}

TEST(ac33_bundle_path_in_load_and_startup_setting) {
    const DirectoryPresetBundle b(kBundle);
    RigOptions o;
    o.bundle = &b;
    o.settings = "[preset]\ntest-seg = examples:startup/seg.test-seg\n";
    Rig r(o);
    CHECK(r.done(r.startCall));
    bool printer = false;
    for (const auto& e : r.machine()->plugged().byPlace) printer = printer || e.peripheral == mt::kPrinter;
    CHECK(printer);   // the startup preset by its bundle path
    const auto& last = r.startupPart->control().lastBuild();
    CHECK(last && last->origin == calcu::startup::BuildOrigin::StartupPreset);
    // The load command; a file the bundled preset names is read from the bundle.
    const CallId c = r.send("load-preset", Value::object({{"ref", "examples:basic/count.test-seg"}}));
    CHECK(r.done(c));
    CHECK(r.machine()->memory()->bytes[0] == 7 && r.machine()->memory()->bytes[1] == 8);
    const CallId gone = r.send("load-preset", Value::object({{"ref", "examples:basic/none.test-seg"}}));
    CHECK(!r.done(gone) && hasLine({r.reason(gone)}, "There is no bundled preset examples:basic/none.test-seg."));
    const CallId out = r.send("load-preset", Value::object({{"ref", "examples:../bundle.yaml"}}));
    CHECK(!r.done(out));
}

TEST(ec9_missing_entry_left_out_and_logged) {
    calcu::testing::TempFolder t("calcu-bundle");
    t.write("good.test-seg", header());
    t.write("old.test-seg", "model: test-seg\n");   // its header cannot be read
    t.write("bundle.yaml", std::string(kHead) + "startup: []\ngroups:\n  - group: G\n    examples:\n"
                                                "      - preset: { file: gone.test-seg }\n"
                                                "      - preset: { file: old.test-seg }\n"
                                                "      - preset: { file: good.test-seg }\n");
    const DirectoryPresetBundle b(t.path());
    RigOptions o;
    o.bundle = &b;
    Rig r(o);
    r.logLines();
    const auto l = listOf(r);
    CHECK(l.size() == 1 && l[0].bundlePath == "examples:good.test-seg");
    const auto log = r.logLines();
    CHECK(hasLine(log, "The preset list leaves out examples:gone.test-seg: There is no bundled preset"));
    CHECK(hasLine(log, "The preset list leaves out examples:old.test-seg: "));
    // The suite's check fails on it (AC-43, AC-51).
    Reading rd;
    CHECK(!checkBundle(b, rd.setup()).empty());
}
