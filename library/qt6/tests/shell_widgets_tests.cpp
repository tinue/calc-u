// The Qt6 shell's widgets on the test window (QT_QPA_PLATFORM=offscreen):
// the drawing of the shell's models (menus, bar, settings, "Loading…",
// notices, the pointer), on an app session with TestShellPart and
// SettingsPart, no code of a product.
#include "TestHarness.hpp"
#include "TestWindow.hpp"
#include "qt6/HostInfo.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/PaperWindow.hpp"
#include "settings/WindowPlace.hpp"
#include "qt6/RunControlButtons.hpp"
#include "qt6/ShellControlBar.hpp"
#include "qt6/ShellSettingsDialog.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/ObjectNames.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QStandardItemModel>
#include <QToolButton>
#include <QtGlobal>
#include <cmath>
#include <cstdio>
#ifdef CALCU_QT_TEST
#include <QTest>
#endif
#include <string>
#include <vector>

using namespace calcu;
using calcu::qt6::test::TestWindow;
using sessions::Value;
namespace names = calcu::shell::names;

namespace {

QStringList titles(const QMenu* m) {
    QStringList out;
    for (QAction* a : m->actions()) out << (a->isSeparator() ? QStringLiteral("-") : a->objectName());
    return out;
}

QMessageBox* noticeBox(TestWindow& w) {
    for (QMessageBox* b : w.window.findChildren<QMessageBox*>(QString::fromLatin1(names::kNotice)))
        if (b->isVisible()) return b;
    return nullptr;
}

void click(QWidget* w, QPoint at) {
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(at), w->mapToGlobal(QPointF(at)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w, &press);
}
void release(QWidget* w, QPoint at) {
    QMouseEvent up(QEvent::MouseButtonRelease, QPointF(at), w->mapToGlobal(QPointF(at)), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w, &up);
}

std::string arg(const Value* v, const char* key) { return v ? shell::read::text(*v, key) : std::string("<none>"); }

QtMessageHandler previousHandler = nullptr;
QStringList infoLines;
void captureInfo(QtMsgType t, const QMessageLogContext& c, const QString& m) {
    if (t == QtInfoMsg) infoLines << m;
    else if (previousHandler) previousHandler(t, c, m);
}

} // namespace

TEST(menus_and_items_on_widgets) {
    TestWindow w;
    QStringList menus;
    for (QAction* a : w.window.menuBar()->actions()) menus << a->text();
    CHECK(menus == (QStringList{"File", "Machine", "Help"}));   // no Edit: the test product has no Edit item
    auto* file = w.find<QMenu>("menu-file");
    CHECK(file != nullptr);
    CHECK(titles(file) == (QStringList{"item-load-preset", "item-examples-menu", "-", "item-load-program", "-",
                                       "item-mount-drive", "-", "item-settings", "item-quit"}));
    auto* machine = w.find<QMenu>("menu-machine");
    CHECK(titles(machine) == (QStringList{"item-model-menu", "item-variants-model-menu", "item-peripherals-menu", "-",
                                          "item-reset", "item-cold-boot", "item-resume", "item-disk-thing"}));
    CHECK(w.action("load-preset")->text() == QStringLiteral("Load Preset…"));
    CHECK(w.action("cold-boot")->text() == QStringLiteral("Cold boot"));
    CHECK(w.action("help")->text() == QStringLiteral("Test-Product Help"));
    CHECK(w.find<QMenu>("item-examples-Games") != nullptr);
    // An example sends load-preset with its bundle path.
    w.action("example-examples:games/two.test")->trigger();
    w.settle();
    CHECK_EQ(arg(w.part.last("load-preset") ? &w.part.last("load-preset")->args : nullptr, "ref"),
             std::string("examples:games/two.test"));
    // Load Preset… opens the file dialog, then sends the file.
    w.fileAnswer = QString::fromStdString(w.home.str() + "/mine.test");
    w.action("load-preset")->trigger();
    w.settle();
    CHECK_EQ(w.fileAsks.size(), std::size_t{1});
    CHECK_EQ(arg(&w.part.last("load-preset")->args, "ref"), w.home.str() + "/mine.test");
}

TEST(model_and_variant_menus_on_widgets) {
    TestWindow w;
    CHECK(w.action("model-Model-A")->isChecked());
    CHECK(!w.action("model-Model-B")->isChecked());
    CHECK(w.find<QMenu>("item-variants-model")->title() == QStringLiteral("Revision"));
    w.action("variant-model-v2")->trigger();
    w.settle();
    CHECK_EQ(arg(&w.part.last("variant")->args, "name"), std::string("v2"));
    w.action("model-Model-B")->trigger();
    w.step();
    CHECK_EQ(arg(&w.part.last("model")->args, "name"), std::string("Model-B"));
    CHECK(w.action("model-Model-B")->isChecked());   // the part published Model-B's control state
}

TEST(peripherals_on_widgets) {
    TestWindow w;
    w.action("peripheral-printer-test-dotter")->trigger();
    w.settle();
    CHECK_EQ(arg(&w.part.last("attach")->args, "peripheral"), std::string("test-dotter"));
    w.command("tsp.plug", Value::object({{"place", "aux"}, {"peripheral", "test-plotter"}}));
    w.step();
    CHECK(w.action("peripheral-aux-test-plotter")->isChecked());
    w.action("peripheral-aux-test-plotter")->trigger();
    w.settle();
    CHECK_EQ(arg(&w.part.last("detach")->args, "place"), std::string("aux"));
    CHECK(w.action("peripheral-host-drive-drive") == nullptr);   // no entry for the host drive
}

