// The DAP's layer in app/dap/ -- its breakpoints with their tests, its line
// steps on the library's `step` with a test, its view of the targets --
// driven with the memtest fixture (MemtestRig.hpp), and the cases of
// listings, bindings, expressions, conditions, hit conditions and log
// messages.

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "../core/Debug/DebugExpression.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "MemtestRig.hpp"
#include "TestHarness.hpp"

namespace {

using Reason = calcu::debugger::RunState::Reason;
using cu1600::dap::StepKind;

// ── Expressions ───────────────────────────────────────────────────────────

debug::ExpressionResult eval(const std::string& text, bool bigEndian = false) {
    static std::map<std::string, int64_t> names = {{"a", 0x10}, {"hl", 0x7A00}, {"cf", 1}, {"LOOP", 0x40C5}};
    static uint8_t mem[65536] = {};
    mem[0x7A00] = 0x34; mem[0x7A01] = 0x12;
    debug::ExpressionContext ctx;
    ctx.lookup = [](const std::string& n, int64_t* v) {
        auto it = names.find(n);
        if (it == names.end()) return false;
        *v = it->second;
        return true;
    };
    ctx.readByte = [](uint16_t addr, bool me1, uint8_t* v) {
        if (me1) return false;
        *v = mem[addr];
        return true;
    };
    ctx.bigEndian = bigEndian;
    return debug::evaluate(text, ctx);
}

bool evalIs(const std::string& text, int64_t want, bool bigEndian = false) {
    debug::ExpressionResult r = eval(text, bigEndian);
    if (!r.ok || r.value != want)
        std::fprintf(stderr, "  expr \"%s\": ok=%d value=%lld error=%s\n", text.c_str(), r.ok, (long long)r.value, r.error.c_str());
    return r.ok && r.value == want;
}

TEST(dap_expressions_evaluate) {
    CHECK(evalIs("A == 0x10", 1));
    CHECK(evalIs("a != 16", 0));
    CHECK(evalIs("1 + 2 * 3", 7));
    CHECK(evalIs("(1 + 2) * 3", 9));
    CHECK(evalIs("$FF & &0F | 0x100", 0x10F));
    CHECK(evalIs("1 << 4 >> 2", 4));
    CHECK(evalIs("-a + 20", 4));
    CHECK(evalIs("!cf || a >= 0x10 && a < 0x20", 1));
    CHECK(evalIs("~0 & 0xFF", 0xFF));
    CHECK(evalIs("[hl]", 0x34));
    CHECK(evalIs("w[hl]", 0x1234));
    CHECK(evalIs("w[hl]", 0x3412, /*bigEndian=*/true));
    CHECK(evalIs("[0x7A00 + 1] == 0x12", 1));
    CHECK(evalIs("LOOP", 0x40C5));
    CHECK(evalIs("10 % 4 + 9 / 2", 6));
    CHECK(!eval("nosuch + 1").ok);
    CHECK(!eval("#[0]").ok);      // unreadable
    CHECK(!eval("1 +").ok);
    CHECK(!eval("(1").ok);
    CHECK(!eval("1 / 0").ok);
    CHECK(!eval("").ok);
    CHECK(eval("nosuch").error.find("nosuch") != std::string::npos);
}

// ── Run-control cases with source lines and tests ─────────────────────────

TEST(line_breakpoint_and_line_steps) {
    memtest::Rig r;
    if (!r.ok) { std::printf("  SKIP: memtest fixture missing\n"); return; }
    // Line 88 is a comment: the breakpoint resolves to 89 (ldi a,0x00).
    auto st = r.lineBreakpoint(88);
    CHECK(st.size() == 1 && st[0].verified && st[0].line == 89 && st[0].addr == 0x40CC);
    r.resume();
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Breakpoint) && r.stopIds() == std::vector<int>{st[0].id});
    CHECK(r.pc() == 0x40CC && r.line() == 89);

    // Line steps: 89 -> 90 -> 94 (91-93 are blank/comments).
    r.step(StepKind::Over, /*line=*/true);
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Step) && r.line() == 90);
    r.step(StepKind::In, true);
    r.runUntilStopped();
    CHECK(r.line() == 94 && r.pc() == 0x40D1);

    // Instruction step.
    r.step(StepKind::Instruction, false);
    r.runUntilStopped();
    CHECK(r.pc() == 0x40D2);

    // Step out of memtest: back in the harness after its rtn.
    r.clearBreakpoints();
    r.step(StepKind::Out, false);
    CHECK(r.runUntilStopped(20000));
    CHECK(r.reasonIs(Reason::Step));
    CHECK(r.pc() == 0x4603 && r.line() == 0);
    uint8_t flag = 0xFF;
    CHECK(r.targets().peek(1, debug::kSpaceMain, 0x40C7, &flag) && flag == 0); // ERR_FLAG: pass
}

