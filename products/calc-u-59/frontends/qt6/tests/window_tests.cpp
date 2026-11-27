// CalcU59QtTests, the window: the Qt6 window of Calc-U-59 put together from
// the shell, on the real ROMs, offscreen: the menus, the bar, settings,
// notices, Help and About, the layout, the faceplate's input, magnetic
// cards, ROMs missing, the bundled examples, drop and Open With, the debug
// panel and attach. The Web cases of the same scenarios are in
// frontends/web/tests/shell.spec.js.
#include "TestHarness.hpp"
#include "QtFixture.hpp"
#include "AppPaths.hpp"
#include "CardControls.hpp"
#include "KeyIds.hpp"
#include "EmulatorController.hpp"
#include "KeyboardWidget.hpp"
#include "QtResourceBundle.hpp"
#include "TIShellProduct.hpp"
#include "debug/DebugPanel.hpp"
#include "qt6/FileOpening.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/Shell.hpp"
#include "qt6/ShellSettingsDialog.hpp"
#include "media/MediaTopics.hpp"
#include "sessions/LibraryTopics.hpp"
#include "shell/HitMap.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"
#include "typing/TypingTopics.hpp"
#include <optional>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDropEvent>
#include <QFileOpenEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QTemporaryDir>
#include <QToolButton>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#ifdef CALCU_ATTACH
#include "HarnessProduct.hpp"
#include "harness/Product.hpp"
#include "qt6/AttachLineClient.hpp"
#endif

using qtfixture::Fixture;
using qtfixture::romsPresent;
namespace names = calcu::shell::names;
using calcu::sessions::Value;

#define SKIP_UNLESS_WINDOW() SKIP_UNLESS(romsPresent(), "no ROMs in roms/")

namespace {

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
    for (QAction* a : bar->actions())
        if (a->isVisible()) out.push_back(text(a).toStdString());
    return out;
}

bool checkList(const char* what, const std::vector<std::string>& got, const std::vector<std::string>& want) {
    if (got == want) return true;
    std::fprintf(stderr, "  %s:", what);
    for (const auto& s : got) std::fprintf(stderr, " [%s]", s.c_str());
    std::fprintf(stderr, "\n");
    return false;
}

std::vector<std::string> held(Fixture& f) {
    std::vector<std::string> out;
    if (const auto* h = f.w().host().topics().stateOf<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic))
        for (const auto& k : h->live) out.push_back(k.value);
    return out;
}

std::string speed(Fixture& f) { return calcu::shell::read::text(f.w().host().value(calcu::sessions::kBusyTopic), "speed"); }

void mouse(QWidget* w, QPointF at, bool down) {
    QMouseEvent e(down ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease, at, w->mapToGlobal(at), Qt::LeftButton,
                  down ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w, &e);
}

// A point of the base image (2064 × 4075 pixels) on the faceplate as shown.
QPointF onFaceplate(KeyboardWidget* k, double x, double y) { return k->origin() + QPointF(x, y) * k->scale(); }

QPointF keyCentre(KeyboardWidget* k, const std::string& key) {
    const calcu::shell::HitMap map = cu59::faceplateHitMap(calcu::machine::ModelId{"TI-59"});
    for (const auto& [id, r] : map.keys)
        if (id.value == key) return onFaceplate(k, r.x + r.w / 2, r.y + r.h / 2);
    return {};
}

std::vector<QString> barButtonTexts(Fixture& f) {
    std::vector<QString> out;
    if (auto* bar = f.find<QWidget>(names::kControlBar))
        for (QAbstractButton* b : bar->findChildren<QAbstractButton*>())
            if (b->isVisibleTo(bar)) out.push_back(b->text());
    return out;
}

} // namespace

// ── The menus ─────────────────────────────────────────────────────────────

TEST(menus_in_family_order) {
    SKIP_UNLESS_WINDOW();
    Fixture f("TI-59");
    CHECK(checkList("menus", menuTitles(f.w().menuBar()), {"File", "Machine", "Help"}));   // no Edit
    CHECK(checkList("File", labels(f.find<QMenu>(names::menu("file"))),
                    {"Load Preset…", "Examples ▸", "-", "Arm Card…", "Arm Fresh Card…", "-", "Settings…", "Quit"}));
    CHECK(checkList("Machine", labels(f.find<QMenu>(names::menu("machine"))),
                    {"Model ▸", "Peripherals ▸", "-", "Reset", "Cold boot", "Resume"}));
    CHECK(checkList("Help", labels(f.find<QMenu>(names::menu("help"))), {"Calc-U-59 Help", "Show App Log", "About Calc-U-59"}));
    CHECK(checkList("Model", labels(f.find<QMenu>(names::item("model"))), {"TI-59", "TI-58", "TI-58C"}));
    // The module slot is the bar's picker (M6): Peripherals ▸ has the PC-100C only.
    CHECK(checkList("Peripherals", labels(f.find<QMenu>(names::item("peripherals"))), {"PC-100C"}));
    CHECK(f.action("resume") && !f.action("resume")->isEnabled());
    // The card items exist on the TI-59 only.
    f.command("model", Value::object({{"name", "TI-58"}}));
    f.run(200);
    CHECK(checkList("File TI-58", labels(f.find<QMenu>(names::menu("file"))),
                    {"Load Preset…", "Examples ▸", "-", "Settings…", "Quit"}));
}

