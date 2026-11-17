// C59-9 (design §6, T1 to T9): the product commands of Calc-U-59 under
// `ti59`, each on a `calcu` run (harness::Server) with a program small
// enough that its reply is written out here. The numbers were compared with
// today's tools on the same program before they went (M-59; the qa-report
// names each difference).
#include "../app/HarnessProduct.hpp"
#include "../app/TIMnemonics.hpp"
#include "../app/TITyping.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace calcu::harness;

namespace {

bool romsThere() { return std::filesystem::exists("roms/TMC0571B.txt"); }

struct Run {
    std::unique_ptr<HarnessProduct> product = cu59::makeHarnessProduct();
    std::unique_ptr<Server> server;
    explicit Run(RunOptions o = {}) {
        std::string error;
        server = Server::start(*product, o, &error);
        if (!server) std::fprintf(stderr, "%s\n", error.c_str());
    }
    ~Run() {
        if (server) server->finish();
    }
    std::string operator()(const std::string& l) { return server->line(l).text; }
    explicit operator bool() const { return server != nullptr; }
};

// The reply's lines after its first (`done <n>`).
std::vector<std::string> lines(const std::string& reply) {
    std::vector<std::string> out;
    std::istringstream in(reply);
    std::string l;
    std::getline(in, l);
    while (std::getline(in, l)) out.push_back(l);
    return out;
}

std::string join(const std::vector<std::string>& v) {
    std::string s;
    for (const auto& x : v) s += x + "\n";
    return s;
}

std::string scratch(const std::string& name, const std::string& text) {
    std::filesystem::create_directories("build");
    const std::string p = "build/c59_9_" + name;
    std::ofstream(p, std::ios::binary) << text;
    return std::filesystem::absolute(p).string();
}

const char* kLinesProgram = "ti59 program 000 76 11 01 99 02 99 98 03 99 91";   // Lbl A 1 Prt 2 Prt Adv 3 Prt R/S
const char* kLoopProgram = "ti59 program 000 76 11 42 00 76 12 97 00 12 91";    // Lbl A STO 00 Lbl B Dsz 0 B R/S

} // namespace

// ── T1: AC-1 ──

TEST(c59_9_ac1_commands_under_ti59_on_ti59_ti58_ti58c) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    CHECK(r);
    if (!r) return;
    std::string usage;
    for (const auto& u : r.server->commands().usage()) usage += u + "\n";
    for (const char* c : {"ti59 coverage", "ti59 line-times", "ti59 run-time", "ti59 hot-steps", "ti59 step-trace",
                          "ti59 step-cost", "ti59 keycodes", "ti59 program", "ti59 asm"})
        CHECK(usage.find(c) != std::string::npos);
    for (const char* model : {"TI-59", "TI-58", "TI-58C"}) {
        CHECK_EQ(r(std::string("model ") + model), std::string("done\n"));
        CHECK_EQ(r("ti59 keycodes list 000 000"), std::string("done 1\n000 00 0\n"));
        CHECK_EQ(r("ti59 program 000 76 11 92"), std::string("done\n"));
        CHECK_EQ(r("ti59 keycodes disassemble"), std::string("done 2\n000  Lbl A\n002  RTN\n"));
    }
    CHECK_EQ(r("ti59 nothing"), std::string("refused: unknown command ti59 nothing\n"));
}

// ── T2: AC-2 ──

TEST(c59_9_ac2_refused_without_profile) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    CHECK_EQ(r("ti59 coverage"), std::string("refused: Profile is off: send `profile on` first.\n"));
    CHECK_EQ(r("profile on"), std::string("done\n"));
    CHECK_EQ(r("profile off"), std::string("done\n"));
    CHECK_EQ(r("ti59 coverage"), std::string("refused: Profile is off: send `profile on` first.\n"));
    CHECK_EQ(r("ti59 coverage most"), std::string("refused: usage: ti59 coverage [all]\n"));
}

