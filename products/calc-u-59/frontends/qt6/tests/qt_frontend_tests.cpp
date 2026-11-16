// CalcU59QtTests (M-53): the Qt6 frontend's paper pane and run-control
// buttons on its EmulatorController, the session on the real ROMs, offscreen
// on the real clock. LIB-8 E8-8 (AC-28) and E8-9 (AC-31), LIB-10 E10-7 (AC-34).

#include "TestHarness.hpp"
#include "EmulatorController.hpp"
#include "PaperPane.hpp"
#include "debug/CpuInspectorWidget.hpp"
#include "debug/LiveDebugWidget.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/RunControlButtons.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QPlainTextEdit>
#include <QThread>
#include <QAbstractButton>
#include <QToolButton>
#include <functional>

namespace {

// Events and the controller's 60 Hz timer until `cond` holds, at most `ms`.
bool waitUntil(const std::function<bool()>& cond, int ms = 5000) {
    QElapsedTimer t;
    t.start();
    while (!cond()) {
        if (t.elapsed() > ms) return false;
        QApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(2);
    }
    return true;
}
void runFor(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        QApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(2);
    }
}

template <class W> W* named(QWidget& root, const char* name) { return root.findChild<W*>(QString::fromLatin1(name)); }

// The controller started on a TI-59 with the PC-100C, at key readiness.
struct Started {
    EmulatorController controller;
    bool ok = false;
    Started() {
        if (!controller.started()) return;
        controller.start(cu59::Model::TI59);
        runFor(1000);   // the ROM's start-up
        ok = true;
    }
};

void tap(EmulatorController& c, int row, int col) {
    c.screenKeyDown(row, col);
    runFor(150);
    c.screenKeyUp();
    runFor(150);
}

} // namespace

// LIB-8 AC-28: the paper pane as MainWindow builds it: the PC-100C owns the
// panel, PRINT and ADV are held buttons, TRACE a switch, the view control
// there (it shows the view that is on).
TEST(lib8_ac28_qt6_print_adv_held_trace_switch_view_control) {
    Started s;
    CHECK(s.ok);
    if (!s.ok) return;
    PaperPane pane(s.controller);
    pane.resize(300, 500);
    pane.show();
    runFor(100);
    calcu::qt6::PaperPanel& panel = *pane.panel();
    CHECK_EQ(panel.ownerName(), std::string("PC-100C"));
    auto* print = named<QToolButton>(pane, "paper-control-PRINT");
    auto* adv = named<QToolButton>(pane, "paper-control-ADV");
    auto* trace = named<QToolButton>(pane, "paper-control-TRACE");
    CHECK(print && adv && trace);
    if (!print || !adv || !trace) return;
    CHECK(!print->isCheckable() && !adv->isCheckable());   // held: pressed and released
    CHECK(trace->isCheckable() && !trace->isChecked());
    trace->click();   // set-control TRACE on
    CHECK(waitUntil([&] { return s.controller.printerTrace(); }));
    CHECK(named<QToolButton>(pane, "paper-control-TRACE")->isChecked());
    auto* view = named<QToolButton>(pane, "paper-view-toggle");
    CHECK(view != nullptr && view->isVisible());
    CHECK(!panel.showsText());
    // Detached: an empty paper without a name (AC-29).
    auto* connect = named<QAbstractButton>(pane, "paper-connect");
    CHECK(connect != nullptr);
    if (connect) connect->click();
    CHECK(waitUntil([&] { return !s.controller.printerConnected(); }));
    runFor(50);
    CHECK_EQ(panel.ownerName(), std::string());
}

// LIB-8 AC-31: the PC-100C prints a number, the text view shows it, Cut
// empties the paper and can no longer be used.
TEST(lib8_ac31_qt6_print_text_cut) {
    Started s;
    CHECK(s.ok);
    if (!s.ok) return;
    PaperPane pane(s.controller);
    pane.resize(300, 500);
    pane.show();
    runFor(100);
    tap(s.controller, 5, 1);   // 7
    auto* print = named<QToolButton>(pane, "paper-control-PRINT");
    CHECK(print != nullptr);
    if (!print) return;
    print->pressed();
    runFor(200);
    print->released();
    calcu::qt6::PaperPanel& panel = *pane.panel();
    CHECK(waitUntil([&] { return !panel.paperEmpty(); }));
    panel.showText(true);
    auto* text = named<QPlainTextEdit>(pane, "paper-text");
    CHECK(text != nullptr);
    if (text) CHECK(waitUntil([&] { return text->toPlainText().contains(QStringLiteral("7.")); }));
    auto* cut = named<QToolButton>(pane, "paper-cut");
    CHECK(cut != nullptr && cut->isEnabled());
    if (!cut) return;
    cut->click();
    CHECK(waitUntil([&] { return panel.paperEmpty(); }));
    CHECK(waitUntil([&] { return !cut->isEnabled(); }));
}

// LIB-10 AC-34: FREEZE, STEP and RESUME of the CALCULATOR tab (program) and
// the CPU tab (cpu) are the run-control buttons and follow the run state.
TEST(lib10_ac34_qt6_calculator_and_cpu_buttons_follow_run_state) {
    Started s;
    CHECK(s.ok);
    if (!s.ok) return;
    LiveDebugWidget calculator(s.controller);
    CpuInspectorWidget cpu(s.controller);
    runFor(50);
    auto* calc = calculator.findChild<calcu::qt6::RunControlButtons*>();
    auto* proc = cpu.findChild<calcu::qt6::RunControlButtons*>();
    CHECK(calc && proc);
    if (!calc || !proc) return;
    CHECK_EQ(calc->pauseButton()->text(), QStringLiteral("FREEZE"));
    CHECK_EQ(calc->stepButton()->text(), QStringLiteral("STEP"));
    CHECK_EQ(calc->resumeButton()->text(), QStringLiteral("RESUME"));
    CHECK(calc->pauseButton()->isEnabled() && !calc->stepButton()->isEnabled() && !calc->resumeButton()->isEnabled());
    CHECK(proc->pauseButton()->isEnabled() && !proc->stepButton()->isEnabled());
    calc->pauseButton()->click();   // pause program
    CHECK(waitUntil([&] { return s.controller.isFrozen(); }));
    runFor(50);
    CHECK(calc->stepButton()->isEnabled() && calc->resumeButton()->isEnabled() && !calc->pauseButton()->isEnabled());
    CHECK(!proc->stepButton()->isEnabled() && !proc->resumeButton()->isEnabled());   // the CALCULATOR tab owns it
    calc->resumeButton()->click();
    CHECK(waitUntil([&] { return !s.controller.isFrozen(); }));
    proc->pauseButton()->click();   // pause cpu
    CHECK(waitUntil([&] { return s.controller.isFrozen(); }));
    runFor(50);
    CHECK(proc->stepButton()->isEnabled() && proc->resumeButton()->isEnabled());
    CHECK(!calc->stepButton()->isEnabled());
    proc->resumeButton()->click();
    CHECK(waitUntil([&] { return !s.controller.isFrozen(); }));
}