TEST(shortcuts_ctrl_o_r_shift_r) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    CHECK(f.action("load-preset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_O));
    CHECK(f.action("reset")->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_R));
    CHECK(f.action("cold-boot")->shortcut() == QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    // Load Preset… asks with the product's filter in the presets' start folder.
    f.fileAnswer.clear();
    f.action("load-preset")->trigger();
    CHECK(f.fileFilters.size() == 1 && f.fileFilters[0] == QStringLiteral("Presets (*.ti59 *.ti58 *.ti58c)"));
}

TEST(menu_roles_settings_about_quit) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    CHECK(f.action("settings")->menuRole() == QAction::PreferencesRole);
    CHECK(f.action("about")->menuRole() == QAction::AboutRole);
    CHECK(f.action("quit")->menuRole() == QAction::QuitRole);
    CHECK(text(f.action("load-preset")) == QStringLiteral("Load Preset…"));   // was "Open Preset…"
}

// ── The control bar ───────────────────────────────────────────────────────

TEST(control_bar_per_model) {
    SKIP_UNLESS_WINDOW();
    Fixture f("TI-59");
    auto* model = f.find<QComboBox>(names::kBarModel);
    CHECK(model != nullptr);
    if (!model) return;
    CHECK(model->count() == 3 && model->itemText(0) == QStringLiteral("59") && model->currentText() == QStringLiteral("59"));
    // The picker of slot-1: the 14 modules and the empty slot.
    auto* slot = f.find<QWidget>(names::barSlot("slot-1"));
    auto* picker = slot ? slot->findChild<calcu::qt6::MediaPicker*>() : nullptr;
    CHECK(picker != nullptr);
    if (picker) CHECK_EQ(picker->count(), 15);
    // The button "PC-100C" for the place `printer`, on the paper panel,
    // pressed while plugged in.
    auto* printer = f.find<QToolButton>(names::barPlace("printer"));
    CHECK(printer && printer->text() == QStringLiteral("PC-100C") && printer->isCheckable() && !printer->isChecked());
    auto* bar = f.find<QWidget>(names::kControlBar);
    CHECK(printer && bar && !bar->isAncestorOf(printer));
    if (printer) printer->click();
    CHECK(f.waitFor([&] {
        auto* p = f.find<QToolButton>(names::barPlace("printer"));
        return p && p->isChecked();
    }, 5000));
    f.waitIdle();
    f.find<QToolButton>(names::barPlace("printer"))->click();
    CHECK(f.waitFor([&] { return !f.c().printerConnected(); }, 5000));
    f.waitIdle();
    // The card controls on the TI-59 only.
    CHECK(f.find<CardControls>(names::barMedia("card-reader")) != nullptr);
    model = f.find<QComboBox>(names::kBarModel);   // the bar is drawn anew at each change
    CHECK(model != nullptr);
    if (!model) return;
    model->setCurrentIndex(1);
    emit model->activated(1);   // TI-58 by the picker
    CHECK(f.waitFor([&] { return f.model() == "TI-58"; }, 10000));
    f.waitIdle();
    f.run(100);
    CHECK(f.find<CardControls>(names::barMedia("card-reader")) == nullptr);
    CHECK(f.find<QWidget>(names::barSlot("slot-1")) != nullptr);
}

// ── Buttons gone where there is a menu bar ────────────────────────────────

TEST(no_reset_preset_settings_on_the_bar) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    for (const QString& t : barButtonTexts(f)) {
        CHECK(!t.contains(QStringLiteral("Reset")) && !t.contains(QStringLiteral("Preset")));
        CHECK(!t.contains(QStringLiteral("Settings")) && !t.contains(QStringLiteral("Clean")));
    }
    CHECK(!barButtonTexts(f).empty());   // the bar is there, with the cards
}

