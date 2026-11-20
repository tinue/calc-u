// Calc-U-59's file
// kinds through the one command `open` (app/TIFileKinds), the keystroke
// program opened into the running machine, the refused kinds, the ways in.
// The magnetic card cases are in part_flows_tests.cpp.

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include "../app/Settings.hpp"
#include "../app/TIFileKinds.hpp"
#include "../core/TIProductKinds.hpp"
#include "../app/TIPresetKeys.hpp"
#include "opening/OpenRequest.hpp"
#include "opening/StartFile.hpp"
#include "settings/OpenFolders.hpp"
#include <filesystem>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::opening::Way;
using calcu::sessions::Value;
using testroms::SessionRig;

namespace {

bool ready(SessionRig& r) { return r.until([&] { return r.machine().isKeyReady() && !r.busy(); }); }

calcu::sessions::ReplyMessage open(SessionRig& r, const std::string& name, const std::optional<std::string>& text,
                                   Way way = Way::Drop, std::optional<std::string> path = std::nullopt) {
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = name;
    f.path = std::move(path);
    if (text) f.bytes = std::vector<std::uint8_t>(text->begin(), text->end());
    q.files = {f};
    q.way = way;
    return r.operation(calcu::opening::kOpenCommand, q.toValue());
}
std::string as(const calcu::sessions::ReplyMessage& m, const char* field = "as") {
    const Value v = m.result ? m.result->toValue() : Value();
    return v.find(field) && v.find(field)->string() ? *v.find(field)->string() : std::string();
}

ProgramMemory& memory(SessionRig& r) { return *programMemoryOf(r.machine()); }

const std::string kKeystrokes =
    "format: ti-keystroke-program\nformat-version: 1\nprogram-name: Two\nkeycodes: |\n  000 76 LBL\n  001 11 A\n  85\n";
const std::string kPreset = "format-version: 2\nmodel: TI-59\n";

} // namespace

// ── The product's data ───────────────────────────────────────────────────────

TEST(preset_extensions_ti59_ti58_ti58c) {
    CHECK(tiPresetExtensions() == (std::vector<std::string>{".ti59", ".ti58", ".ti58c"}));
    SessionRig r;
    CHECK(ready(r));
    const std::pair<const char*, const char*> files[] = {{"a.ti59", "TI-59"}, {"b.TI58", "TI-58"}, {"c.Ti58C", "TI-58C"}};
    for (const auto& [name, model] : files) {
        const auto m = open(r, name, std::string("format-version: 2\nmodel: ") + model + "\n");
        CHECK(m.done);
        CHECK_EQ(as(m), std::string("preset"));
        CHECK(ready(r));
    }
    // Any other name with a top-level model: line is a preset too.
    CHECK_EQ(as(open(r, "notes.txt", kPreset)), std::string("preset"));
}

TEST(one_medium_magnetic_card) {
    // The magnetic card is the one medium of Calc-U-59
    // (`.magnetcard.yaml`), opened by the library's media part; no product
    // file kind takes a card file, and 2.3.0's .u59 and .bin are no files of
    // the product.
    SessionRig r;
    CHECK(ready(r));
    const auto* media = &r.part().mediaPart();
    CHECK(media && media->kinds().size() == 1 && media->kinds()[0].id == "magnetic-card");
    CHECK(media && media->shelf().kindOfFile("x.magnetcard.yaml") && !media->shelf().kindOfFile("x.u59") &&
          !media->shelf().kindOfFile("x.bin"));
    TIDebugPart debug(TIDebugSetup{});
    TIFileKinds kinds(debug);
    for (auto* k : kinds.kinds()) CHECK(!k->takesName("x.magnetcard.yaml") && !k->takesName("x.u59") && !k->takesName("x.bin"));
}

TEST(kinds_keystrokes_and_asm_in_order) {
    TIDebugPart debug(TIDebugSetup{});
    TIFileKinds kinds(debug);
    std::vector<std::string> names;
    for (auto* k : kinds.kinds()) names.emplace_back(k->name());
    CHECK(names == (std::vector<std::string>{"asm-overlay", "keystroke-program", "rom-module-file", "constant-memory-file"}));
    // Each by its name alone: the drag is taken for every one.
    const auto k = kinds.kinds();
    CHECK(k[0]->takesName("x.asm") && !k[0]->takesName("x.asm.txt"));
    CHECK(k[1]->takesName("x.keystrokes.yaml") && !k[1]->takesName("x.yaml"));
    CHECK(k[2]->takesName("ml.rom.yaml"));
    CHECK(k[3]->takesName("ti58c.memory.yaml") && !k[3]->takesName("other.memory.yaml"));
}

// No limit on the size of a file by any way: a file of 4 MiB
// is read as what it is, never refused for its size.
TEST(no_size_limit_any_way) {
    SessionRig r;
    CHECK(ready(r));
    std::string big = kKeystrokes;
    big.append(4u << 20, '\n');
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness}) {
        const auto m = open(r, "big.keystrokes.yaml", big, w);
        CHECK(m.done);
        CHECK_EQ(as(m, "detail"), std::string("keystroke-program"));
    }
}

