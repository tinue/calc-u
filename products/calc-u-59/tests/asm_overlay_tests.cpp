// C59-4 (design §6, T9, T14, T17): the ASM overlay format 2.0 (AC-14, AC-15)
// and the five shipped overlays (AC-20).

#include "TestHarness.hpp"
#include "TestRoms.hpp"
#include "../app/AsmOverlay.hpp"
#include "../core/TIDebugKinds.hpp"
#include <filesystem>
#include <sstream>
#include <variant>

using namespace cu59;
using Words = std::vector<std::uint16_t>;

namespace {

const std::string kHead = "Calc-U-59 ASM 2.0\n";

Words words(const std::string& text) {
    auto r = readAsmOverlay("t.asm", text);
    if (auto* s = std::get_if<std::string>(&r)) {
        std::fprintf(stderr, "  refused: %s\n", s->c_str());
        return {};
    }
    return std::get<Words>(r);
}
std::string refusal(const std::string& text) {
    auto r = readAsmOverlay("t.asm", text);
    if (auto* s = std::get_if<std::string>(&r)) return *s;
    return "(read)";
}

// tests/fixtures/asm/<name>.words: the instructions 2.3.0's reader gave the
// file, recorded once before the reader changed (T14).
Words recorded(const std::string& name) {
    const auto text = testroms::readFile("tests/fixtures/asm/" + name + ".words");
    Words out;
    if (!text) return out;
    std::istringstream in(*text);
    for (std::string w; in >> w;) out.push_back(static_cast<std::uint16_t>(std::stoul(w, nullptr, 16)));
    return out;
}

} // namespace

// ── AC-14: the rules ─────────────────────────────────────────────────────────

TEST(c59_4_ac14_first_line_version) {
    CHECK(words(kHead + "HEX:\n0A01\n") == Words{0x0A01});
    CHECK(refusal("Calc-U-59 ASM 2.0 \nHEX:\n0A01\n").find("older format") != std::string::npos);   // exactly
}

TEST(c59_4_ac14_prose_before_hex_not_read) {
    CHECK(words(kHead + "PROGRAM:\n1800: 01D8 MOV A.ALL,#0 ; zz\nwords 12 and FFFF\nHEX:\n01D8\n") == Words{0x01D8});
    CHECK(words(kHead + "x\n  hex:  \n01D8\n") == Words{0x01D8});   // any case, alone on its line
}

TEST(c59_4_ac14_semicolon_comments) {
    CHECK(words(kHead + "HEX:\n; a comment 1234\n01D8 ; 01DB\n;\n01DE;tail\n") == (Words{0x01D8, 0x01DE}));
}

TEST(c59_4_ac14_runs_of_four_eight_twelve) {
    CHECK(words(kHead + "HEX:\n0A01 0A020A03 0A040A050A06\n") == (Words{0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05, 0x0A06}));
}

TEST(c59_4_ac14_0x_prefix_either_case) {
    CHECK(words(kHead + "HEX:\n0x0a01 0X0A02 1bcD 0x1f001F01\n") == (Words{0x0A01, 0x0A02, 0x1BCD, 0x1F00, 0x1F01}));
}

TEST(c59_4_ac14_crlf) {
    CHECK(words("Calc-U-59 ASM 2.0\r\nPROGRAM:\r\nHEX:\r\n0A01\r\n0A02 ; c\r\n") == (Words{0x0A01, 0x0A02}));
}

TEST(c59_4_ac14_first_at_1800) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto* o = calcu::machine::first<AsmOverlay>(*m);
    CHECK(o && o->load(words(kHead + "HEX:\n0A01 01D8\n")));
    calcu::machine::DebugTarget& t = m->debugging()->target(0);
    CHECK_EQ(t.peek(0, 0x1800, {}).value, std::uint64_t{0x0A01});
    CHECK_EQ(t.peek(0, 0x1801, {}).value, std::uint64_t{0x01D8});
}

// ── AC-15: one case per refusal (design §3.7, D-5) ───────────────────────────