TEST(cold_boot_is_the_menu_item) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    const std::vector<QString> before = barButtonTexts(f);
    // "Clean" is Machine ▸ Cold boot: it runs the cold boot as an operation.
    CHECK(text(f.action("cold-boot")) == QStringLiteral("Cold boot"));
    f.action("cold-boot")->trigger();
    CHECK(f.waitFor([&] { return f.busy(); }, 3000));
    CHECK(f.waitIdle());
    // Nothing on the bar changes with a modifier: there is no Reset button to turn into Clean.
    f.run(100);
    CHECK(barButtonTexts(f) == before);
}

// ── Settings ──────────────────────────────────────────────────────────────

TEST(settings_sections_and_rows) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action("settings")->trigger();
    f.run(100);
    auto* d = f.find<QDialog>(names::kSettingsDialog);
    CHECK(d && d->isVisible());
    if (!d) return;
    CHECK(d->windowTitle() == QStringLiteral("Settings"));
    std::vector<std::string> sections;
    for (QGroupBox* g : d->findChildren<QGroupBox*>()) sections.push_back(g->title().toStdString());
    CHECK(checkList("sections", sections, {"General", "Startup presets", "Storage", "Folders", "Tracing", "Display", "App log"}));
    for (const auto& [section, key] : std::vector<std::pair<std::string, std::string>>{
             {"startup", "model"}, {"preset", "TI-59"}, {"preset", "TI-58"}, {"preset", "TI-58C"},
             {"storage", "save-folder"}, {"folders", "presets"}, {"folders", "asm-overlays"}, {"trace", "folder"}, {"trace", "max-file-size-mb"},
             {"display", "led-style"}})
        CHECK(d->findChild<QWidget*>(QString::fromStdString(names::setting(section, key))) != nullptr);
    // No Solid State Module row (the module is chosen on the bar).
    for (QWidget* w : d->findChildren<QWidget*>()) CHECK(!w->objectName().contains(QStringLiteral("module")));
    // General: "Startup model" with "Last used" first; the version line.
    auto* startup = d->findChild<QComboBox*>(QString::fromStdString(names::setting("startup", "model")));
    CHECK(startup && startup->itemText(0) == QStringLiteral("Last used"));
    bool version = false;
    for (QLabel* l : d->findChildren<QLabel*>()) version = version || l->text().startsWith(QStringLiteral("Version "));
    CHECK(version);
    // Segment Style, labelled; a change written at once, and the faceplate follows.
    auto* style = d->findChild<QComboBox*>(QString::fromStdString(names::setting("display", "led-style")));
    CHECK(style && style->count() == 2 && style->itemText(0) == QStringLiteral("Modernized") &&
          style->itemText(1) == QStringLiteral("Classic"));
    if (style) emit style->activated(1);
    CHECK(f.waitFor([&] { return f.c().ledStyle() == cu59::LedStyle::Classic; }, 3000));
    CHECK_EQ(AppPaths::file().text("display", "led-style"), std::string("classic"));
    if (style) emit style->activated(0);
    CHECK(f.waitFor([&] { return f.c().ledStyle() == cu59::LedStyle::Modernized; }, 3000));
    d->close();
}

// ── "Loading…" and notices ────────────────────────────────────────────────

TEST(loading_after_500_ms_blocks_nothing) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    // A preset that keeps an operation at full speed for a while: a program
    // that loops (Lbl A, GTO A) runs while the preset waits.
    auto reply = std::make_shared<std::optional<calcu::sessions::ReplyMessage>>();
    f.w().host().send(calcu::sessions::CommandName{"load-preset"},
                      Value::object({{"content", "format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n    76 11 61 11\nkeys:\n  - full-speed:\n  - key: A\n  - wait: 600s\n"},
                                     {"name", "long.ti59"}}),
                      [reply](const calcu::sessions::ReplyMessage& r) { *reply = r; });
    f.run(300);
    const bool busyAt300 = f.busy();
    CHECK(busyAt300 && !f.w().shell().loadingShown());
    if (!busyAt300) return;
    f.waitFor([&] { return f.w().shell().loadingShown() || !f.busy(); }, 1500);
    CHECK(f.w().shell().loadingShown());
    // Nothing blocked: the model picker, Reset and Cold boot can be used.
    CHECK(f.find<QComboBox>(names::kBarModel)->isEnabled());
    CHECK(f.action("reset")->isEnabled() && f.action("cold-boot")->isEnabled());
    f.action("reset")->trigger();   // ends the operation
    CHECK(f.waitFor([&] { return !f.busy(); }, 10000));
    f.run(50);
    CHECK(!f.w().shell().loadingShown());
}