TEST(conditions_hit_counts_and_logpoints) {
    memtest::Rig r;
    if (!r.ok) return;
    // Line 137 is `sta (x)` in pass 1's loop, once per tested byte.
    r.lineBreakpoint(137, "x == 0x4210");
    r.resume();
    CHECK(r.runUntilStopped());
    uint32_t x = 0;
    CHECK(r.reasonIs(Reason::Breakpoint));
    CHECK(r.targets().readRegister(1, "x", &x) && x == 0x4210);

    // Hit condition: the third hit after re-arming.
    r.lineBreakpoint(137, {}, "== 3");
    r.resume();
    r.runUntilStopped();
    CHECK(r.targets().readRegister(1, "x", &x) && x == 0x4213);

    // A logpoint prints and never stops: 0x4214..0x43FF remain in pass 1.
    r.lineBreakpoint(137, {}, {}, "x={x} a={a}");
    cu1600::dap::DapBreakpoints::InstructionRequest park;
    park.addr = 0x4603;
    r.breakpoints.setInstructions({park});
    r.apply();
    r.output.clear();
    r.resume();
    CHECK(r.runUntilStopped(20000));
    CHECK(r.pc() == 0x4603);
    CHECK_EQ(r.output.size(), std::size_t(0x4400 - 0x4214));
    CHECK(!r.output.empty() && r.output.front() == "x=0x4214 a=0x55");

    // A broken condition stops and says why.
    r.targets().writeRegister(1, "p", 0x4600);
    r.lineBreakpoint(89, "nosuch == 1");
    r.output.clear();
    r.resume();
    CHECK(r.runUntilStopped());
    CHECK(r.pc() == 0x40CC && r.output.size() == 1);
}

TEST(bank_qualified_listing) {
    memtest::Rig r;
    if (!r.ok) return;
    // The same listing bound statically for PV=1 only: with PV=0 its
    // breakpoints are set but never stop.
    debug::Listing listing;
    std::string error;
    CHECK(debug::loadListing("tests/fixtures/listings/sdas-lh5801/memtest.rst", &listing, &error));
    r.map.clear();
    r.map.addStatic(1, listing, {-1, -1, -1, 1}, "pv1");
    auto st = r.lineBreakpoint(89);
    CHECK(st[0].verified && st[0].message.find("pv=1") != std::string::npos);
    cu1600::dap::DapBreakpoints::InstructionRequest park;
    park.addr = 0x4603;
    r.breakpoints.setInstructions({park});
    r.apply();
    r.resume();
    r.runUntilStopped(20000);
    CHECK(r.pc() == 0x4603);
    // With PV=1 the same breakpoint stops.
    r.targets().writeRegister(1, "p", 0x4600);
    r.targets().writeRegister(1, "pv", 1);
    r.resume();
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Breakpoint) && r.pc() == 0x40CC);
}

TEST(hit_condition_parsing) {
    using cu1600::dap::hitConditionMet;
    CHECK(hitConditionMet("3", 3) && !hitConditionMet("3", 4));
    CHECK(hitConditionMet(">= 5", 5) && !hitConditionMet(">=5", 4));
    CHECK(hitConditionMet("% 3", 6) && !hitConditionMet("%3", 7));
    CHECK(hitConditionMet("< 2", 1) && !hitConditionMet("<2", 2));
    bool valid = true;
    CHECK(hitConditionMet("often", 1, &valid) && !valid);
}

// ── Line steps ───────────────────────────────────────────────────────────