// ── The keystroke program and the refused kinds ──────────────────────────────

TEST(keystrokes_written_from_000_without_cold_boot) {
    SessionRig r;
    CHECK(ready(r));
    const auto t = r.ticks();
    const std::uint64_t generation = r.session().machineSession().generation();
    const auto m = open(r, "two.keystrokes.yaml", kKeystrokes);
    CHECK(m.done);
    CHECK_EQ(as(m), std::string("file-kind"));
    CHECK_EQ(as(m, "detail"), std::string("keystroke-program"));
    CHECK_EQ(int(memory(r).readStep(0)), 76);
    CHECK_EQ(int(memory(r).readStep(1)), 11);
    CHECK_EQ(int(memory(r).readStep(2)), 85);
    CHECK_EQ(r.session().machineSession().generation(), generation);   // the same machine: no cold boot, no build
    CHECK(r.ticks() >= t);
    CHECK(r.machine().isKeyReady());
}

TEST(partition_registers_other_steps_stay) {
    SessionRig r;
    CHECK(ready(r));
    ProgramMemory& pm = memory(r);
    const int partition = pm.partitionSteps();
    pm.writeStep(10, 42);
    RegisterNibbles v{};
    v[15] = 5;
    pm.writeRegister(3, v);
    CHECK(open(r, "two.keystrokes.yaml", kKeystrokes).done);
    CHECK_EQ(pm.partitionSteps(), partition);
    CHECK_EQ(int(pm.readStep(10)), 42);
    CHECK(pm.readRegister(3) == v);
    // Also into a paused machine, which stays paused.
    auto& rc = r.part().debuggerPart().runControl();
    rc.pause(calcu::debugger::kCpu);
    CHECK(r.until([&] { return r.part().isFrozen(); }, 2s));
    const std::string other =
        "format: ti-keystroke-program\nformat-version: 1\nprogram-name: One\nkeycodes: |\n  000 91 R/S\n";
    CHECK(open(r, "one.keystrokes.yaml", other).done);
    CHECK_EQ(int(pm.readStep(0)), 91);
    CHECK(r.part().isFrozen());
    rc.resume();
}

TEST(opened_step_beyond_last_refused) {
    SessionRig r(Model::TI58);
    CHECK(ready(r));
    const std::string past =
        "format: ti-keystroke-program\nformat-version: 1\nprogram-name: Far\nkeycodes: |\n  480 76 LBL\n";
    memory(r).writeStep(0, 33);
    const auto m = open(r, "far.keystrokes.yaml", past);
    CHECK(!m.done);
    CHECK(m.reason.rfind("far.keystrokes.yaml", 0) == 0);
    CHECK_EQ(int(memory(r).readStep(0)), 33);   // nothing written
    r.frame();
    CHECK_EQ(r.notices().size(), std::size_t{1});
}

TEST(unreadable_refused) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, "gone.keystrokes.yaml", std::nullopt);
    CHECK(!m.done);
    CHECK(m.reason.find("gone.keystrokes.yaml") != std::string::npos);
    const auto bad = open(r, "bad.keystrokes.yaml", "format: something-else\n");
    CHECK(!bad.done);
    r.frame();
    CHECK_EQ(r.notices().size(), std::size_t{2});
}

TEST(rom_yaml_refused_by_id_only) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, "ML.rom.yaml", "anything\n");
    CHECK(!m.done);
    CHECK_EQ(m.reason, std::string("ML.rom.yaml was not opened: this release takes a Solid State ROM module by its ID only."));
    r.frame();
    CHECK(!r.notices().empty() && r.notices().back() == m.reason);
}

TEST(constant_memory_file_refused) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, "TI58C.Memory.yaml", "model: TI-58C\n");   // any case; a model: line does not make it a preset
    CHECK(!m.done);
    CHECK_EQ(m.reason, std::string("TI58C.Memory.yaml was not opened: it is Calc-U-59's own constant memory file."));
}

// ── .asm is the debug panel's kind ───────────────────────────────────────────

TEST(asm_is_the_debug_panels_kind) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, "c.asm", "Calc-U-59 ASM 2.0\nHEX:\n0A01\n");
    CHECK(m.done);
    CHECK_EQ(as(m, "detail"), std::string("asm-overlay"));
}

// ── The ways in ──────────────────────────────────────────────────────────────

