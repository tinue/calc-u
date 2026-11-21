#include "MainWindow.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "CalculatorPanel.hpp"
#include "CardControls.hpp"
#include "CardPickerDialog.hpp"
#include "ChromeStyle.hpp"
#include "EmulatorController.hpp"
#include "KeyboardWidget.hpp"
#include "TICatalogue.hpp"
#include "TIShellProduct.hpp"
#include "Version.h"
#include "debug/DebugPanel.hpp"
#include "qt6/AskedSend.hpp"
#include "qt6/FileOpening.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/Shell.hpp"
#include "shell/Inputs.hpp"
#include <QCloseEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>

MainWindow::MainWindow(QWidget* parent, calcu::pacing::WallClock clock)
    : QMainWindow(parent), m_controller(std::make_unique<EmulatorController>(nullptr, std::move(clock))) {
    setWindowTitle(QStringLiteral("Calc-U-59"));
    setAcceptDrops(true);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName(QStringLiteral("columns"));
    m_splitter->setChildrenCollapsible(false);
    setCentralWidget(m_splitter);
    m_calculator = new CalculatorPanel(*m_controller, m_splitter);
    m_keyboard = m_calculator->keyboard();

    // The shell over the session host, with Calc-U-59's data and the
    // Qt6 frontend's settings dialog.
    m_shell = new calcu::qt6::Shell(*this, host(), product(),
                                    cu59::settingsSpec(cu59::ShellFrontend::Qt6, calcu::shell::about(product()).version));
    m_calculator->setShellWidgets(m_shell->controlBar(), m_shell->romsMissingView());

    // The paper panel in the shell's panel area, the splitter's second
    // column.
    calcu::qt6::PanelArea* panels = m_shell->panelArea();
    panels->setParent(m_splitter);
    panels->setAutoFillBackground(true);
    QPalette pal = panels->palette();
    pal.setColor(QPalette::Window, ChromeStyle::kPanel);
    panels->setPalette(pal);
    panels->setContentsMargins(8, 8, 8, 8);
    panels->setMaximumWidth(320);
    m_debug = new DebugPanel(*m_controller, m_splitter);
    m_debug->setObjectName(QStringLiteral("debugpanel"));
    m_splitter->addWidget(m_calculator);
    m_splitter->addWidget(panels);
    m_splitter->addWidget(m_debug);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setStretchFactor(2, 1);  // the debug panel absorbs extra width

    // The pointer on the faceplate: the hit map in base-image pixels at the
    // canvas's scale and corner; a tap on the fitted card is the product's.
    m_shell->setFaceplate(m_keyboard, cu59::faceplateHitMap(calcu::machine::ModelId{"TI-59"}),
                          calcu::qt6::Shell::FaceplateGeometry{[this] { return m_keyboard->scale(); },
                                                               [this] { return m_keyboard->origin(); },
                                                               [this](double x, double y) { return m_keyboard->cardTakes(x, y); }});
    // The library's slot picker (the modules by description in menu-sort
    // order and the empty slot) and the magnetic card controls.
    m_shell->registerSlotPicker([this](const calcu::sessions::Value& entry, QWidget* p) -> QWidget* {
        auto* picker = new calcu::qt6::MediaPicker(p);
        const std::string place = calcu::shell::read::text(entry, "place");
        picker->setSender([this](const calcu::sessions::CommandName& n, calcu::sessions::Value v) {
            m_controller->sendCommand(n, std::move(v));
        });
        auto refresh = [this, picker, place] {
            if (const auto* s = m_controller->mediaPlace(place)) picker->setPlace(*s);
        };
        refresh();
        connect(m_controller.get(), &EmulatorController::cardStateChanged, picker, refresh);
        return picker;
    });
    m_shell->registerMediaControls("magnetic-card", [this](const calcu::sessions::Value&, QWidget* p) -> QWidget* {
        auto* cards = new CardControls(*m_controller, p);
        connect(cards, &CardControls::armRequested, this, &MainWindow::armCard);
        connect(cards, &CardControls::armFreshRequested, this, &MainWindow::armFreshCard);
        return cards;
    });
    connect(m_controller.get(), &EmulatorController::printerChanged, this, &MainWindow::followPrinter);
    connect(m_debug, &DebugPanel::selectAsmFileRequested, this, &MainWindow::selectAsmFile);

    restoreWindowState();
    m_keyboard->setFocus();
}

MainWindow::~MainWindow() {
    // The widgets send to the host and read the controller: they go first.
    m_controller->host().stop();
    delete takeCentralWidget();
    m_calculator = nullptr;
    m_debug = nullptr;
    m_keyboard = nullptr;
    delete m_shell;
    m_shell = nullptr;
    menuBar()->clear();
    m_controller.reset();
}

calcu::qt6::SessionHost& MainWindow::host() { return m_controller->host(); }

