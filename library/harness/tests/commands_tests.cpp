// LIB-11 design T6, T7: the harness host and the first commands (AC-5, AC-7,
// AC-9, AC-11; EC-2, EC-5, EC-7).
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include <string>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;
using calcu::machine::KeyId;
namespace mt = calcu::machine::test;

// ── Topics (T6) ─────────────────────────────────────────────────────────

TEST(ac11_state_reads_library_and_product_topics) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK(run.line("state display").rfind("done 1\n{\"on\":true,\"frame\":{\"kind\":\"segments\"", 0) == 0);
    CHECK_EQ(run.line("state busy"), std::string("done 1\n{\"operationRuns\":false,\"speed\":\"regular\"}\n"));
    CHECK_EQ(run.line("state notices"), std::string("done\n"));   // nothing appended
    CHECK_EQ(run.line("state log"), std::string("done\n"));   // no reset marker of the listen
    CHECK_EQ(run.line("state test.count"), std::string("done 1\n{\"operations\":0}\n"));   // the product's own
    run.line("test operation 100");
    CHECK_EQ(run.line("state test.count"), std::string("done 1\n{\"operations\":1}\n"));
}

TEST(ac11_stream_gives_what_was_appended_since_last_read) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("test fail");
    run.line("test fail");
    const std::string first = run.line("state notices");
    CHECK_EQ(first, std::string("done 2\n{\"sentence\":\"Cannot read <save folder>/missing.txt.\"}\n"
                                "{\"sentence\":\"Cannot read <save folder>/missing.txt.\"}\n"));
    CHECK_EQ(run.line("state notices"), std::string("done\n"));   // nothing since
    run.line("test fail");
    CHECK_EQ(run.line("state notices").substr(0, 7), std::string("done 1\n"));
}

TEST(ac5_notices_only_through_state) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // The reply is the refusal alone: the notice the command made is not written.
    CHECK_EQ(run.line("test fail"), std::string("refused: Cannot read <save folder>/missing.txt.\n"));
    CHECK(run.line("state notices").find("Cannot read") != std::string::npos);
}

TEST(ec5_state_of_no_topic_is_refused) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("state nothing"), std::string("refused: There is no topic nothing.\n"));
    CHECK(run.line("state").rfind("refused: usage: state <topic>", 0) == 0);
}

// ── The first commands (T7) ─────────────────────────────────────────────

TEST(ac9_wait_reports_time_passed_within_one_instruction) {
    TestRun run;   // test-seg: 100 kHz, instructions of 3 and 5 ticks
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // The run starts at key readiness: one run piece of 1667 ticks (LIB-18
    // D-4; LIB-11 AC-2). 1 ms is 100 ticks; whole instructions from there: 101.
    const auto t0 = run.machine().tickCount();
    CHECK_EQ(t0, std::uint64_t{1667});
    CHECK_EQ(run.line("wait 1ms"), std::string("done 2\nticks: 101\nseconds: 0.001010000\n"));
    // 500 ms is 50 000 ticks; what passed exceeds it by less than one instruction.
    const std::string r = run.line("wait 500ms");
    const auto at = r.find("ticks: ");
    const long ticks = at == std::string::npos ? 0 : std::stol(r.substr(at + 7));
    CHECK(ticks >= 50'000 && ticks < 50'005);
    CHECK_EQ(run.machine().tickCount(), t0 + std::uint64_t(101 + ticks));
    RunOptions o;
    o.json = true;
    TestRun json(o);
    CHECK(json.server && json.line("wait 1.5s").rfind("{\"done\":true,\"result\":{\"ticks\":150000,\"seconds\":1.5", 0) == 0);
}

TEST(ec2_wait_without_unit_is_refused) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const auto t0 = run.machine().tickCount();
    CHECK_EQ(run.line("wait 2"), std::string("refused: A duration needs a unit, as in 2s, 1.5s or 500ms: 2.\n"));
    CHECK_EQ(run.machine().tickCount(), t0);   // nothing passed
}