TEST(c59_9_ac2_addresses_of_one_key_in_order_without_key_readiness) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("profile on");
    r("key 5");
    // tools/headless/coverage.py's scenario of the 5 key at the base of M-59,
    // without 0627-062A: the old rule left out an instruction of the
    // key-detect range only with the idle flag set; the profile keeps no
    // flag, so the whole range 0627-0660 is left out (D-7; qa-report).
    const std::string want =
        "done 9\n"
        "0037 0167 0168 0169 016A 016B 016C 016D 016E 02E2 02E3 02E4 02E5 02E6 02E7 02E8\n"
        "02E9 02EA 02F4 02F5 02F6 02F7 02F8 02FA 02FB 02FC 02FD 02FE 02FF 0300 0301 0302\n"
        "0303 0304 0305 0306 0307 0308 0309 030A 030B 031B 031C 031D 031E 031F 0320 0324\n"
        "0325 0326 0327 0328 0329 032A 032B 032C 032D 032E 032F 0330 0331 0332 0333 0338\n"
        "0339 033A 033B 033C 033D 033F 0340 0341 0342 0343 0344 0345 060A 060B 0617 0618\n"
        "0619 061A 061B 061C 061D 061E 061F 0620 0621 0622 0623 0624 0625 0626 0661 0662\n"
        "0663 0666 0667 0672 0673 0679 067A 0689 068A 068B 068C 068D 06A2 06A3 06A4 06A5\n"
        "06A6 06A7 06A8 06A9 06AA 06AB 06AC 06AD 06AE 06AF 06B0 06B1 06B2\n"
        "reached: 125 of 6144\n";
    CHECK_EQ(r("ti59 coverage"), want);
}

TEST(c59_9_ac2_all_includes_key_readiness) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("profile on");
    r("wait 100ms");   // at key readiness only
    const std::string none = r("ti59 coverage");
    CHECK_EQ(none, std::string("done 1\nreached: 0 of 6144\n"));
    const auto all = lines(r("ti59 coverage all"));
    CHECK(!all.empty() && all.back().rfind("reached: ", 0) == 0 && all.back() != "reached: 0 of 6144");
    CHECK(!all.empty() && all.front().find("063C") != std::string::npos);
    // JSON: the addresses as data.
    Run j(RunOptions{true});
    j("profile on");
    j("wait 100ms");
    const std::string js = j("ti59 coverage all");
    CHECK(js.rfind("{\"done\":true,\"result\":{\"addresses\":[\"", 0) == 0);
    CHECK(js.find(",\"of\":6144}") != std::string::npos);
}

// ── T3: AC-3, AC-4, EC-2, EC-6 ──

TEST(c59_9_ac3_lines_gaps_summary_floor) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    r(kLinesProgram);
    r("key A");
    // tools/agentic/linetimes.py on the same program (from before the key,
    // in samples of 10 ms): 0.11, 0.40 (+0.29), 0.69 (+0.29), 0.79 (+0.10),
    // 0.89 (+0.10); here from after the key, in slices of 200 instructions.
    CHECK_EQ(r("ti59 line-times"), std::string("done 9\n"
                                               "0.014065934 0.014065934 |          1.        |\n"
                                               "0.309450549 0.295384615 |          2.        |\n"
                                               "0.604835164 0.295384615 ||\n"
                                               "0.689230769 0.084395604 ||\n"
                                               "0.801758241 0.112527472 |          3.        |\n"
                                               "feeds: n=2 mean 0.189890109 min 0.084395604 max 0.295384615 sum 0.379780219\n"
                                               "lines: n=3 mean 0.140659340 min 0.014065934 max 0.295384615 sum 0.421978021\n"
                                               "printer floor: 1.071472527\n"
                                               "limit: no\n"));
}

TEST(c59_9_ac3_limit_ends_it) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    r(kLinesProgram);
    r("key A");
    const auto l = lines(r("ti59 line-times limit=400ms"));
    CHECK_EQ(l.size(), std::size_t{6});
    if (l.size() == 6) {
        CHECK_EQ(l[0], std::string("0.014065934 0.014065934 |          1.        |"));
        CHECK_EQ(l[2], std::string("feeds: n=0"));
        CHECK_EQ(l[5], std::string("limit: yes"));
    }
    CHECK_EQ(r("ti59 line-times limit=x"), std::string("refused: A duration needs a unit, as in 2s, 1.5s or 500ms: x.\n"));
}

