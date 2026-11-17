// CalcU1600QtTests (LIB-14 Part D, D.8; LIB-4 Part D, D.6; C1600-2 AC-15;
// LIB-13 AC-18, AC-19, AC-21): the Qt6 window of Calc-U-1600 put together
// from the shell, on the real ROMs, offscreen. Each case carries the
// scenario ID of LIB-14 AC-25 in its name; the Web cases of the same IDs
// are in frontends/web/tests/shell.spec.js.
#include "QtFixture.hpp"
#include "TestHarness.hpp"

#include "AppSettings.hpp"
#include "Ce158PrinterWidget.hpp"
#include "LcdWidget.hpp"
#include "DebugPanel.hpp"
#include "FaceplateWidget.hpp"
#include "qt6/FileOpening.hpp"
#include "app/SharpShellProduct.hpp"
#include "opening/StartFile.hpp"
#include "control/ControlState.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/Shell.hpp"
#include "qt6/ShellSettingsDialog.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"
#include "typing/TypingTopics.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDropEvent>
#include <QFileOpenEvent>
#include <QGroupBox>
#include <QAbstractItemView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#ifdef CALCU_ATTACH
#include "app/HarnessProduct.hpp"
#include "screenshots/ShotRunner.hpp"
#include "screenshots/ShotScenario.hpp"
#endif

using qtfixture::Fixture;
using qtfixture::romsMissingRun;
using qtfixture::romsPresent;
namespace names = calcu::shell::names;
using calcu::sessions::Value;