TEST(notice_without_title) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.w().shell().sendChecked(calcu::sessions::CommandName{"load-preset"}, Value::object({{"ref", "/no/such/preset.ti59"}}));
    CHECK(f.waitFor([&] { return f.find<QMessageBox>(names::kNotice) != nullptr; }, 5000));
    auto* box = f.find<QMessageBox>(names::kNotice);
    if (!box) return;
    CHECK(box->windowTitle().isEmpty() && !box->text().isEmpty());
    if (auto* ok = box->findChild<QAbstractButton*>(names::kNoticeOk)) ok->click();
    f.run(50);
}

// ── Help and About ────────────────────────────────────────────────────────

TEST(help_opens_calc_u_org_59) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    f.action("help")->trigger();
    CHECK(f.opened.size() == 1 && f.opened[0] == QUrl(QStringLiteral("https://calc-u.org/59/")));
}

TEST(about_icon_name_version_summary_links) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QPixmap pix(32, 32);
    pix.fill(Qt::darkGreen);
    const QIcon before = QApplication::windowIcon();
    QApplication::setWindowIcon(QIcon(pix));   // the app's icon (main.cpp sets :/app/icon.png)
    f.action("about")->trigger();
    f.run(50);
    auto* d = f.find<QDialog>(names::kAboutDialog);
    CHECK(d && d->isVisible());
    if (!d) return;
    CHECK(d->windowTitle() == QStringLiteral("About Calc-U-59"));
    CHECK(d->findChild<QLabel*>("about-icon") != nullptr);
    CHECK(d->findChild<QLabel*>("about-name")->text().contains(QStringLiteral("Calc-U-59")));
    CHECK(d->findChild<QLabel*>("about-version")->text().startsWith(QStringLiteral("Version ")));
    QStringList texts;
    for (QLabel* l : d->findChildren<QLabel*>()) texts << l->text();
    const QString all = texts.join(QLatin1Char('\n'));
    CHECK(all.contains(QStringLiteral("Calc-U-59 Calculator Emulation")));
    CHECK(all.contains(QStringLiteral("Cycle-accurate TI-59")));
    CHECK(all.contains(QStringLiteral("https://github.com/tinue/Calc-U\"")));
    CHECK(all.contains(QStringLiteral("issues")) && all.contains(QStringLiteral("PRIVACY.md")));
    CHECK(all.contains(QStringLiteral("Built with Qt (LGPL-3.0).")));
    CHECK(all.contains(QStringLiteral("Package: ")));   // self-built here, the package jobs set theirs
    CHECK(all.contains(QStringLiteral("Built: 20")));    // generated/BuildTime.cpp
    d->close();
    QApplication::setWindowIcon(before);
}

// ── The layout (and one picture of the window) ────────────────────────────

TEST(three_columns_with_dividers) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QSplitter* s = f.w().splitter();
    CHECK(s && s->count() == 3);
    if (!s || s->count() != 3) return;
    CHECK(s->widget(0)->objectName() == QStringLiteral("calculator"));
    CHECK(s->widget(1) == f.w().shell().panelArea());
    CHECK(s->widget(2)->objectName() == QStringLiteral("debugpanel"));
    QWidget* paper = f.w().shell().panelArea()->panel("paper");
    CHECK(paper && paper->isVisible());   // the paper panel always
    // The bar below the faceplate.
    QWidget* bar = f.find<QWidget>(names::kControlBar);
    CHECK(bar && f.w().keyboard()->geometry().bottom() <= bar->geometry().top());
    // The dividers move.
    const QList<int> sizes = s->sizes();
    s->setSizes({sizes[0] + 40, sizes[1], sizes[2] - 40});
    f.run(50);
    CHECK(s->sizes()[0] != sizes[0]);
    // The picture of the window, into the test's output folder.
    const QString out = QStringLiteral(CALCU59_QT_TEST_OUT);
    QDir().mkpath(out);
    CHECK(f.w().grab().save(out + QStringLiteral("/calc-u-59-qt6.png")));
}

// The view rule: a window too narrow for the printer beside the faceplate
// shows one page at a time, the chevron switching them; a short one leaves
// the debugger out.
TEST(narrow_window_pages_short_window_no_debugger) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QWidget* calculator = f.w().splitter()->widget(0);
    QWidget* panels = f.w().shell().panelArea();
    QWidget* debug = f.w().splitter()->widget(2);
    CHECK(!f.w().paged() && debug->isVisible());
    f.w().resize(1200, 500);
    f.run(50);
    CHECK(!f.w().paged() && panels->isVisible() && !debug->isVisible());
    f.w().resize(400, 900);
    f.run(50);
    CHECK(f.w().paged());
    CHECK(calculator->isVisible() && !panels->isVisible() && !debug->isVisible());
    QToolButton* chevron = f.w().pageChevron();
    CHECK(chevron && chevron->isVisible());
    // The bar right below the faceplate, whatever the page's height.
    QWidget* bar = f.find<QWidget>(names::kControlBar);
    CHECK(bar && bar->geometry().top() - f.w().keyboard()->geometry().bottom() <= 2);
    chevron->click();
    f.run(50);
    CHECK(!calculator->isVisible() && panels->isVisible());
    chevron->click();
    f.run(50);
    CHECK(calculator->isVisible() && !panels->isVisible());
    f.w().resize(1200, 900);
    f.run(50);
    CHECK(!f.w().paged() && calculator->isVisible() && panels->isVisible() && debug->isVisible() && !chevron->isVisible());
}

