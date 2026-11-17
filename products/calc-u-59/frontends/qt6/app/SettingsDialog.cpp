#include "SettingsDialog.hpp"
#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "EmulatorController.hpp"
#include "Version.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(EmulatorController& controller, QWidget* parent) : QDialog(parent), m_controller(controller) {
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(460);
    auto* col = new QVBoxLayout(this);

    auto* general = new QGroupBox(QStringLiteral("General"), this);
    auto* gf = new QFormLayout(general);
    m_startup = new QComboBox(general);
    m_startup->addItem(QStringLiteral("TI-59"), 0);
    m_startup->addItem(QStringLiteral("TI-58"), 1);
    m_startup->addItem(QStringLiteral("TI-58C"), 2);
    m_startup->insertSeparator(3);
    m_startup->addItem(QStringLiteral("Last Used"), -1);
    const auto chosen = AppSettings::settings().startupModelChoice();
    m_startup->setCurrentIndex(m_startup->findData(chosen ? static_cast<int>(*chosen) : -1));
    connect(m_startup, qOverload<int>(&QComboBox::activated), this, [this] {
        const int raw = m_startup->currentData().toInt();
        AppSettings::settings().setStartupModelChoice(raw < 0 ? std::nullopt
                                                              : std::optional<cu59::Model>(static_cast<cu59::Model>(raw)));
    });
    gf->addRow(QStringLiteral("Startup Calculator"), m_startup);

    // slot-1's picker of the library's media topic (LIB-7 AC-47; C59-6 AC-8).
    m_module = new QComboBox(general);
    for (const auto& e : m_controller.modulePicker())
        m_module->addItem(QString::fromStdString(e.label), QString::fromStdString(e.id));
    m_module->setCurrentIndex(m_module->findData(QStringLiteral("module:") + m_controller.moduleID()));
    gf->addRow(QStringLiteral("Solid State Module"), m_module);
    gf->addRow(QStringLiteral("Version"), new QLabel(QStringLiteral(APP_VERSION " (" APP_BUILD_ID ")"), general));
    col->addWidget(general);

    auto* display = new QGroupBox(QStringLiteral("Display"), this);
    auto* df = new QFormLayout(display);
    m_style = new QComboBox(display);
    m_style->addItem(QStringLiteral("Modernized"), 0);
    m_style->addItem(QStringLiteral("Classic"), 1);
    m_style->setCurrentIndex(AppSettings::settings().ledStyle() == cu59::LedStyle::Classic ? 1 : 0);
    connect(m_style, qOverload<int>(&QComboBox::activated), this, [this] {
        AppSettings::settings().setLedStyle(static_cast<cu59::LedStyle>(m_style->currentData().toInt()));
    });
    df->addRow(QStringLiteral("Segment Style"), m_style);
    col->addWidget(display);

    // Folders, as in Calc-U-1600: each row shows the folder in use; Reset
    // returns to the default (the save folder: ~/Calc-U-59, traces: the save folder,
    // presets: the folder a preset was last opened from).
    auto* files = new QGroupBox(QStringLiteral("Files"), this);
    auto* grid = new QGridLayout(files);
    grid->setColumnStretch(1, 1);
    addFolderRow(grid, QStringLiteral("Preset Folder"), QStringLiteral("Folder Open Preset starts in"),
                 [] {
                     QString dir = AppSettings::openDir(AppSettings::OpenFolder::Presets);
                     return dir.isEmpty() ? QStringLiteral("<last used>") : AppPaths::displayPath(dir);
                 },
                 [] { return AppSettings::openStartDir(AppSettings::OpenFolder::Presets); },
                 [] { return !AppSettings::openDir(AppSettings::OpenFolder::Presets).isEmpty(); },
                 [](const QString& dir) { AppSettings::setOpenDir(AppSettings::OpenFolder::Presets, dir); });
    // The new folder is used from the next start (the TI-58C's constant memory
    // file is read once, at the build of the TI-58C; C59-3 TR).
    addFolderRow(grid, QStringLiteral("Save Folder"), QStringLiteral("Folder for cards and ti58c.memory.yaml"),
                 [] { return AppPaths::displayPath(AppPaths::saveDir()); }, [] { return AppPaths::saveDir(); },
                 [] { return !AppSettings::saveFolderOverride().isEmpty(); },
                 [](const QString& dir) { AppSettings::setSaveFolderOverride(dir); });
    addFolderRow(grid, QStringLiteral("Trace Folder"), QStringLiteral("Folder for CALCU59_TRACE.bin"),
                 [] { return AppPaths::displayPath(AppPaths::traceDir()); }, [] { return AppPaths::traceDir(); },
                 [] { return !AppSettings::traceFolderOverride().isEmpty(); },
                 [](const QString& dir) { AppSettings::setTraceFolderOverride(dir); });
    m_maxMB = new QSpinBox(files);
    m_maxMB->setRange(1, 100000);
    m_maxMB->setSuffix(QStringLiteral(" MB"));
    m_maxMB->setValue(AppSettings::settings().traceMaxFileSizeMB());
    connect(m_maxMB, qOverload<int>(&QSpinBox::valueChanged), this,
            [](int v) { AppSettings::settings().setTraceMaxFileSizeMB(v); });
    const int row = grid->rowCount();
    grid->addWidget(new QLabel(QStringLiteral("Maximum Trace File Size"), files), row, 0);
    grid->addWidget(m_maxMB, row, 1, Qt::AlignLeft);
    col->addWidget(files);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    col->addWidget(buttons);
}

void SettingsDialog::addFolderRow(QGridLayout* grid, const QString& label, const QString& dialogTitle,
                                  std::function<QString()> display, std::function<QString()> startDir,
                                  std::function<bool()> isOverridden, std::function<void(const QString&)> set) {
    QWidget* parent = grid->parentWidget();
    const int row = grid->rowCount();
    auto* field = new QLineEdit(parent);
    field->setReadOnly(true);
    auto* change = new QPushButton(QStringLiteral("Change…"), parent);
    auto* reset = new QPushButton(QStringLiteral("Reset"), parent);
    reset->setToolTip(QStringLiteral("Revert to the default"));
    grid->addWidget(new QLabel(label, parent), row, 0);
    grid->addWidget(field, row, 1);
    grid->addWidget(change, row, 2);
    grid->addWidget(reset, row, 3);

    auto refresh = [field, reset, display, isOverridden] {
        field->setText(display());
        field->setCursorPosition(0);
        field->setToolTip(field->text());
        reset->setEnabled(isOverridden());
    };
    refresh();
    connect(change, &QPushButton::clicked, this, [this, dialogTitle, startDir, set, refresh] {
        const QString dir = QFileDialog::getExistingDirectory(this, dialogTitle, startDir());
        if (dir.isEmpty()) return;
        set(dir);
        refresh();
    });
    connect(reset, &QPushButton::clicked, this, [set, refresh] {
        set(QString());
        refresh();
    });
}

void SettingsDialog::done(int r) {
    m_controller.chooseModule(m_module->currentData().toString());
    QDialog::done(r);
}
