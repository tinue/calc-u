// C1600-2 AC-7 (design §3.4, T4): typing a listing in through the typing
// engine (sharp::loaders::typeProgram), as the session command
// `c1600.type-basic` and as a preset's typed `program:` -- in a session with
// the typing part and the loader part (SessionFixture), virtual time.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../core/PC1500/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600BasicTyper.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1600/PC1600ProgramPlacement.hpp"
#include "TestRoms.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "../app/tests/SessionFixture.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = shared::commands;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;

sharp::LoaderMemory& memory(SessionFixture& f) { return *sharp::firstOf<sharp::LoaderMemory>(*f.part().machine()); }

std::uint16_t be16(const sharp::LoaderMemory& m, std::uint16_t a) {
    return static_cast<std::uint16_t>((m.peek(a) << 8) | m.peek(static_cast<std::uint16_t>(a + 1)));
}

// The program's records, [start, end), in the main CPU's view (S0).
std::vector<std::uint8_t> programBytes(const sharp::LoaderMemory& m) {
    std::uint32_t st = 0, end = 0;
    if (m.pc1600()) {
        st = pc1600::lh5803ToZ80(be16(m, 0xF865));
        end = pc1600::lh5803ToZ80(be16(m, 0xF867));
    } else {
        st = be16(m, 0x7865);
        end = be16(m, 0x7867);
    }
    std::vector<std::uint8_t> out;
    for (std::uint32_t a = st; a < end && a < 0x10000; ++a) out.push_back(m.peek(static_cast<std::uint16_t>(a)));
    return out;
}

// Started, the program area set up, in program mode: the PC-1500A after
// CL and NEW0 (PRO), the PC-1600 after MODE (PRO) and NEW.
bool ready(SessionFixture& f, const char* model) {
    if (!f.start(model)) return false;
    if (std::string(model) == "PC-1600") {
        f.tap("mode");
        f.typeLine("NEW");
    } else {
        f.tap("cl");
        f.typeLine("NEW0");
    }
    if (!f.runUntilTyped(20)) return false;
    f.runFor(1.5);
    return true;
}

Value listing(const std::string& text, const char* name = "prog.bas") {
    return Value::object({{"bytes", Value::Bytes(text.begin(), text.end())}, {"name", name}});
}

bool indicator(SessionFixture& f, const char* name) {
    const calcu::machine::DisplayOutput* d = f.listener.display();
    if (!d) return false;
    for (const auto& i : d->indicators)
        if (i.name == name) return i.on;
    return false;
}

const char* const kProgram = "10 A=1\n20 PRINT A\n30 DATA 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20\n40 END\n";

// The old typer's memory on a core machine of the same model, the oracle
// (design T4: the memory equals the old typer's).
std::vector<std::uint8_t> oldTyperBytes1500(const std::string& text) {
    PC1500Machine m(PC1500Variant::PC1500A);
    if (!m.loadROMFile("roms/PC-1500_A04.ROM")) return {};
    m.reset();
    m.runCycles(static_cast<uint64_t>(1300000.0 * 2));
    waitIdle(m, static_cast<uint64_t>(1300000.0 * 5));
    tapKey(m, "cl");
    std::string err;
    typeLine(m, "NEW0", true, &err);
    if (!typeBasicProgramText(m, text).ok) return {};
    std::vector<std::uint8_t> out;
    const std::uint16_t st = static_cast<std::uint16_t>(m.memory().peek(0x7865) << 8 | m.memory().peek(0x7866));
    const std::uint16_t end = static_cast<std::uint16_t>(m.memory().peek(0x7867) << 8 | m.memory().peek(0x7868));
    for (std::uint32_t a = st; a < end; ++a) out.push_back(m.memory().peek(static_cast<std::uint16_t>(a)));
    return out;
}

std::vector<std::uint8_t> oldTyperBytes1600(const std::string& text) {
    PC1600Machine m;
    if (!bootPC1600(m)) return {};
    tapKey(m, "mode");
    waitIdle(m, PC1600Machine::kTStateHz);
    std::string err;
    typeLine(m, "NEW", true, &err);
    waitIdle(m, PC1600Machine::kTStateHz);
    if (!typeBasicProgramText(m, text).ok) return {};
    std::vector<std::uint8_t> out;
    const std::uint16_t st = pc1600::lh5803ToZ80(static_cast<std::uint16_t>(m.debugPeek(0xF865) << 8 | m.debugPeek(0xF866)));
    const std::uint16_t end = pc1600::lh5803ToZ80(static_cast<std::uint16_t>(m.debugPeek(0xF867) << 8 | m.debugPeek(0xF868)));
    for (std::uint32_t a = st; a < end; ++a) out.push_back(m.debugPeek(static_cast<std::uint16_t>(a)));
    return out;
}