// The paper in its own window: the splitter keeps the calculator and the
// debugger, which takes the room; the debugger beside the faceplate in a
// window too narrow for three columns; no pages. Back: three columns.
TEST(paper_own_window_debugger_takes_the_room) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    calcu::qt6::PanelArea* area = f.w().shell().panelArea();
    QWidget* paper = area->panel("paper");
    QWidget* debug = f.w().splitter()->widget(2);
    auto* button = paper->findChild<QToolButton*>(QStringLiteral("paper-window"));
    CHECK(button && button->isVisible());
    if (!button) return;
    f.w().resize(760, 900);   // the printer beside the faceplate, no room for the debugger too
    f.run(50);
    CHECK(!f.w().paged() && area->isVisible() && !debug->isVisible());
    button->click();
    f.run(50);
    CHECK(area->paperOwnWindow() && paper->isVisible() && paper->window() != &f.w());
    CHECK(!area->isVisible() && debug->isVisible() && !f.w().paged());
    f.w().resize(400, 900);   // too narrow for the debugger: the calculator alone, no pages
    f.run(50);
    CHECK(!debug->isVisible() && f.w().splitter()->widget(0)->isVisible() && !f.w().pageChevron()->isVisible());
    f.w().resize(1200, 900);
    f.run(50);
    button->click();
    f.run(50);
    CHECK(!area->paperOwnWindow() && area->isVisible() && paper->window() == &f.w() && debug->isVisible());
}

// ── The faceplate's input ─────────────────────────────────────────────────

TEST(key_under_pointer_ti59_ti58) {
    SKIP_UNLESS_WINDOW();
    for (const char* model : {"TI-59", "TI-58"}) {
        Fixture f(model);
        KeyboardWidget* k = f.w().keyboard();
        // A key's centre: that key, held while the pointer is down.
        const QPointF seven = keyCentre(k, "7");
        mouse(k, seven, true);
        CHECK(f.waitFor([&] { return held(f) == std::vector<std::string>{"7"}; }, 1000));
        mouse(k, seven, false);
        CHECK(f.waitFor([&] { return held(f).empty(); }, 1000));
        // A point in a gap between two columns is no key (distance 0).
        const cu59::NRect& a = cu59::kKeyRects[5][1];   // 7 (matrix code 62)
        const cu59::NRect& b = cu59::kKeyRects[5][2];   // 8
        const double gapX = ((a.x + a.w + b.x) / 2) * cu59::kCanvasWidth;
        const double midY = (cu59::kKeyboardYStart + (a.y + a.h / 2) * cu59::kKeyboardYScale) * cu59::kCanvasHeight;
        const double ew = a.w * cu59::kHitExpandHorizontal, ewb = b.w * cu59::kHitExpandHorizontal;
        if ((b.x - ewb) - (a.x + a.w + ew) > 0) {   // the widened areas leave a gap there
            mouse(k, onFaceplate(k, gapX, midY), true);
            f.run(150);
            CHECK(held(f).empty());
            mouse(k, onFaceplate(k, gapX, midY), false);
        }
        // A point in a widened area, just right of 7's rectangle: 7.
        mouse(k, onFaceplate(k, (a.x + a.w + ew / 2) * cu59::kCanvasWidth, midY), true);
        CHECK(f.waitFor([&] { return held(f) == std::vector<std::string>{"7"}; }, 1000));
        mouse(k, onFaceplate(k, (a.x + a.w + ew / 2) * cu59::kCanvasWidth, midY), false);
        CHECK(f.waitFor([&] { return held(f).empty(); }, 1000));
    }
}

