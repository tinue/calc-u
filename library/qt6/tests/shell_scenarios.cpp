// LIB-14 L10 (design §6; spec AC-25, ADR 0032): the GUI scenarios of AC-25
// on the Qt6 test window, under the same IDs as the React set's
// (library/web/tests/shell.spec.js): ac1 the menus, ac4 a shortcut, ac6 the
// platform's places, ac8 the bar, ac11 the settings dialog, ac15 loading,
// ac17 a notice, ac20 help opens, ac24 a key under the pointer. They find
// widgets by the object names of §3.10 only, as M-49's shot scenarios do.
// The pictures per layout (AC-33, AC-40) are the products'.
#include "TestHarness.hpp"
#include "TestWindow.hpp"
#include "shell/ObjectNames.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QToolButton>
#ifdef CALCU_QT_TEST
#include <QTest>
#endif

using namespace calcu;
using calcu::qt6::test::TestWindow;
using sessions::Value;
namespace names = calcu::shell::names;

TEST(ac1_the_menus) {
    TestWindow w;
    for (const char* m : {"menu-file", "menu-machine", "menu-help"}) CHECK(w.find<QMenu>(m) != nullptr);
    for (const char* i : {"load-preset", "settings", "quit", "reset", "cold-boot", "resume", "help", "about"})
        CHECK(w.action(i) != nullptr);
}

TEST(ac4_a_shortcut) {
    TestWindow w;
    CHECK(w.action("reset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_R));
#ifdef CALCU_QT_TEST
    w.window.activateWindow();
    SKIP_UNLESS(QTest::qWaitForWindowActive(&w.window, 1000), "the offscreen window does not become active");
    QTest::keyClick(w.faceplate, Qt::Key_R, Qt::ControlModifier);
    w.settle();
    CHECK_EQ(w.part.count("reset"), 1);
#endif
}

TEST(ac6_the_platforms_places) {
    TestWindow w;
    CHECK(w.action("settings")->menuRole() == QAction::PreferencesRole);
    CHECK(w.action("about")->menuRole() == QAction::AboutRole);
    CHECK(w.action("quit")->menuRole() == QAction::QuitRole);
}

TEST(ac8_the_bar) {
    TestWindow w;
    CHECK(w.find<QWidget>(names::kControlBar)->isVisible());
    CHECK(w.find<QComboBox>(names::kBarModel) != nullptr);
    CHECK(w.find<QWidget>(names::barSlot("slot-1")) != nullptr);
    CHECK(w.find<QToolButton>(names::barPlace("printer")) != nullptr);
    CHECK(w.find<QComboBox>(names::barPlace("aux")) != nullptr);
}

TEST(ac11_the_settings_dialog) {
    TestWindow w;
    w.action("settings")->trigger();
    w.settle();
    auto* d = w.find<QDialog>(names::kSettingsDialog);
    CHECK(d && d->isVisible() && d->windowTitle() == QStringLiteral("Settings"));
    auto* model = d->findChild<QComboBox*>(QString::fromStdString(names::setting("startup", "model")));
    model->activated(model->findData(QStringLiteral("Model-A")));
    w.step();
    CHECK_EQ(w.file.text("startup", "model"), std::string("Model-A"));
}

TEST(ac15_loading) {
    TestWindow w;
    w.command("tsp.op");
    w.runFor(200);
    CHECK(!w.find<QWidget>(names::kLoading)->isVisible());
    w.runFor(400);
    CHECK(w.find<QWidget>(names::kLoading)->isVisible());
    w.action("reset")->trigger();   // AC-16: Reset can be used
    w.step(2);
    CHECK(!w.find<QWidget>(names::kLoading)->isVisible());
}

TEST(ac17_a_notice) {
    TestWindow w;
    w.command("tsp.notice", Value::object({{"text", "Something failed."}}));
    w.step();
    QMessageBox* box = nullptr;
    for (auto* b : w.window.findChildren<QMessageBox*>(QString::fromLatin1(names::kNotice)))
        if (b->isVisible()) box = b;
    CHECK(box && box->text() == QStringLiteral("Something failed."));
    if (box) box->findChild<QAbstractButton*>(names::kNoticeOk)->click();
    w.settle();
}

TEST(ac20_help_opens) {
    TestWindow w;
    w.action("help")->trigger();
    w.settle();
    CHECK(w.opened.size() == 1 && w.opened[0] == QUrl(QStringLiteral("https://calc-u.org/test/")));
}

TEST(ac24_a_key_under_the_pointer) {
    TestWindow w;
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), w.faceplate->mapToGlobal(QPointF(100, 100)),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w.faceplate, &press);
    QMouseEvent up(QEvent::MouseButtonRelease, QPointF(100, 100), w.faceplate->mapToGlobal(QPointF(100, 100)),
                   Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w.faceplate, &up);
    w.settle();
    CHECK(w.part.last("screen-key-down") && shell::read::text(w.part.last("screen-key-down")->args, "key") == "2");
    CHECK(w.part.last("screen-key-up") != nullptr);
}