TEST(ac9_key_down_and_up) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 1ms");
    CHECK_EQ(run.line("key-down 1"), std::string("done\n"));
    CHECK(run.machine().keysDown() == std::vector<KeyId>{KeyId{"1"}});
    CHECK_EQ(run.line("key-up 1"), std::string("done\n"));
    CHECK(run.machine().keysDown().empty());
    // Other spellings and any case, as the key step resolves them (D-9).
    CHECK_EQ(run.line("key-down Clear"), std::string("done\n"));
    CHECK_EQ(run.line("key-up CLR"), std::string("done\n"));
    const auto& ev = run.machine().keyEvents();
    CHECK(ev.size() == 4 && ev[2].key == KeyId{"clr"} && ev[2].down && ev[3].key == KeyId{"clr"} && !ev[3].down);
}

TEST(ac9_unknown_key_is_refused) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("key-down 9"), std::string("refused: Unknown key: 9.\n"));
    CHECK(run.machine().keyEvents().empty());
}

TEST(ec7_repeated_key_down_or_up_is_done_and_changes_nothing) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("key-up 2"), std::string("done\n"));   // not down
    CHECK(run.machine().keyEvents().empty());
    run.line("key-down 2");
    CHECK_EQ(run.line("key-down 2"), std::string("done\n"));   // already down
    CHECK_EQ(run.machine().keyEvents().size(), std::size_t{1});
}

TEST(ac9_ticks_gives_count_and_seconds) {
    TestRun run({}, mt::kDot);   // 1 MHz, instructions of 4 ticks
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // At key readiness: one run piece of 16 668 ticks (LIB-18 D-4).
    CHECK_EQ(run.line("ticks"), std::string("done 2\nticks: 16668\nseconds: 0.016668000\n"));
    run.line("wait 1s");
    CHECK_EQ(run.line("ticks"), std::string("done 2\nticks: 1016668\nseconds: 1.016668000\n"));
}

TEST(ac9_capabilities_with_kind_id_owner_and_places) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("capabilities"), std::string("done 5\n"
                                                   "capability test-generic cpu/test-generic component:cpu\n"
                                                   "capability machine-state cpu/machine-state component:cpu\n"
                                                   "place slot-1 empty\nplace printer empty\nplace aux empty\n"));
    RunOptions o;
    o.json = true;
    TestRun dot(o, mt::kDot);
    CHECK(dot.server != nullptr);
    if (!dot.server) return;
    const std::string r = dot.line("capabilities");
    CHECK(r.find("{\"kind\":\"real-time-clock\",\"id\":\"rtc/real-time-clock\",\"owner\":\"component:rtc\"}") !=
          std::string::npos);
    CHECK(r.find("{\"place\":\"card-1\",\"plugged\":[]}") != std::string::npos);
}

// LIB-4 L12 (LIB-4 AC-9): the control state read like any other state topic.
TEST(ac9_state_control_state_reads_the_topic) {
    TestRun run;   // test-seg; HarnessProduct::controlSetup() by default: no printers, no rules
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const std::string bare = run.line("state control-state");
    CHECK(bare.rfind("done 1\n{\"models\":[{\"id\":\"test-seg\",\"available\":true},{\"id\":\"test-dot\",\"available\":true}],"
                     "\"model\":\"test-seg\",",
                     0) == 0);
    CHECK(bare.find("\"panels\":[]") != std::string::npos);
    CHECK(bare.find("\"product\":{}") != std::string::npos);
    CHECK_EQ(run.line("attach printer test-printer"), std::string("done\n"));
    const std::string after = run.line("state control-state");
    CHECK(after.find("{\"id\":\"test-printer\",\"name\":\"Test printer\",\"plugged\":true,\"variants\":[],"
                     "\"available\":true}") != std::string::npos);
    CHECK(after.find("{\"id\":\"test-uart\",\"name\":\"Test UART\",\"plugged\":false,\"variants\":[],\"available\":true}") !=
          std::string::npos);   // a replace of the printer
    CHECK(after.find("\"reason\":\"The test printer and the test UART cannot be used together.\"") != std::string::npos);
}