namespace {

#define SKIP_UNLESS_WINDOW()                                                        \
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");                                 \
    SKIP_UNLESS(!romsMissingRun(), "the ROMs-missing run has its own case only")

QString text(QAction* a) {
    QString t = a->text();
    t.remove(QLatin1Char('&'));
    return t;
}

// The labels of a menu's items in order, separators as "-", submenus as "X ▸".
std::vector<std::string> labels(QMenu* m) {
    std::vector<std::string> out;
    if (!m) return out;
    for (QAction* a : m->actions()) {
        if (a->isSeparator()) out.push_back("-");
        else if (a->menu()) out.push_back(text(a).toStdString() + " ▸");
        else out.push_back(text(a).toStdString());
    }
    return out;
}

std::vector<std::string> menuTitles(QMenuBar* bar) {
    std::vector<std::string> out;
    for (QAction* a : bar->actions()) out.push_back(text(a).toStdString());
    return out;
}

void print(const char* what, const std::vector<std::string>& v) {
    std::fprintf(stderr, "  %s:", what);
    for (const auto& s : v) std::fprintf(stderr, " [%s]", s.c_str());
    std::fprintf(stderr, "\n");
}

bool checkList(const char* what, const std::vector<std::string>& got, const std::vector<std::string>& want) {
    if (got != want) {
        print(what, got);
        return false;
    }
    return true;
}

// The Machine menu's submenu labels (ROM menus).
std::vector<std::string> machineSubmenus(Fixture& f) {
    std::vector<std::string> out;
    for (const auto& l : labels(f.find<QMenu>(names::menu("machine"))))
        if (l.size() > 4 && l.substr(l.size() - 4) == " ▸") out.push_back(l);
    return out;
}

// Answers the next modal dialog (a QMessageBox: its first button; a
// QDialog: accept) once it shows.
void answerNextModal() {
    QTimer::singleShot(50, [] {
        for (int i = 0; i < 200; ++i) {
            if (QWidget* m = QApplication::activeModalWidget()) {
                if (auto* box = qobject_cast<QMessageBox*>(m)) {
                    if (!box->buttons().isEmpty()) box->buttons().first()->click();
                    else box->accept();
                } else if (auto* d = qobject_cast<QDialog*>(m)) {
                    d->accept();
                }
                return;
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
    });
}

std::string barPlaceText(Fixture& f, const char* place) {
    if (auto* b = f.find<QToolButton>(names::barPlace(place))) return b->text().toStdString();
    if (auto* c = f.find<QComboBox>(names::barPlace(place))) {
        std::string out = "picker:";
        for (int i = 0; i < c->count(); ++i) out += (i ? "," : "") + c->itemText(i).toStdString();
        return out;
    }
    return "";
}

} // namespace

// ── Q2: the menus (AC-34; AC-1, AC-4, AC-6 for this product) ───────────

TEST(lib14_ac34_qt_menus_in_family_order) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1500");
    CHECK(checkList("menus", menuTitles(f.w().menuBar()), {"File", "Edit", "Machine", "Help"}));
    CHECK(checkList("File", labels(f.find<QMenu>(names::menu("file"))),
                    {"Load Preset…", "Examples ▸", "-", "Load BASIC Program…", "Load Machine Code…", "-",
                     "Mount Directory…", "Unmount", "-", "Settings…", "Quit"}));
    CHECK(checkList("Edit", labels(f.find<QMenu>(names::menu("edit"))), {"Copy Screen", "Paste Text"}));
    CHECK(checkList("Machine", labels(f.find<QMenu>(names::menu("machine"))),
                    {"Model ▸", "ROM Revision ▸", "Peripherals ▸", "-", "Reset", "Cold boot", "Resume"}));
    CHECK(checkList("Help", labels(f.find<QMenu>(names::menu("help"))), {"Calc-U-1600 Help", "About Calc-U-1600…"}));
    CHECK(checkList("ROM Revision", labels(f.find<QMenu>(names::item("variants-model"))), {"A01", "A03", "A04"}));
    CHECK(checkList("Peripherals", labels(f.find<QMenu>(names::item("peripherals"))), {"CE-150", "-", "CE-158"}));
    // Mount Directory… is the PC-1600's (its product entry); Resume while paused.
    CHECK(f.action(shared::kItemMountDirectory) && !f.action(shared::kItemMountDirectory)->isEnabled());
    CHECK(f.action("resume") && !f.action("resume")->isEnabled());
    // AC-6: the platform's places.
    CHECK(f.action("settings")->menuRole() == QAction::PreferencesRole);
    CHECK(f.action("about")->menuRole() == QAction::AboutRole);
    CHECK(f.action("quit")->menuRole() == QAction::QuitRole);
    // The PC-1600: ROM Version ▸, and CE-1600P ROM ▸ while it is plugged in.
    f.command("model", Value::object({{"name", "PC-1600"}}));
    f.run(100);
    CHECK(checkList("PC-1600 submenus", machineSubmenus(f), {"Model ▸", "ROM Version ▸", "Peripherals ▸"}));
    CHECK(f.action(shared::kItemMountDirectory)->isEnabled());
    CHECK(!f.action(shared::kItemUnmountDirectory)->isEnabled());
    f.command("attach", Value::object({{"place", "printer"}, {"peripheral", "CE-1600P"}}));
    f.run(100);
    CHECK(checkList("PC-1600 + CE-1600P", machineSubmenus(f),
                    {"Model ▸", "ROM Version ▸", "CE-1600P ROM ▸", "Peripherals ▸"}));
    CHECK(checkList("PC-1600 Peripherals", labels(f.find<QMenu>(names::item("peripherals"))),
                    {"CE-150", "CE-1600P", "-", "CE-158"}));
    // The PC-1500A: one ROM, no submenu.
    f.command("model", Value::object({{"name", "PC-1500A"}}));
    f.run(100);
    CHECK(checkList("PC-1500A submenus", machineSubmenus(f), {"Model ▸", "Peripherals ▸"}));
}

TEST(lib14_ac34_copy_log_selection_while_inspector_text_selected) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QAction* copy = f.action("copy-screen");
    CHECK(copy && text(copy) == "Copy Screen");
    auto* out = f.w().inspector()->findChild<QPlainTextEdit*>();
    CHECK(out != nullptr);
    if (!out || !copy) return;
    out->setPlainText(QStringLiteral("0000: 12 34"));
    out->selectAll();
    f.run(50);
    CHECK(text(copy) == "Copy Log Selection");
    copy->trigger();
    CHECK(f.clipboardText == QStringLiteral("0000: 12 34") && f.clipboardImage.isNull());
    QTextCursor c = out->textCursor();
    c.clearSelection();
    out->setTextCursor(c);
    f.run(50);
    CHECK(text(copy) == "Copy Screen");
}