TEST(step_goes_on_until_the_line_changes) {
    memtest::Rig r;
    if (!r.ok) return;
    r.lineBreakpoint(89);
    r.resume();
    CHECK(r.runUntilStopped());
    r.clearBreakpoints();
    // One DAP step is one library step: it ends once, on a new line,
    // having carried out more than one instruction where the line has more.
    const std::uint64_t before = r.targets().retired(1);
    r.step(StepKind::In, true);
    CHECK(!r.rig.paused());
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Step) && r.line() == 90);
    CHECK(r.targets().retired(1) >= before + 1);
    // Code without source ends a line step: step out, then a line step in
    // the harness is one instruction.
    r.step(StepKind::Out, false);
    r.runUntilStopped(20000);
    const std::uint64_t at = r.targets().retired(1);
    r.step(StepKind::Over, true);
    r.runUntilStopped();
    CHECK_EQ(r.targets().retired(1), at + 1);
}

TEST(instruction_granularity_one_instruction) {
    memtest::Rig r;
    if (!r.ok) return;
    r.lineBreakpoint(89);
    r.resume();
    CHECK(r.runUntilStopped());
    const std::uint64_t before = r.targets().retired(1);
    r.step(StepKind::Instruction, true);   // the client asked for single instructions
    CHECK(r.runUntilStopped());
    CHECK_EQ(r.targets().retired(1), before + 1);
    CHECK(r.reasonIs(Reason::Step));
}

TEST(breakpoints_belong_to_dap) {
    memtest::Rig r;
    if (!r.ok) return;
    // A breakpoint the harness set stays when the DAP's are set again.
    calcu::debugger::BreakpointTable& table = r.rig.rc().table();
    const auto harness = table.addBreakpoint(calcu::debugger::kHarnessSetter, r.rig.debugging(),
                                             calcu::machine::DebugTargetId{"LH5801"}, 0x4603, {});
    CHECK(std::holds_alternative<calcu::debugger::BreakpointId>(harness));
    r.lineBreakpoint(89);
    r.lineBreakpoint(90);
    const auto dap = table.breakpoints(&cu1600::dap::kDapSetter);
    CHECK_EQ(dap.size(), std::size_t(1));
    CHECK(dap.size() == 1 && dap[0].address == 0x40CE);
    CHECK_EQ(table.breakpoints(&calcu::debugger::kHarnessSetter).size(), std::size_t(1));
    r.clearBreakpoints();
    CHECK(table.breakpoints(&cu1600::dap::kDapSetter).empty());
    CHECK_EQ(table.breakpoints().size(), std::size_t(1));
}

TEST(log_message_writes_and_does_not_pause) {
    memtest::Rig r;
    if (!r.ok) return;
    r.lineBreakpoint(89, {}, {}, "at {p}");
    cu1600::dap::DapBreakpoints::InstructionRequest park;
    park.addr = 0x4603;
    r.breakpoints.setInstructions({park});
    r.apply();
    r.resume();
    CHECK(r.runUntilStopped(20000));
    CHECK(r.pc() == 0x4603);   // passed line 89 without pausing there
    CHECK(r.output.size() == 1 && r.output[0] == "at 0x40CC");
}

TEST(unworkable_condition_pauses_and_says_why) {
    memtest::Rig r;
    if (!r.ok) return;
    r.lineBreakpoint(89, "[nosuch] == 1");
    r.resume();
    CHECK(r.runUntilStopped());
    CHECK(r.reasonIs(Reason::Breakpoint) && r.pc() == 0x40CC);
    CHECK(r.output.size() == 1 && r.output[0].find("Breakpoint condition '[nosuch] == 1'") == 0);
    CHECK(r.output.size() == 1 && r.output[0].find("nosuch") != std::string::npos);
}

TEST(other_bank_passed_status_only_while) {
    memtest::Rig r;
    if (!r.ok) return;
    debug::Listing listing;
    std::string error;
    CHECK(debug::loadListing("tests/fixtures/listings/sdas-lh5801/memtest.rst", &listing, &error));
    r.map.clear();
    r.map.addStatic(1, listing, {-1, -1, 1, -1}, "pu1");   // PU = 1 only; the CPU runs with PU = 0
    auto st = r.lineBreakpoint(89);
    CHECK(st[0].verified && st[0].message == "Only while pu=1");
    cu1600::dap::DapBreakpoints::InstructionRequest park;
    park.addr = 0x4603;
    r.breakpoints.setInstructions({park});
    r.apply();
    r.resume();
    CHECK(r.runUntilStopped(20000));
    CHECK(r.pc() == 0x4603);
}

