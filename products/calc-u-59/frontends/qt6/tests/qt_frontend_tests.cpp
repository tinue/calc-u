// CalcU59QtTests: the Qt6 frontend's paper panel and run-control buttons,
// the card controls and the Arm Card dialog, on the real window
// (tests/QtFixture), the session on the real ROMs, offscreen on the real
// clock.

#include "TestHarness.hpp"
#include "QtFixture.hpp"
#include "AppPaths.hpp"
#include "CardControls.hpp"
#include "CardPickerDialog.hpp"
#include "EmulatorController.hpp"
#include "TIMagneticCardKind.hpp"
#include "debug/CpuInspectorWidget.hpp"
#include "debug/LiveDebugWidget.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/RunControlButtons.hpp"
#include "qt6/Shell.hpp"
#include "shell/ObjectNames.hpp"
#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QToolButton>

using qtfixture::Fixture;
using qtfixture::romsPresent;
namespace names = calcu::shell::names;
using calcu::sessions::Value;

namespace {

template <class W> W* named(QWidget& root, const char* name) { return root.findChild<W*>(QString::fromLatin1(name)); }

calcu::qt6::PaperPanel* paperOf(Fixture& f) {
    return qobject_cast<calcu::qt6::PaperPanel*>(f.w().shell().panelArea()->panel("paper"));
}

// The PC-100C attached with its button on the paper panel, at key readiness again (the
// power cycle the attach brings).
bool withPrinter(Fixture& f) {
    auto* button = f.find<QToolButton>(names::barPlace("printer"));
    if (!button) return false;
    if (!f.c().printerConnected()) button->click();
    if (!f.waitFor([&] { return f.c().printerConnected(); })) return false;
    f.waitIdle();
    f.run(500);
    return true;
}

void tap(Fixture& f, const char* key) {
    f.command("screen-key-down", Value::object({{"key", key}, {"pointer", 9}}));
    f.run(150);
    f.command("screen-key-up", Value::object({{"key", key}, {"pointer", 9}}));
    f.run(150);
}

} // namespace

// The paper panel in the window's panel area: the PC-100C owns
// it, PRINT and ADV are held buttons, TRACE a switch, the view control
// there; detached with the bar's button: no owner.
TEST(paper_panel_print_adv_held_trace_switch_view_control) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    Fixture f;
    CHECK(withPrinter(f));
    calcu::qt6::PaperPanel* panel = paperOf(f);
    CHECK(panel != nullptr);
    if (!panel) return;
    CHECK(f.waitFor([&] { return panel->ownerName() == "PC-100C"; }, 2000));
    auto* print = named<QToolButton>(*panel, "paper-control-PRINT");
    auto* adv = named<QToolButton>(*panel, "paper-control-ADV");
    auto* trace = named<QToolButton>(*panel, "paper-control-TRACE");
    CHECK(print && adv && trace);
    if (!print || !adv || !trace) return;
    CHECK(!print->isCheckable() && !adv->isCheckable());   // held: pressed and released
    // The row as the control state labels it, left to right: PRINT, T, ADV
    // (the short labels only in a narrow panel).
    const QString printText = panel->compact() ? QStringLiteral("PRN") : QStringLiteral("PRINT");
    CHECK(print->text() == printText && trace->text() == QStringLiteral("T") && adv->text() == QStringLiteral("ADV"));
    CHECK(print->x() < trace->x() && trace->x() < adv->x());
    CHECK(trace->isCheckable() && !trace->isChecked());
    // TRACE's position as the control state has it.
    const auto traceOn = [&] {
        for (const auto& c : f.c().printerControls())
            if (c.id.value == "TRACE") return c.on;
        return false;
    };
    trace->click();   // set-control TRACE on
    CHECK(f.waitFor(traceOn));
    CHECK(f.waitFor([&] { return named<QToolButton>(*panel, "paper-control-TRACE")->isChecked(); }, 2000));
    auto* view = named<QToolButton>(*panel, "paper-view-toggle");
    CHECK(view != nullptr && view->isVisible());
    CHECK(!panel->showsText());
    trace->click();   // TRACE back off for the other cases
    CHECK(f.waitFor([&] { return !traceOn(); }));
    f.find<QToolButton>(names::barPlace("printer"))->click();
    CHECK(f.waitFor([&] { return !f.c().printerConnected(); }));
    CHECK(f.waitFor([&] { return panel->ownerName().empty(); }, 2000));
}