TEST(lib14_ac34_shortcuts_ctrl_o_r_shift_r) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    CHECK(f.action("load-preset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_O));
    CHECK(f.action("reset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_R));
    CHECK(f.action("cold-boot")->shortcut() == QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    CHECK(f.action("copy-screen")->shortcut() == QKeySequence(QKeySequence::Copy));
    CHECK(f.action("paste-text")->shortcut() == QKeySequence(QKeySequence::Paste));
    // Paste Text types the clipboard's text (the fake's).
    f.action("paste-text")->trigger();
    CHECK(f.waitFor([&] {
        LcdText t;
        return f.w().controller()->lcdText(&t) && t.text().find("10 PRINT 1") != std::string::npos;
    }, 5000));
}

// ── Q4: the bar per model (AC-36; AC-8 to AC-10; LIB-4 AC-19) ──────────

TEST(lib14_ac36_bar_pc1500_pc1500a) {
    SKIP_UNLESS_WINDOW();
    for (const char* model : {"PC-1500", "PC-1500A"}) {
        Fixture f(model);
        auto* bar = f.find<QWidget>(names::kControlBar);
        CHECK(bar && bar->isVisible());
        auto* m = f.find<QComboBox>(names::kBarModel);
        CHECK(m && m->currentText().toStdString() == model);
        CHECK(f.find<QWidget>(names::barSlot("slot-1")) && !f.find<QWidget>(names::barSlot("slot-2")));
        CHECK(f.find<QWidget>("media-picker-slot-1") && f.find<QWidget>("media-save-slot-1"));
        CHECK_EQ(barPlaceText(f, "printer"), std::string("CE-150"));
        CHECK_EQ(barPlaceText(f, "interface"), std::string("CE-158"));
        CHECK(!f.find<QWidget>(names::barPlace("bus-rom")) && !f.find<QWidget>(names::barPlace("host-drive")));
        CHECK(!f.find<QWidget>("bar-disk")->isVisible());
        CHECK(!f.find<QWidget>("media-picker-tape"));
        // The CE-150: the button pressed, the tape controls with its cassette.
        f.find<QToolButton>(names::barPlace("printer"))->click();
        CHECK(f.waitFor([&] { return f.find<QWidget>("media-picker-tape") != nullptr; }, 15000));
        CHECK(f.find<QToolButton>(names::barPlace("printer"))->isChecked());
    }
}

TEST(lib14_ac36_bar_pc1600) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    auto* slot1 = f.find<QWidget>(names::barSlot("slot-1"));
    auto* slot2 = f.find<QWidget>(names::barSlot("slot-2"));
    CHECK(slot1 && slot2);
    if (slot1 && slot2) {
        CHECK(slot1->findChild<QLabel*>() && slot1->findChild<QLabel*>()->text() == "1:");
        CHECK(slot2->findChild<QLabel*>() && slot2->findChild<QLabel*>()->text() == "2:");
    }
    CHECK_EQ(barPlaceText(f, "printer"), std::string("picker:None,CE-150,CE-1600P"));
    CHECK_EQ(barPlaceText(f, "interface"), std::string("CE-158"));
    CHECK(!f.find<QWidget>(names::barPlace("host-drive")));
    // The disk controls: on every PC-1600, disabled with the reason.
    auto* disk = f.find<QWidget>("bar-disk");
    auto* picker = f.find<QComboBox>("media-picker-floppy");
    CHECK(disk && disk->isVisible() && picker && !picker->isEnabled());
    CHECK(picker && picker->toolTip().toStdString() == shared::kDiskInCe1600p);
    // The CE-1600P: the disk controls usable, the tape's with its cassette.
    f.command("attach", Value::object({{"place", "printer"}, {"peripheral", "CE-1600P"}}));
    CHECK(f.waitFor([&] { return f.find<QComboBox>("media-picker-floppy")->isEnabled(); }, 5000));
    CHECK(f.waitFor([&] { return f.find<QWidget>("media-picker-tape") != nullptr; }, 5000));
}

// ── Q5: the settings (AC-37; AC-11 to AC-13) ───────────────────────────

TEST(lib14_ac37_settings_sections_and_rows) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action("settings")->trigger();
    f.run(100);
    auto* d = f.w().shell().settingsDialog();
    CHECK(d && d->isVisible() && d->windowTitle() == "Settings");
    if (!d) return;
    std::vector<std::string> sections;
    for (const auto& s : d->shown()) {
        std::string line = s.title + ":";
        for (const auto& r : s.rows) line += " " + r.row.label;
        sections.push_back(line);
    }
    std::vector<std::string> want{"General: Startup model",
                                  "Startup presets: PC-1500 PC-1500A PC-1600",
                                  "Storage: Save folder Tapes",
                                  "Folders: Examples folder Basic folder Assembly folder Host drive folder",
                                  "Tracing: Trace folder Maximum file size (MB)"};
#ifndef Q_OS_WIN
    want.push_back("Serial ports: Link folder");
#endif
    want.push_back("Debugger: Debug server (DAP) Port");
    CHECK(checkList("sections", sections, want));
    // The two status lines of Serial ports and the debug server's status.
    f.run(100);
#ifndef Q_OS_WIN
    CHECK(d->findChild<QLabel*>("serial-status-rs232c") && d->findChild<QLabel*>("serial-status-ce158"));
#endif
    CHECK(d->findChild<QLabel*>("debug-status") != nullptr);
    // A change is written at once (S2): the startup model by its name.
    auto* model = d->findChild<QComboBox*>(QString::fromStdString(names::setting("startup", "model")));
    CHECK(model != nullptr);
    if (model) {
        CHECK(model->findData(QStringLiteral("PC-1500")) >= 0);   // by the model's name (ledger 1600-35)
        model->activated(model->findData(QStringLiteral("PC-1600")));
        f.run(50);
        CHECK(AppSettings::startupModelPreference() == QStringLiteral("PC-1600"));
    }
    d->close();
}

// ── Q6: "Loading…" and notices (AC-38; AC-15 to AC-17; LIB-4 AC-20) ───

TEST(lib14_ac38_loading_after_500_ms_blocks_nothing) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    // A preset that runs at full speed for long (DiskWorks boots its disk).
    f.w().host().send(calcu::sessions::CommandName{"load-preset"},
                      Value::object({{"ref", "examples:plotter/lissajou-1600.pc1600"}}));
    f.run(300);
    const bool busyAt300 = f.busy();
    CHECK(busyAt300 && !f.w().shell().loadingShown());
    if (!busyAt300) return;
    f.waitFor([&] { return f.w().shell().loadingShown() || !f.busy(); }, 1500);
    CHECK(f.w().shell().loadingShown());
    // Nothing blocked: the model picker, Reset and Cold boot can be used.
    CHECK(f.find<QComboBox>(names::kBarModel)->isEnabled());
    CHECK(f.action("reset")->isEnabled() && f.action("cold-boot")->isEnabled());
    // LIB-4 AC-20: the peripherals' and the variants' choices are not.
    CHECK(!f.action("variant-model-Old")->isEnabled());
    f.action("reset")->trigger();   // ends the operation (LIB-18 AC-16)
    CHECK(f.waitFor([&] { return !f.busy(); }, 10000));
    f.run(50);
    CHECK(!f.w().shell().loadingShown());
}

TEST(lib4_ac20_qt_controls_disabled_while_loading) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    f.command("attach", Value::object({{"place", "printer"}, {"peripheral", "CE-1600P"}}));
    f.w().host().send(calcu::sessions::CommandName{"load-preset"},
                      Value::object({{"ref", "examples:plotter/lissajou-1600.pc1600"}}));
    CHECK(f.waitFor([&] { return f.busy(); }, 2000));
    f.run(100);
    CHECK(f.busy());
    CHECK(!f.find<QToolButton>(names::barPlace("interface"))->isEnabled());
    auto* printer = f.find<QComboBox>(names::barPlace("printer"));
    CHECK(printer && printer->isEnabled());   // the place as a whole; its choices are disabled
    CHECK(!f.action("peripheral-interface-CE-158")->isEnabled());
    CHECK(f.find<QComboBox>(names::kBarModel)->isEnabled());
    f.action("cold-boot")->trigger();
    CHECK(f.waitFor([&] { return !f.busy(); }, 10000));
}

