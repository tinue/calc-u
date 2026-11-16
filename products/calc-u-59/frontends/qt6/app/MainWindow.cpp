#include "MainWindow.hpp"
#include "AboutDialog.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "CalculatorPanel.hpp"
#include "CardPickerDialog.hpp"
#include "ControlBar.hpp"
#include "EmulatorController.hpp"
#include "KeyboardWidget.hpp"
#include "PaperPane.hpp"
#include "SettingsDialog.hpp"
#include "debug/DebugPanel.hpp"
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSplitter>
#include <QUrl>

MainWindow::MainWindow(EmulatorController& controller, QWidget* parent) : QMainWindow(parent), m_controller(controller) {
    setWindowTitle(QStringLiteral("Calc-U-59"));
    setAcceptDrops(true);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    auto* calculator = new CalculatorPanel(controller, m_splitter);
    auto* printer = new PaperPane(controller, m_splitter);   // LIB-8: the library's paper panel (M-53)
    auto* debug = new DebugPanel(controller, m_splitter);
    m_splitter->addWidget(calculator);
    m_splitter->addWidget(printer);
    m_splitter->addWidget(debug);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setStretchFactor(2, 1);  // the debug panel absorbs extra width
    setCentralWidget(m_splitter);

    connect(calculator->controlBar(), &ControlBar::openPresetRequested, this, &MainWindow::openPreset);
    connect(calculator->controlBar(), &ControlBar::loadCardRequested, this, &MainWindow::loadCard);
    connect(calculator->controlBar(), &ControlBar::newCardRequested, this, &MainWindow::newCard);
    connect(debug, &DebugPanel::selectAsmFileRequested, this, &MainWindow::selectAsmFile);
    connect(&m_controller, &EmulatorController::errorOccurred, this,
            [this](const QString& title, const QString& message) { QMessageBox::warning(this, title, message); });

    buildMenus();
    restoreWindowState();
    m_keyboard = calculator->keyboard();
    m_keyboard->setFocus();
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu(QStringLiteral("&File"));
    file->addAction(QStringLiteral("Open Preset…"), QKeySequence::Open, this, &MainWindow::openPreset);
    file->addAction(QStringLiteral("Arm Card…"), this, &MainWindow::loadCard);
    file->addAction(QStringLiteral("Arm Fresh Card…"), this, &MainWindow::newCard);
    file->addSeparator();
    QAction* settings = file->addAction(QStringLiteral("Settings…"), QKeySequence::Preferences, this, [this] {
        SettingsDialog dialog(m_controller, this);
        dialog.exec();
        m_keyboard->reloadSettings();
    });
    settings->setMenuRole(QAction::PreferencesRole);
    QAction* quit = file->addAction(QStringLiteral("Quit"), QKeySequence::Quit, this, &QWidget::close);
    quit->setMenuRole(QAction::QuitRole);

    QMenu* help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(QStringLiteral("Calc-U-59 Help"), this,
                    [] { QDesktopServices::openUrl(QUrl(QStringLiteral("https://calc-u.org/59/"))); });
    QAction* about = help->addAction(QStringLiteral("About Calc-U-59"), this, [this] {
        AboutDialog dialog(this);
        dialog.exec();
    });
    about->setMenuRole(QAction::AboutRole);
}

void MainWindow::openPreset() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select a .ti59, .ti58, or .ti58c preset"),
                                                AppSettings::openStartDir(AppSettings::OpenFolder::Presets),
                                                QStringLiteral("Presets (*.ti59 *.ti58 *.ti58c);;All files (*)"));
    if (path.isEmpty()) return;
    AppSettings::rememberOpenFile(AppSettings::OpenFolder::Presets, path);
    openStateFile(path);
}

void MainWindow::openStateFile(const QString& path) { m_controller.loadPreset(path); }

// Arm Card (C59-7 AC-14): the saved cards of the media topic; the chosen one is fitted.
void MainWindow::loadCard() {
    if (!m_controller.cardReader() || m_controller.cardPassing()) return;
    CardPickerDialog dialog(CardPickerDialog::Mode::Arm, m_controller.savedCards(), AppPaths::saveDir(), this);
    if (dialog.exec() == QDialog::Accepted) m_controller.armCard(dialog.selectedEntry());
}

// Arm Fresh Card (AC-15): a name, then the replace question the session asks.
void MainWindow::newCard() {
    if (!m_controller.cardReader() || m_controller.cardPassing()) return;
    CardPickerDialog dialog(CardPickerDialog::Mode::ArmFresh, m_controller.savedCards(), AppPaths::saveDir(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    QString refusal = m_controller.armFreshCard(dialog.freshName(), false);
    if (refusal.startsWith(QStringLiteral("asks: "))) {
        if (!CardPickerDialog::confirmReplace(refusal.mid(6), this)) return;
        refusal = m_controller.armFreshCard(dialog.freshName(), true);
    }
    if (!refusal.isEmpty()) QMessageBox::warning(this, QStringLiteral("Arm Fresh Card"), refusal);
}

void MainWindow::selectAsmFile() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select an ASM overlay"),
                                                AppSettings::openStartDir(AppSettings::OpenFolder::AsmOverlays),
                                                QStringLiteral("ASM files (*.asm);;All files (*)"));
    if (path.isEmpty()) return;
    AppSettings::rememberOpenFile(AppSettings::OpenFolder::AsmOverlays, path);
    m_controller.loadAsmFile(path);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveWindowState();   // the constant memory: the session's closing (EmulatorController's end)
    QMainWindow::closeEvent(e);
}

// [window] as plain numbers (LIB-9 AC-35, design D-7): the normal geometry
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

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    using Drop = cu59::DropAction;
    const auto urls = e->mimeData()->urls();
    if (urls.size() != 1) return;
    const Drop action = m_controller.dropAction(urls.first().toLocalFile());
    if (action == Drop::LoadPreset || action == Drop::FitCard) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    const auto urls = e->mimeData()->urls();
    if (urls.size() == 1) m_controller.openDroppedFile(urls.first().toLocalFile());
}
