// Typing a listing in through the typing
// engine (sharp::loaders::typeProgram), as the session command
// `c1600.type-basic` and as a preset's typed `program:` -- in a session with
// the typing part and the loader part (SessionFixture), virtual time.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../core/PC1600/PC1600ProgramPlacement.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "../app/tests/SessionFixture.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = cu1600::commands;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;

sharp::LoaderMemory& memory(SessionFixture& f) { return *calcu::machine::first<sharp::LoaderMemory>(*f.part().machine()); }

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

TEST(lines_typed_with_enter_by_engine_pc1500) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing(kProgram));
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
    const std::vector<std::uint8_t> typed = programBytes(memory(f));
    CHECK(!typed.empty() && typed[1] == 10);   // the first record is line 10
}

TEST(lines_typed_with_enter_by_engine_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1600"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing(kProgram));
    CHECK(r.done);
    if (!r.done) std::fprintf(stderr, "  %s\n", r.reason.c_str());
    CHECK(!programBytes(memory(f)).empty());
}

TEST(empty_line_left_out) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n\n\r\n20 END\n"));
    CHECK(r.done);   // an empty line typed would be "not stored"
}

TEST(char_without_key_refuses_before_typing) {
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
TEST(waits_prompt_then_end_stable) {
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

TEST(line_over_79_not_typed_pc1600) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1600"));
    const std::string longLine = "20 REM " + std::string(80, 'X');
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n" + longLine + "\n30 END\n"));
    CHECK(!r.done && r.reason.find("1 program line(s) not stored") != std::string::npos);
    CHECK(r.reason.find(longLine) != std::string::npos);
}

TEST(names_lines_not_stored) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(ready(f, "PC-1500A"));
    // A line without a number is a direct command, not stored.
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\nA=2\n20 END\n"));
    CHECK(!r.done && r.reason.find("prog.bas: 1 program line(s) not stored") == 0);
    CHECK(r.reason.find("\"A=2\"") != std::string::npos);
}

TEST(no_line_stored_fails_with_pro_hint) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));   // RUN mode after the start: lines are commands
    f.runFor(1);
    const sessions::ReplyMessage r = f.run(c::kTypeBasic, listing("10 A=1\n20 B=2\n"));
    CHECK(!r.done && r.reason.find("No program line was stored") != std::string::npos &&
          r.reason.find("PRO mode") != std::string::npos);
}

TEST(mode_not_pressed) {
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
// preset's operation.
TEST(preset_typed_program_through_engine) {
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
    // The same bytes as the command's.
    SessionFixture other;
    CHECK(ready(other, "PC-1500A"));
    CHECK(other.run(c::kTypeBasic, listing(kProgram)).done);
    CHECK(!programBytes(memory(other)).empty() && programBytes(memory(f)) == programBytes(memory(other)));
}

} // namespace