TEST(lib14_ac38_notice_without_title) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.w().shell().sendChecked(calcu::sessions::CommandName{"load-preset"}, Value::object({{"ref", "/no/such/preset.pc1500"}}));
    CHECK(f.waitFor([&] { return f.find<QMessageBox>(names::kNotice) != nullptr; }, 5000));
    auto* box = f.find<QMessageBox>(names::kNotice);
    if (!box) return;
    CHECK(box->windowTitle().isEmpty() && !box->text().isEmpty());
    if (auto* ok = box->findChild<QAbstractButton*>(names::kNoticeOk)) ok->click();
    f.run(50);
}

// ── Q7: Help and About (AC-39) ─────────────────────────────────────────

TEST(lib14_ac39_help_opens_calc_u_org_1600) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action("help")->trigger();
    CHECK(f.opened.size() == 1 && f.opened[0] == QUrl(QStringLiteral("https://calc-u.org/1600/")));
}

TEST(lib14_ac39_about_as_today) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action("about")->trigger();
    f.run(50);
    auto* d = f.find<QDialog>(names::kAboutDialog);
    CHECK(d && d->isVisible());
    if (!d) return;
    CHECK(d->findChild<QLabel*>("about-name")->text().contains(QStringLiteral("Calc-U-1600")));
    CHECK(d->findChild<QLabel*>("about-version")->text().startsWith(QStringLiteral("Version ")));
    bool summary = false;
    for (QLabel* l : d->findChildren<QLabel*>())
        summary = summary || l->text().contains(QStringLiteral("pocket-computer emulator"));
    CHECK(summary);
    d->close();
}