// The program's own options and their values are taken
// out, then the library's rule: the last argument that does not start with
// `-` and names an existing file.
TEST(options_taken_out_before_the_rule) {
    calcu::testing::TempFolder dir("calcu59-start");
    dir.write("shot.png", "x");
    dir.write("p.ti59", kPreset);
    dir.write("q.ti59", kPreset);
    const std::string shot = (dir.path() / "shot.png").string(), p = (dir.path() / "p.ti59").string(), q = (dir.path() / "q.ti59").string();
    auto isFile = [](const std::string& f) { return std::filesystem::is_regular_file(f); };
    using calcu::opening::startFile;
    CHECK(programArguments({"calc-u-59", "--grab", shot, "--after", "500"}) == (std::vector<std::string>{"calc-u-59"}));
    const auto a = startFile(programArguments({"calc-u-59", p, "--grab", shot}), isFile);   // never the --grab path
    const auto b = startFile(programArguments({"calc-u-59", p, q}), isFile);                // the last one counts
    const auto c = startFile(programArguments({"calc-u-59", q, "/no/such.ti59"}), isFile);  // one that exists
    CHECK(a && *a == p);
    CHECK(b && *b == q);
    CHECK(c && *c == q);
    CHECK(!startFile(programArguments({"calc-u-59", "-NSDocumentRevisionsDebugMode", "YES"}), isFile));
    CHECK(!startFile(programArguments({"calc-u-59", "--pacing-probe", p}), isFile));       // an option's value
}

// A file named at the start or opened with Open With is opened as what it
// is, not always as a preset.
TEST(open_with_and_start_open_as_what_it_is) {
    SessionRig r;
    CHECK(ready(r));
    for (Way w : {Way::OpenWith, Way::Start}) {
        const auto k = open(r, "two.keystrokes.yaml", kKeystrokes, w);
        CHECK(k.done && as(k, "detail") == "keystroke-program");
        const auto a = open(r, "c.asm", "Calc-U-59 ASM 2.0\nHEX:\n0A01\n", w);
        CHECK(a.done && as(a, "detail") == "asm-overlay");
        const auto p = open(r, "p.ti59", kPreset, w);
        CHECK(p.done && as(p) == "preset");
        CHECK(ready(r));
    }
}

TEST(open_by_any_way_keeps_dialog_folders) {
    SessionRig r;
    CHECK(ready(r));
    const std::string presets = settings::openFolderKind(settings::OpenFolder::Presets);
    const std::string asmOverlays = settings::openFolderKind(settings::OpenFolder::AsmOverlays);
    calcu::settings::setFixedOpenFolder(r.settings, presets, "/fixed/presets");
    const auto before = calcu::settings::openStartFolder(r.settings, presets, "/home");
    const auto beforeAsm = calcu::settings::openStartFolder(r.settings, asmOverlays, "/home");
    calcu::testing::TempFolder dir("calcu59-ec7");
    dir.write("p.ti59", kPreset);
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness}) {
        CHECK(open(r, "p.ti59", kPreset, w, (dir.path() / "p.ti59").string()).done);
        CHECK(open(r, "c.asm", "Calc-U-59 ASM 2.0\nHEX:\n0A01\n", w, (dir.path() / "c.asm").string()).done);
        CHECK(ready(r));
    }
    CHECK_EQ(calcu::settings::openStartFolder(r.settings, presets, "/home"), before);
    CHECK_EQ(calcu::settings::openStartFolder(r.settings, asmOverlays, "/home"), beforeAsm);
}

// ── A refusal is a notice that names the file ────────────────────────────────

TEST(refusal_notice_names_the_file) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, "readme.txt", "just some text\n");
    CHECK(!m.done);
    CHECK_EQ(m.reason, std::string("Calc-U-59 cannot open readme.txt."));
    r.frame();
    CHECK(!r.notices().empty() && r.notices().back().find("readme.txt") != std::string::npos);
}

// ── Extensions and outdated files ────────────────────────────────────────────

TEST(preset_by_extension_in_any_case) {
    SessionRig r;
    CHECK(ready(r));
    CHECK_EQ(as(open(r, "P.TI59", kPreset)), std::string("preset"));
    // An outdated preset is a preset by its extension, refused with the loader's hint.
    const auto old = open(r, "old.ti59", "Calc-U-59 2.0\nPROGRAM:\n");
    CHECK(!old.done && old.reason.find("format") != std::string::npos);
}

TEST(hidden_file_refused) {
    SessionRig r;
    CHECK(ready(r));
    const auto m = open(r, ".a.u59", "x");
    CHECK(!m.done);
    CHECK(m.reason.find("hidden") != std::string::npos);
    const auto p = open(r, ".p.ti59", kPreset);
    CHECK(!p.done);
}

TEST(dropped_preset_loads_as_the_dialog) {
    SessionRig r;
    CHECK(ready(r));
    const std::string text = "format-version: 2\nmodel: TI-58\nprogram:\n  keycodes: |\n    76 11\n";
    const auto d = open(r, "p.ti58", text, Way::Drop);
    CHECK(d.done && r.part().model() == Model::TI58);
    CHECK_EQ(int(memory(r).readStep(1)), 11);
    CHECK(r.model("TI-59").done);
    const auto f = open(r, "p.ti58", text, Way::Dialog);
    CHECK(f.done && r.part().model() == Model::TI58);
    CHECK_EQ(int(memory(r).readStep(1)), 11);
}