// A press on the fitted card's corner feeds it (the
// product takes it first), and a key of the top row is still a key.
TEST(press_on_the_fitted_card_feeds_it) {
    SKIP_UNLESS_WINDOW();
    Fixture f("TI-59");
    KeyboardWidget* k = f.w().keyboard();
    CHECK(f.c().armFreshCard(QStringLiteral("Qt24"), true).isEmpty());
    CHECK(f.waitFor([&] { return f.c().cardOnShow(); }, 3000));
    // The top row with a card fitted: 2nd is a key.
    const std::string topLeft = cu59::keyIdAt(cu59::matrixCode(0, 0))->value;
    mouse(k, keyCentre(k, topLeft), true);
    CHECK(f.waitFor([&] { return held(f) == std::vector<std::string>{topLeft}; }, 1000));
    mouse(k, keyCentre(k, topLeft), false);
    CHECK(f.waitFor([&] { return held(f).empty(); }, 1000));
    // The card's left top corner: side 0 passes through the reader.
    const QPointF corner = onFaceplate(k, (cu59::kCardRect.x + cu59::kCardRect.w * 0.05) * cu59::kCanvasWidth,
                                       (cu59::kCardRect.y + cu59::kCardRect.h * 0.05) * cu59::kCanvasHeight);
    mouse(k, corner, true);
    mouse(k, corner, false);
    CHECK(f.waitFor([&] { return f.c().cardPassing(); }, 3000));
    CHECK(held(f).empty());
    f.c().ejectCard();
    f.run(200);
}

// ── Magnetic cards: a fresh card written, a card read by its corner ───────

namespace {

const char* const kCardHead = "format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\nprinter: PC-100C\n";
// Lbl A 1 2 3 R/S; then CLR 1 2nd Write at regular speed: the machine waits for a card.
std::string program123Write() {
    return std::string(kCardHead) + "program:\n  keycodes: |\n    76 11 01 02 03 91\nkeys:\n  - regular-speed:\n  - key: CLR 1 Write\n";
}

// A press on the fitted card's left top corner: side 0.
void pressCardCorner(Fixture& f) {
    KeyboardWidget* k = f.w().keyboard();
    const QPointF corner = onFaceplate(k, (cu59::kCardRect.x + cu59::kCardRect.w * 0.05) * cu59::kCanvasWidth,
                                       (cu59::kCardRect.y + cu59::kCardRect.h * 0.05) * cu59::kCanvasHeight);
    mouse(k, corner, true);
    mouse(k, corner, false);
}

// Arm Fresh Card `name` (what the bar's button and its dialog do), fed by
// its corner while the program waits in `1 2nd Write`: written and given back.
bool writeFreshCard(Fixture& f, const QString& name) {
    f.command("load-preset", Value::object({{"content", program123Write()}, {"name", "write.ti59"}}));
    if (!f.waitIdle()) return false;
    f.run(500);
    if (!f.c().armFreshCard(name, true).isEmpty()) return false;
    if (!f.waitFor([&] { return f.c().cardOnShow(); }, 3000)) return false;
    pressCardCorner(f);
    if (!f.waitFor([&] { return f.c().cardPassing(); }, 3000)) return false;
    return f.waitFor([&] { return !f.c().cardPassing(); }, 20000);
}

std::optional<std::string> savedEntry(Fixture& f, const std::string& name) {
    f.c().sendCommand(calcu::media::kRefreshCommand, Value::object({}));
    f.run(100);
    for (const auto& e : f.c().savedCards())
        if (e.label == name) return e.id;
    return std::nullopt;
}

} // namespace

TEST(fresh_card_written_and_listed) {
    SKIP_UNLESS_WINDOW();
    Fixture f("TI-59");
    CHECK(writeFreshCard(f, QStringLiteral("QtProg")));
    CHECK(savedEntry(f, "QtProg").has_value());   // Arm Card lists it by its name
}

TEST(card_read_by_its_corner) {
    SKIP_UNLESS_WINDOW();
    Fixture f("TI-59");
    CHECK(writeFreshCard(f, QStringLiteral("QtRead")));
    const auto entry = savedEntry(f, "QtRead");
    CHECK(entry.has_value());
    if (!entry) return;
    // A fresh machine: no program, nothing fitted.
    f.command("load-preset", Value::object({{"content", std::string(kCardHead) + "partition: 479\n"}, {"name", "fresh.ti59"}}));
    CHECK(f.waitIdle());
    f.c().armCard(QString::fromStdString(*entry));
    CHECK(f.waitFor([&] { return f.c().cardOnShow(); }, 3000));
    pressCardCorner(f);
    CHECK(f.waitFor([&] { return f.c().cardPassing(); }, 3000));
    CHECK(f.waitFor([&] { return !f.c().cardPassing(); }, 20000));
    f.run(500);
    // The program the card holds is in program memory: Lbl A 1 2 3 R/S.
    const auto r = f.command("c59.program", Value::object({{"step", "000"}, {"count", 6}}));
    CHECK(r.done && r.result);
    if (!r.done || !r.result) return;
    std::string listing;
    const Value v = r.result->toValue();
    if (const auto* lines = v.elements())
        for (const auto& l : *lines)
            if (l.string()) listing += *l.string() + "\n";
    CHECK(listing.find("000 76") != std::string::npos && listing.find("005 91") != std::string::npos);
    if (listing.find("000 76") == std::string::npos) std::fprintf(stderr, "  program: %s\n", listing.c_str());
}

