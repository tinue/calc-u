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
#include "qt6/PanelArea.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/SessionHost.hpp"
#include "qt6/Shell.hpp"
#include "shell/Inputs.hpp"
#include "shell/View.hpp"
#include <QCloseEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QToolButton>

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
    calcu::qt6::ShellOptions options;
    options.windowSettings = &AppSettings::settings().file();
    m_shell = new calcu::qt6::Shell(*this, host(), product(),
                                    cu59::settingsSpec(cu59::ShellFrontend::Qt6, calcu::shell::about(product()).version),
                                    std::move(options));
    m_shell->setChrome(ChromeStyle::chrome());
    m_calculator->setShellWidgets(*m_shell);

    // The paper panel in the shell's panel area, the splitter's second
    // column.
    calcu::qt6::PanelArea* panels = m_shell->panelArea();
    panels->setParent(m_splitter);
    panels->setAutoFillBackground(true);
    QPalette pal = panels->palette();
    pal.setColor(QPalette::Window, ChromeStyle::qcolour(ChromeStyle::colours().window));
    panels->setPalette(pal);
    // The printer column's width range (app/'s TIChrome): its buttons in
    // one row below the paper; inset as the calculator's faceplate.
    const calcu::shell::ChromeSizes& sizes = ChromeStyle::chrome().sizes;
    const int inset = qRound(sizes.panelInset);
    panels->setContentsMargins(inset, inset, inset, inset);
    panels->setMinimumWidth(qRound(sizes.printerMinWidth) + 2 * inset);
    panels->setMaximumWidth(qRound(sizes.printerMaxWidth) + 2 * inset);
    // The paper panel's text grows with the faceplate, as the bar's does.
    const QFont base = panels->font();
    connect(m_calculator, &CalculatorPanel::fontScaleChanged, panels, [panels, base](double scale) {
        QFont f = base;
        f.setPointSizeF(base.pointSizeF() * scale);
        panels->setFont(f);
    });
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
    // The magnetic card controls (the module slot's picker is the shell's).
    m_shell->registerMediaControls("magnetic-card", [this](const calcu::sessions::Value&, QWidget* p) -> QWidget* {
        auto* cards = new CardControls(*m_controller, p);
        connect(cards, &CardControls::armRequested, this, &MainWindow::armCard);
        connect(cards, &CardControls::armFreshRequested, this, &MainWindow::armFreshCard);
        return cards;
    });
    connect(m_controller.get(), &EmulatorController::printerChanged, this, &MainWindow::followPrinter);
    // The PC-100C's controls on the paper panel from the moment the shell shows it.
    connect(&host(), &calcu::qt6::SessionHost::topicChanged, this, [this] {
        auto* paper = m_shell->panelArea()->panel("paper");
        if (paper && paper != m_paperStyled) followPrinter();
    });
    connect(m_debug, &DebugPanel::selectAsmFileRequested, this, &MainWindow::selectAsmFile);

    // The pages' chevron, over the column's top corner (not one of the
    // splitter's columns).
    m_chevron = new QToolButton(this);
    m_chevron->setObjectName(QStringLiteral("page-chevron"));
    const calcu::shell::PillShape& pill = sizes.pill;
    m_chevron->setFixedSize(qRound(pill.height + pill.paddingX), qRound(pill.height));
    m_chevron->setStyleSheet(QStringLiteral("QToolButton { background: %1; color: %2; border: none; border-radius: %3px;"
                                            " font-size: %4px; font-weight: 600; }")
                                 .arg(ChromeStyle::rgb(cu59::chrome::kChevron), ChromeStyle::rgb(ChromeStyle::qcolour(ChromeStyle::colours().text)))
                                 .arg(qRound(pill.radius))
                                 .arg(qRound(pill.height - 2 * pill.paddingY)));
    m_chevron->hide();
    connect(m_chevron, &QToolButton::clicked, this, [this] {
        m_printerPage = !m_printerPage;
        applyView();
    });
    m_splitter->installEventFilter(this);
    connect(m_shell->panelArea(), &calcu::qt6::PanelArea::paperOwnWindowChanged, this, [this] { applyView(); });

    restoreWindowState();
    applyView();
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
    p.buildTime = APP_BUILD_TIME;
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
    m_paperStyled = paper;
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

// The library's view rule on the window's room: the printer beside the
// faceplate where it fits (the debugger too where the room takes both and
// the window is tall enough), else the pages. With the paper in its own
// window: the debugger beside the faceplate where it fits, else the
// calculator alone.
void MainWindow::applyView() {
    if (!m_splitter || !m_debug) return;
    const calcu::shell::ChromeSizes& s = ChromeStyle::chrome().sizes;
    calcu::qt6::PanelArea* panels = m_shell->panelArea();
    const bool own = panels->paperOwnWindow();
    // The splitter's dividers beside the faceplate.
    const calcu::shell::View v = calcu::shell::view(
        cu59::viewInputs(m_splitter->width(), m_splitter->height(), 2 * m_splitter->handleWidth(), false, own));
    const bool wide = v.side == calcu::shell::ViewSide::Beside && v.panelsFit;
    const bool debugger = own ? wide && m_splitter->height() >= s.debuggerMinHeight : v.debuggerFits;
    if (own) m_printerPage = false;
    m_paged = !wide && !own;
    m_calculator->setVisible(wide || !m_printerPage);
    panels->setVisible(!own && (wide || m_printerPage));
    m_debug->setVisible(debugger);
    m_chevron->setVisible(m_paged);
    if (m_paged) {
        const QRect area = m_splitter->geometry();
        const int margin = qRound(s.panelInset);
        m_chevron->setText(m_printerPage ? QStringLiteral("‹") : QStringLiteral("›"));
        m_chevron->setToolTip(m_printerPage ? tr("Calculator") : tr("Printer"));
        m_chevron->move(m_printerPage ? area.left() + margin : area.right() - margin - m_chevron->width(), area.top() + margin);
        m_chevron->raise();
    }
}

bool MainWindow::eventFilter(QObject* o, QEvent* e) {
    if (o == m_splitter && e->type() == QEvent::Resize) applyView();
    return QMainWindow::eventFilter(o, e);
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
    resize(static_cast<int>(w.place.width), static_cast<int>(w.place.height));
    if (w.place.x && w.place.y) move(static_cast<int>(*w.place.x), static_cast<int>(*w.place.y));
    if (w.calculatorWidth && w.printerWidth && w.debugWidth)
        m_splitter->setSizes({static_cast<int>(*w.calculatorWidth), static_cast<int>(*w.printerWidth),
                              static_cast<int>(*w.debugWidth)});
}

void MainWindow::saveWindowState() {
    const QRect g = normalGeometry();
    const QPoint at = isMaximized() || isFullScreen() ? g.topLeft() : pos();
    // The columns' widths as last seen side by side, the paper among them
    // (with the paper in its own window its column is 0: kept as it was).
    cu59::settings::WindowState w = AppSettings::settings().windowState();
    w.place = {at.x(), at.y(), g.width(), g.height()};
    const QList<int> sizes = m_splitter->sizes();
    if (sizes.size() == 3 && sizes[0] > 0 && sizes[1] > 0 && sizes[2] > 0) {
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