TEST(c59_9_ac4_floor_4165_per_line_1370_per_feed) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    r(kLinesProgram);
    r("key A");
    const auto l = lines(r("ti59 line-times"));
    // 3 × 4165 + 2 × 1370 instructions of 32 ticks at the TI-59's rate.
    const std::uint64_t ticks = (3ULL * 4165 + 2ULL * 1370) * 32;
    const std::string seconds = toText(secondsOf(ticks, 455'000));
    CHECK(l.size() == 9 && l[7] == "printer floor: " + seconds);
    CHECK_EQ(seconds, std::string("1.071472527"));
}

TEST(c59_9_ec2_nothing_printed_after_15_s_idle) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    CHECK_EQ(r("ti59 line-times"), std::string("done 5\nno lines printed\nfeeds: n=0\nlines: n=0\nprinter floor: 0.000000000\nlimit: no\n"));
    const auto t = lines(r("ticks"));
    CHECK(t.size() == 2 && t[1].rfind("seconds: 15.0", 0) == 0);
}

TEST(c59_9_ec6_line_times_refused_without_printer) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    CHECK_EQ(r("ti59 line-times"), std::string("refused: There is no PC-100C: attach printer PC-100C first.\n"));
}

// ── T4: AC-5 ──

TEST(c59_9_ac5_time_per_key_and_total) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r(kLoopProgram);
    // tools/agentic/runtime.py on the same program and keys: A:3.19.
    CHECK_EQ(r("ti59 run-time 2 0 A"), std::string("done 4\n2 0.000000000\n0 0.000000000\nA 3.187481318\ntotal 3.187481318\n"));
    CHECK_EQ(r("lcd"), std::string("done 2\n         20. \nindicators: C\n"));   // the display after the run
    // A Pause flash does not end the time (memory run_time_measurement).
    r("ti59 program 000 76 11 01 66 02 91");   // Lbl A 1 Pause 2 R/S
    CHECK_EQ(r("ti59 run-time A"), std::string("done 2\nA 0.550329670\ntotal 0.550329670\n"));
    CHECK_EQ(r("ti59 run-time Q"), std::string("refused: Unknown key: Q.\n"));
}

TEST(c59_9_ac5_not_finished_after_4000_s_marked_done) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("ti59 program 000 76 11 61 00 02");   // Lbl A GTO 002: never ends
    CHECK_EQ(r("ti59 run-time A 5"), std::string("done 2\nA 4000.001547252 not finished\ntotal 4000.001547252\n"));
}

// ── T5: AC-6, AC-7, EC-6 ──

TEST(c59_9_ac6_shares_sources_and_top_steps) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r(kLoopProgram);
    r("key 2 0 A");
    // hotsteps.py --until 3 on the same program (every millisecond): 007
    // 43.4%, 006 33.2%, 008 21.2%, 003 1.4%, 004 0.4%, 005 0.4%.
    CHECK_EQ(r("ti59 hot-steps limit=1s"), std::string("done 12\nnotes: 147\nprinter busy 0.0%\nprinter wait 0.0%\n"
                                                       "memory 147 (fast mode 0)\nrom 0\nmodule 0\n"
                                                       "007: 40.1%\n006: 32.6%\n008: 21.0%\n003: 4.0%\n004: 1.3%\n005: 0.6%\n"));
    // A module program (ML-01 through SBR 000): the module's notes.
    Run m;
    m("key Pgm 0 1 SBR 0 0 0");
    CHECK_EQ(m("ti59 hot-steps limit=1s"), std::string("done 7\nnotes: 147\nprinter busy 0.0%\nprinter wait 0.0%\n"
                                                       "memory 59 (fast mode 0)\nrom 0\nmodule 88\n000: 100.0%\n"));
    // With a PC-100C printing: the printer's shares.
    Run p;
    p("attach printer PC-100C");
    p("ti59 program 000 76 11 99 61 11");   // Lbl A Prt GTO A
    p("key A");
    CHECK_EQ(p("ti59 hot-steps limit=2s"), std::string("done 9\nnotes: 294\nprinter busy 98.6%\nprinter wait 41.4%\n"
                                                       "memory 294 (fast mode 0)\nrom 0\nmodule 0\n"
                                                       "002: 79.5%\n003: 11.5%\n004: 8.8%\n"));
}