// ── Q8: the layout (AC-40) ─────────────────────────────────────────────

TEST(lib14_ac40_layout_faceplate_bar_panel_row) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QWidget* face = f.w().faceplate();
    QWidget* bar = f.find<QWidget>(names::kControlBar);
    calcu::qt6::PanelArea* panels = f.w().shell().panelArea();
    CHECK(face->geometry().bottom() < bar->geometry().top() && bar->geometry().bottom() < panels->geometry().top());
    QWidget* inspector = f.w().inspector();
    QWidget* paper = panels->panel("paper");
    CHECK(inspector && inspector->isVisible() && paper && paper->isVisible());
    if (inspector && paper) CHECK(inspector->x() < paper->x());
    auto* ce158 = f.find<QWidget>("ce158printer");
    CHECK(ce158 && !ce158->isVisible());
    f.command("attach", Value::object({{"place", "interface"}, {"peripheral", "CE-158"}}));
    CHECK(f.waitFor([&] { return ce158->isVisible(); }, 3000));
    if (paper) CHECK(paper->x() < ce158->x());
}

// ── Q9: Copy Screen (AC-41) ────────────────────────────────────────────

TEST(lib14_ac41_copy_screen_picture_and_text) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    f.command("type", Value::object({{"text", "PRINT 815"}}));
    f.waitFor([&] {
        LcdText t;
        return f.w().controller()->lcdText(&t) && t.text().find("815") != std::string::npos;
    }, 3000);
    f.action("copy-screen")->trigger();
    CHECK(!f.clipboardImage.isNull() && f.clipboardImage.dotsPerMeterX() > 0);
    CHECK(f.clipboardText.contains(QStringLiteral("815")));
}

// ── Q10: the faceplate's input (AC-24, AC-43) ──────────────────────────

namespace {
void click(QWidget* w, QPoint at, bool down) {
    QMouseEvent e(down ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease, QPointF(at), w->mapToGlobal(QPointF(at)),
                  Qt::LeftButton, down ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(w, &e);
}
std::vector<std::string> held(Fixture& f) {
    std::vector<std::string> out;
    const Value v = f.w().host().value(calcu::typing::kHeldKeysTopic);
    for (const auto& k : calcu::shell::read::elements(v, "live"))
        if (k.string()) out.push_back(*k.string());
    return out;
}
} // namespace

TEST(lib14_ac24_key_under_pointer_pc1500_pc1600) {
    SKIP_UNLESS_WINDOW();
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        Fixture f(model);
        FaceplateWidget* face = f.w().faceplate();
        // A key's centre: that key, held while the pointer is down.
        const QPoint e = face->keyCentre(QStringLiteral("enter"));
        CHECK(e.x() > 0);
        click(face, e, true);
        CHECK(f.waitFor([&] { return held(f) == std::vector<std::string>{"enter"}; }, 1000));
        click(face, e, false);
        CHECK(f.waitFor([&] { return held(f).empty(); }, 1000));
        // A point in the gap below a key, within the distance: that key
        // (ledger 1600-37).
        const calcu::shell::HitMap map = shared::faceplateHitMap(calcu::machine::ModelId{model});
        const double s = face->artScale();
        for (const auto& [id, r] : map.keys) {
            if (id.value != "enter") continue;
            const QPoint gap(static_cast<int>((r.x + r.w / 2) * s), static_cast<int>((r.y + r.h) * s) + 6);
            CHECK(calcu::shell::keyAt(map, s, gap.x(), gap.y()) == calcu::machine::KeyId{"enter"});
        }
    }
}