TEST(shortcuts_on_widgets) {
    TestWindow w;
    CHECK(w.action("load-preset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_O));
    CHECK(w.action("reset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_R));
    CHECK(w.action("cold-boot")->shortcut() == QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    CHECK(w.action("settings")->shortcut() == QKeySequence(QKeySequence::Preferences));
    CHECK(w.action("reset")->shortcutContext() == Qt::WindowShortcut);
    CHECK(w.action("resume")->shortcut().isEmpty());
#ifdef CALCU_QT_TEST
    w.window.activateWindow();
    w.faceplate->setFocus();
    SKIP_UNLESS(QTest::qWaitForWindowActive(&w.window, 1000), "the offscreen window does not become active");
    // The shortcut is taken before the faceplate sees the key.
    QTest::keyClick(w.faceplate, Qt::Key_R, Qt::ControlModifier);
    w.settle();
    CHECK_EQ(w.part.count("reset"), 1);
    QTest::keyClick(w.faceplate, Qt::Key_R, Qt::ControlModifier | Qt::ShiftModifier);
    w.settle();
    CHECK_EQ(w.part.count("cold-boot"), 1);
#endif
}

TEST(disabled_with_reason_on_widgets) {
    TestWindow w;
    auto cs = shell::test::controlState();
    cs.models[1].available = shell::test::unavailable("Model-B needs a module.");
    cs.actions[0].available = shell::test::unavailable("Reset is not possible now.");
    cs.places[2].fitting[0].available = shell::test::unavailable("TD-20 does not fit now.");
    w.part.publish("control-state", cs.toValue());
    w.step();
    CHECK(!w.action("model-Model-B")->isEnabled());
    CHECK(w.action("model-Model-B")->toolTip() == QStringLiteral("Model-B needs a module."));
    CHECK(!w.action("reset")->isEnabled());
    CHECK(w.action("reset")->toolTip() == QStringLiteral("Reset is not possible now."));
    CHECK(w.find<QMenu>("menu-machine")->toolTipsVisible());
    auto* picker = w.find<QComboBox>(names::kBarModel);
    auto* model = qobject_cast<QStandardItemModel*>(picker->model());
    CHECK(!model->item(1)->isEnabled());
    CHECK(model->item(1)->data(Qt::ToolTipRole).toString() == QStringLiteral("Model-B needs a module."));
    auto* button = w.find<QToolButton>(names::barPlace("printer"));
    CHECK(!button->isEnabled());
    CHECK(button->toolTip() == QStringLiteral("TD-20 does not fit now."));
    // A product item follows its product entry.
    CHECK(!w.action("disk-thing")->isEnabled());
    CHECK(w.action("disk-thing")->toolTip() == QStringLiteral("No disk drive."));
}

TEST(roles_on_widgets) {
    TestWindow w;
    CHECK(w.action("settings")->menuRole() == QAction::PreferencesRole);
    CHECK(w.action("about")->menuRole() == QAction::AboutRole);
    CHECK(w.action("quit")->menuRole() == QAction::QuitRole);
    for (const char* id : {"load-preset", "load-program", "reset", "cold-boot", "resume", "help", "model-Model-A"})
        CHECK(w.action(id)->menuRole() == QAction::NoRole);
}

TEST(product_item_on_widgets) {
    TestWindow w;
    CHECK(w.action("load-program") != nullptr);
    CHECK(w.action("mount-drive") != nullptr);
    // The act of a product item runs when it is chosen.
    w.action("load-program")->trigger();
    w.settle();
    CHECK_EQ(w.productActs, 1);
    CHECK(w.part.sent.empty());   // it sends nothing itself
}

TEST(bar_on_widgets) {
    TestWindow w;
    w.shell->registerMediaControls("test-card", [](const Value& entry, QWidget* parent) -> QWidget* {
        return new QLabel(QString::fromStdString("card " + shell::read::text(entry, "id")), parent);
    });
    w.settle();
    auto* model = w.find<QComboBox>(names::kBarModel);
    CHECK(model != nullptr && model->count() == 2 && model->currentIndex() == 0);
    auto* slot1 = w.find<QWidget>(names::barSlot("slot-1"));
    CHECK(slot1 != nullptr);
    CHECK(slot1->findChild<QLabel*>() && slot1->findChild<QLabel*>()->text() == QStringLiteral("1:"));
    // The library's slot picker: the place's media picker, no Name & Save
    // unless the product asks for it.
    CHECK(slot1->findChild<qt6::MediaPicker*>(QStringLiteral("media-picker-slot-1")) != nullptr);
    CHECK(slot1->findChild<QPushButton*>(QStringLiteral("media-save-slot-1")) == nullptr);
    CHECK(w.find<QToolButton>(names::barPlace("printer"))->text() == QStringLiteral("TD-20"));
    auto* aux = w.find<QComboBox>(names::barPlace("aux"));
    CHECK(aux && aux->count() == 3 && aux->itemText(0) == QStringLiteral("No aux"));
    CHECK(w.find<QWidget>(names::barPlace("host-drive")) == nullptr);
    CHECK(w.find<QLabel>(names::barMedia("card")) != nullptr);
    // Left to right: model, slots, places, media.
    auto x = [&](QWidget* c) { return c->mapTo(w.shell->controlBar(), QPoint(0, 0)).x(); };
    CHECK(x(model) < x(slot1));
    CHECK(x(slot1) < x(w.find<QWidget>(names::barSlot("slot-2"))));
    CHECK(x(w.find<QWidget>(names::barSlot("slot-2"))) < x(w.find<QWidget>(names::barPlace("printer"))));
    CHECK(x(w.find<QWidget>(names::barPlace("printer"))) < x(aux));
    CHECK(x(aux) < x(w.find<QWidget>(names::barMedia("card"))));
    // More at the end, with every menu item: Load Preset…, Settings…, Quit.
    auto* more = w.find<QToolButton>(names::kBarMore);
    CHECK(more != nullptr && more->menu() != nullptr);
    if (more) {
        CHECK(x(w.find<QWidget>(names::barMedia("card"))) < x(more));
        QStringList ids;
        for (QAction* a : more->menu()->actions()) ids << a->objectName();
        CHECK(ids.contains(QString::fromStdString(names::moreItem("load-preset"))));
        CHECK(ids.contains(QString::fromStdString(names::moreItem("settings"))));
        CHECK(ids.contains(QString::fromStdString(names::moreItem("quit"))));
    }
    // A product control at the end of the group it names.
    auto* extra = new QToolButton();
    extra->setObjectName(QStringLiteral("tp-disk"));
    extra->setText(QStringLiteral("Disk"));
    w.shell->addBarControl(shell::BarGroup::Places, extra);
    w.settle();
    CHECK(x(w.find<QWidget>(names::barPlace("printer"))) < x(extra));
    CHECK(x(aux) < x(extra));
    // The model picker sends the switch.
    model->activated(1);
    w.settle();
    CHECK_EQ(arg(&w.part.last("model")->args, "name"), std::string("Model-B"));
}

// The product asks for Name & Save after a slot's picker: unusable while the
// slot holds no medium that can be named and saved. More never draws the
// platform's menu arrow.
TEST(slot_picker_name_and_save_and_more_look) {
    TestWindow w(true, TestWindow::defaultOptions(), [](shell::ShellProduct& p) { p.nameAndSave.slotButtons = true; });
    w.settle();
    auto* save = w.find<QPushButton>("media-save-slot-1");
    CHECK(save != nullptr && !save->isEnabled());
    auto* more = w.find<QToolButton>(names::kBarMore);
    CHECK(more && more->styleSheet().contains(QStringLiteral("menu-indicator")));
}

TEST(button_attaches_and_detaches_on_widgets) {
    TestWindow w;
    auto* b = w.find<QToolButton>(names::barPlace("printer"));
    CHECK(b->isCheckable() && !b->isChecked());
    b->click();
    w.settle();
    CHECK_EQ(w.part.count("attach"), 1);
    w.command("tsp.plug", Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    w.step();
    b = w.find<QToolButton>(names::barPlace("printer"));
    CHECK(b->isChecked());
    b->click();
    w.settle();
    CHECK_EQ(arg(&w.part.last("detach")->args, "place"), std::string("printer"));
    // A picker: a choice attaches it.
    auto* aux = w.find<QComboBox>(names::barPlace("aux"));
    aux->activated(2);
    w.settle();
    CHECK_EQ(arg(&w.part.last("attach")->args, "peripheral"), std::string("test-dotter-2"));
}

TEST(no_control_no_gap_on_widgets) {
    TestWindow w;
    w.part.publish("control-state", shell::test::controlValue("Model-B"));
    w.step();
    CHECK(w.find<QWidget>(names::barSlot("slot-1")) == nullptr);
    CHECK(w.find<QWidget>(names::barPlace("aux")) == nullptr);
    // The slots' and the media's groups are hidden: no gap.
    int visibleGroups = 0;
    for (int i = 0; i < 5; ++i)
        if (auto* g = w.find<QWidget>("bar-group-" + std::to_string(i)); g && g->isVisible()) ++visibleGroups;
    CHECK_EQ(visibleGroups, 3);   // the model picker, the printer and More
}

TEST(settings_dialog_on_widgets) {
    TestWindow w;
    w.action("settings")->trigger();
    w.settle();
    auto* d = w.find<qt6::ShellSettingsDialog>(names::kSettingsDialog);
    CHECK(d && d->isVisible());
    CHECK(d->windowTitle() == QStringLiteral("Settings"));
    QStringList sections;
    for (auto* g : d->findChildren<QGroupBox*>()) sections << g->title();
    CHECK(sections == (QStringList{"General", "Startup presets", "Storage", "Folders", "Tracing"}));
    auto* buttons = d->findChild<QDialogButtonBox*>(names::kSettingsClose);
    CHECK(buttons && buttons->buttons().size() == 1);   // Close only
    auto* model = d->findChild<QComboBox*>(QString::fromStdString(names::setting("startup", "model")));
    CHECK(model && model->itemText(0) == QStringLiteral("Last used"));
    model->activated(model->findData(QStringLiteral("Model-B")));
    w.step();
    CHECK(settings::test::readText(w.file.path()).find("model = Model-B") != std::string::npos);   // at once
    CHECK(model->currentText() == QStringLiteral("Model-B"));
    buttons->buttons().front()->click();
    w.settle();
    CHECK(!d->isVisible());
}

TEST(startup_preset_row_on_widgets) {
    TestWindow w;
    w.shell->openSettings();
    w.step();
    auto* d = w.shell->settingsDialog();
    auto* row = d->findChild<QComboBox*>(QString::fromStdString(names::setting("preset", "Model-A")));
    CHECK(row != nullptr);
    QStringList items;
    for (int i = 0; i < row->count(); ++i) items << row->itemText(i);
    CHECK(items == (QStringList{"None", "Start A — The A start", "Start A2", "Choose…"}));
    w.fileAnswer = QString::fromStdString(w.home.str() + "/own.test");
    row->activated(row->count() - 1);   // Choose…
    w.step();
    CHECK_EQ(w.fileAsks.size(), std::size_t{1});
    // Sent to be checked first (opening's OpenPart sets it); the row shows what is set.
    CHECK_EQ(w.part.count("choose-startup-preset"), 1);
    CHECK(w.part.sent.back().args == sessions::Value::object({{"model", "Model-A"}, {"ref", w.home.str() + "/own.test"}}));
    CHECK(row->currentText() == QStringLiteral("None"));
    // As set: shown with ~ for home.
    w.command("set-setting", sessions::Value::object({{"section", "preset"}, {"key", "Model-A"}, {"value", w.home.str() + "/own.test"}}));
    w.step();
    row = d->findChild<QComboBox*>(QString::fromStdString(names::setting("preset", "Model-A")));
    CHECK(row->currentText() == QStringLiteral("~/own.test"));
}

TEST(folder_row_on_widgets) {
    TestWindow w;
    w.shell->openSettings();
    w.step();
    auto* d = w.shell->settingsDialog();
    const QString name = QString::fromStdString(names::setting("storage", "save-folder"));
    auto* shown = d->findChild<QLabel*>(name + "-shown");
    auto* reset = d->findChild<QPushButton*>(name + "-reset");
    CHECK(shown->text() == QStringLiteral("~/Test-Product"));
    CHECK(reset->text() == QStringLiteral("Reset") && !reset->isEnabled());
    w.folderAnswer = QString::fromStdString(w.home.str() + "/Saves");
    d->findChild<QPushButton*>(name + "-change")->click();
    w.step();
    CHECK(shown->text() == QStringLiteral("~/Saves"));
    CHECK(reset->isEnabled());
    reset->click();
    w.step();
    CHECK(shown->text() == QStringLiteral("~/Test-Product"));
    CHECK(!reset->isEnabled());
}

TEST(settings_while_operation_runs_on_widgets) {
    TestWindow w;
    w.command("tsp.op");
    CHECK(w.session->busy());
    w.shell->openSettings();
    w.step();
    auto* model = w.shell->settingsDialog()->findChild<QComboBox*>(QString::fromStdString(names::setting("startup", "model")));
    CHECK(model->isEnabled());
    model->activated(model->findData(QStringLiteral("Model-A")));
    w.step();
    CHECK_EQ(w.file.text("startup", "model"), std::string("Model-A"));
    CHECK(w.session->busy());
}

TEST(loading_after_500_ms_on_widgets) {
    TestWindow w;
    w.command("tsp.op");
    w.runFor(300);
    CHECK(!w.shell->loadingShown());
    w.runFor(300);
    CHECK(w.shell->loadingShown());
    auto* loading = w.find<QWidget>(names::kLoading);
    CHECK(loading && loading->findChild<QLabel*>()->text() == QStringLiteral("Loading…"));
    CHECK(loading->findChildren<QPushButton*>().isEmpty());   // no button to cancel
    CHECK(loading->parentWidget() == w.faceplate);            // on the faceplate
    CHECK(loading->y() >= static_cast<int>(15 * w.scale));     // below the display
    w.command("reset");
    w.step(2);
    CHECK(!w.session->busy());
    CHECK(!w.shell->loadingShown());
}

TEST(loading_blocks_nothing) {
    TestWindow w;
    w.command("tsp.op");
    w.runFor(700);
    CHECK(w.shell->loadingShown());
    auto* loading = w.find<QWidget>(names::kLoading);
    CHECK(loading->testAttribute(Qt::WA_TransparentForMouseEvents));
    CHECK(w.shell->controlBar()->isVisible() && w.shell->controlBar()->isEnabled());
    CHECK(w.find<QComboBox>(names::kBarModel)->isEnabled());
    CHECK(w.action("reset")->isEnabled() && w.action("cold-boot")->isEnabled());
    CHECK(w.window.menuBar()->isEnabled());
    // A key under the overlay still reaches the faceplate.
    click(w.faceplate, QPoint(100, 100));
    release(w.faceplate, QPoint(100, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), 1);
    w.action("reset")->trigger();   // Reset stops the operation
    w.step(2);
    CHECK_EQ(w.part.count("reset"), 1);
    CHECK(!w.session->busy());
    CHECK(!w.shell->loadingShown());
    CHECK(noticeBox(w) == nullptr);   // the stopped operation is no notice
}

TEST(notices_one_after_the_other_on_widgets) {
    TestWindow w;
    w.command("tsp.notice", Value::object({{"text", "The first went wrong."}}));
    w.command("tsp.notice", Value::object({{"text", "The second went wrong."}}));
    w.step();
    QMessageBox* box = noticeBox(w);
    CHECK(box != nullptr);
    CHECK(box->text() == QStringLiteral("The first went wrong."));
    CHECK(box->windowTitle().isEmpty());
    CHECK(box->windowModality() == Qt::WindowModal);
    w.step(3);   // the session goes on behind it
    box->findChild<QAbstractButton*>(names::kNoticeOk)->click();
    w.settle();
    box = noticeBox(w);
    CHECK(box && box->text() == QStringLiteral("The second went wrong."));
    box->findChild<QAbstractButton*>(names::kNoticeOk)->click();
    w.settle();
    CHECK(noticeBox(w) == nullptr);
    // A refused command the shell sent: its reason, as a notice.
    w.shell->sendChecked(sessions::CommandName{"tsp.refuse"}, Value::object({{"reason", "That cannot be done now."}}));
    w.settle();
    CHECK(noticeBox(w) && noticeBox(w)->text() == QStringLiteral("That cannot be done now."));
}

TEST(notice_question_yes_resends_confirmed) {
    TestWindow w;
    w.shell->sendChecked(sessions::CommandName{"tsp.confirm"}, Value::object({{"name", "A"}}));
    w.settle();
    QMessageBox* box = noticeBox(w);
    CHECK(box && box->text() == QStringLiteral("Replace the card?"));
    CHECK(box->findChild<QAbstractButton*>(names::kNoticeNo) != nullptr);
    box->findChild<QAbstractButton*>(names::kNoticeYes)->click();
    w.settle();
    CHECK_EQ(w.part.count("tsp.confirm"), 2);
    CHECK(w.part.last("tsp.confirm")->args == Value::object({{"name", "A"}, {"confirmed", true}}));
    CHECK(noticeBox(w) == nullptr);
}

TEST(notice_over_loading_on_widgets) {
    TestWindow w;
    w.command("tsp.op");
    w.runFor(700);
    CHECK(w.shell->loadingShown());
    w.command("tsp.notice", Value::object({{"text", "A file could not be read."}}));
    w.step();
    CHECK(noticeBox(w) != nullptr);
    CHECK(w.shell->loadingShown());
    noticeBox(w)->findChild<QAbstractButton*>(names::kNoticeOk)->click();
    w.step();
    CHECK(w.shell->loadingShown());   // closing the notice does not end the operation
    CHECK(w.session->busy());
}

// The calculator's log is not the app log: none of its lines reaches Qt's
// messages, which go to the app log.
TEST(log_not_to_platform_no_view) {
    TestWindow w;
    infoLines.clear();
    previousHandler = qInstallMessageHandler(captureInfo);
    w.command("tsp.log", Value::object({{"text", "A line of the log."}}));
    w.step();
    qInstallMessageHandler(previousHandler);
    CHECK(!infoLines.contains(QStringLiteral("A line of the log.")));
    CHECK(w.window.findChild<QWidget*>(QStringLiteral("log")) == nullptr);   // no view of its own
}

TEST(roms_missing_no_bar_no_machine_menu) {
    TestWindow w(false);
    w.part.publish("roms-missing", Value::object({{"text", "The ROM files of Model-A are missing."}}));
    w.step();
    auto* missing = w.find<QLabel>(names::kRomsMissing);
    CHECK(missing && missing->isVisible());
    CHECK(missing->text() == QStringLiteral("The ROM files of Model-A are missing."));
    CHECK(!w.faceplate->isVisible());
    CHECK(!w.shell->controlBar()->isVisible());
    CHECK(w.find<QMenu>("menu-machine") == nullptr);
    CHECK(w.action("help") != nullptr && w.action("about") != nullptr);
    w.action("about")->trigger();   // Help and About still work
    w.settle();
    CHECK(w.find<QWidget>(names::kAboutDialog) && w.find<QWidget>(names::kAboutDialog)->isVisible());
}

TEST(help_opens_product_address_on_widgets) {
    TestWindow w;
    w.action("help")->trigger();
    w.settle();
    CHECK_EQ(w.opened.size(), std::size_t{1});
    CHECK(w.opened[0] == QUrl(QStringLiteral("https://calc-u.org/test/")));
}

TEST(help_failure_changes_nothing) {
    TestWindow w;
    w.openSucceeds = false;
    const auto sent = w.part.sent.size();
    w.action("help")->trigger();
    w.step();
    CHECK_EQ(w.opened.size(), std::size_t{1});
    CHECK(noticeBox(w) == nullptr);
    CHECK_EQ(w.part.sent.size(), sent);
    CHECK(!w.session->busy());
}

TEST(about_dialog_on_widgets) {
    TestWindow w;
    w.action("about")->trigger();
    w.settle();
    auto* about = w.find<QDialog>(names::kAboutDialog);
    CHECK(about && about->isVisible());
    CHECK(about->windowTitle() == QStringLiteral("About Test-Product"));
    QStringList texts;
    for (auto* l : about->findChildren<QLabel*>()) texts << l->text();
    CHECK(texts.contains(QStringLiteral("<b>Test-Product</b>")));
    CHECK(texts.contains(QStringLiteral("Version 1.2.3 (abc123)")));
    CHECK(texts.contains(QStringLiteral("A product made for the tests.")));
    CHECK(texts.contains(QStringLiteral("<a href=\"https://example.org/source\">Source</a>")));
    auto* support = about->findChild<QLabel*>(QStringLiteral("about-support"));
    CHECK(support && support->text() == QStringLiteral("<b>Support Information</b>"));   // OS, Hardware of this host
    auto* buttons = about->findChild<QDialogButtonBox*>();
    CHECK(buttons && buttons->buttons().size() == 2);   // Copy and OK
    auto* copy = about->findChild<QPushButton*>(QStringLiteral("about-copy"));
    CHECK(copy != nullptr);
    QGuiApplication::clipboard()->clear();
    copy->click();
    const QString copied = QGuiApplication::clipboard()->text();
    // The text's form is shell::aboutText's (help_about_tests.cpp): here the
    // dialog's model with this host's rows.
    CHECK(copied == QString::fromStdString(shell::aboutText(shell::about(shell::test::product(), qt6::hostSupportRows()))));
    CHECK(copied.contains(QStringLiteral("OS: ")));
    CHECK(about->isVisible());   // Copy leaves it open
}

TEST(paper_panel_always_on_widgets) {
    TestWindow w;
    auto* area = w.find<qt6::PanelArea>(names::kPanelArea);
    CHECK(area != nullptr);
    auto* paper = qobject_cast<qt6::PaperPanel*>(area->panel("paper"));
    CHECK(paper && paper->isVisible());
    w.command("tsp.plug", Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    w.step();
    CHECK(area->panel("paper") == paper);   // the same panel, its content kept
    w.part.publish("control-state", shell::test::controlValue());
    w.step();
    CHECK(area->panel("paper") == paper && paper->isVisible());
    // A product's panel goes into the same area.
    auto* own = new QLabel(QStringLiteral("INSPECTOR"));
    area->addPanel(own);
    CHECK(own->parentWidget() == area);
}

// No widget of the shell takes the keyboard focus: the window owns typing,
// and with a focusable widget in it Qt spends Tab on moving the focus,
// so the key never reaches the product's keyboard map.
TEST(shell_widgets_take_no_focus) {
    TestWindow w;
    w.shell->registerMediaControls("test-card", [](const Value& entry, QWidget* parent) -> QWidget* {
        return new QLabel(QString::fromStdString("card " + shell::read::text(entry, "id")), parent);
    });
    w.command("tsp.plug", Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    w.step();
    qt6::RunControlButtons run(debugger::LevelName{"cpu"});
    auto* area = w.find<qt6::PanelArea>(names::kPanelArea);
    CHECK(area && qobject_cast<qt6::PaperPanel*>(area->panel("paper")));
    int widgets = 0;
    for (QWidget* root : {w.shell->controlBar(), static_cast<QWidget*>(area), static_cast<QWidget*>(&run)})
        for (QWidget* c : root->findChildren<QWidget*>()) {
            // A combo box's list is a popup window of its own, open only while chosen.
            if (c->focusPolicy() == Qt::NoFocus || c->window() != root->window()) continue;
            ++widgets;
            std::fprintf(stderr, "  takes focus: %s (%s)\n", c->metaObject()->className(), qPrintable(c->objectName()));
        }
    CHECK_EQ(widgets, 0);
}

TEST(mouse_and_touch_on_test_faceplate) {
    TestWindow w;
    // Key 2 is at 20..30 × 20..30 hit-map units; at scale 4 that is 80..120.
    click(w.faceplate, QPoint(100, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), 1);
    CHECK_EQ(arg(&w.part.last("screen-key-down")->args, "key"), std::string("2"));
    CHECK(shell::read::integer(w.part.last("screen-key-down")->args, "pointer", -1) == 0);
    release(w.faceplate, QPoint(170, 100));   // released elsewhere: the key that went down goes up
    w.settle();
    CHECK_EQ(arg(&w.part.last("screen-key-up")->args, "key"), std::string("2"));
    // Within the distance of 4 logical pixels of key 1 (0..40).
    click(w.faceplate, QPoint(43, 100));
    release(w.faceplate, QPoint(43, 100));
    w.settle();
    CHECK_EQ(arg(&w.part.last("screen-key-down")->args, "key"), std::string("1"));
    // No key at the down, nothing on the move onto one.
    const int downs = w.part.count("screen-key-down");
    click(w.faceplate, QPoint(60, 100));
    QMouseEvent move(QEvent::MouseMove, QPointF(100, 100), w.faceplate->mapToGlobal(QPointF(100, 100)), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w.faceplate, &move);
    release(w.faceplate, QPoint(100, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), downs);
#ifdef CALCU_QT_TEST
    // Two touch points hold two keys.
    QPointingDevice* touch = QTest::createTouchDevice();
    QTest::touchEvent(w.faceplate, touch).press(1, QPoint(20, 100)).press(2, QPoint(180, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), downs + 2);
    QTest::touchEvent(w.faceplate, touch).release(1, QPoint(20, 100)).release(2, QPoint(180, 100));
    w.settle();
    CHECK_EQ(arg(&w.part.last("screen-key-up")->args, "key"), std::string("3"));
#endif
}

TEST(display_hold_on_widgets) {
    TestWindow w;
    auto speed = [&] { return shell::read::text(w.host->value(sessions::kBusyTopic), "speed"); };
    CHECK_EQ(speed(), std::string("regular"));
    click(w.faceplate, QPoint(40, 20));   // on the display (0..200 × 0..60)
    w.step();
    CHECK_EQ(speed(), std::string("full"));
    CHECK(!w.shell->loadingShown());   // no operation: no "Loading…"
    release(w.faceplate, QPoint(40, 20));
    w.step();
    CHECK_EQ(speed(), std::string("regular"));
}

TEST(model_switch_changes_menus_in_place) {
    TestWindow w;
    QMenu* machine = w.find<QMenu>("menu-machine");
    CHECK(w.action("peripheral-aux-test-plotter") != nullptr);
    w.action("model-Model-B")->trigger();
    w.step();
    CHECK(w.find<QMenu>("menu-machine") == machine);   // the same menu, filled anew
    CHECK(w.action("peripheral-aux-test-plotter") == nullptr);
    CHECK(w.action("model-Model-B")->isChecked());
    CHECK(w.window.isVisible());
}

// The media items of File (Import, Saved Media) are the Web frontend's; the
// Qt6 shell has no saved media list, so its File menu has neither item, also
// when the product has kinds of medium.
TEST(qt6_file_menu_without_media_items) {
    TestWindow w;
    w.part.publish("media", shell::test::mediaWithKind());
    w.settle();
    auto* file = w.find<QMenu>("menu-file");
    CHECK(file != nullptr);
    CHECK(w.action("import-media") == nullptr);
    CHECK(w.action("saved-media") == nullptr);
    CHECK(file && titles(file) == (QStringList{"item-load-preset", "item-examples-menu", "-", "item-load-program", "-",
                                               "item-mount-drive", "-", "item-settings", "item-quit"}));
}

// ── The faceplate's geometry and the About icon ──

namespace {

// Records the presses that reach the faceplate past the shell's filter.
struct PressRecorder : QObject {
    int presses = 0, releases = 0;
    bool eventFilter(QObject*, QEvent* e) override {
        if (e->type() == QEvent::MouseButtonPress) ++presses;
        if (e->type() == QEvent::MouseButtonRelease) ++releases;
        return false;
    }
};

} // namespace

TEST(faceplate_origin_moves_the_hit_map) {
    TestWindow w;
    // The faceplate drawn 40 px right and 8 px down in its widget.
    w.shell->setFaceplate(w.faceplate, shell::test::hitMap(),
                          qt6::Shell::FaceplateGeometry{[&w] { return w.scale; }, [] { return QPointF(40, 8); }});
    // Key 2 is 80..120 × 80..120 at scale 4; with the origin, 120..160 × 88..128.
    click(w.faceplate, QPoint(140, 100));
    release(w.faceplate, QPoint(140, 100));
    w.settle();
    CHECK_EQ(arg(&w.part.last("screen-key-down")->args, "key"), std::string("2"));
    // Where key 2 was without the origin is now key 1's (0..40, shifted by 40).
    click(w.faceplate, QPoint(70, 100));
    release(w.faceplate, QPoint(70, 100));
    w.settle();
    CHECK_EQ(arg(&w.part.last("screen-key-down")->args, "key"), std::string("1"));
}

TEST(faceplate_press_the_product_takes_passes_through) {
    TestWindow w;
    PressRecorder rec;
    w.faceplate->installEventFilter(&rec);   // before the shell's new filter: sees what it lets pass
    w.shell->setFaceplate(w.faceplate, shell::test::hitMap(),
                          qt6::Shell::FaceplateGeometry{[&w] { return w.scale; }, {},
                                                        [](double x, double y) { return x >= 25 && x < 30 && y >= 20 && y < 30; }});
    const int downs = w.part.count("screen-key-down");
    // The right half of key 2 is the product's: no key; the press, its move and its release pass.
    click(w.faceplate, QPoint(110, 100));
    QMouseEvent move(QEvent::MouseMove, QPointF(90, 100), w.faceplate->mapToGlobal(QPointF(90, 100)), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(w.faceplate, &move);
    release(w.faceplate, QPoint(90, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), downs);
    CHECK_EQ(rec.presses, 1);
    CHECK_EQ(rec.releases, 1);
    // Its left half is still key 2, consumed by the shell.
    click(w.faceplate, QPoint(90, 100));
    release(w.faceplate, QPoint(90, 100));
    w.settle();
    CHECK_EQ(w.part.count("screen-key-down"), downs + 1);
    CHECK_EQ(rec.presses, 1);
    w.faceplate->removeEventFilter(&rec);
}

TEST(about_shows_the_window_icon) {
    TestWindow w;
    const QIcon before = QApplication::windowIcon();
    QPixmap pix(32, 32);
    pix.fill(Qt::red);
    QApplication::setWindowIcon(QIcon(pix));
    w.action("about")->trigger();
    w.settle();
    auto* about = w.find<QDialog>(names::kAboutDialog);
    auto* icon = about ? about->findChild<QLabel*>(QStringLiteral("about-icon")) : nullptr;
    CHECK(icon != nullptr);
    CHECK(icon && !icon->pixmap().isNull());
    QApplication::setWindowIcon(before);
}

// The bar below a faceplate by the library's rule (shell::barFit): its
// labels (a control's BarLabels, More's "More") where the whole bar fits the
// faceplate with them, then as wide as the faceplate; narrower, without
// them and as wide as its controls need.
TEST(bar_fitted_below_a_faceplate) {
    TestWindow w;
    auto* extra = new QWidget();
    auto* row = new QHBoxLayout(extra);
    row->setContentsMargins(0, 0, 0, 0);
    auto* label = new QLabel(QStringLiteral("Tape:"), extra);
    row->addWidget(label);
    row->addWidget(new QToolButton(extra));
    qt6::barLabel(label);
    w.shell->addBarControl(shell::BarGroup::Media, extra);
    w.settle();
    auto* bar = w.find<qt6::ShellControlBar>(names::kControlBar);
    auto* more = w.find<QToolButton>(names::kBarMore);
    CHECK(bar && more);
    if (!bar || !more) return;
    w.shell->fitBar(4000);
    w.settle();
    CHECK(bar->showsLabels() && label->isVisibleTo(bar) && more->text().contains(QStringLiteral("More")));
    CHECK_EQ(bar->maximumWidth(), 4000);
    w.shell->fitBar(4000, 3000);   // never wider than the room
    w.settle();
    CHECK_EQ(bar->maximumWidth(), 3000);
    w.shell->fitBar(10);
    w.settle();
    CHECK(!bar->showsLabels() && !label->isVisibleTo(bar) && !more->text().contains(QStringLiteral("More")));
    CHECK(std::abs(bar->maximumWidth() - bar->sizeHint().width()) < 4);
}

// The paper's own window: the same panel moved there and back, its place
// and the setting in the settings file; closing it puts the paper back.
TEST(paper_own_window_moves_the_panel_and_remembers) {
    TestWindow w(true, TestWindow::defaultOptions(), {}, true);
    w.window.resize(600, 400);
    w.window.show();
    w.settle();
    auto* area = w.find<qt6::PanelArea>(names::kPanelArea);
    auto* paper = qobject_cast<qt6::PaperPanel*>(area->panel("paper"));
    auto* button = paper->findChild<QToolButton*>(QStringLiteral("paper-window"));
    CHECK(button && button->isVisible());
    CHECK_EQ(button->toolTip().toStdString(), std::string("Open in Own Window"));
    w.command("tsp.plug", Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    w.step();
    int changes = 0;
    QObject::connect(area, &qt6::PanelArea::paperOwnWindowChanged, [&changes] { ++changes; });

    button->click();
    w.settle();
    CHECK(area->paperOwnWindow() && changes == 1);
    qt6::PaperWindow* own = area->paperWindow();
    CHECK(own && own->isVisible() && own->isWindow());
    CHECK(paper->window() == own && area->panel("paper") == paper);   // the same panel, its paper kept
    CHECK_EQ(own->windowTitle().toStdString(), std::string("Test-Product — Printer"));
    CHECK_EQ(button->toolTip().toStdString(), std::string("Back to Main Window"));
    CHECK(w.file.boolean("window", settings::kPaperOwnWindowKey));
    // The control state's panels leave it shown in its own window.
    area->setShown("paper", false);
    w.part.publish("control-state", shell::test::controlValue());
    w.step();
    CHECK(paper->isVisible());
    area->setShown("paper", true);

    own->resize(333, 444);
    own->move(120, 90);
    w.settle();
    own->close();   // closing the window puts the paper back
    w.settle();
    CHECK(!area->paperOwnWindow() && changes == 2);
    CHECK(paper->window() == &w.window && paper->isVisible());
    CHECK_EQ(button->toolTip().toStdString(), std::string("Open in Own Window"));
    CHECK(!w.file.boolean("window", settings::kPaperOwnWindowKey));
    const settings::WindowPlace place = settings::windowPlace(w.file, settings::kPaperWindowPrefix);
    CHECK(place.width == 333 && place.height == 444 && place.x && place.y);
}

// The main window's close takes the paper's window with it, still its own:
// the next start opens it again.
TEST(paper_own_window_closes_with_main_window_and_comes_back) {
    {
        TestWindow w(true, TestWindow::defaultOptions(), {}, true);
        w.window.show();
        w.settle();
        auto* area = w.find<qt6::PanelArea>(names::kPanelArea);
        area->setPaperOwnWindow(true);
        w.settle();
        qt6::PaperWindow* own = area->paperWindow();
        CHECK(own && own->isVisible());
        w.window.close();
        w.settle();
        CHECK(area->paperOwnWindow() && own && !own->isVisible());
        CHECK(w.file.boolean("window", settings::kPaperOwnWindowKey));
    }
    TestWindow again(true, TestWindow::defaultOptions(), {}, true);
    again.settle();
    auto* area = again.find<qt6::PanelArea>(names::kPanelArea);
    CHECK(!area->paperOwnWindow());
    again.file.set("window", settings::kPaperOwnWindowKey, "true");   // as the last run left it
    again.shell->restorePaperWindow();
    again.settle();
    CHECK(area->paperOwnWindow() && area->paperWindow()->isVisible());
}

// Without the window settings (a mobile window, a test) there is no own
// window and no button.
TEST(paper_own_window_not_offered_without_settings) {
    TestWindow w;
    w.window.show();
    w.settle();
    auto* area = w.find<qt6::PanelArea>(names::kPanelArea);
    auto* button = area->panel("paper")->findChild<QToolButton*>(QStringLiteral("paper-window"));
    CHECK(button && !button->isVisible());
    area->setPaperOwnWindow(true);
    CHECK(!area->paperOwnWindow());
}
