// LIB-10 L14, L15 (design §6, §3.9): the debugger's harness commands on the
// test product, whose machines carry the test processor (TestCpu.hpp, the
// program of TestDebugParts.hpp) and whose session holds a DebuggerPart
// (AC-20, AC-21, AC-50, AC-51; EC-9).
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include <filesystem>
#include <fstream>
#include <string>

using namespace calcu::harness;
using namespace calcu::harness::test;

namespace {

bool has(const std::string& reply, const std::string& part) { return reply.find(part) != std::string::npos; }
bool done(const std::string& reply) { return reply.rfind("done", 0) == 0; }

// Runs single instructions until test-cpu stands at `address` (pc).
bool runTo(TestRun& run, std::uint64_t address) {
    auto* d = run.machine().debugging();
    for (int i = 0; i < 10'000; ++i) {
        if (d->target(0).nextAddress() == address) return true;
        run.server->host().advance(1);
    }
    return false;
}

} // namespace

TEST(ac20_targets_registers_set_register) {
    TestRun run;
    const std::string t = run.line("targets");
    CHECK(has(t, "test-cpu\n  registers: a/8 x/8 pc/16 (read-only) sp/16 z/1\n"));
    CHECK(has(t, "  space memory: 256 cells of 8 bits, banked 0x80+64: 4 banks, 2 vertical banks\n"));
    CHECK(has(t, "  space io-written: 16 cells of 8 bits, the values last written\n"));
    CHECK(has(t, "  code space: memory\n"));
    CHECK(has(t, "  offers: watches step-over-out decode history 8\n"));
    CHECK(has(t, "  qualifier bank: 0 1 2 3 (now 0)\n"));
    run.line("pause");
    CHECK_EQ(run.line("set-register test-cpu a 0x5a"), std::string("done\n"));
    const std::string r = run.line("registers");
    CHECK(has(r, "a = 0x5a\n"));
    CHECK(has(r, "sp = 0x"));
    CHECK(has(r, "z = 0x"));
    CHECK(has(run.line("registers test-cpu"), "a = 0x5a\n"));
    CHECK(has(run.line("set-register test-cpu pc 4"), "refused: The register pc cannot be written."));
    CHECK(has(run.line("set-register test-cpu q 4"), "refused: The target test-cpu has no register q."));
    CHECK(has(run.line("registers nothing"), "refused: The machine has no debug target nothing."));
}

TEST(ac20_peek_poke_marks_unreadable_refuses_at_first) {
    TestRun run;
    run.line("pause");
    CHECK_EQ(run.line("poke test-cpu memory 0x40 1 2 $3"), std::string("done\n"));
    CHECK_EQ(run.line("peek test-cpu memory 0x40 3"), std::string("done 3\n0x40: 0x01\n0x41: 0x02\n0x42: 0x03\n"));
    CHECK_EQ(run.line("peek test-cpu io 8"), std::string("done 1\n0x8: unreadable\n"));
    CHECK_EQ(run.line("peek test-cpu io-written 3"), std::string("done 1\n0x3: none\n"));
    // Refused at the first cell that cannot be written; a refused poke changes nothing.
    CHECK_EQ(run.line("poke test-cpu memory 0xee 9 9 9"), std::string("refused: The cell 0xf0 of memory cannot be written.\n"));
    CHECK_EQ(run.line("peek test-cpu memory 0xee 2"), std::string("done 2\n0xee: 0x00\n0xef: 0x00\n"));
    // EC-9: outside the space.
    CHECK(has(run.line("peek test-cpu memory 0x100"), "refused: The address is outside the space memory."));
    CHECK(has(run.line("peek test-cpu memory 0xff 2"), "refused: The address is outside the space memory."));
    CHECK(has(run.line("poke test-cpu io 16 1"), "refused: The address is outside the space io."));
    // AC-49 through the commands: a bank by name, a range across banks refused.
    CHECK_EQ(run.line("peek test-cpu memory 0x80 1 bank=3 vbank=1"), std::string("done 1\n0x80: 0x50\n"));
    CHECK(done(run.line("poke test-cpu memory 0x81 0x66 bank=2")));
    CHECK_EQ(run.line("peek test-cpu memory 0x81 1 bank=2"), std::string("done 1\n0x81: 0x66\n"));
    CHECK_EQ(run.line("peek test-cpu memory 0x81"), std::string("done 1\n0x81: 0x10\n"));   // no bank switched
    CHECK(has(run.line("peek test-cpu memory 0x7f 2 bank=1"), "refused: The range does not lie inside one bank of memory."));
    CHECK(has(run.line("peek test-cpu memory 0x80 1 bank=4"), "refused: The space memory has no bank 4."));
}