TEST(expressions_evaluate_as_written) {
    static std::map<std::string, int64_t> names = {{"a", 0x10}, {"hl", 0x7A00}, {"cf", 1}, {"LOOP", 0x40C5}, {"Mixed", 7}};
    static uint8_t mem[65536] = {};
    mem[0x7A00] = 0x34; mem[0x7A01] = 0x12; mem[0x0010] = 0x99;
    debug::ExpressionContext full;
    full.lookup = [](const std::string& n, int64_t* v) {
        auto it = names.find(n);
        if (it == names.end()) return false;
        *v = it->second;
        return true;
    };
    full.readByte = [](uint16_t addr, bool me1, uint8_t* v) {
        if (addr == 0x1234 || (me1 && addr >= 0x8000)) return false;
        *v = mem[addr];
        return true;
    };
    debug::ExpressionContext bigEndian = full;
    bigEndian.bigEndian = true;
    debug::ExpressionContext noMemory = full;
    noMemory.readByte = nullptr;
    debug::ExpressionContext nothing;
    enum Context { Full, BigEndian, NoMemory, Nothing };
    const debug::ExpressionContext* contexts[] = {&full, &bigEndian, &noMemory, &nothing};

    struct Case {
        Context context;
        const char* text;
        bool ok;
        int64_t value;
        const char* error;
    };
    // Every text in the full context; in the others only where the answer
    // differs from the full one (the rest is checked there too).
    const Case cases[] = {
        {Full, "", false, 0, "empty expression"},
        {Full, "  ", false, 0, "empty expression"},
        {Full, "1", true, 1, ""},
        {Full, "0x10", true, 0x10, ""},
        {Full, "$ff", true, 0xFF, ""},
        {Full, "&7A00", true, 0x7A00, ""},
        {Full, "0x", false, 0, "missing hex digits"},
        {Full, "$", false, 0, "missing hex digits"},
        {Full, "12+3*4", true, 0x18, ""},
        {Full, "(1+2)*3", true, 9, ""},
        {Full, "-5 % 3", true, -2, ""},
        {Full, "~0", true, -1, ""},
        {Full, "!0", true, 1, ""},
        {Full, "!!7", true, 1, ""},
        {Full, "+3", true, 3, ""},
        {Full, "1<<4", true, 0x10, ""},
        {Full, "256>>2", true, 0x40, ""},
        {Full, "1<2", true, 1, ""},
        {Full, "2<=2", true, 1, ""},
        {Full, "3>2", true, 1, ""},
        {Full, "3>=4", true, 0, ""},
        {Full, "1==1", true, 1, ""},
        {Full, "1!=1", true, 0, ""},
        {Full, "6&3", true, 2, ""},
        {Full, "6|3", true, 7, ""},
        {Full, "6^3", true, 5, ""},
        {Full, "1&&0", true, 0, ""},
        {Full, "0||2", true, 1, ""},
        {Full, "a", true, 0x10, ""},
        {Full, "A", true, 0x10, ""},
        {Full, "hl", true, 0x7A00, ""},
        {Full, "Mixed", true, 7, ""},
        {Full, "mixed", false, 0, "unknown name 'mixed'"},
        {Full, "LOOP", true, 0x40C5, ""},
        {Full, "loop", false, 0, "unknown name 'loop'"},
        {Full, "foo", false, 0, "unknown name 'foo'"},
        {Full, "foo +", false, 0, "unknown name 'foo'"},
        {Full, "foo bar", false, 0, "unknown name 'foo'"},
        {Full, "a b", false, 0, "unexpected 'b'"},
        {Full, "1 2", false, 0, "unexpected '2'"},
        {Full, "1/0", false, 0, "division by zero"},
        {Full, "1/0*", false, 0, "division by zero"},
        {Full, "1%0", false, 0, "division by zero"},
        {Full, "1/foo", false, 0, "unknown name 'foo'"},
        {Full, "foo/0", false, 0, "unknown name 'foo'"},
        {Full, "[hl]", true, 0x34, ""},
        {Full, "w[hl]", true, 0x1234, ""},
        {Full, "#[0x10]", true, 0x99, ""},
        {Full, "#[0x9000]", false, 0, "memory at 36864 is not readable"},
        {Full, "[0x1234]", false, 0, "memory at 4660 is not readable"},
        {Full, "w[0x1233]", false, 0, "memory at 4659 is not readable"},
        {Full, "[hl", false, 0, "missing ']'"},
        {Full, "w[", false, 0, "unexpected end of expression"},
        {Full, "#[", false, 0, "unexpected end of expression"},
        {Full, "(1+2", false, 0, "missing ')'"},
        {Full, "1+2)", false, 0, "unexpected ')'"},
        {Full, "1+", false, 0, "unexpected end of expression"},
        {Full, "*2", false, 0, "unexpected '*'"},
        {Full, "!=3", false, 0, "unexpected '!'"},
        {Full, "a == 0x10 && cf", true, 1, ""},
        {Full, "[hl] == 0x34 || foo", false, 0, "unknown name 'foo'"},
        {Full, "(foo", false, 0, "unknown name 'foo'"},
        {Full, "[foo]", false, 0, "unknown name 'foo'"},
        {Full, "[1/0]", false, 0, "division by zero"},
        {Full, "1 + [0x1234] + foo", false, 0, "memory at 4660 is not readable"},
        {Full, "foo + [0x1234]", false, 0, "unknown name 'foo'"},
        {Full, "0x7FFFFFFFFFFFFFFF+1", true, INT64_MIN, ""},
        {Full, "-(-9223372036854775807-1)", true, INT64_MIN, ""},
        {Full, "(-9223372036854775807-1)/-1", true, INT64_MIN, ""},
        {Full, "(-9223372036854775807-1)%-1", true, 0, ""},
        {Full, "0x7FFFFFFFFFFFFFFF*2", true, -2, ""},
        {Full, "1<<64", true, 1, ""},
        {Full, "1<<63", true, INT64_MIN, ""},
        {Full, "-1>>1", true, -1, ""},
        {Full, "a&&&cf", true, 1, ""},
        {Full, "a|||cf", false, 0, "unexpected '|'"},
        {Full, "a<<<1", false, 0, "unexpected '<'"},
        {Full, "a===1", false, 0, "unexpected '='"},
        {Full, "_x.y$'", false, 0, "unknown name '_x.y$''"},
        {Full, "w [hl]", false, 0, "unknown name 'w'"},
        {Full, "W[hl]", true, 0x1234, ""},
        {Full, "#(1)", false, 0, "unexpected '#'"},
        {Full, "((((((1))))))", true, 1, ""},
        {Full, "1 ? 2", false, 0, "unexpected '?'"},
        {Full, "@", false, 0, "unexpected '@'"},
        {Full, "a @", false, 0, "unexpected '@'"},
        {Full, "[hl] @", false, 0, "unexpected '@'"},
        {Full, "1 +\t2", true, 3, ""},
        {BigEndian, "w[hl]", true, 0x3412, ""},
        {BigEndian, "W[hl]", true, 0x3412, ""},
        {NoMemory, "[hl]", false, 0, "memory not available"},
        {NoMemory, "w[hl]", false, 0, "memory not available"},
        {NoMemory, "#[0x10]", false, 0, "memory not available"},
        {NoMemory, "#[0x9000]", false, 0, "memory not available"},
        {NoMemory, "[0x1234]", false, 0, "memory not available"},
        {NoMemory, "w[0x1233]", false, 0, "memory not available"},
        {NoMemory, "[hl] == 0x34 || foo", false, 0, "memory not available"},
        {NoMemory, "1 + [0x1234] + foo", false, 0, "memory not available"},
        {NoMemory, "W[hl]", false, 0, "memory not available"},
        {NoMemory, "[hl] @", false, 0, "memory not available"},
        {Nothing, "a", false, 0, "unknown name 'a'"},
        {Nothing, "A", false, 0, "unknown name 'A'"},
        {Nothing, "hl", false, 0, "unknown name 'hl'"},
        {Nothing, "Mixed", false, 0, "unknown name 'Mixed'"},
        {Nothing, "LOOP", false, 0, "unknown name 'LOOP'"},
        {Nothing, "a b", false, 0, "unknown name 'a'"},
        {Nothing, "[hl]", false, 0, "unknown name 'hl'"},
        {Nothing, "w[hl]", false, 0, "unknown name 'hl'"},
        {Nothing, "#[0x10]", false, 0, "memory not available"},
        {Nothing, "#[0x9000]", false, 0, "memory not available"},
        {Nothing, "[0x1234]", false, 0, "memory not available"},
        {Nothing, "w[0x1233]", false, 0, "memory not available"},
        {Nothing, "[hl", false, 0, "unknown name 'hl'"},
        {Nothing, "a == 0x10 && cf", false, 0, "unknown name 'a'"},
        {Nothing, "[hl] == 0x34 || foo", false, 0, "unknown name 'hl'"},
        {Nothing, "1 + [0x1234] + foo", false, 0, "memory not available"},
        {Nothing, "a&&&cf", false, 0, "unknown name 'a'"},
        {Nothing, "a|||cf", false, 0, "unknown name 'a'"},
        {Nothing, "a<<<1", false, 0, "unknown name 'a'"},
        {Nothing, "a===1", false, 0, "unknown name 'a'"},
        {Nothing, "W[hl]", false, 0, "unknown name 'hl'"},
        {Nothing, "a @", false, 0, "unknown name 'a'"},
        {Nothing, "[hl] @", false, 0, "unknown name 'hl'"},
    };
    int mismatches = 0;
    auto check = [&](Context c, const char* text, bool ok, int64_t value, const std::string& error) {
        const debug::ExpressionResult got = debug::evaluate(text, *contexts[c]);
        if (got.ok != ok || got.error != error || (ok && got.value != value)) {
            if (mismatches++ < 10)
                std::fprintf(stderr, "  expr \"%s\" in context %d: want ok=%d %lld '%s', got ok=%d %lld '%s'\n", text, int(c),
                             ok, (long long)value, error.c_str(), got.ok, (long long)got.value, got.error.c_str());
        }
    };
    for (const Case& k : cases) check(k.context, k.text, k.ok, k.value, k.error);
    for (const Case& k : cases) {
        if (k.context != Full) continue;
        for (Context c : {BigEndian, NoMemory, Nothing}) {
            bool listed = false;
            for (const Case& other : cases) listed = listed || (other.context == c && std::string(other.text) == k.text);
            if (!listed) check(c, k.text, k.ok, k.value, k.error);
        }
    }
    CHECK(mismatches == 0);
}