// The PC-100C prints a number, the text view shows it, Cut
// empties the paper and can no longer be used.
TEST(paper_print_text_cut) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    Fixture f;
    CHECK(withPrinter(f));
    calcu::qt6::PaperPanel* panel = paperOf(f);
    if (!panel) return;
    f.waitFor([&] { return named<QToolButton>(*panel, "paper-control-PRINT") != nullptr; }, 2000);
    tap(f, "7");
    auto* print = named<QToolButton>(*panel, "paper-control-PRINT");
    CHECK(print != nullptr);
    if (!print) return;
    print->pressed();
    f.run(200);
    print->released();
    CHECK(f.waitFor([&] { return !panel->paperEmpty(); }));
    panel->showText(true);
    auto* text = named<QPlainTextEdit>(*panel, "paper-text");
    CHECK(text != nullptr);
    if (text) CHECK(f.waitFor([&] { return text->toPlainText().contains(QStringLiteral("7.")); }));
    auto* cut = named<QToolButton>(*panel, "paper-cut");
    CHECK(cut != nullptr && cut->isEnabled());
    if (!cut) return;
    cut->click();
    CHECK(f.waitFor([&] { return panel->paperEmpty(); }));
    CHECK(f.waitFor([&] { return !cut->isEnabled(); }));
    f.command("detach", Value::object({{"place", "printer"}}));
}

// FREEZE, STEP and RESUME of the CALCULATOR tab (program) and
// the CPU tab (cpu) are the run-control buttons and follow the run state.
TEST(calculator_and_cpu_buttons_follow_run_state) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    Fixture f;
    LiveDebugWidget calculator(f.c());
    CpuInspectorWidget cpu(f.c());
    f.run(50);
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
    CHECK(f.waitFor([&] { return f.c().isFrozen(); }));
    f.run(50);
    CHECK(calc->stepButton()->isEnabled() && calc->resumeButton()->isEnabled() && !calc->pauseButton()->isEnabled());
    CHECK(!proc->stepButton()->isEnabled() && !proc->resumeButton()->isEnabled());   // the CALCULATOR tab owns it
    calc->resumeButton()->click();
    CHECK(f.waitFor([&] { return !f.c().isFrozen(); }));
    proc->pauseButton()->click();   // pause cpu
    CHECK(f.waitFor([&] { return f.c().isFrozen(); }));
    f.run(50);
    CHECK(proc->stepButton()->isEnabled() && proc->resumeButton()->isEnabled());
    CHECK(!calc->stepButton()->isEnabled());
    proc->resumeButton()->click();
    CHECK(f.waitFor([&] { return !f.c().isFrozen(); }));
}

namespace {

QToolButton* buttonNamed(QWidget& root, const QString& accessible) {
    for (QToolButton* b : root.findChildren<QToolButton*>())
        if (b->accessibleName() == accessible) return b;
    return nullptr;
}

void writeFile(const QString& path, const std::string& text) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(text.data(), static_cast<qint64>(text.size()));
}

} // namespace