TEST(ac20_break_watch_on_off_list_own_only) {
    TestRun run;
    CHECK_EQ(run.line("break test-cpu 0x12"), std::string("done 1\nbreakpoint 1\n"));
    CHECK_EQ(run.line("break test-cpu 0x10 bank=0"), std::string("done 1\nbreakpoint 2\n"));
    CHECK(has(run.line("break test-cpu 0x10 colour=red"), "refused: The target test-cpu has no qualifier field colour."));
    // A debugger beside the harness sets its own (AC-47).
    auto& host = run.server->host();
    host.command(calcu::sessions::Command{calcu::sessions::CommandName{"break"},
                                          calcu::sessions::Value::object({{"setter", "dap"}, {"target", "test-cpu"}, {"address", 0x14}})});
    CHECK_EQ(run.line("break list"), std::string("done 2\n1 harness test-cpu 0x0012\n2 harness test-cpu 0x0010\n"));
    CHECK(has(run.line("wait 1s"), "run state: paused, owner cpu, reason breakpoint"));
    CHECK(done(run.line("break off test-cpu 0x10")));
    CHECK_EQ(run.line("break list"), std::string("done 1\n1 harness test-cpu 0x0012\n"));
    CHECK(done(run.line("break off")));
    CHECK_EQ(run.line("break list"), std::string("done\n"));
    const auto dap = host.ask(calcu::sessions::Request{calcu::sessions::RequestName{"breakpoints"}, {}});
    CHECK(dap.result->toValue().find("entries")->elements()->size() == 1);   // the debugger's stays
    CHECK_EQ(run.line("watch test-cpu memory 0x30 1 write"), std::string("done 1\nwatch 4\n"));
    CHECK(has(run.line("watch list"), "4 harness test-cpu space 0 0x0030+1 write"));
    CHECK(has(run.line("watch test-cpu memory 0xff 2 both"), "refused: The range runs past the end of the space memory."));
    CHECK(has(run.line("watch test-cpu memory 0x30 1 sideways"), "refused: A watch is read, write or both."));
    CHECK(done(run.line("watch off")));
    CHECK_EQ(run.line("watch list"), std::string("done\n"));
}

TEST(ac20_history_disassemble_with_text) {
    TestRun run;
    run.line("pause");
    CHECK_EQ(run.line("disassemble test-cpu 0 4"),
             std::string("done 4\n0x00  nop\n0x01  ld a, #0x05\n0x03  st a, [0x30]\n0x05  call 0x10\n"));
    run.line("resume");
    CHECK(runTo(run, 0x07));
    run.line("pause");
    const std::string h = run.line("history 3");
    CHECK(h.rfind("done 3\n0x14  ret  a=0x", 0) == 0);
    CHECK(has(h, "0x12  out 0x02, a  a=0x05 x=0x00 pc=0x0012"));
    CHECK(has(h, "0x10  ld a, [0x30]"));
}

TEST(ac20_profile_counts_and_report) {
    TestRun run;
    CHECK(done(run.line("profile on")));
    run.line("wait 10ms");   // 1000 ticks: 250 instructions, 25 rounds of the loop of 10
    CHECK(done(run.line("profile off")));
    const std::string r = run.line("profile report 3");
    CHECK(r.rfind("done 4\ntest-cpu 0x", 0) == 0);
    CHECK(has(r, "10 different addresses\n"));
    run.line("wait 10ms");   // not counted while off
    CHECK_EQ(run.line("profile report 3"), r);
    CHECK(has(run.line("profile sideways"), "refused: usage: profile on|off|report [<count>]"));
}

