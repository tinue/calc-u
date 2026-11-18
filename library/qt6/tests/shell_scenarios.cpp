// The shell's GUI scenarios on the Qt6 test window, the same as the React
// set's (library/web/tests/shell.spec.js): the menus, a shortcut, the
// platform's places, the bar, the settings dialog, loading, a notice, help
// opens, a key under the pointer. They find widgets by the object names of
// shell/ObjectNames.hpp only, as the products' shot scenarios do. The
// pictures per layout are the products'.
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

TEST(menus_and_their_items) {
    TestWindow w;
    for (const char* m : {"menu-file", "menu-machine", "menu-help"}) CHECK(w.find<QMenu>(m) != nullptr);
    for (const char* i : {"load-preset", "settings", "quit", "reset", "cold-boot", "resume", "help", "about"})
        CHECK(w.action(i) != nullptr);
}

TEST(reset_shortcut_ctrl_r) {
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

TEST(platform_menu_roles) {
    TestWindow w;
    CHECK(w.action("settings")->menuRole() == QAction::PreferencesRole);
    CHECK(w.action("about")->menuRole() == QAction::AboutRole);
    CHECK(w.action("quit")->menuRole() == QAction::QuitRole);
}

TEST(control_bar_controls) {
    TestWindow w;
    CHECK(w.find<QWidget>(names::kControlBar)->isVisible());
    CHECK(w.find<QComboBox>(names::kBarModel) != nullptr);
    CHECK(w.find<QWidget>(names::barSlot("slot-1")) != nullptr);
    CHECK(w.find<QToolButton>(names::barPlace("printer")) != nullptr);
    CHECK(w.find<QComboBox>(names::barPlace("aux")) != nullptr);
}

TEST(settings_dialog_sets_the_model) {
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

TEST(loading_after_500_ms_and_reset_hides_it) {
    TestWindow w;
    w.command("tsp.op");
    w.runFor(200);
    CHECK(!w.find<QWidget>(names::kLoading)->isVisible());
    w.runFor(400);
    CHECK(w.find<QWidget>(names::kLoading)->isVisible());
    w.action("reset")->trigger();   // Reset can be used
    w.step(2);
    CHECK(!w.find<QWidget>(names::kLoading)->isVisible());
}

TEST(notice_shown_and_ok_closes_it) {
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

TEST(help_opens_the_product_address) {
    TestWindow w;
    w.action("help")->trigger();
    w.settle();
    CHECK(w.opened.size() == 1 && w.opened[0] == QUrl(QStringLiteral("https://calc-u.org/test/")));
}

TEST(key_under_the_pointer) {
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
