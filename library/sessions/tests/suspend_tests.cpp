// LIB-3 L15: suspend and continue (AC-24, AC-25, EC-4).
#include "TestHarness.hpp"
#include "TestParts.hpp"

using namespace calcu::sessions;
using namespace calcu::sessions::test;

TEST(ac24_suspended_no_run_continue_runs) {
    Rig r;
    r.build();
    r.call();
    CHECK(r.fe.reply(r.send("suspend"))->done);
    CHECK_EQ(r.part.suspendings, 1);   // e.g. the TI-58C's constant memory is saved
    const Ticks t = r.ticks();
    const int before = r.part.beforeCalls;
    for (int i = 0; i < 10; ++i) r.call();   // the host goes on calling
    CHECK_EQ(r.ticks(), t);
    CHECK_EQ(r.part.beforeCalls, before);    // nothing at all
    CHECK(r.fe.reply(r.send("continue"))->done);
    r.call();
    CHECK(r.ticks() > t);
}

TEST(ac24_session_never_suspends_itself) {
    Rig r;
    r.build();
    Ticks last = 0;
    for (int i = 0; i < 20; ++i) {
        r.call(i % 5 == 0 ? std::chrono::seconds(10) : std::chrono::milliseconds(16));   // long gaps (a hidden tab)
        CHECK(r.ticks() > last);
        last = r.ticks();
    }
    CHECK(!r.session->machineSession().suspended());
    CHECK_EQ(r.part.suspendings, 0);
}

TEST(ac25_pause_survives_suspend_and_back) {
    Rig r;
    r.build();
    r.call();
    MachineSession& ms = r.session->machineSession();
    ms.setPaused(true);   // the debugger (LIB-10)
    const Ticks t = r.ticks();
    r.send("suspend");
    r.call();
    r.send("continue");
    r.call();
    CHECK(ms.paused());
    CHECK_EQ(r.ticks(), t);
    ms.setPaused(false);
    r.call();
    CHECK(r.ticks() > t);
}

TEST(ac25_resume_while_suspended_does_not_run) {
    Rig r;
    r.build();
    r.call();
    MachineSession& ms = r.session->machineSession();
    ms.setPaused(true);
    r.send("suspend");
    ms.setPaused(false);   // the debugger resumes
    const Ticks t = r.ticks();
    r.call();
    CHECK_EQ(r.ticks(), t);
    CHECK(ms.suspended());
    r.send("continue");
    r.call();
    CHECK(r.ticks() > t);
}

TEST(ec4_operation_no_progress_while_suspended_busy_stays) {
    Rig r;
    r.build();
    r.part.operation = runningFor(300'000);
    const CallId op = r.send("tp.op");
    r.call();
    const Ticks t = r.ticks();
    r.send("suspend");
    for (int i = 0; i < 10; ++i) r.call();
    CHECK_EQ(r.ticks(), t);
    CHECK(r.session->busy());
    CHECK(r.fe.busy()->operationRuns);   // busy stays
    CHECK_EQ(r.fe.replies(op), 0);
    r.send("continue");
    r.callWhileBusy();
    CHECK(r.fe.reply(op) && r.fe.reply(op)->done);
    CHECK(r.ticks() >= 300'000);
}