TEST(ac20_step_over_out_refused_without_offer) {
    TestRun run;
    run.product.setBareCpu(true);
    CHECK(done(run.line("model test-dot")));
    run.line("pause");
    CHECK(has(run.line("step-over"), "offers no stepping over and out."));
    CHECK(has(run.line("step-out main"), "refused: The target main offers no stepping over and out."));
    CHECK(has(run.line("watch main memory 0x30 1 write"), "refused: The target main offers no watches."));
    CHECK(has(run.line("disassemble main 0"), "refused: The target main does not decode."));
    run.product.setBareCpu(false);
    CHECK(done(run.line("model test-seg")));
    run.line("pause");
    run.line("resume");
    CHECK(runTo(run, 0x05));
    run.line("pause");
    const std::string over = run.line("step-over");
    CHECK(has(over, "paused, owner cpu, reason step, target test-cpu"));
}

TEST(ac20_pause_at_start) {
    TestRun run;
    CHECK(done(run.line("pause-at-start")));
    CHECK(has(run.line("state run-state"), "\"pauseAtStart\":true"));
    const std::string reset = run.line("reset");
    CHECK(has(reset, "refused: The operation did not finish."));   // D-8: held before its first instruction
    CHECK(has(run.line("state run-state"), "\"reason\":\"entry\""));
    CHECK_EQ(run.line("registers"), run.line("registers"));
    CHECK(has(run.line("registers"), "pc = 0x0000"));
    CHECK(has(run.line("resume"), "running"));
}

TEST(ac20_state_run_state_and_trace) {
    TestRun run;
    CHECK(has(run.line("state run-state"), "\"paused\":false"));
    CHECK(has(run.line("state trace"), "{\"running\":false,\"file\":\"\"}"));
    CHECK(done(run.line("trace t.bin")));
    CHECK(has(run.line("state trace"), "{\"running\":true,\"file\":\"t.bin\"}"));
    run.line("wait 1ms");
    CHECK(done(run.line("trace off")));
    const auto path = std::filesystem::path(run.server->host().setup().traceFolder()) / "t.bin";
    CHECK(std::filesystem::exists(path));   // in the run's folder (LIB-11 AC-8)
    CHECK(std::filesystem::file_size(path) > 6);
    CHECK(has(run.line("trace a/b.bin"), "refused: trace: takes a file name without a folder, or off."));
    CHECK(has(run.line("pause"), "done 1\npaused, owner cpu, reason pause, target test-cpu\n"));
    CHECK(has(run.line("state run-state"), "\"reason\":\"pause\""));
    CHECK(has(run.line("pause-on-start program"), "done"));
    CHECK(has(run.line("state run-state"), "\"armed\":\"program\""));
    CHECK(has(run.line("pause-on-start off"), "done"));
}

TEST(ac20_step_not_paused_refused) {
    TestRun run;
    CHECK_EQ(run.line("step"), std::string("refused: The machine is not paused.\n"));
    CHECK(has(run.line("step 0"), "refused"));
    run.line("pause");
    CHECK(has(run.line("step { level: cpu, count: 3 }"), "reason step"));
    CHECK(has(run.line("step program 2"), "paused, owner program, reason step"));
    CHECK(has(run.line("step { level: cpu, count: 100001 }"), "refused: step takes a count from 1 to 100000."));
    CHECK(has(run.line("pause basic"), "refused: The machine has no level basic."));
}

TEST(ac21_wait_ends_at_pause_gives_time_and_state) {
    TestRun run;
    run.line("break test-cpu 0x12");
    const std::string w = run.line("wait 1s");
    CHECK(w.rfind("done 3\nticks: ", 0) == 0);
    CHECK(!has(w, "ticks: 100000\n"));   // ended before the second
    CHECK(has(w, "run state: paused, owner cpu, reason breakpoint\n"));
}