TEST(lib14_ac43_display_hold_full_speed) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    FaceplateWidget* face = f.w().faceplate();
    const QPoint lcd = face->lcdWidget()->geometry().center();
    click(face, lcd, true);
    CHECK(f.waitFor([&] { return calcu::shell::read::text(f.w().host().value(calcu::sessions::kBusyTopic), "speed") == "full"; }, 1000));
    click(face, lcd, false);
    CHECK(f.waitFor([&] { return calcu::shell::read::text(f.w().host().value(calcu::sessions::kBusyTopic), "speed") == "regular"; }, 1000));
}

// ── Q12: the shot runner on attach (AC-44) ─────────────────────────────

#ifdef CALCU_ATTACH
TEST(lib14_ac44_scenario_through_attach) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    auto product = shared::makeHarnessProduct();
    CHECK(f.w().host().openAttach(*product, 0,
                                  f.w().controller()->app().libraryParts())
              .empty());
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("t.shots.yaml"));
    {
        std::ofstream out(path.toStdString());
        out << "out: shots\n"
               "shots:\n"
               "  - name: one\n"
               "    preset: examples:startup/default-pc1600.pc1600\n"
               "    steps:\n"
               "      - key: cl\n"
               "      - type: PRINT 4711\n"
               "      - wait-idle\n"
               "      - expect-lcd: \"4711\"\n"
               "    capture: { target: lcd, file: one.png }\n"
               "  - name: two\n"
               "    steps:\n"
               "      - cold-boot\n"
               "      - wait-idle\n"
               "      - expect-no-lcd: \"4711\"\n"
               "    capture: { target: control-bar, file: two.png }\n";
    }
    ShotScenario scenario;
    QString error;
    CHECK(parseShotScenario(path, &scenario, &error));
    if (!error.isEmpty()) std::fprintf(stderr, "  %s\n", qPrintable(error));
    int exit = -1;
    ShotRunner runner(&f.w(), scenario, ShotRunner::Options{}, f.w().host().attachPort());
    QObject::connect(&runner, &ShotRunner::finished, [&exit](int code) { exit = code; });
    runner.start();
    CHECK(f.waitFor([&] { return exit >= 0; }, 60000));
    CHECK_EQ(exit, 0);
    CHECK(QFileInfo::exists(dir.filePath(QStringLiteral("shots/one.png"))) && QFileInfo::exists(dir.filePath(QStringLiteral("shots/two.png"))));
}
#endif

// ── Q13: ROMs missing (AC-19 for this product) ─────────────────────────

TEST(lib14_ac19_roms_missing_in_window) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SKIP_UNLESS(romsMissingRun(), "run by CalcU1600QtTests.romsMissing");
    Fixture f("PC-1600", /*started=*/false);
    f.run(500);
    QWidget* missing = f.w().shell().romsMissingView();
    CHECK(missing && missing->isVisible());
    CHECK(!f.w().faceplate()->isVisible());
    CHECK(!f.find<QWidget>(names::kControlBar)->isVisible());
    CHECK(checkList("menus", menuTitles(f.w().menuBar()), {"File", "Help"}));
    auto* label = qobject_cast<QLabel*>(missing);
    CHECK(label && label->text().contains(QStringLiteral("PC1600-P2-B6-new.bin")));
    f.action("about")->trigger();
    f.run(50);
    CHECK(f.find<QDialog>(names::kAboutDialog) && f.find<QDialog>(names::kAboutDialog)->isVisible());
    f.action("help")->trigger();
    CHECK(f.opened.size() == 1);
    CHECK(f.w().isVisible());   // the app runs on (ledger 1600-36)
}

// ── Q15: C1600-2 AC-15, the two File items ─────────────────────────────