TEST(display_hold_full_speed) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    KeyboardWidget* k = f.w().keyboard();
    const QPointF display = onFaceplate(k, cu59::kCanvasWidth / 2, (cu59::kDisplayRect.y + cu59::kDisplayRect.h / 2) * cu59::kCanvasHeight);
    mouse(k, display, true);
    CHECK(f.waitFor([&] { return speed(f) == "full"; }, 1000));
    mouse(k, display, false);
    CHECK(f.waitFor([&] { return speed(f) == "regular"; }, 1000));
}

// ── ROMs missing: the window opens and names the missing files ────────────

TEST(roms_missing_in_window) {
    SKIP_UNLESS_WINDOW();
    QtResourceBundle::hideForTests("TMC0583.txt");
    {
        Fixture f("TI-59", /*started=*/false);
        f.run(500);
        CHECK(!f.c().romsFound());
        QWidget* missing = f.w().shell().romsMissingView();
        CHECK(missing && missing->isVisible());
        CHECK(!f.w().keyboard()->isVisible());
        CHECK(!f.find<QWidget>(names::kControlBar)->isVisible());
        CHECK(checkList("menus", menuTitles(f.w().menuBar()), {"File", "Help"}));
        auto* label = qobject_cast<QLabel*>(missing);
        CHECK(label && label->text().contains(QStringLiteral("TMC0583.txt")));
        f.action("about")->trigger();
        f.run(50);
        CHECK(f.find<QDialog>(names::kAboutDialog) && f.find<QDialog>(names::kAboutDialog)->isVisible());
        f.action("help")->trigger();
        CHECK(f.opened.size() == 1);
        CHECK(f.w().isVisible());   // the app runs on
    }
    QtResourceBundle::hideForTests("");
}

// ── A bundled example from Examples ▸ ─────────────────────────────────────

TEST(bundled_example_sum_shows_its_cue_card) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QAction* sum = nullptr;
    for (QAction* a : f.w().menuBar()->findChildren<QAction*>())
        if (a->objectName().startsWith(QStringLiteral("item-example-")) && a->objectName().endsWith(QStringLiteral("sum.ti59"))) sum = a;
    CHECK(sum != nullptr);
    if (!sum) return;
    sum->trigger();
    CHECK(f.waitFor([&] {
        const auto& card = f.c().cueCard();
        return card && card->title == "Sum 1..N";
    }, 30000));
    f.waitIdle();
}

// ── A drop and Open With, as Calc-U-1600's ────────────────────────────────

namespace {
const QString kSum = QStringLiteral(CALCU59_DIR "/examples/sum.ti59");
bool sumCueCard(Fixture& f) {
    const auto& card = f.c().cueCard();
    return card && card->title == "Sum 1..N";
}
} // namespace

TEST(drop_accepted_and_opened_after_the_drop) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(kSum)});
    QDragEnterEvent enter(QPoint(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&f.w(), &enter);
    CHECK(enter.isAccepted());
    QDropEvent drop(QPointF(100, 100), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    drop.setAccepted(false);
    QCoreApplication::sendEvent(&f.w(), &drop);
    CHECK(drop.isAccepted() && drop.dropAction() == Qt::CopyAction);
    CHECK(!sumCueCard(f));   // opened after the drop handler
    CHECK(f.waitFor([&] { return sumCueCard(f); }, 30000));
    f.waitIdle();
}

TEST(open_with_opens_the_preset) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    // The Dock icon and Finder's Open With reach the app as QFileOpenEvent.
    calcu::qt6::FileOpenFilter filter;
    filter.setHandler([&f](const QString& p) { f.w().openFile(p, calcu::opening::Way::OpenWith); });
    qApp->installEventFilter(&filter);
    QFileOpenEvent e(kSum);
    QCoreApplication::sendEvent(qApp, &e);
    qApp->removeEventFilter(&filter);
    CHECK(f.waitFor([&] { return sumCueCard(f); }, 30000));
    f.waitIdle();
}

// ── The debug panel: FREEZE/STEP/RESUME, an ASM overlay run ──────────────