TEST(c59_4_ac15_outdated_names_the_format_version_and_where) {
    for (const std::string& text : {std::string("HEX:\n0A01\n"), std::string("Calc-U-59 ASM 1.0\nHEX:\n0A01\n"), std::string()}) {
        const std::string s = refusal(text);
        CHECK_EQ(s, std::string("t.asm is an ASM overlay in an older format. This release reads the ASM overlay format "
                                "version 2.0, described in the help's reference chapter."));
    }
}
TEST(c59_4_ac15_no_hex_line) { CHECK_EQ(refusal(kHead + "0A01\n"), std::string("t.asm has no HEX: line.")); }
TEST(c59_4_ac15_nothing_after_hex) {
    CHECK_EQ(refusal(kHead + "HEX:\n; only a comment\n\n"), std::string("t.asm has no instructions after HEX:."));
}
TEST(c59_4_ac15_run_not_a_multiple_of_four) {
    CHECK_EQ(refusal(kHead + "HEX:\n0A01\n0x123\n"), std::string("t.asm, line 4: 0x123 is not a multiple of four hexadecimal digits."));
    CHECK_EQ(refusal(kHead + "HEX:\n12\n"), std::string("t.asm, line 3: 12 is not a multiple of four hexadecimal digits."));   // 2.3.0 passed it over
}
TEST(c59_4_ac15_above_1fff) {
    CHECK_EQ(refusal(kHead + "HEX:\n0A01 2000\n"), std::string("t.asm, line 3: 2000 is above 1FFF."));   // 2.3.0 cut it to 13 bits
}
TEST(c59_4_ac15_neither_hex_nor_comment) {
    CHECK_EQ(refusal(kHead + "HEX:\n0A01 foo\n"), std::string("t.asm, line 3: foo is neither hexadecimal digits nor a comment."));
}
TEST(c59_4_ac15_more_than_2048) {
    std::string text = kHead + "HEX:\n";
    for (int i = 0; i < 2049; ++i) text += "0A01\n";
    CHECK_EQ(refusal(text), std::string("t.asm holds 2049 instructions; at most 2048 fit from 1800 to 1FFF."));
    text.resize(text.size() - 5);
    CHECK_EQ(words(text).size(), std::size_t{2048});
}

// ── AC-20, AC-23: the five shipped overlays ──────────────────────────────────

// Each is in version 2.0 and gives the instructions 2.3.0's reader gave it
// (2.3.0's parser case, without the two lenient cases AC-15 now refuses).
TEST(c59_4_ac20_five_overlays_in_2_0_give_2_3_0s_instructions) {
    const std::pair<const char*, const char*> files[] = {
        {"examples/assembly/Decoder.asm", "Decoder"},          {"examples/assembly/DPT.asm", "DPT"},
        {"examples/assembly/simple_count.asm", "simple_count"}, {"examples/assembly/stopwatch.asm", "stopwatch"},
        {"examples/screentest/test_stopwatch.asm", "test_stopwatch"}};
    int count = 0;
    for (const auto& [path, name] : files) {
        const auto text = testroms::readFile(path);
        CHECK(text.has_value());
        if (!text) continue;
        CHECK_EQ(text->substr(0, kHead.size()), kHead);
        const Words got = words(*text);
        const Words want = recorded(name);
        CHECK(!want.empty());
        if (got != want) std::fprintf(stderr, "  %s: %zu instructions, 2.3.0 gave %zu\n", path, got.size(), want.size());
        CHECK(got == want);
        ++count;
    }
    CHECK_EQ(count, 5);
    // Every .asm in examples/assembly is one of them.
    int all = 0;
    for (const auto& e : std::filesystem::directory_iterator("examples/assembly")) all += e.path().extension() == ".asm";
    CHECK_EQ(all, 4);
}

// DPT.asm needs COND set at its entry (AC-17): entered, it runs.
TEST(c59_4_ac20_dpt_runs_with_cond) {
    auto m = testroms::makeMachine(Model::TI59);
    testroms::runToScanLoop(*m, Model::TI59);
    auto* o = calcu::machine::first<AsmOverlay>(*m);
    CHECK(o && o->load(words(*testroms::readFile("examples/assembly/DPT.asm"))));
    const AsmOverlay::Entry e = o->enter(8192);
    CHECK(e.entered && o->running());
    const CpuFrame f = testroms::dbg(*m).snapshotCPU();
    CHECK(f.pc >= 0x1800 && f.pc < 0x1800 + 14);
    testroms::run(*m, 5000);
    const std::uint16_t pc = testroms::dbg(*m).pc();
    CHECK(pc >= 0x1800 && pc < 0x1800 + 14);   // it stays in its loop
}