TEST(c59_9_ec6_printer_shares_zero_without_printer) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("ti59 program 000 76 11 99 61 11");
    r("key A");
    const auto l = lines(r("ti59 hot-steps limit=1s"));
    CHECK(l.size() >= 3 && l[1] == "printer busy 0.0%" && l[2] == "printer wait 0.0%");
}

TEST(c59_9_ac7_steps_between_two_times_folded) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r(kLoopProgram);
    r("key 2 0 A");
    CHECK_EQ(r("ti59 step-trace 100ms 120ms"), std::string("done 1\n006:3 007:11\n"));
    CHECK_EQ(r("ti59 step-trace 120ms 100ms"), std::string("refused: The second time must come after the first.\n"));
}

// ── T6: AC-8, EC-3 ──

TEST(c59_9_ac8_cost_of_rcl_in_fast_and_normal_mode) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    // tools/agentic/stepcost.py "43 22" at the base of M-59 (a fresh TI-59
    // with a PC-100C for each run): fast 580, normal 1332 instructions
    // (41 / 94 ms). Here on the session's machine (qa-report).
    CHECK_EQ(r("ti59 step-cost 43 22"), std::string("done 3\nfast 584 instructions 41 ms\nnormal 1333 instructions 94 ms\nprinter: yes\n"));
    CHECK_EQ(r("ti59 step-cost"), std::string("refused: usage: ti59 step-cost <keycode> [<keycode> ...] (e.g. ti59 step-cost 43 22)\n"));
    CHECK_EQ(r("ti59 step-cost 4"), std::string("refused: A keycode is two digits, 00 to 99: 4.\n"));
}

TEST(c59_9_ac8_leaves_the_machine_changed) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("ti59 step-cost 68");
    // The last run's program (the sequence once, without it: 91 00 00 00 00 00 00 86 25 81)
    // and the registers of 2.3.0's tool stay.
    CHECK_EQ(r("ti59 program 000 count=10"), std::string("done 10\n000 91\n001 00\n002 00\n003 00\n004 00\n005 00\n006 00\n007 86\n008 25\n009 81\n"));
    CHECK_EQ(r("ti59 registers 00 22"), std::string("done 2\n00: 1e1\n22: 9.899e1\n"));
}

TEST(c59_9_ec3_endless_sequence_fails_after_300_s) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    CHECK_EQ(r("ti59 step-cost 61 00 08"), std::string("refused: The program still runs after 300 s: a jump that loops back?\n"));
}

// ── T7: AC-9, EC-4 ──

TEST(c59_9_ac9_listing_as_list_prints_it) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("attach printer PC-100C");
    std::string codes;
    for (int k = 0; k < 100; ++k) codes += (k < 10 ? " 0" : " ") + std::to_string(k);
    r("ti59 program 000" + codes);
    r("ti59 partition 159.99");
    r("key RST List");
    r("wait 60s");
    std::vector<std::string> printed;
    for (const auto& l : lines(r("paper text"))) {   // the printer's columns as one space each
        std::istringstream in(l);
        std::string w, s;
        while (in >> w) s += (s.empty() ? "" : " ") + w;
        if (!s.empty()) printed.push_back(s);
    }
    const auto listed = lines(r("ti59 keycodes list 000 099"));
    CHECK_EQ(listed.size(), std::size_t{100});
    CHECK(printed.size() >= 100);
    for (std::size_t i = 0; i < listed.size() && i < printed.size(); ++i) CHECK_EQ(listed[i], printed[i]);
    CHECK_EQ(listed.empty() ? std::string() : listed[41], std::string("041 41 SST"));
    CHECK_EQ(listed.empty() ? std::string() : listed[43], std::string("043 43 43"));   // STO's operand
}