void MainWindow::start() { m_controller->start(); }

calcu::shell::ShellProduct MainWindow::product() {
    calcu::shell::ShellProduct p = cu59::shellProduct(APP_VERSION, APP_BUILD_ID);
    p.aboutLines.push_back("Built with Qt (LGPL-3.0).");   // the Qt6 frontend's line (AboutDialog.cpp:28)
    for (auto& item : p.items) {
        if (item.id == cu59::kItemArmCard) item.act = [this] { armCard(); };
        else if (item.id == cu59::kItemArmFreshCard) item.act = [this] { armFreshCard(); };
    }
    return p;
}

// The PC-100C's controls on the paper panel while it owns it:
// PRINT and ADV keys, TRACE a switch at its position, as the control state
// lists them.
void MainWindow::followPrinter() {
    auto* paper = qobject_cast<calcu::qt6::PaperPanel*>(m_shell->panelArea()->panel("paper"));
    if (!paper) return;
    paper->setControls(m_controller->printerControls());
}

void MainWindow::openFile(const QString& path, calcu::opening::Way way) {
    calcu::qt6::openFile(*this, host(), path, way, m_controller->app().libraryParts().open().setup().maxBytes);
}

// Arm Card: the saved cards of the media topic; the chosen one is fitted.
void MainWindow::armCard() {
    if (!m_controller->cardReader() || m_controller->cardPassing()) return;
    CardPickerDialog dialog(CardPickerDialog::Mode::Arm, m_controller->savedCards(), AppPaths::saveDir(), this);
    if (dialog.exec() == QDialog::Accepted) m_controller->armCard(dialog.selectedEntry());
}

// Arm Fresh Card: a name, then the replace question the session asks.
void MainWindow::armFreshCard() {
    if (!m_controller->cardReader() || m_controller->cardPassing()) return;
    CardPickerDialog dialog(CardPickerDialog::Mode::ArmFresh, m_controller->savedCards(), AppPaths::saveDir(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    const calcu::qt6::AskedSend r = calcu::qt6::sendAsking(
        [&](bool confirmed) { return m_controller->armFreshCard(dialog.freshName(), confirmed).toStdString(); },
        [this](const QString& question) { return CardPickerDialog::confirmReplace(question, this); });
    if (!r.done && !r.declined) QMessageBox::warning(this, QStringLiteral("Arm Fresh Card"), QString::fromStdString(r.refusal));
}

void MainWindow::selectAsmFile() {
    const std::string kind = cu59::settings::openFolderKind(cu59::settings::OpenFolder::AsmOverlays);
    const QString path = m_shell->choosers().file(this, QStringLiteral("Select an ASM overlay"), m_shell->startFolder(kind),
                                                  QStringLiteral("ASM files (*.asm);;All files (*)"));
    if (path.isEmpty()) return;
    m_shell->rememberFolder(kind, path);
    m_controller->loadAsmFile(path);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveWindowState();   // the constant memory: the session's closing (EmulatorController's end)
    QMainWindow::closeEvent(e);
}

// [window] as plain numbers: the normal geometry
// (a maximised window comes back at its last normal size) and the widths
// of the three panels.
void MainWindow::restoreWindowState() {
    const cu59::settings::WindowState w = AppSettings::settings().windowState();
    resize(static_cast<int>(w.width), static_cast<int>(w.height));
    if (w.x && w.y) move(static_cast<int>(*w.x), static_cast<int>(*w.y));
    if (w.calculatorWidth && w.printerWidth && w.debugWidth)
        m_splitter->setSizes({static_cast<int>(*w.calculatorWidth), static_cast<int>(*w.printerWidth),
                              static_cast<int>(*w.debugWidth)});
}

void MainWindow::saveWindowState() {
    const QRect g = normalGeometry();
    const QPoint at = isMaximized() || isFullScreen() ? g.topLeft() : pos();
    cu59::settings::WindowState w;
    w.x = at.x();
    w.y = at.y();
    w.width = g.width();
    w.height = g.height();
    const QList<int> sizes = m_splitter->sizes();
    if (sizes.size() == 3) {
        w.calculatorWidth = sizes[0];
        w.printerWidth = sizes[1];
        w.debugWidth = sizes[2];
    }
    AppSettings::settings().setWindowState(w);
}

// One file the session would take, by its name and size (no limit here);
// the drop is accepted and goes through all of `open` after the drop
// handler (a refusal is its notice).
void MainWindow::dragEnterEvent(QDragEnterEvent* e) { calcu::qt6::dragEnter(e, host()); }

void MainWindow::dragMoveEvent(QDragMoveEvent* e) { calcu::qt6::dragMove(e); }

void MainWindow::dropEvent(QDropEvent* e) {
    calcu::qt6::drop(e, this, [this](const QString& path) { openFile(path, calcu::opening::Way::Drop); });
}