TEST(ac21_wait_while_paused_ends_at_once) {
    TestRun run;
    run.line("pause");
    CHECK_EQ(run.line("wait 2s"), std::string("done 3\nticks: 0\nseconds: 0.000000000\nrun state: paused, owner cpu, reason pause\n"));
}

// ── L15 ──────────────────────────────────────────────────────────────────

TEST(ac50_peek_into_file_gives_count) {
    TestRun run;
    run.line("pause");
    run.line("poke test-cpu memory 0x40 0xde 0xad");
    CHECK_EQ(run.line("peek test-cpu memory 0x40 4 file=dump.bin"), std::string("done 1\n4 cells written to dump.bin\n"));
    const auto path = std::filesystem::path(run.server->host().setup().traceFolder()) / "dump.bin";
    std::ifstream in(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK_EQ(bytes, std::string("\xde\xad\x00\x00", 4));
}

TEST(ac50_expect_register_refused_with_value) {
    TestRun run;
    run.line("pause");
    run.line("set-register test-cpu a 0x21");
    CHECK_EQ(run.line("expect-register test-cpu a 0x21"), std::string("done\n"));
    CHECK_EQ(run.line("expect-register test-cpu a 33"), std::string("done\n"));
    CHECK_EQ(run.line("expect-register test-cpu a 0x22"), std::string("refused: The register a is 0x21.\n"));
}

TEST(ac50_expect_memory_refused_at_first_difference) {
    TestRun run;
    run.line("pause");
    run.line("poke test-cpu memory 0x40 1 2 3");
    CHECK_EQ(run.line("expect-memory test-cpu memory 0x40 1 2 3"), std::string("done\n"));
    CHECK_EQ(run.line("expect-memory test-cpu memory 0x40 1 9 3"), std::string("refused: The cell 0x41 of memory is 0x02.\n"));
    CHECK_EQ(run.line("expect-memory test-cpu memory 0x80 0x40 bank=3"), std::string("done\n"));
    CHECK_EQ(run.line("expect-memory test-cpu io 8 0"), std::string("refused: The cell 0x8 of io is unreadable.\n"));
}

TEST(ac51_symbol_names_as_addresses_and_in_output) {
    TestRun run;
    calcu::testing::TempFolder f("calcu-symbols");
    f.write("prog.sym", "start = 0\nroutine = 0x10\nstore = 0x30\n");
    CHECK_EQ(run.line("symbols " + (f / "prog.sym")), std::string("done 1\n3 symbols\n"));
    CHECK(done(run.line("break test-cpu routine")));
    CHECK(has(run.line("break list"), "1 harness test-cpu 0x0010"));
    run.line("wait 1s");
    CHECK(has(run.line("registers"), "pc = 0x0010"));
    CHECK_EQ(run.line("disassemble test-cpu routine 2"),
             std::string("done 2\n0x10 <routine>  ld a, [0x30]\n0x12 <routine+0x2>  out 0x02, a\n"));
    CHECK(has(run.line("peek test-cpu memory store"), "0x30: 0x05"));
    run.line("break off test-cpu routine");
    CHECK_EQ(run.line("break list"), std::string("done\n"));
    run.line("step 3");
    CHECK(has(run.line("history 1"), "<routine+0x4>  ret"));
    CHECK(has(run.line("history 4"), "0x05 <start+0x5>  call 0x10"));
}

TEST(ac51_unknown_name_refused) {
    TestRun run;
    calcu::testing::TempFolder f("calcu-symbols");
    f.write("prog.sym", "routine = 0x10\n");
    run.line("symbols " + (f / "prog.sym"));
    CHECK_EQ(run.line("break test-cpu nowhere"), std::string("refused: There is no symbol nowhere for test-cpu.\n"));
    f.write("prog.lst", "routine 0x10\n");
    CHECK(has(run.line("symbols " + (f / "prog.lst")), "refused: No assembler that this product supports wrote"));
}