TEST(c59_9_ac9_listing_pasted_into_a_preset_loads_the_same_program) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    r("ti59 program 000 76 11 42 05 61 01 23 97 40 03 11 72 05 92 43 22 99 91");
    const auto listed = lines(r("ti59 keycodes list"));
    CHECK_EQ(listed.size(), std::size_t{18});
    std::string preset = "format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n";
    for (const auto& l : listed) preset += "    " + l + "\n";
    const std::string before = r("ti59 program 000 count=18");
    const std::string path = scratch("listing.ti59", preset);
    CHECK_EQ(r("preset " + path), std::string("done\n"));
    CHECK_EQ(r("ti59 program 000 count=18"), before);
}

TEST(c59_9_ec4_empty_program_no_lines) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    CHECK_EQ(r("ti59 keycodes list"), std::string("done\n"));
    CHECK_EQ(r("ti59 keycodes disassemble"), std::string("done\n"));
    CHECK_EQ(r("ti59 keycodes list 010 011"), std::string("done 2\n010 00 0\n011 00 0\n"));   // D-4: a range is listed whole
    CHECK_EQ(r("ti59 keycodes list 960"), std::string("refused: A step is three digits, 000 to 959: 960.\n"));
}

// ── T8: AC-10, AC-11, AC-14 ──

TEST(c59_9_ac14_assembly_lines_as_the_spec_example) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    // AC-14's example, then a merged code (D-5), INV SBR not merged, RTN, 2nd CLR.
    r("ti59 program 000 76 11 42 05 61 01 23 97 40 03 11 72 05 22 71 92 20 87 01 15");
    CHECK_EQ(r("ti59 keycodes disassemble"), std::string("done 10\n"
                                                         "000  Lbl A\n002  STO 05\n004  GTO 123\n007  Dsz Ind 03 A\n"
                                                         "011  STO Ind 05\n013  INV\n014  SBR\n015  RTN\n016  2nd CLR\n"
                                                         "017  IFF 01 E\n"));
    CHECK_EQ(r("ti59 keycodes disassemble 003 007"), std::string("done 2\n004  GTO 123\n007  Dsz Ind 03 A\n"));
    Run j(RunOptions{true});
    j("ti59 program 000 76 11");
    CHECK_EQ(j("ti59 keycodes disassemble"), std::string("{\"done\":true,\"result\":{\"lines\":[{\"step\":0,\"text\":\"000  Lbl A\"}]},\"reason\":null}\n"));
}

TEST(c59_9_ac10_round_trip) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    // A program as the calculator stores it (the bundled sum.ti59).
    CHECK_EQ(r("preset examples:sum.ti59"), std::string("done\n"));
    const std::string program = r("ti59 program 000 count=40");
    const auto assembly = lines(r("ti59 keycodes disassemble"));
    CHECK(assembly.size() > 5);
    const std::string path = scratch("sum.asm59", "# sum.ti59\n\n" + join(assembly));
    r("cold-boot");
    CHECK(r("ti59 program 000 count=40") != program);
    const auto written = lines(r("ti59 keycodes assemble " + path));
    CHECK(written.size() == 1 && written[0].rfind("steps: 000-", 0) == 0);
    CHECK_EQ(r("ti59 program 000 count=40"), program);
    CHECK_EQ(join(lines(r("ti59 keycodes disassemble"))), join(assembly));
}