TEST(hit_conditions_and_log_templates) {
    struct Hit {
        const char* condition;
        bool valid;
        uint32_t stopsAt; // bit n: the condition is met at hit count n (0..20)
    };
    const Hit hits[] = {
        {"", true, 0x1FFFFF},
        {"  ", true, 0x1FFFFF},
        {"5", true, 0x000020},
        {"== 5", true, 0x000020},
        {"=5", true, 0x000020},
        {">= 3", true, 0x1FFFF8},
        {"<=2", true, 0x000007},
        {"!= 4", true, 0x1FFFEF},
        {"> 1", true, 0x1FFFFC},
        {"<3", true, 0x000007},
        {"% 3", true, 0x049249},
        {"%0", true, 0x000000},
        {"% -2", true, 0x000000},
        {"0x10", true, 0x010000},
        {">= 0x2", true, 0x1FFFFC},
        {"abc", false, 0x1FFFFF},
        {"5x", false, 0x1FFFFF},
        {"==", false, 0x1FFFFF},
        {" % ", false, 0x1FFFFF},
        {"-1", true, 0x000000},
        {"= 3 ", true, 0x000008},
    };
    int mismatches = 0;
    for (const Hit& h : hits)
        for (int n = 0; n <= 20; n++) {
            bool valid = false;
            const bool met = cu1600::dap::hitConditionMet(h.condition, n, &valid);
            if (met != (((h.stopsAt >> n) & 1) != 0) || valid != h.valid) {
                if (mismatches++ < 5) std::fprintf(stderr, "  hit \"%s\" at %d: got %d/%d\n", h.condition, n, met, valid);
            }
        }
    debug::ExpressionContext ctx;
    ctx.lookup = [](const std::string& n, int64_t* v) {
        if (n != "a" && n != "x") return false;
        *v = n == "a" ? 0x42 : 0x1234;
        return true;
    };
    ctx.readByte = [](uint16_t addr, bool, uint8_t* v) { *v = uint8_t(addr); return addr != 7; };
    struct Log {
        const char* message;
        const char* text;
    };
    const Log logs[] = {
        {"", ""},
        {"plain", "plain"},
        {"a={a}", "a=0x42"},
        {"{a}{x}", "0x420x1234"},
        {"x={x} a={a}!", "x=0x1234 a=0x42!"},
        {"{foo}", "{unknown name 'foo'}"},
        {"{[7]}", "{memory at 7 is not readable}"},
        {"{[8]}", "0x08"},
        {"{-1}", "-1"},
        {"{1/0}", "{division by zero}"},
        {"open {a", "open {a"},
        {"}{a}", "}0x42"},
        {"{}", "{empty expression}"},
        {"{{a}}", "{unexpected '{'}}"},
        {"a={a} b={", "a=0x42 b={"},
        {"{ a + 1 }", "0x43"},
    };
    for (const Log& l : logs) {
        const std::string got = cu1600::dap::interpolateLog(l.message, ctx);
        if (got != l.text) {
            if (mismatches++ < 10) std::fprintf(stderr, "  log \"%s\": want '%s', got '%s'\n", l.message, l.text, got.c_str());
        }
    }
    CHECK(mismatches == 0);
}

