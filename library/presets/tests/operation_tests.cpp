// LIB-6 L5: a load is one operation (AC-12, AC-13, AC-16, AC-61, EC-3,
// EC-4, EC-6).
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
using calcu::pacing::Speed;
using calcu::sessions::Hosting;

namespace {
RigOptions paced(Ticks program = 50'000) {
    RigOptions o;
    o.hosting = Hosting::Paced;
    o.program = program;
    return o;
}
constexpr Ticks kForever = 1'000'000'000'000ULL;
} // namespace

TEST(ac12_busy_live_input_dropped_second_load_refused) {
    Rig r(paced());
    const CallId c = r.load(header() + "keys:\n  - regular-speed:\n  - key: 1\n  - key: 2\n");
    CHECK(r.busy()->operationRuns);
    for (int i = 0; i < 5; ++i) r.call();
    CHECK(r.session->busy());
    // Live input is dropped (LIB-5 AC-18) ...
    r.send("screen-key-down", Value::object({{"key", "3"}, {"pointer", 1}}));
    r.send("screen-key-up", Value::object({{"key", "3"}, {"pointer", 1}}));
    // ... and another load refused (LIB-3 AC-16).
    const CallId second = r.load(header() + "keys:\n  - key: 3\n", "second.test-seg");
    CHECK(!r.done(second));
    CHECK_EQ(r.reason(second), std::string("Another operation is running."));
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK(r.downs("3").empty());
    CHECK_EQ(r.downs("2").size(), std::size_t(1));
    CHECK(!r.busy()->operationRuns);
}

TEST(ac12_reset_cold_boot_switch_end_it_without_error) {
    for (const char* command : {"reset", "cold-boot", "model"}) {
        Rig r(paced(kForever));
        const CallId c = r.load(header() + "keys:\n  - key: 1\n  - let-it-finish:\n");
        for (int i = 0; i < 10; ++i) r.call();
        CHECK(r.session->busy());   // it hangs: the program never ends
        r.notices();
        const Value args = std::string(command) == "model" ? Value::object({{"name", "test-dot"}}) : Value();
        const CallId stop = r.send(command, args);
        CHECK(r.replied(c) && !r.done(c));
        CHECK_EQ(r.reason(c), std::string("Stopped."));
        r.callWhileBusy();
        CHECK(r.done(stop));
        CHECK(r.notices().empty());   // without an error
    }
}

TEST(ac13_in_parts_paced_equals_one_go_harness) {
    const std::string text = header() +
                             "test-memory: { at: 0, bytes: 0A }\n"
                             "keys:\n"
                             "  - regular-speed:\n"
                             "  - key: 1\n"
                             "  - wait: 20ms\n"
                             "  - full-speed:\n"
                             "  - key: 2 3\n"
                             "  - elapsed-time: report\n"
                             "test-memory: { at: 1, bytes: 0B }\n"
                             "keys:\n"
                             "  - key: 2\n";
    struct Result {
        bool done = false;
        std::string reason;
        std::vector<std::string> log;
        std::vector<std::uint8_t> memory;
        std::vector<std::uint8_t> display;
        Ticks ticks = 0;
    };
    auto run = [&](Hosting h) {
        RigOptions o;
        o.hosting = h;
        o.program = 20'000;
        Rig r(o);
        r.logLines();
        const CallId c = r.load(text);
        r.callWhileBusy();
        Result out;
        out.done = r.done(c);
        out.reason = r.reason(c);
        for (std::string l : r.logLines()) {   // the rig's own folder aside
            const std::size_t at = l.find(r.folder.str());
            if (at != std::string::npos) l.replace(at, r.folder.str().size(), "<folder>");
            out.log.push_back(l);
        }
        out.memory = r.machine()->memory()->bytes;
        out.display = displayBytes(r.machine()->readDisplay());
        out.ticks = r.ticks();
        return out;
    };
    const Result parts = run(Hosting::Paced);
    const Result oneGo = run(Hosting::VirtualTime);
    CHECK(parts.done && oneGo.done);
    CHECK_EQ(parts.reason, oneGo.reason);
    CHECK(parts.log == oneGo.log);
    CHECK(parts.memory == oneGo.memory);
    CHECK(parts.display == oneGo.display);
    CHECK_EQ(parts.ticks, oneGo.ticks);
    CHECK(hasLine(oneGo.log, "Elapsed: "));
}

TEST(ac16_full_speed_while_built_and_carried_out) {
    Rig r(paced(200'000));   // a program of 2 s
    const CallId c = r.load(header() + "keys:\n  - key: 1\n  - key: 2\n");
    int calls = 0;
    bool allFull = true;
    while (r.session->busy() && calls < 10'000) {
        allFull = allFull && r.busy()->speed == Speed::Full;
        r.call();
        ++calls;
    }
    CHECK(r.done(c));
    CHECK(allFull);
    CHECK(calls < 60);   // 2 s of emulated time in less than 1 s of host calls
}

TEST(ac61_speed_command_changes_nothing_while_it_runs) {
    Rig r(paced(50'000));   // 0.5 s at regular speed
    const CallId c = r.load(header() + "keys:\n  - regular-speed:\n  - key: 1\n  - key: 2\n");
    r.call();
    r.send("speed", Value::object({{"full", true}}));   // the frontend's gesture (LIB-17)
    int calls = 0;
    bool allRegular = true;
    while (r.session->busy() && calls < 10'000) {
        allRegular = allRegular && r.busy()->speed == Speed::Regular;
        r.call();
        ++calls;
    }
    CHECK(r.done(c));
    CHECK(allRegular);
    CHECK(calls >= 25);   // paced: half a second of 16 ms calls
}

TEST(ec3_stopped_reply_machine_as_the_command_leaves_it) {
    Rig r(paced(kForever));
    const CallId c = r.load(header() + "keys:\n  - key: 1\n  - key: 2\n");
    for (int i = 0; i < 5; ++i) r.call();
    mt::TestMachine* m = r.machine();
    r.send("reset");
    CHECK_EQ(r.reason(c), std::string("Stopped."));
    r.callWhileBusy();
    CHECK(r.machine() == m);            // Reset keeps the machine
    CHECK(!r.machine()->programRuns());  // as Reset leaves it
    CHECK(r.downs("2").empty());         // the remaining steps are not carried out
    for (int i = 0; i < 5; ++i) r.call();
    CHECK(r.downs("2").empty());
}

TEST(ec4_let_it_finish_without_limit_ended_by_reset) {
    Rig r(paced(kForever));
    const CallId c = r.load(header() + "timeout: 1\nkeys:\n  - key: 1\n  - let-it-finish:\n");
    for (int i = 0; i < 300; ++i) r.call();   // far beyond the timeout in emulated time
    CHECK(r.session->busy());
    CHECK(r.ticks() > 300'000);
    r.send("cold-boot");
    CHECK_EQ(r.reason(c), std::string("Stopped."));
}

TEST(ec6_suspended_makes_no_progress) {
    Rig r(paced(100'000));
    const CallId c = r.load(header() + "keys:\n  - regular-speed:\n  - key: 1\n  - key: 2\n");
    for (int i = 0; i < 3; ++i) r.call();
    r.send("suspend");
    const Ticks t = r.ticks();
    for (int i = 0; i < 100; ++i) r.call();
    CHECK_EQ(r.ticks(), t);
    CHECK(r.session->busy());
    r.send("continue");
    r.callWhileBusy();
    CHECK(r.done(c));
    CHECK_EQ(r.downs("2").size(), std::size_t(1));
}