TEST(c59_9_ac10_refused_unknown_mnemonic_missing_operand_beyond_last_step_nothing_written) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    const std::string empty = r("ti59 program 000 count=3");
    auto refused = [&](const std::string& name, const std::string& text) {
        return r("ti59 keycodes assemble " + scratch(name, text));
    };
    CHECK_EQ(refused("bad1.asm59", "000  Lbl A\n002  Frob 05\n"), std::string("refused: Line 2: Unknown mnemonic: Frob.\n"));
    CHECK_EQ(refused("bad2.asm59", "000  Lbl A\n002  STO\n"), std::string("refused: Line 2: STO needs its operand.\n"));
    CHECK_EQ(refused("bad3.asm59", "000  Lbl A\n958  GTO 123\n"), std::string("refused: Line 2: Beyond the last step, 959.\n"));
    CHECK_EQ(refused("bad4.asm59", "000  Lbl A\n001  1\n"), std::string("refused: Line 2: Step 001 is written twice.\n"));
    CHECK_EQ(refused("bad5.asm59", "000  STO 123\n"), std::string("refused: Line 1: STO: not an operand: 123.\n"));
    CHECK_EQ(refused("bad6.asm59", "000  1 2\n"), std::string("refused: Line 1: More than the instruction takes: 2.\n"));
    CHECK_EQ(r("ti59 program 000 count=3"), empty);   // nothing written
    // On the TI-58 the last step is 479.
    r("model TI-58");
    CHECK_EQ(refused("bad7.asm59", "480  1\n"), std::string("refused: Line 1: Beyond the last step, 479.\n"));
}

TEST(c59_9_ac10_program_reads_the_same_memory) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    const std::string path = scratch("prog.asm59", "010  STO Ind 05\n012  x\xE2\x87\x84t\n013  Dsz 01 E'\n");
    CHECK_EQ(r("ti59 keycodes assemble " + path), std::string("done 1\nsteps: 010-015\n"));
    CHECK_EQ(r("ti59 program 010 count=7"), std::string("done 7\n010 72\n011 05\n012 32\n013 97\n014 01\n015 10\n016 00\n"));
    CHECK_EQ(r("ti59 program 020 76 13"), std::string("done\n"));
    CHECK_EQ(join(lines(r("ti59 keycodes disassemble 020"))), std::string("020  Lbl C\n"));
}

TEST(c59_9_ac11_every_mnemonic_is_a_key_name) {
    // Each keycode's mnemonic is pressed by `key` as that keycode's keys, and
    // the assembler reads it back to the keycode (with an operand where it
    // takes one); 26, 27 and HIR (82) have no key (spec decision log).
    for (int k = 0; k < 100; ++k) {
        const std::string m = cu59::assemblyMnemonic(static_cast<std::uint8_t>(k));
        if (k == 26 || k == 27 || k == 82) {
            CHECK(m == (k == 82 ? "HIR" : std::to_string(k)));
        } else {
            std::vector<std::string> names;
            std::istringstream in(m);
            for (std::string w; in >> w;) names.push_back(w);
            CHECK(cu59::tiKeyNames(names).has_value());
        }
        const std::vector<std::uint8_t> steps{static_cast<std::uint8_t>(k), 11, 1, 23, 45};
        const auto dis = cu59::disassemble(steps, 0, 0);
        CHECK_EQ(dis.size(), std::size_t{1});
        if (dis.empty()) continue;
        const auto back = cu59::assemble(dis[0].text, 959);
        const auto* got = std::get_if<std::vector<cu59::ProgramStep>>(&back);
        CHECK(got && !got->empty() && (*got)[0].keycode == k);
    }
}

// ── T9: EC-5 ──

TEST(c59_9_ec5_ti58c_instructions_are_ticks_over_32) {
    SKIP_UNLESS(romsThere(), "the ROMs are not fetched (roms/fetch_roms.sh)");
    Run r;
    if (!r) return;
    CHECK_EQ(r("model TI-58C"), std::string("done\n"));
    r("ti59 program 000 76 11 42 00 76 12 97 00 12 91");
    r("key 2 0 A");
    // 1 s of notes every 97 instructions of 32 ticks at the TI-58C's 394 000
    // ticks per second: 127 (147 at the TI-59's 455 000).
    const auto l = lines(r("ti59 hot-steps limit=1s"));
    CHECK(!l.empty() && l[0] == "notes: 127");
    // The cost in instructions is ticks / 32 here too: 142 instructions of
    // 32 ticks are 11.5 ms at 394 000 ticks per second.
    Run f;
    f("model TI-58C");
    CHECK_EQ(f("ti59 step-cost 68"), std::string("done 3\nfast 142 instructions 12 ms\nnormal 212 instructions 17 ms\nprinter: no\n"));
}