TEST(freeze_step_resume_on_the_calculator_tab) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    // Lbl A 1 2 3 R/S, run with A: FREEZE lands on a program step.
    f.command("load-preset",
              Value::object({{"content", "format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n    76 11 01 02 03 85 01 95 61 00 02\n"},
                             {"name", "freeze.ti59"}}));
    f.waitIdle();
    f.command("screen-key-down", Value::object({{"key", "A"}, {"pointer", 9}}));
    f.run(150);
    f.command("screen-key-up", Value::object({{"key", "A"}, {"pointer", 9}}));
    auto* panel = f.find<DebugPanel>("debugpanel");
    CHECK(panel != nullptr);
    if (!panel) return;
    QToolButton* freeze = nullptr;
    QToolButton* resume = nullptr;
    QToolButton* step = nullptr;
    for (QToolButton* b : panel->findChildren<QToolButton*>()) {
        if (!b->isVisibleTo(panel)) continue;
        if (b->text() == QStringLiteral("FREEZE") && !freeze) freeze = b;
        if (b->text() == QStringLiteral("RESUME") && !resume) resume = b;
        if (b->text() == QStringLiteral("STEP") && !step) step = b;
    }
    CHECK(freeze && resume && step);
    if (!freeze || !resume || !step) return;
    freeze->click();
    CHECK(f.waitFor([&] { return f.c().isFrozen() && resume->isEnabled(); }, 5000));
    step->click();   // STEP: one program step, paused again after it
    CHECK(f.waitFor([&] {
        const auto* r = f.c().runState();
        return r && r->paused && r->reason == calcu::debugger::RunState::Reason::Step;
    }, 5000));
    resume->click();
    CHECK(f.waitFor([&] { return !f.c().isFrozen() && !resume->isEnabled(); }, 5000));
}

TEST(asm_overlay_chosen_run_cpu_tab_shows_it) {
    SKIP_UNLESS_WINDOW();
    Fixture f;
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("simple_count.asm"));
    {
        std::ofstream out(path.toStdString());
        out << "Calc-U-59 ASM 2.0\nPROGRAM:\n; a counter\nHEX:\n01D8 01DB 0D00 0A37 1805 1007\n";
    }
    auto* panel = f.find<DebugPanel>("debugpanel");
    CHECK(panel != nullptr);
    if (!panel) return;
    for (QToolButton* b : panel->findChildren<QToolButton*>())
        if (b->text() == QStringLiteral("CPU")) b->click();
    f.fileAnswer = path;   // Select File through the shell's file chooser
    QPushButton* select = nullptr;
    QPushButton* run = nullptr;
    for (QPushButton* b : panel->findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("Select File")) select = b;
        if (b->text() == QStringLiteral("Run")) run = b;
    }
    CHECK(select && run);
    if (!select || !run) return;
    select->click();
    CHECK(f.waitFor([&] { return f.c().asmFileName() == QStringLiteral("simple_count.asm") && f.c().asmWordCount() == 6; }, 5000));
    CHECK(f.waitFor([&] { return run->isEnabled(); }, 3000));
    run->click();
    CHECK(f.waitFor([&] { return f.c().asmStatus().contains(QStringLiteral("ASM entered at 0x1800")); }, 5000));
    CHECK(f.waitFor([&] {
        for (const auto& e : f.c().cpuHistory())
            if ((e.pc & 0xFF00) == 0x1800) return true;
        return false;
    }, 5000));
    f.c().clearAsmOverlay();
    f.run(100);
}

// ── Qt6 attach ────────────────────────────────────────────────────────────

#ifdef CALCU_ATTACH
TEST(qt_attach_drives_calc_u_59) {
    SKIP_UNLESS_WINDOW();
    auto product = cu59::makeHarnessProduct();   // outlives the window's attach
    Fixture f;
    CHECK(f.w().host().openAttach(*product, 0,
                                  f.c().app().libraryParts())
              .empty());
    calcu::qt6::AttachLineClient client;
    CHECK(client.connectTo(f.w().host().attachPort()));
    std::vector<calcu::qt6::AttachLineClient::Reply> replies;
    for (const char* line : {"model TI-58", "key 7", "wait 300ms", "display-text"})
        client.send(QString::fromLatin1(line), [&](const calcu::qt6::AttachLineClient::Reply& r) { replies.push_back(r); });
    CHECK(f.waitFor([&] { return replies.size() == 4; }, 30000));
    for (const auto& r : replies) CHECK(r.done);
    CHECK(f.model() == "TI-58");   // the window follows what attach did
    CHECK(f.find<CardControls>(names::barMedia("card-reader")) == nullptr);
    CHECK(replies.size() == 4 && replies[3].lines.join(QLatin1Char('\n')).contains(QLatin1Char('7')));
}
#endif
