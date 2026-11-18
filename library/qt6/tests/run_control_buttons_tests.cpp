// The run-control buttons of the Qt6 component set on the offscreen platform:
// the rule's table, and a real app session with the debugger's DebuggerPart
// on test-seg with the test processor (TestDebugParts.hpp) through a
// listener, as a frontend hosts them.
#include "ButtonCases.hpp"
#include "TestDebugParts.hpp"
#include "TestHarness.hpp"
#include "qt6/RunControlButtons.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/Listener.hpp"
#include "sessions/MachineSession.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QToolButton>

namespace {

using namespace calcu;
using namespace calcu::debugger;
using namespace calcu::debugger::test;
using calcu::qt6::RunControlButtons;

void mouse(QToolButton* b, QEvent::Type type) {
    const QPointF at(b->width() / 2.0, b->height() / 2.0);
    QMouseEvent e(type, at, b->mapToGlobal(at), Qt::LeftButton,
                  type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(b, &e);
}

// One window: the session in virtual time with the debugger, test-seg with
// the test processor.
struct ButtonRig {
    TestLevelRules rules;
    DebuggerPart part{setup()};
    std::unique_ptr<sessions::AppSession> session;
    DebuggerSetup setup() {
        DebuggerSetup d;
        d.rules = &rules;
        return d;
    }
    ButtonRig() {
        sessions::AppSessionSetup s;
        s.clock = [] { return pacing::WallTime{0}; };
        s.parts = {&part};
        s.hosting = sessions::Hosting::VirtualTime;
        session = std::make_unique<sessions::AppSession>(std::move(s));
        auto m = machine::test::buildTestMachine(machine::test::kSeg);
        m->enableCpu(defaultProgram());
        session->machineSession().setMachine(std::move(m), {}, 0);
        session->advanceUnpaced(100);   // tells the parts of the machine
    }
    bool paused() { return session->machineSession().paused(); }
    machine::DebugTarget& target() { return session->context().machine()->debugging()->target(0); }
};

void wait(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}

} // namespace

TEST(qt6_run_control_buttons_follow_run_state_and_send_commands) {
    // The rule's table (the same cases as the C++ rule and the React component).
    for (const auto& c : readButtonCases().cases) {
        RunControlButtons w(c.level);
        w.setRunState(c.state ? &*c.state : nullptr);
        CHECK(w.use() == c.use);
        CHECK_EQ(w.pauseButton()->isEnabled(), c.use.pause);
        CHECK_EQ(w.stepButton()->isEnabled(), c.use.step);
        CHECK_EQ(w.resumeButton()->isEnabled(), c.use.resume);
    }

    // On a session: the buttons of `cpu` and of `program`, fed from the topic.
    ButtonRig r;
    sessions::Listener listener;
    RunControlButtons cpu(kCpu, RunControlButtons::Labels{QStringLiteral("FREEZE"), QStringLiteral("STEP"), QStringLiteral("RESUME")});
    RunControlButtons program(kProgram);
    int steps = 0;
    auto sender = [&](const sessions::CommandName& n, sessions::Value a) {
        if (n == kStepCommand) ++steps;
        listener.send(n, std::move(a));
        r.session->advanceUnpaced(100);   // the host's next call: a step lands there
    };
    cpu.setSender(sender);
    program.setSender(sender);
    listener.onMessage = [&](const sessions::Message& m) {
        cpu.deliver(m);
        program.deliver(m);
    };
    listener.listen(*r.session);
    CHECK(cpu.use().pause && !cpu.use().step && !cpu.use().resume);
    CHECK(cpu.pauseButton()->text() == QStringLiteral("FREEZE"));   // the product's labels
    cpu.resize(300, 40);
    cpu.show();
    cpu.pauseButton()->click();
    CHECK(r.paused());
    CHECK(listener.state<RunState>(kRunStateTopic)->owner == kCpu);
    CHECK(cpu.use().step && cpu.use().resume && !cpu.use().pause);
    CHECK(!program.use().step && !program.use().resume);   // the pause is cpu's

    // Step: once on press, then four times a second while held.
    const std::uint64_t retired = r.target().retired();
    mouse(cpu.stepButton(), QEvent::MouseButtonPress);
    CHECK_EQ(steps, 1);
    wait(620);   // the repeats at 250 and 500 ms
    mouse(cpu.stepButton(), QEvent::MouseButtonRelease);
    CHECK_EQ(steps, 3);
    CHECK_EQ(r.target().retired(), retired + 3);
    wait(300);
    CHECK_EQ(steps, 3);   // released: no more

    cpu.resumeButton()->click();
    CHECK(!r.paused());
    CHECK(program.use().pause);
    program.pauseButton()->click();
    r.session->advanceUnpaced(10'000);   // `program` lands at the next program step boundary
    CHECK(r.paused() && listener.state<RunState>(kRunStateTopic)->owner == kProgram);
    CHECK(program.use().step && !cpu.use().step);
}