TEST(function_breakpoint_on_lh5803_symbol) {
    // A function breakpoint arms on the CPU whose binding defines the
    // symbol: here the LH5803 (thread 2).
    sharpdebugtest::Rig rig(sharpdebugtest::bareMachine("PC-1600"));
    PC1600Machine& m = static_cast<PC1600Machine&>(*rig.machine);
    std::vector<uint8_t> lower(PC1600Memory::kBankSize, 0x00), upper(PC1600Memory::kBankSize, 0x00);
    lower[0] = 0xD3; lower[1] = 0x38; lower[2] = 0x76; // out (38h),a ; halt
    CHECK(m.loadBank0(lower.data(), lower.size(), upper.data(), upper.size()));
    std::vector<uint8_t> rom(16384, 0x00);
    const uint8_t code[] = {0xAE, 0x10, 0x00, 0x38, 0x9E, 0x06}; // sta (0x1000) ; nop ; bch C000
    for (size_t i = 0; i < sizeof code; i++) rom[i] = code[i];
    rom[16384 - 2] = 0xC0; rom[16384 - 1] = 0x00;
    CHECK(m.loadLH5803Rom(rom.data(), rom.size()));
    m.reset();

    debug::SourceMap map;
    debug::Listing symbols;
    symbols.symbols["LOOP"] = 0xC003;
    map.addStatic(2, symbols, {}, "lh.sym");
    cu1600::dap::DapBreakpoints table;
    cu1600::dap::DapBreakpoints::FunctionRequest r;
    r.name = "LOOP";
    const auto statuses = table.setFunctions({r}, map);
    CHECK(statuses.size() == 1 && statuses[0].verified && statuses[0].thread == 2 && statuses[0].addr == 0xC003);
    table.apply(rig.rc().table(), cu1600::dap::DapTargets(rig.machine->debugging()), {});
    CHECK(rig.runUntilPaused(100, 1000));
    const auto st = rig.rc().state();
    CHECK(st.reason && *st.reason == calcu::debugger::RunState::Reason::Breakpoint);
    CHECK(st.target && st.target->value == "LH5803");
    CHECK(table.dapIds(st.hits) == std::vector<int>{statuses[0].id});
    CHECK(cu1600::dap::DapTargets(rig.machine->debugging()).pc(2) == 0xC003);
}

} // namespace