TEST(c1600_2_ac15_file_items_and_filters) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action(shared::kItemLoadBasic)->trigger();
    f.action(shared::kItemLoadCode)->trigger();
    CHECK(f.fileAsks.size() == 2);
    if (f.fileAsks.size() != 2) return;
    CHECK(f.fileAsks[0] == "Load BASIC Program" && f.fileFilters[0].startsWith(QStringLiteral("BASIC Programs (*.bas *.bbin *.wav)")));
    CHECK(f.fileAsks[1] == "Load Machine Code" && f.fileFilters[1].startsWith(QStringLiteral("Machine Code (*.bin *.wav)")));
    // The start folder of [folders]: the one the dialog of its kind last used.
    f.fileAnswer = QString::fromStdString(std::string(CALCU1600_DIR) + "/examples/basic/hanoi.bas");
    answerNextModal();
    f.action(shared::kItemLoadBasic)->trigger();
    f.run(300);
    f.fileAnswer.clear();
    f.action(shared::kItemLoadBasic)->trigger();
    CHECK(f.fileStarts.back().endsWith(QStringLiteral("examples/basic")));
}

// ── LIB-13 AC-18, AC-19, AC-21 (the Qt6 hand checks, user 2026-11-16) ──

namespace {
const QString kExample = QString::fromStdString(std::string(CALCU1600_DIR) + "/examples/basic/hanoi.pc1600");
} // namespace

TEST(lib13_ac19_drop_onto_window_opens_preset) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1500A");
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(kExample)});
    QDragEnterEvent enter(QPoint(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&f.w(), &enter);
    CHECK(enter.isAccepted());
    QDropEvent drop(QPointF(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&f.w(), &drop);
    CHECK(f.waitFor([&] { return f.model() == "PC-1600"; }, 10000));
    f.waitIdle();
}

TEST(lib13_ac21_open_with_and_dock_drop_open_preset) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1500A");
    // The Dock icon and Finder's Open With reach the app as QFileOpenEvent.
    calcu::qt6::FileOpenFilter filter;
    filter.setHandler([&f](const QString& p) { f.w().openFile(p, calcu::opening::Way::OpenWith); });
    qApp->installEventFilter(&filter);
    QFileOpenEvent e(kExample);
    QCoreApplication::sendEvent(qApp, &e);
    qApp->removeEventFilter(&filter);
    CHECK(f.waitFor([&] { return f.model() == "PC-1600"; }, 10000));
    f.waitIdle();
}

TEST(lib13_ac21_open_with_bin_runs_the_machine_code_flow) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    const std::string bin = std::string(CALCU1600_DIR) + "/examples/machine-code/CALCULAT.BIN";
    SKIP_UNLESS(std::filesystem::exists(bin), "no machine-code fixture");
    // The flow's dialogs: the address (headerless) or the result; answered.
    answerNextModal();
    QTimer::singleShot(400, [] { answerNextModal(); });
    f.w().openFile(QString::fromStdString(bin), calcu::opening::Way::OpenWith);
    f.run(1500);
    CHECK(QApplication::activeModalWidget() == nullptr);
}

TEST(lib13_ac21_start_with_a_file_name) {
    SKIP_UNLESS_WINDOW();
    // main.cpp: the last argument that names a file, opened as the start's.
    const std::vector<std::string> args{"Calc-U-1600", "-NSDocumentRevisionsDebugMode", "YES", kExample.toStdString()};
    const auto start = calcu::opening::startFile(args, [](const std::string& p) { return std::filesystem::exists(p); });
    CHECK(start && *start == kExample.toStdString());
    Fixture f("PC-1500A", /*started=*/false);
    f.w().openFile(QString::fromStdString(*start), calcu::opening::Way::Start);
    CHECK(f.waitFor([&] { return f.model() == "PC-1600"; }, 15000));
    f.waitIdle();
}

// ── C1600-2 AC-20: the two Qt6 scenarios (AC-16, AC-18) ────────────────

