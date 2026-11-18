// Breakpoints and watches, on the test processor.
#include "DebugRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::debugger;
using namespace calcu::debugger::test;
namespace cpu = calcu::machine::test::cpu;

namespace {

Value lv(const char* level) { return Value::object({{"level", level}}); }
Value watchArgs(std::int64_t address, std::int64_t count, const char* access, const char* setter = "harness") {
    return Value::object({{"setter", setter}, {"target", "test-cpu"}, {"space", "memory"}, {"address", address},
                          {"count", count}, {"access", access}});
}
BreakpointId idOf(const Value& v) { return static_cast<BreakpointId>(*v.find("id")->integer()); }

} // namespace

// ── Breakpoints ──────────────────────────────────────────────────────────

TEST(pauses_before_instruction) {
    DebugRig r;
    r.breakAt(0x12);
    r.advance(10'000);
    CHECK(r.paused());
    CHECK_EQ(r.pc(), std::uint64_t(0x12));
    const auto h = r.target().history(1);
    CHECK_EQ(h[0].address, std::uint64_t(0x10));   // out at 0x12 not carried out
    CHECK(r.runState()->reason == RunState::Reason::Breakpoint && r.runState()->owner == kCpu);
}

TEST(qualifier_narrows) {
    DebugRig r;
    const CallId a = r.send("break", Value::object({{"setter", "harness"}, {"target", "test-cpu"}, {"address", 0x12},
                                                    {"qualifier", Value::object({{"bank", "3"}})}}));
    CHECK(r.done(a));
    CHECK(r.advance(10'000) >= 10'000);   // bank 0 is switched in: no hit
    CHECK(!r.paused());
    const CallId b = r.send("break", Value::object({{"setter", "harness"}, {"target", "test-cpu"}, {"address", 0x12},
                                                    {"qualifier", Value::object({{"bank", "0"}})}}));
    r.advance(10'000);
    CHECK(r.paused());
    const RunState* s = r.runState();
    CHECK(s->hits.size() == 1 && s->hits[0].id == idOf(r.fe.reply(b)->result->toValue()));
    const CallId bad = r.send("break", Value::object({{"setter", "harness"}, {"target", "test-cpu"}, {"address", 0x12},
                                                      {"qualifier", Value::object({{"bank", "9"}})}}));
    CHECK(!r.done(bad));
}

TEST(resume_and_step_pass_it_once) {
    DebugRig r;
    r.breakAt(0x12);
    r.advance(10'000);
    const std::uint64_t retired = r.target().retired();
    r.send("resume");
    r.advance(10'000);
    CHECK(r.paused());
    CHECK_EQ(r.pc(), std::uint64_t(0x12));
    CHECK_EQ(r.target().retired(), retired + 10);   // once round the loop, the instruction at 0x12 carried out
    const CallId c = r.send("step", Value::object({{"level", "cpu"}, {"count", 1}}));
    r.advance(100);
    CHECK(r.done(c));
    CHECK_EQ(r.pc(), std::uint64_t(0x14));
}

TEST(two_targets_only_the_one_to_run_passes) {
    DebugOptions o;
    o.model = "test-dot";
    DebugRig r(o);
    r.send("pause", lv("cpu"));
    const auto* d = r.machine()->debugging();
    const std::size_t current = d->current(), other = 1 - current;
    // Both stand on a breakpoint.
    r.breakAt(r.pc(0), "main");
    r.breakAt(r.pc(1), "inner");
    const std::uint64_t currentRetired = r.target(current).retired(), otherRetired = r.target(other).retired();
    r.send("resume");
    r.advance(1'000);
    CHECK(r.paused());
    CHECK_EQ(r.target(current).retired(), currentRetired + 1);   // it passed its breakpoint
    CHECK_EQ(r.target(other).retired(), otherRetired);           // the other did not
    CHECK(r.runState()->target == d->target(other).info().name);
}

TEST(test_says_go_on) {
    DebugRig r;
    calcu::machine::Debugging& d = *r.machine()->debugging();
    int asked = 0;
    auto added = r.debugger.runControl().table().addBreakpoint("dap", d, calcu::machine::DebugTargetId{"test-cpu"}, 0x12, {},
                                                               [&asked](const calcu::machine::DebugTarget&) { return ++asked >= 3; });
    CHECK(std::holds_alternative<BreakpointId>(added));
    r.debugger.runControl().table().apply(d);
    const std::uint64_t retired = r.target().retired();
    r.advance(100'000);
    CHECK(r.paused());
    CHECK_EQ(asked, 3);   // asked at each hit; went on twice
    CHECK(r.target().retired() >= retired + 18);
}

TEST(ids_owners_remove_only_own) {
    DebugRig r;
    const BreakpointId h = idOf(r.breakAt(0x12, "test-cpu", "harness"));
    const BreakpointId p = idOf(r.breakAt(0x10, "test-cpu", "dap"));
    CHECK(h != p);
    const BreakpointId w = idOf(r.fe.reply(r.send("watch", watchArgs(0x30, 1, "write", "dap")))->result->toValue());
    CHECK(w != h && w != p);
    r.send("break-off", Value::object({{"setter", "harness"}}));
    auto& table = r.debugger.runControl().table();
    CHECK_EQ(table.breakpoints().size(), std::size_t(1));
    const auto kept = table.breakpoints();
    CHECK_EQ(kept[0].setter, std::string("dap"));
    CHECK_EQ(table.watches().size(), std::size_t(1));
    r.send("watch-off", Value::object({{"setter", "harness"}}));   // not the harness's: kept
    CHECK_EQ(table.watches().size(), std::size_t(1));
    r.send("break-off", Value::object({{"setter", "dap"}, {"id", static_cast<std::int64_t>(p)}}));
    CHECK(table.breakpoints().empty());
    CHECK_EQ(table.watches().size(), std::size_t(1));   // only that ID
    const BreakpointId again = idOf(r.breakAt(0x12));
    CHECK(again > w);   // never reused in a session
    const auto list = r.ask("breakpoints", Value::object({{"setter", "harness"}}));
    CHECK(list.done && list.result->toValue().find("entries")->elements()->size() == 1);
}

TEST(hit_names_ids_and_owners) {
    DebugRig r;
    const BreakpointId h = idOf(r.breakAt(0x12, "test-cpu", "harness"));
    const BreakpointId p = idOf(r.breakAt(0x12, "test-cpu", "dap"));
    r.advance(10'000);
    const RunState* s = r.runState();
    CHECK(s->paused && s->hits.size() == 2);
    CHECK(s->hits[0].id == h && s->hits[0].setter == "harness");
    CHECK(s->hits[1].id == p && s->hits[1].setter == "dap");
}

// ── Watches ──────────────────────────────────────────────────────────────

TEST(read_write_both_pause_after_instruction) {
    {
        DebugRig r;
        CHECK(r.done(r.send("watch", watchArgs(0x30, 1, "write"))));
        r.advance(10'000);
        CHECK(r.paused() && r.runState()->reason == RunState::Reason::Watch);
        CHECK_EQ(r.pc(), std::uint64_t(0x05));   // after st a, [0x30]
    }
    {
        DebugRig r;
        r.send("watch", watchArgs(0x30, 1, "read"));
        r.advance(10'000);
        CHECK_EQ(r.pc(), std::uint64_t(0x12));   // after ld a, [0x30] in the routine; the write passed
    }
    {
        DebugRig r;
        CHECK(r.runTo(0x05));
        r.send("watch", watchArgs(0x30, 1, "both"));
        r.advance(10'000);
        CHECK_EQ(r.pc(), std::uint64_t(0x12));   // the read comes first from 0x05 on
        r.send("resume");
        r.advance(10'000);
        CHECK_EQ(r.pc(), std::uint64_t(0x05));   // then the write
    }
}

TEST(fetch_is_no_access) {
    DebugRig r;
    r.send("watch", watchArgs(0x00, 0x20, "read"));   // the whole program's code
    CHECK(r.advance(10'000) >= 10'000);
    CHECK(!r.paused());
}

TEST(watch_refused_without_offer) {
    DebugOptions o;
    calcu::machine::test::TestProgram p = defaultProgram();
    p.bare = true;
    o.program = p;
    DebugRig r(o);
    const CallId c = r.send("watch", watchArgs(0x30, 1, "write"));
    CHECK_EQ(r.reason(c), std::string("The target test-cpu offers no watches."));
}

TEST(past_end_of_space_refused) {
    DebugRig r;
    const CallId c = r.send("watch", watchArgs(0xff, 2, "write"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("The range runs past the end of the space memory."));
    CHECK(r.done(r.send("watch", watchArgs(0xff, 1, "write"))));
}

TEST(several_hits_pause_once_lowest_address_named) {
    // ld a, #1; st2 a, [0x40]; jmp 0
    DebugOptions o;
    o.program = calcu::machine::test::TestProgram({cpu::kLdA, 1, cpu::kSt2, 0x40, cpu::kJmp, 0x00});
    DebugRig r(o);
    const BreakpointId high = idOf(r.fe.reply(r.send("watch", watchArgs(0x41, 1, "write")))->result->toValue());
    const BreakpointId low = idOf(r.fe.reply(r.send("watch", watchArgs(0x40, 1, "write")))->result->toValue());
    (void)high;
    r.advance(10'000);
    CHECK(r.paused());
    CHECK_EQ(r.pc(), std::uint64_t(0x04));
    const RunState* s = r.runState();
    CHECK(s->hits.size() == 1 && s->hits[0].id == low);
    const std::uint64_t retired = r.target().retired();
    r.send("resume");
    r.advance(10'000);
    CHECK_EQ(r.pc(), std::uint64_t(0x04));
    CHECK_EQ(r.target().retired(), retired + 3);   // jmp, ld, st2: one pause for the two hits
    CHECK(r.runState()->hits.size() == 1);
}