// On the shell's bar (the media place's controls), as the control state
// declares them: two "Crd" buttons, Arm Card and Arm Fresh Card, while no
// card passes; Eject Card while one does; on the TI-58 none at all.
TEST(card_buttons_and_eject_by_card_state) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    Fixture f;
    QWidget* bar = f.find<QWidget>(names::kControlBar);
    CHECK(bar != nullptr);
    if (!bar) return;
    auto controls = [&] { return f.find<CardControls>(names::barMedia("card-reader")); };
    CHECK(f.waitFor([&] { return controls() != nullptr; }, 2000));
    auto button = [&](const char* name) { return controls() ? buttonNamed(*controls(), QString::fromLatin1(name)) : nullptr; };
    CHECK(f.waitFor([&] { return button("Arm Card") && button("Arm Fresh Card"); }));
    if (!button("Arm Card")) return;
    CHECK(!button("Eject Card") || button("Eject Card")->isHidden());   // made once the control state declares it
    CHECK(button("Arm Card")->text().endsWith(QStringLiteral("Crd")) && button("Arm Fresh Card")->text().endsWith(QStringLiteral("Crd")));
    CHECK(button("Arm Card")->toolTip() == QStringLiteral("Arm Card") && button("Arm Card")->isEnabled());
    CHECK(f.c().armFreshCard(QStringLiteral("Qt13"), true).isEmpty());
    f.c().sendCommand(calcu::media::kFeedCommand, Value::object({{"place", "card-reader"}, {"side", 0}}));
    CHECK(f.waitFor([&] { return button("Arm Card") && button("Arm Card")->isHidden() && button("Eject Card") && !button("Eject Card")->isHidden(); }));
    if (!button("Eject Card")) return;
    button("Eject Card")->click();
    CHECK(f.waitFor([&] { return button("Arm Card") && !button("Arm Card")->isHidden() && button("Eject Card")->isHidden(); }));
    f.command("model", Value::object({{"name", "TI-58"}}));   // no card reader: no card control
    CHECK(f.waitFor([&] { return controls() == nullptr; }, 3000));
}

// Arm Card's dialog: the saved cards of the save folder by name, "No cards found"
// when none; a card file that cannot be read is not listed.
TEST(arm_card_dialog_lists_saved_cards_none_found) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    const QString folder = AppPaths::saveDir();
    CHECK(QDir(folder).dirName() == QStringLiteral("Calc-U-59"));   // ~/Calc-U-59 (the test HOME)
    QDir(folder).removeRecursively();
    QDir().mkpath(folder);
    {
        Fixture f;
        CardPickerDialog none(CardPickerDialog::Mode::Arm, f.c().savedCards(), folder);
        CHECK(none.list()->count() == 1 && none.list()->item(0)->text() == QStringLiteral("No cards found"));
        CHECK(!(none.list()->item(0)->flags() & Qt::ItemIsEnabled));
    }
    writeFile(folder + QStringLiteral("/Mine.magnetcard.yaml"),
              cu59::writeMagneticCardFile(cu59::TIMagneticCardData{}, "My Card", "", false, std::nullopt, ""));
    writeFile(folder + QStringLiteral("/Old.magnetcard.yaml"), "format: ti59-magnetic-card\nformat-version: 2\ncard-name: Old\n");
    Fixture f;
    f.c().sendCommand(calcu::media::kRefreshCommand, Value::object({}));
    f.run(100);
    CardPickerDialog dialog(CardPickerDialog::Mode::Arm, f.c().savedCards(), folder);
    QStringList names;
    for (int i = 0; i < dialog.list()->count(); ++i) names << dialog.list()->item(i)->text();
    CHECK(names == QStringList{QStringLiteral("My Card")});
    CHECK(dialog.list()->item(0)->flags() & Qt::ItemIsEnabled);
    dialog.list()->setCurrentItem(dialog.list()->item(0));
    dialog.accept();
    CHECK_EQ(dialog.selectedEntry(), QStringLiteral("saved:Mine.magnetcard.yaml"));
    f.c().armCard(dialog.selectedEntry());
    CHECK(f.waitFor([&] {
        const auto* p = f.c().cardReader();
        return p && p->holds && p->holds->name == "My Card";
    }));
    QDir(folder).removeRecursively();
}