namespace {

// Inspects the next modal dialog once it shows, then rejects it.
void inspectNextModal(std::function<void(QWidget*)> inspect) {
    QTimer::singleShot(50, [inspect = std::move(inspect)] {
        for (int i = 0; i < 300; ++i) {
            if (QWidget* m = QApplication::activeModalWidget()) {
                inspect(m);
                if (auto* d = qobject_cast<QDialog*>(m)) d->reject();
                return;
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
        inspect(nullptr);
    });
}

std::vector<char> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The tape's data chunk twice: a WAV with two files (as loader_part_tests).
std::vector<char> doubledTape(const std::vector<char>& wav) {
    auto u8 = [&](std::size_t i) { return static_cast<std::uint32_t>(static_cast<unsigned char>(wav[i])); };
    for (std::size_t i = 12; i + 8 <= wav.size();) {
        const std::uint32_t size = u8(i + 4) | u8(i + 5) << 8 | u8(i + 6) << 16 | u8(i + 7) << 24;
        if (std::equal(wav.begin() + static_cast<long>(i), wav.begin() + static_cast<long>(i) + 4, "data")) {
            std::vector<char> out(wav.begin(), wav.begin() + static_cast<long>(i + 8 + size));
            out.insert(out.end(), wav.begin() + static_cast<long>(i + 8), wav.begin() + static_cast<long>(i + 8 + size));
            const std::uint32_t doubled = size * 2, riff = static_cast<std::uint32_t>(out.size() - 8);
            for (int b = 0; b < 4; ++b) {
                out[i + 4 + static_cast<std::size_t>(b)] = static_cast<char>(doubled >> (8 * b));
                out[4 + static_cast<std::size_t>(b)] = static_cast<char>(riff >> (8 * b));
            }
            return out;
        }
        i += 8 + size + (size & 1);
    }
    return {};
}

} // namespace

TEST(c1600_2_ac16_qt_wav_with_several_programs_asks_which_one) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1500A");
    QTemporaryDir dir;
    const QString wav = dir.filePath(QStringLiteral("two.wav"));
    {
        const std::vector<char> two = doubledTape(readFile(std::string(CALCU1600_DIR) + "/tests/fixtures/tape/pc1500_tape.wav"));
        std::ofstream(wav.toStdString(), std::ios::binary).write(two.data(), static_cast<std::streamsize>(two.size()));
    }
    f.fileAnswer = wav;
    int items = -1;
    inspectNextModal([&items](QWidget* m) {
        if (auto* box = m ? m->findChild<QComboBox*>() : nullptr) items = box->count();
        else if (auto* list = m ? m->findChild<QAbstractItemView*>() : nullptr) items = list->model()->rowCount();
    });
    f.action(shared::kItemLoadBasic)->trigger();
    f.run(100);
    CHECK_EQ(items, 2);
}

TEST(c1600_2_ac18_qt_headerless_code_asks_the_address) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1500A");
    QTemporaryDir dir;
    const QString bin = dir.filePath(QStringLiteral("rtn.bin"));
    {
        const char code[] = {static_cast<char>(0xFD), static_cast<char>(0x88), static_cast<char>(0x9A), static_cast<char>(0x9A)};
        std::ofstream(bin.toStdString(), std::ios::binary).write(code, sizeof code);
    }
    f.fileAnswer = bin;
    QString title;
    bool addressField = false;
    inspectNextModal([&](QWidget* m) {
        if (!m) return;
        title = m->windowTitle();
        addressField = m->findChild<QLineEdit*>() != nullptr;
    });
    f.action(shared::kItemLoadCode)->trigger();
    f.run(100);
    CHECK(addressField);
    if (!addressField) std::fprintf(stderr, "  dialog: %s\n", qPrintable(title));
}

TEST(lib13_ac19_drop_disk_file_used_where_it_lies) {
    SKIP_UNLESS_WINDOW();
    Fixture f("PC-1600");
    f.command("attach", Value::object({{"place", "printer"}, {"peripheral", "CE-1600P"}}));
    const QString disk = QString::fromStdString(std::string(CALCU1600_DIR) + "/examples/dwx/Progs.floppy.yaml");
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(disk)});
    QDragEnterEvent enter(QPoint(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&f.w(), &enter);
    CHECK(enter.isAccepted());
    QDropEvent drop(QPointF(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&f.w(), &drop);
    // The disk in the CE-1600F, by the name of the file it lies in.
    CHECK(f.waitFor([&] {
        auto* picker = f.find<QComboBox>("media-picker-floppy");
        return picker && picker->currentText().contains(QStringLiteral("Progs"));
    }, 10000));
}