TEST(c1600_2_ac7_lines_typed_with_enter_by_engine_pc1500) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing(kProgram));
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
    const std::vector<std::uint8_t> typed = programBytes(memory(f));
    CHECK(!typed.empty() && typed[1] == 10);   // the first record is line 10
}

TEST(c1600_2_ac7_lines_typed_with_enter_by_engine_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1600"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing(kProgram));
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
    CHECK(!programBytes(memory(f)).empty());
}

TEST(c1600_2_ac7_memory_equals_old_typer) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        SessionFixture f;
        CHECK(ready(f, model));
        CHECK(f.run(c::kTypeBasic, listing(kProgram)).done);
        const std::vector<std::uint8_t> oracle =
            std::string(model) == "PC-1600" ? oldTyperBytes1600(kProgram) : oldTyperBytes1500(kProgram);
        CHECK(!oracle.empty());
        CHECK(programBytes(memory(f)) == oracle);
    }
}

TEST(c1600_2_ac7_empty_line_left_out) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n\n\r\n20 END\n"));
    CHECK(r.done);   // an empty line typed would be "not stored"
}

TEST(c1600_2_ac7_char_without_key_refuses_before_typing) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    const std::vector<std::uint8_t> before = programBytes(memory(f));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n20 PRINT \"\xE2\x82\xAC\"\n"));
    CHECK(!r.done && r.reason.find("No key types the character") != std::string::npos);
    CHECK(programBytes(memory(f)) == before);   // line 10 was not typed either
}

// Long lines in a row: each is stored before the next is typed (the wait
// for prompt readiness, then for the end of the program to stay put).
TEST(c1600_2_ac7_waits_prompt_then_end_stable) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    std::string text;
    for (int n = 10; n <= 50; n += 10)
        text += std::to_string(n) + " DATA 11111,22222,33333,44444,55555,66666,77777,88888,99999,12345\n";
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing(text));
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
}

TEST(c1600_2_ac7_line_over_79_not_typed_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1600"));
    const std::string longLine = "20 REM " + std::string(80, 'X');
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n" + longLine + "\n30 END\n"));
    CHECK(!r.done && r.reason.find("1 program line(s) not stored") != std::string::npos);
    CHECK(r.reason.find(longLine) != std::string::npos);
}

TEST(c1600_2_ac7_names_lines_not_stored) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    // A line without a number is a direct command, not stored.
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\nA=2\n20 END\n"));
    CHECK(!r.done && r.reason.find("prog.bas: 1 program line(s) not stored") == 0);
    CHECK(r.reason.find("\"A=2\"") != std::string::npos);
}

TEST(c1600_2_ac7_no_line_stored_fails_with_pro_hint) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));   // RUN mode after the start: lines are commands
    f.runFor(1);
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n20 B=2\n"));
    CHECK(!r.done && r.reason.find("No program line was stored") != std::string::npos &&
          r.reason.find("PRO mode") != std::string::npos);
}

TEST(c1600_2_ac7_mode_not_pressed) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.runFor(1);
    const bool run = indicator(f, "RUN"), pro = indicator(f, "PRO");
    f.run(c::kTypeBasic, listing("10 A=1\n"));
    f.call();
    CHECK(indicator(f, "RUN") == run && indicator(f, "PRO") == pro);
}

// A preset's typed `program:` goes through the same typer, inside the
// preset's operation (design §3.10, §11).
TEST(c1600_2_ac7_preset_typed_program_through_engine) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    const fs::path preset = f.root / "typed.pc1500a";
    {
        std::ofstream out(preset);
        out << "format-version: 2\nmodel: PC-1500A\nkeys:\n  - key: cl\n  - type: NEW0\nprogram:\n  text: |\n"
               "    10 A=1\n    20 PRINT A\n    30 DATA 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20\n    40 END\n";
    }
    CHECK(f.ok(calcu::presets::kLoadPresetCommand, Value::object({{"ref", preset.generic_string()}})));
    CHECK(programBytes(memory(f)) == oldTyperBytes1500(kProgram));
}

} // namespace