TEST(ac9_quit_ends_serve) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    std::vector<std::string> in = {"ticks", "quit", "wait 1s", "ticks"}, out;
    std::size_t at = 0;
    run.server->serve([&]() -> std::optional<std::string> { return at < in.size() ? std::optional(in[at++]) : std::nullopt; },
                      [&](const std::string& s) { out.push_back(s); });
    CHECK_EQ(out.size(), std::size_t{1});   // quit has no reply; nothing after it runs
    CHECK_EQ(at, std::size_t{2});
    // In the one-shot form quit ends the run as well.
    TestRun one;
    CHECK(one.server != nullptr);
    if (!one.server) return;
    out.clear();
    CHECK_EQ(one.server->oneShot({"ticks", "quit", "wait 1s"}, [&](const std::string& s) { out.push_back(s); }), 0);
    CHECK_EQ(out.size(), std::size_t{1});
}

TEST(ac7_operation_runs_to_its_end_before_the_reply) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const auto t0 = run.machine().tickCount();   // the start: 16 668 ticks
    CHECK_EQ(run.line("test operation 2000000"), std::string("done\n"));   // 2 s of emulated time
    CHECK_EQ(run.machine().tickCount(), t0 + 2'000'000);
    // The busy topic is idle again before the reply.
    CHECK_EQ(run.line("state busy"), std::string("done 1\n{\"operationRuns\":false,\"speed\":\"regular\"}\n"));
    CHECK_EQ(run.line("ticks"), std::string("done 2\nticks: 2016668\nseconds: 2.016668000\n"));
}

// ── key, type (LIB-5 design §6, L12) ───────────────────────────────────

namespace {
std::string keyLog(const mt::TestMachine& m) {
    std::string out;
    for (const auto& e : m.keyEvents()) out += e.key.value + (e.down ? "+" : "-");
    return out;
}
} // namespace

TEST(ac15_key_names_and_refusal) {
    TestRun run;   // test-seg: 0-3, clr (other spellings CLR, clear)
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 1ms");
    CHECK_EQ(run.line("key 1 2 Clear"), std::string("done\n"));
    CHECK_EQ(keyLog(run.machine()), std::string("1+1-2+2-clr+clr-"));
    CHECK_EQ(run.line("key 1 x"), std::string("refused: Unknown key: x.\n"));   // nothing typed
    CHECK_EQ(keyLog(run.machine()), std::string("1+1-2+2-clr+clr-"));
    CHECK(run.line("key").rfind("refused: usage: key", 0) == 0);
}

TEST(ac16_type_presses_enter_refuses_unknown_char) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("type 1203"), std::string("done\n"));
    CHECK_EQ(keyLog(run.machine()), std::string("1+1-2+2-0+0-3+3-clr+clr-"));   // ENTER is clr here
    CHECK_EQ(run.line("type 19"), std::string("refused: No key types the character \"9\".\n"));
    CHECK_EQ(keyLog(run.machine()), std::string("1+1-2+2-0+0-3+3-clr+clr-"));
}

TEST(ac17_key_replies_when_last_key_up_not_idle) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 1ms");
    const auto before = run.machine().tickCount();
    CHECK_EQ(run.line("key 1 2"), std::string("done\n"));
    const auto& ev = run.machine().keyEvents();
    CHECK_EQ(ev.size(), std::size_t{4});
    if (ev.size() != 4) return;
    // 10 ms down, 5 ms up, 10 ms down: the reply at the last key-up, within
    // one instruction; no wait after it.
    CHECK(ev[3].tick >= before + 2500 && ev[3].tick < before + 2520);
    CHECK(run.machine().tickCount() - ev[3].tick < 8);
}

TEST(ac17_type_replies_when_enter_up) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("type 3"), std::string("done\n"));
    const auto& ev = run.machine().keyEvents();
    CHECK(ev.size() == 4 && ev[3].key.value == "clr" && !ev[3].down);
    CHECK(ev.size() == 4 && run.machine().tickCount() - ev[3].tick < 8);
}

TEST(harness_key_gives_up_after_an_hour) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.product.typingPaused = true;   // the press waits for the machine to run, for ever
    const auto before = run.machine().tickCount();
    CHECK_EQ(run.line("key 1"), std::string("refused: The keys were not taken within an hour.\n"));
    const auto ran = run.machine().tickCount() - before;
    CHECK(ran >= 3600ull * 100'000 && ran < 3600ull * 100'000 + 2000);
    CHECK(run.machine().keyEvents().empty());
}
