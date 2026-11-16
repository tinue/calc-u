// CalcU59QtTests (M-53): the Qt6 frontend's paper pane and run-control
// buttons on its EmulatorController, the session on the real ROMs, offscreen
// on the real clock. LIB-8 E8-8 (AC-28) and E8-9 (AC-31), LIB-10 E10-7 (AC-34);
// since M-55 the card buttons and the Arm Card dialog on the library's media
// topic (C59-7 T10: AC-13, AC-14; LIB-7 §11 E4: AC-33).

#include "TestHarness.hpp"
#include "EmulatorController.hpp"
#include "AppPaths.hpp"
#include "CardPickerDialog.hpp"
#include "ControlBar.hpp"
#include "PaperPane.hpp"
#include "TIMagneticCardKind.hpp"
#include <QDir>
#include <QFile>
#include <QListWidget>
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

// The controller started (LIB-18's start: the TI-59 with the Master Library
// module and no printer, row 59-10), then the PC-100C attached, at key readiness.
struct Started {
    EmulatorController controller;
    bool ok = false;
    Started() {
        if (!controller.started()) return;
        runFor(1000);   // the ROM's start-up
        if (!controller.printerConnected()) controller.togglePrinterConnection();
        if (!waitUntil([&] { return controller.printerConnected(); })) return;
        runFor(500);    // the power cycle the attach brings (row 59-11)
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

namespace {

QToolButton* buttonNamed(QWidget& root, const QString& accessible) {
    for (QToolButton* b : root.findChildren<QToolButton*>())
        if (b->accessibleName() == accessible) return b;
    return nullptr;
}

// The TI-59 (the startup model may be the last one used, [startup] last-model).
bool onTI59(EmulatorController& c) {
    if (!c.started()) return false;
    runFor(500);
    if (c.model() != cu59::Model::TI59) c.start(cu59::Model::TI59);
    return waitUntil([&] { return c.model() == cu59::Model::TI59 && c.cardReader() != nullptr; });
}

void writeFile(const QString& path, const std::string& text) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(text.data(), static_cast<qint64>(text.size()));
}

} // namespace

// C59-7 AC-13: two "Crd" buttons, Arm Card and Arm Fresh Card, while no card
// passes; Eject Card while one does (the media topic's card reader).
TEST(c59_7_ac13_buttons_and_eject_by_card_state) {
    EmulatorController controller;
    if (!controller.started()) return;
    CHECK(onTI59(controller));
    ControlBar bar(controller);
    QToolButton* arm = buttonNamed(bar, QStringLiteral("Arm Card"));
    QToolButton* fresh = buttonNamed(bar, QStringLiteral("Arm Fresh Card"));
    QToolButton* eject = buttonNamed(bar, QStringLiteral("Eject Card"));
    CHECK(arm && fresh && eject);
    if (!arm || !fresh || !eject) return;
    CHECK(arm->text().endsWith(QStringLiteral("Crd")) && fresh->text().endsWith(QStringLiteral("Crd")));
    CHECK(waitUntil([&] { return !arm->isHidden() && !fresh->isHidden() && eject->isHidden(); }));
    CHECK(controller.armFreshCard(QStringLiteral("Qt13"), true).isEmpty());
    controller.sendCommand(calcu::media::kFeedCommand,
                           calcu::sessions::Value::object({{"place", "card-reader"}, {"side", 0}}));
    CHECK(waitUntil([&] { return arm->isHidden() && fresh->isHidden() && !eject->isHidden(); }));
    eject->click();
    CHECK(waitUntil([&] { return !arm->isHidden() && eject->isHidden(); }));
    controller.start(cu59::Model::TI58);   // no card reader: no card button
    CHECK(waitUntil([&] { return arm->isHidden() && fresh->isHidden() && eject->isHidden(); }));
    controller.start(cu59::Model::TI59);   // [startup] last-model back to the TI-59 for the other cases
    CHECK(waitUntil([&] { return controller.model() == cu59::Model::TI59; }));
}

// C59-7 AC-14; LIB-7 AC-33 (E4, `lib7_ac33_qt6_cards_in_lib9_save_folder`):
// the saved cards of the LIB-9 save folder by name, an outdated one disabled
// with its hint, "No cards found" when none; a 2.3.0 card is not listed.
TEST(c59_7_ac14_dialog_lists_saved_cards_outdated_disabled_none_found) {
    const QString folder = AppPaths::saveDir();
    CHECK(QDir(folder).dirName() == QStringLiteral("Calc-U-59"));   // LIB-9 AC-13: ~/Calc-U-59 (the test HOME)
    QDir(folder).removeRecursively();
    QDir().mkpath(folder);
    {
        EmulatorController controller;
        if (!controller.started()) return;
        CHECK(onTI59(controller));
        CardPickerDialog none(CardPickerDialog::Mode::Arm, controller.savedCards(), folder);
        CHECK(none.list()->count() == 1 && none.list()->item(0)->text() == QStringLiteral("No cards found"));
        CHECK(!(none.list()->item(0)->flags() & Qt::ItemIsEnabled));
    }
    writeFile(folder + QStringLiteral("/Mine.magnetcard.yaml"),
              cu59::writeMagneticCardFile(cu59::TIMagneticCardData{}, "My Card", "", false, std::nullopt, ""));
    writeFile(folder + QStringLiteral("/Old.magnetcard.yaml"), "format: ti59-magnetic-card\nformat-version: 2\ncard-name: Old\n");
    writeFile(folder + QStringLiteral("/card.U59"), "Calc-U-59 Card 2.0\nEND-CARD\n");   // C59-7 AC-4: not listed
    EmulatorController controller;
    if (!controller.started()) return;
    CHECK(onTI59(controller));
    controller.sendCommand(calcu::media::kRefreshCommand, calcu::sessions::Value::object({}));
    runFor(100);
    CardPickerDialog dialog(CardPickerDialog::Mode::Arm, controller.savedCards(), folder);
    QStringList names;
    for (int i = 0; i < dialog.list()->count(); ++i) names << dialog.list()->item(i)->text();
    CHECK(names.contains(QStringLiteral("My Card")) && names.contains(QStringLiteral("Old")) && names.size() == 2);
    for (int i = 0; i < dialog.list()->count(); ++i) {
        QListWidgetItem* item = dialog.list()->item(i);
        if (item->text() == QStringLiteral("Old")) {
            CHECK(!(item->flags() & Qt::ItemIsEnabled));
            CHECK(item->toolTip().contains(QStringLiteral("outdated")));
        } else {
            CHECK(item->flags() & Qt::ItemIsEnabled);
            dialog.list()->setCurrentItem(item);
        }
    }
    dialog.accept();
    CHECK_EQ(dialog.selectedEntry(), QStringLiteral("saved:Mine.magnetcard.yaml"));
    controller.armCard(dialog.selectedEntry());
    CHECK(waitUntil([&] {
        const auto* p = controller.cardReader();
        return p && p->holds && p->holds->name == "My Card";
    }));
    QDir(folder).removeRecursively();
}
