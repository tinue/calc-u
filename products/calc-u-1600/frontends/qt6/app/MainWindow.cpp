#include "MainWindow.hpp"

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "AudioOutput.hpp"
#include "Ce158PrinterWidget.hpp"
#include "DebugPanel.hpp"
#include "FaceplateWidget.hpp"
#include "LcdWidget.hpp"
#include "MachineCodeLoadDialog.hpp"
#include "MediaControls.hpp"
#include "Version.h"
#include "app/SharpChrome.hpp"
#include "app/SharpControlRules.hpp"
#include "app/SharpFileKinds.hpp"
#include "app/SharpShellProduct.hpp"
#include "control/ControlState.hpp"
#include "opening/OpenRequest.hpp"
#include "qt6/ChromeStyle.hpp"
#include "qt6/FileOpening.hpp"
#include "qt6/MacClipboard.h"
#include "qt6/PanelArea.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/ShellSettingsDialog.hpp"
#include "shell/Inputs.hpp"
#include "shell/ObjectNames.hpp"
#include "shell/View.hpp"
#include "typing/TypingTopics.hpp"

#include <QAction>
#include <algorithm>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <cstring>
#include <utility>

namespace {
// Longest host-Shift press still counted as a tap (see m_shiftTapArmed).
constexpr qint64 kShiftTapMaxMs = 400;

calcu::sessions::Value productEntry(const calcu::sessions::Value& controlState, const char* name) {
    const calcu::sessions::Value* p = controlState.find("product");
    const calcu::sessions::Value* e = p ? p->find(name) : nullptr;
    return e ? *e : calcu::sessions::Value();
}
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("Calc-U-1600"));
    setFocusPolicy(Qt::StrongFocus);
    // Take part in input-method composition, or dead keys never compose:
    // Qt hands a widget without it the raw Key_Dead_* press (empty text,
    // dropped by resolve()) and the next letter uncombined. Plain keys
    // still arrive as key events; a composed character comes as
    // inputMethodEvent()'s commit string.
    setAttribute(Qt::WA_InputMethodEnabled);

    m_controller = std::make_unique<MachineController>(this);

    // The layout: the faceplate, below it the bar, below it the panel row
    // (where the view rule gives it room, applyView).
    auto* central = new QWidget(this);
    m_faceplate = new FaceplateWidget(central);
    m_faceplate->setObjectName(QStringLiteral("faceplate"));
    m_faceplate->lcdWidget()->setObjectName(QStringLiteral("lcd"));
    setCentralWidget(central);

    bool serialPorts = true;
#ifdef Q_OS_WIN
    serialPorts = false;   // no host links on Windows
#endif
    m_shell = new calcu::qt6::Shell(*this, host(), product(), cu1600::settingsSpec(true, serialPorts));
    m_shell->setChrome(cu1600::sharpChrome());
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 4);
    layout->setSpacing(4);
    // Faceplate is stretch 0 (pinned to its own aspect-locked heightForWidth
    // size, top-aligned) so extra vertical space goes to the panel row
    // instead of letterboxing the calculator body.
    layout->addWidget(m_faceplate, 0);
    layout->addWidget(m_shell->romsMissingView(), 0);
    // The bar centred under the faceplate, as wide as the library's rule
    // lets it be (fitBar).
    layout->addLayout(m_shell->centredBarRow(), 0);
    connect(m_faceplate, &FaceplateWidget::artChanged, this, &MainWindow::fitBar);
    calcu::qt6::PanelArea* panels = m_shell->panelArea();
    panels->setParent(central);
    layout->addWidget(panels, 1);
    // Without the panels the faceplate keeps its height for its width
    // and the bar stays right below it.
    layout->addStretch(0);
    m_panels = panels;
    central->installEventFilter(this);
    // The INSPECTOR always, first, at 2:1 to the paper panel; the
    // CE-158's printer panel last, while its product entry says so.
    m_inspector = new DebugPanel(m_controller.get(), panels);
    m_inspector->setObjectName(QStringLiteral("debugpanel"));
    if (auto* box = qobject_cast<QBoxLayout*>(panels->layout())) box->insertWidget(0, m_inspector, 2);
    m_ce158Printer = new Ce158PrinterWidget(m_controller.get(), panels);
    m_ce158Printer->setObjectName(QStringLiteral("ce158printer"));
    m_ce158Printer->hide();
    if (auto* box = qobject_cast<QBoxLayout*>(panels->layout())) box->addWidget(m_ce158Printer, 1);

    // The pointer on the faceplate: the hit map in the art's
    // pixels, at the art's scale as shown.
    m_shell->setFaceplate(m_faceplate, cu1600::faceplateHitMap(calcu::machine::ModelId{"PC-1500A"}), faceplateGeometry());
    // The library's media controls on the bar: the module slots (with Name &
    // Save) and the cassette by the family rule; the disk controls are the product's.
    m_shell->setSlotPicker({true, tr("Instance name:")});
    m_shell->registerMediaControls("cassette", [this](const calcu::sessions::Value&, QWidget* p) -> QWidget* {
        return new TapeControl(*m_controller, p);
    });
    m_disk = new DiskControl(*m_controller, nullptr);
    m_shell->addBarControl(calcu::shell::BarGroup::Media, m_disk);
    m_disk->refresh();

    connect(&host(), &calcu::qt6::SessionHost::topicChanged, this, &MainWindow::onTopic);
    connect(&host(), &calcu::qt6::SessionHost::called, this, [this] {
        if (m_ce158Printer->isVisible()) m_ce158Printer->onFrameTick();
        if (auto* d = m_shell->settingsDialog(); d && d->isVisible()) refreshSerialStatus();
    });
    connect(m_inspector, &DebugPanel::outputSelectionChanged, this, [this] { applyCopyLabel(); });
    // A paused machine shows in the title bar, whoever paused it.
    connect(m_controller.get(), &MachineController::runStateChanged, this, [this] {
        setWindowTitle(m_controller->machinePaused() ? tr("Calc-U-1600 — Paused (debugger)") : tr("Calc-U-1600"));
    });
    connect(m_controller.get(), &MachineController::programOpened, this, &MainWindow::onProgramOpened);
    // A key held while focus moves to another widget (a combo box, a
    // dialog) releases there, not here -- let go of it now instead.
    m_focusChanged = connect(qApp, &QApplication::focusChanged, this, [this] { releaseHeldKeys(); });

    // The sink is a child of the window, so it outlives m_controller (and
    // with it the session's audio pump).
    m_audio = new AudioOutput(this);
    // Pay the audio backend's first-start cost right after startup.
    QTimer::singleShot(0, this, [this] { m_controller->attachAudio(m_audio->sink()); });

    resize(AppSettings::windowSize());
    setFocus();
    setAcceptDrops(true);
}

MainWindow::~MainWindow() {
    // ~QWidget clears the focus and so emits focusChanged after this
    // window's members (m_hostKeys) are gone: let go of it first.
    disconnect(m_focusChanged);
    // The widgets send to the host and read the controller: they go first.
    m_controller->host().stop();
    delete takeCentralWidget();
    m_faceplate = nullptr;
    m_inspector = nullptr;
    m_ce158Printer = nullptr;
    m_disk = nullptr;
    delete m_shell;
    m_shell = nullptr;
    menuBar()->clear();
}

void MainWindow::start() { m_controller->start(); }

void MainWindow::closeEvent(QCloseEvent* event) {
    AppSettings::setWindowSize(size());
    // What a medium still has to save is written when the session closes
    // (MachineController's destructor: AppSession::close()).
    QMainWindow::closeEvent(event);
}

calcu::shell::ShellProduct MainWindow::product() {
    calcu::shell::ShellProduct p = cu1600::shellProduct(APP_VERSION, APP_BUILD_ID, /*withHostFolders=*/true, /*withTrace=*/true);
    p.copyScreen = [this] { copyScreen(); };
    for (auto& item : p.items) {
        if (item.id == cu1600::kItemLoadBasic) item.act = [this] { chooseProgram(false); };
        else if (item.id == cu1600::kItemLoadCode) item.act = [this] { chooseProgram(true); };
        else if (item.id == cu1600::kItemHostDrive) item.act = [this] { mountHostDirectory(); };
        else if (item.id == cu1600::kItemUnmountHostDrive) item.act = [this] { unmountHostDirectory(); };
    }
    return p;
}

void MainWindow::onTopic(const QString& topic) {
    if (topic == QLatin1String("display")) {
        if (const auto* d = m_controller->display()) m_faceplate->lcdWidget()->setOutput(*d);
    } else if (topic == QLatin1String("held-keys")) {
        const auto* h = m_controller->state<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic);
        std::vector<std::string> keys;
        if (h)
            for (const auto& k : h->live) keys.push_back(k.value);
        m_faceplate->setHeldKeys(std::move(keys));
    } else if (topic == QLatin1String("control-state")) {
        followControlState();
    }
}

// The control state: the faceplate's art and hit map, the CE-158's printer
// panel and the Unmount item's text from the product entries.
// The art's scale and its origin in the widget (the art is letterboxed: in a
// widget wider than the art, a hit map from the widget's corner put every
// press several keys to the right), and every press noted by the faceplate
// (which of two keys with one name was touched) before the shell takes it.
calcu::qt6::Shell::FaceplateGeometry MainWindow::faceplateGeometry() {
    return {[this] { return m_faceplate->artScale(); }, [this] { return m_faceplate->artOrigin(); },
            [this](double x, double y) { return m_faceplate->notePress(x, y); }};
}

void MainWindow::followControlState() {
    const calcu::sessions::Value cs = host().value(calcu::control::kControlStateTopic);
    const std::string model = calcu::shell::read::text(cs, "model");
    if (!model.empty() && model != m_model) {
        m_model = model;
        m_faceplate->setModel(calcu::machine::ModelId{model});
        m_shell->setFaceplate(m_faceplate, cu1600::faceplateHitMap(calcu::machine::ModelId{model}), faceplateGeometry());
    }
    applyView();   // the model's faceplate may have other proportions
    applyPanels();
    m_disk->refresh();
    // Queued after the shell's own rebuild of the menus (it runs first).
    QTimer::singleShot(0, this, [this] { applyCopyLabel(); });
}

// Edit ▸ Copy Screen reads "Copy Log Selection" while text of the INSPECTOR
// is selected; File ▸ Unmount names the folder mounted.
void MainWindow::applyCopyLabel() {
    if (!m_inspector) return;
    const QString copyName = QString::fromStdString(calcu::shell::names::item(calcu::shell::actions::kCopyScreen));
    if (QAction* copy = menuBar()->findChild<QAction*>(copyName))
        copy->setText(m_inspector->selectedOutputText().isEmpty() ? tr("Copy Screen") : tr("Copy Log Selection"));
    const QString unmountName = QString::fromStdString(calcu::shell::names::item(cu1600::kItemUnmountHostDrive));
    if (QAction* unmount = menuBar()->findChild<QAction*>(unmountName)) {
        const std::string dir = calcu::shell::read::text(
            productEntry(host().value(calcu::control::kControlStateTopic), cu1600::kHostDriveFolderEntry), "folder");
        unmount->setText(dir.empty() ? tr("Unmount") : tr("Unmount \"%1\"").arg(QDir(QString::fromStdString(dir)).dirName()));
        unmount->setToolTip(QString::fromStdString(dir));
    }
}

// The two status lines of Serial ports (not settings) and the debug server's
// status with Disconnect (in the Debugger section), added to the
// shell's dialog while it is open.
void MainWindow::refreshSerialStatus() {
    calcu::qt6::ShellSettingsDialog* d = m_shell->settingsDialog();
    if (!d) return;
    for (QGroupBox* box : d->findChildren<QGroupBox*>()) {
        auto* form = qobject_cast<QFormLayout*>(box->layout());
        if (!form) continue;
        auto line = [&](const char* name, const QString& label) {
            auto* l = box->findChild<QLabel*>(QString::fromLatin1(name));
            if (!l) {
                l = new QLabel(box);
                l->setObjectName(QString::fromLatin1(name));
                l->setTextInteractionFlags(Qt::TextSelectableByMouse);
                form->addRow(label, l);
            }
            return l;
        };
        if (box->title() == QLatin1String("Serial ports")) {
            box->setObjectName(QStringLiteral("settings-serial"));   // the scenarios' capture
            const QString rs = m_controller->serialLinkStatus();
            const QString ce = m_controller->ce158SerialLinkStatus();
            const QString rsText = rs.isEmpty() ? tr("(PC-1600 not active)") : AppPaths::displayPath(rs);
            const QString ceText = ce.isEmpty() ? tr("(CE-158 not attached)") : AppPaths::displayPath(ce);
            if (QLabel* l = line("serial-status-rs232c", tr("PC-1600 RS-232C:")); l->text() != rsText) l->setText(rsText);
            if (QLabel* l = line("serial-status-ce158", tr("CE-158 port:")); l->text() != ceText) l->setText(ceText);
        } else if (box->title() == QLatin1String("Debugger")) {
            // The topic c1600.dap and its command.
            QLabel* status = line("debug-status", tr("Status:"));
            const QString text = m_controller->dapStatus();
            if (status->text() != text) status->setText(text);
            auto* disconnect = box->findChild<QPushButton*>(QStringLiteral("debug-disconnect"));
            if (!disconnect) {
                disconnect = new QPushButton(tr("Disconnect"), box);
                disconnect->setObjectName(QStringLiteral("debug-disconnect"));
                disconnect->setToolTip(tr("Drop the attached debugger; the calculator runs on"));
                connect(disconnect, &QPushButton::clicked, m_controller.get(), &MachineController::disconnectDap);
                form->addRow(QString(), disconnect);
            }
            disconnect->setEnabled(m_controller->dapClient());
        }
    }
}

// The picture of the display at its physical size, plus the screen as
// readable text (LcdText::plainText()) when any of it is text -- a text
// editor pastes the text, an image app the dots. While text of the
// INSPECTOR is selected: that text (Copy Log Selection).
void MainWindow::copyScreen() {
    const QString selection = m_inspector ? m_inspector->selectedOutputText() : QString();
    if (!selection.isEmpty()) {
        if (m_clipboardSink) m_clipboardSink(QImage(), selection);
        else QGuiApplication::clipboard()->setText(selection);
        return;
    }
    const GrayImage screen = m_controller->currentScreenImage();
    const QImage image = toQImage(screen);
    if (image.isNull()) return;
    LcdText lcd;
    const QString text = m_controller->lcdText(&lcd) ? QString::fromStdString(lcd.plainText()) : QString();
    if (m_clipboardSink) {
        m_clipboardSink(image, text);
        return;
    }
#ifdef Q_OS_MACOS
    // Qt's QClipboard::setImage() drops the physical size on macOS (see
    // qt6/MacClipboard.h) -- go through Cocoa so a paste lands at the
    // real display's size.
    const double widthPt = screen.width * 72.0 / screen.dpi;
    const double heightPt = screen.height * 72.0 / screen.dpi;
    if (calcu::qt6::macSetClipboardImage(image, widthPt, heightPt, text)) return;
#endif
    auto* mime = new QMimeData;
    mime->setImageData(image);
    if (!text.isEmpty()) mime->setText(text);
    QGuiApplication::clipboard()->setMimeData(mime);
}

QImage MainWindow::screenImage() const { return toQImage(m_controller->currentScreenImage()); }

QImage MainWindow::toQImage(const GrayImage& screen) {
    if (screen.width <= 0 || screen.height <= 0) return {};
    QImage image(screen.width, screen.height, QImage::Format_Grayscale8);
    for (int y = 0; y < screen.height; ++y) {
        std::memcpy(image.scanLine(y), screen.pixels.data() + static_cast<std::size_t>(y) * screen.width,
                    static_cast<std::size_t>(screen.width));
    }
    const int dotsPerMeter = static_cast<int>(screen.pixelsPerMeter());
    image.setDotsPerMeterX(dotsPerMeter);
    image.setDotsPerMeterY(dotsPerMeter);
    return image;
}

// File ▸ Host Drive…: the host directory becomes drive S3: (alias Y:
// without a CE-1600F). The first mount plugs the host drive in, with the
// power cycle every peripheral attach gets; mounting another directory
// while one is mounted is a live media swap.
void MainWindow::mountHostDirectory() {
    const QString dir = m_shell->choosers().folder(this, tr("Host Drive (S3:)"), m_shell->startFolder("host-drive"));
    if (dir.isEmpty()) return;
    host().send(calcu::shell::commands::kRememberFolder,
                calcu::sessions::Value::object({{"kind", "host-drive"}, {"folder", dir.toStdString()}}));
    m_shell->sendChecked(cu1600::commands::kMountHostDrive, calcu::sessions::Value::object({{"dir", dir.toStdString()}}));
}

void MainWindow::unmountHostDirectory() {
    m_shell->sendChecked(cu1600::commands::kMountHostDrive, calcu::sessions::Value::object({{"dir", ""}}));
}

// File ▸ Load BASIC Program… / Load Machine Code…: the file dialog starts in
// Settings' folder of its kind ([folders]), which remembers it.
void MainWindow::chooseProgram(bool code) {
    const QString title = code ? tr("Load Machine Code") : tr("Load BASIC Program");
    const std::string kind = code ? "machine-code" : "basic-programs";
    const QString filter = code ? tr("Machine Code (*.bin *.wav);;All Files (*)") : tr("BASIC Programs (*.bas *.bbin *.wav);;All Files (*)");
    const QString path = m_shell->choosers().file(this, title, m_shell->startFolder(kind), filter);
    if (path.isEmpty()) return;
    m_shell->rememberFolder(kind, path);
    const auto bytes = readChosen(path, title);
    if (!bytes) return;
    if (code) loadMachineCode(QFileInfo(path).fileName(), *bytes);
    else loadBasicProgram(QFileInfo(path).fileName(), *bytes);
}

std::optional<QByteArray> MainWindow::readChosen(const QString& path, const QString& title) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, title, tr("Could not read %1: %2").arg(path, file.errorString()));
        return std::nullopt;
    }
    return file.readAll();
}

std::optional<calcu::sessions::Value::Object> MainWindow::programArgs(const QString& name, const QByteArray& raw,
                                                                      const QString& title, std::optional<int> chosen) {
    namespace sessions = calcu::sessions;
    sessions::Value::Object args{{"bytes", sessions::Value(sessions::Value::Bytes(raw.begin(), raw.end()))},
                                 {"name", sessions::Value(name.toStdString())}};
    if (chosen) {
        args.push_back({"file", sessions::Value(*chosen)});
        return args;
    }
    // A WAV with several files asks which one.
    const sessions::ReplyMessage kind = m_controller->ask(
        cu1600::requests::kFileKind, sessions::Value::object({{"bytes", args.front().value}}));
    const std::optional<int> file = pickWavFile(title, name, sessions::payloadAs<cu1600::FileKindReply>(kind.result));
    if (!file) return std::nullopt;
    if (*file > 0) args.push_back({"file", sessions::Value(*file)});
    return args;
}

std::optional<int> MainWindow::pickWavFile(const QString& title, const QString& name, const cu1600::FileKindReply* k) {
    if (!k || k->tapeFiles.size() <= 1) return 0;
    QStringList names;
    for (std::size_t i = 0; i < k->tapeFiles.size(); ++i)
        names << QStringLiteral("%1. %2").arg(i + 1).arg(QString::fromStdString(k->tapeFiles[i]));
    bool ok = false;
    const QString picked = QInputDialog::getItem(
        this, title, tr("%1 holds %2 files. Load which one?").arg(name).arg(names.size()), names, 0, false, &ok);
    if (!ok) return std::nullopt;
    return static_cast<int>(names.indexOf(picked)) + 1;
}

void MainWindow::loadBasicProgram(const QString& name, const QByteArray& bytes, std::optional<int> file) {
    const QString title = tr("Load BASIC Program");
    std::optional<calcu::sessions::Value::Object> args = programArgs(name, bytes, title, file);
    if (!args) return;
    m_shell->sendChecked(cu1600::commands::kLoadBasic, calcu::sessions::Value(std::move(*args)));
}

void MainWindow::loadMachineCode(const QString& name, const QByteArray& bytes, std::optional<int> file) {
    namespace sessions = calcu::sessions;
    const QString title = tr("Load Machine Code");
    std::optional<sessions::Value::Object> args = programArgs(name, bytes, title, file);
    if (!args) return;
    // The one plan: a refusal is the notice; headerless code
    // asks the address.
    const sessions::ReplyMessage planned = m_controller->ask(cu1600::requests::kCodePlan, sessions::Value(*args));
    const auto* plan = sessions::payloadAs<cu1600::CodePlanReply>(planned.result);
    if (!planned.done || !plan) {
        QMessageBox::warning(this, title, QString::fromStdString(planned.reason));
        return;
    }
    if (plan->needsAddress) {
        sessions::Value::Object base = *args;
        MachineCodeLoadDialog dialog(this, [this](const sessions::RequestName& n, sessions::Value a) { return m_controller->ask(n, std::move(a)); }, sessions::Value(std::move(base)), *plan);
        if (dialog.exec() != QDialog::Accepted) return;
        args->push_back({"address", sessions::Value(dialog.address().toStdString())});
    } else if (!plan->refusal.empty()) {
        QMessageBox::warning(this, title, name + QStringLiteral(": ") + QString::fromStdString(plan->refusal));
        return;
    }
    m_controller->send(cu1600::commands::kLoadCode, sessions::Value(std::move(*args)),
                       [this, title](const sessions::ReplyMessage& r) {
                           const auto* loaded = sessions::payloadAs<cu1600::CodeLoadedReply>(r.result);
                           if (!r.done || !loaded) {
                               QMessageBox::warning(this, title, QString::fromStdString(r.reason));
                               return;
                           }
                           showMachineCodeLoaded(*loaded);
                       });
}

void MainWindow::showMachineCodeLoaded(const cu1600::CodeLoadedReply& loaded) {
    QString html = QStringLiteral("<p>%1.</p>").arg(QString::fromStdString(loaded.summary).toHtmlEscaped());
    html += QStringLiteral("<p>%1<br>").arg(tr("Keep BASIC from overwriting it:").toHtmlEscaped());
    if (!loaded.newCommand.empty())
        html += QStringLiteral("<b><tt>%1</tt></b><br>").arg(QString::fromStdString(loaded.newCommand).toHtmlEscaped());
    html += QStringLiteral("<small>%1</small></p>").arg(QString::fromStdString(loaded.newNote).toHtmlEscaped());
    html += QStringLiteral("<p>%1<br><b><tt>%2</tt></b><br><small>%3</small></p>")
                .arg(tr("Start it:").toHtmlEscaped(),
                     QString::fromStdString(loaded.callCommand).toHtmlEscaped(),
                     QString::fromStdString(loaded.callNote).toHtmlEscaped());

    QMessageBox box(QMessageBox::Information, tr("Machine Code Loaded"), html, QMessageBox::Ok, this);
    box.setTextFormat(Qt::RichText);
    box.setTextInteractionFlags(Qt::TextSelectableByMouse);
    box.exec();

    // Type the proposed CALL, ready to run: the user presses ENTER.
    m_controller->pasteText(loaded.callCommand);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        // Qt delivers OS auto-repeat as repeated .down events -- without
        // this, holding a shift-needing key would re-fire the whole
        // tapShiftedKey choreography on every repeat tick.
        event->accept();
        return;
    }

    // A Shift press arms the tap; any other key (letters, Cmd/Ctrl/Alt,
    // Shift+Delete, ...) means Shift is being used as a modifier.
    if (event->key() == Qt::Key_Shift) {
        armShiftTap();
        QWidget::keyPressEvent(event);
        return;
    }
    disarmShiftTap();

    // The host key as Qt reports it; the product's keyboard map in the
    // session decides what it types. A bare modifier
    // (the Cmd of a second Cmd-V, Cmd-Tab) is no key: it does not stop a
    // paste.
    const auto down = m_hostKeys.press(*event);
    if (!down) {
        QWidget::keyPressEvent(event);
        return;
    }
    m_controller->hostKeyDown(*down);
    if (calcu::qt6::commandModifier(event->modifiers())) {   // a shortcut no action claimed
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

void MainWindow::inputMethodEvent(QInputMethodEvent* event) {
    // The preedit (a pending ¨) isn't shown -- the calculator shows nothing
    // until the letter either. The committed text goes to the session, which
    // types each character as a press of the minimum hold by the keyboard
    // map; one with no key is dropped (a bare ¨ after dead key + Space,
    // ...).
    disarmShiftTap();
    if (!event->commitString().isEmpty()) m_controller->hostText(event->commitString().toStdString());
    event->accept();
}

QVariant MainWindow::inputMethodQuery(Qt::InputMethodQuery query) const {
    switch (query) {
    case Qt::ImEnabled:
        return true;
    case Qt::ImHints:
        return static_cast<int>(Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase);
    case Qt::ImCursorRectangle:
        // No text cursor: put input-method popups (candidate lists, the
        // character viewer) at the LCD, where the typing shows.
        if (m_faceplate && m_faceplate->lcdWidget()) {
            const QWidget* lcd = m_faceplate->lcdWidget();
            return QRect(lcd->mapTo(this, QPoint(0, 0)), lcd->size());
        }
        return QRect();
    default:
        return QMainWindow::inputMethodQuery(query);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Shift) {
        const bool tapped = m_shiftTapArmed && m_shiftTapClock.elapsed() <= kShiftTapMaxMs;
        disarmShiftTap();
        if (tapped) {
            // One press of the machine's shift key (the tap is measured in
            // wall time, here); the engine holds it its minimum.
            m_controller->pressKey("shift");
            m_controller->releaseKey("shift");
            m_faceplate->flashKey("shift");   // too short for the held keys to show
            event->accept();
            return;
        }
    }

    const auto up = m_hostKeys.release(*event);
    if (!up) {
        QWidget::keyReleaseEvent(event);
        return;
    }
    m_controller->hostKeyUp(*up);
    event->accept();
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange && !isActiveWindow()) releaseHeldKeys();
    QMainWindow::changeEvent(event);
}

void MainWindow::releaseHeldKeys() {
    if (m_controller) m_controller->focusLost();   // every held key comes up, what waits goes (none while the window is torn down)
    m_hostKeys.clear();
    disarmShiftTap(); // the Shift release may land elsewhere
}

// The app-wide filter is only installed while a tap is armed (at most
// kShiftTapMaxMs), not for the window's life: it sees every event of
// every object, just to catch a Shift-click.
void MainWindow::armShiftTap() {
    if (!m_shiftTapArmed) qApp->installEventFilter(this);
    m_shiftTapArmed = true;
    m_shiftTapClock.start();
}

void MainWindow::disarmShiftTap() {
    if (!m_shiftTapArmed) return;
    m_shiftTapArmed = false;
    qApp->removeEventFilter(this);
}

bool MainWindow::panelsShown() const { return m_panels && !m_panels->isHidden(); }

void MainWindow::applyView() {
    QWidget* central = centralWidget();
    if (!central || !m_panels || !m_faceplate) return;
    const calcu::shell::ChromeSizes& s = cu1600::sharpChrome().sizes;
    const QMargins margins = central->layout()->contentsMargins();
    const int spacing = central->layout()->spacing();
    calcu::shell::ViewInputs in;
    in.width = central->width();
    in.height = central->height();
    const int probe = 1000;
    in.faceplateAspect = static_cast<double>(probe) / std::max(1, m_faceplate->heightForWidth(probe));
    in.fixedWidth = margins.left() + margins.right();
    const int bar = std::max(static_cast<int>(s.barHeight), m_shell->controlBar()->sizeHint().height());
    in.fixedHeight = margins.top() + margins.bottom() + bar + 2 * spacing;
    in.panelsMinWidth = s.panelsMinWidth;
    in.panelsMinHeight = s.panelsMinHeight;
    in.debuggerMinWidth = s.debuggerMinWidth;
    in.debuggerMinHeight = s.debuggerMinHeight;
    const calcu::shell::View v = calcu::shell::view(in);
    // The panels have a row of their own only below the faceplate.
    m_panels->setVisible(v.side == calcu::shell::ViewSide::Below && v.panelsFit);
    if (v.debuggerFits != m_debuggerFits) {
        m_debuggerFits = v.debuggerFits;
        applyPanels();
    }
    fitBar();
}

void MainWindow::applyPanels() {
    if (!m_panels || !m_inspector || !m_ce158Printer) return;
    const calcu::sessions::Value cs = host().value(calcu::control::kControlStateTopic);
    std::string paperOwner;
    for (const auto& p : calcu::shell::read::elements(cs, "panels"))
        if (calcu::shell::read::text(p, "kind") == "paper")
            if (const calcu::sessions::Value* o = p.find("owner")) paperOwner = calcu::shell::read::text(*o, "peripheral");
    const bool printer = calcu::shell::read::flag(productEntry(cs, cu1600::kCe158PrinterPanelEntry), "shown");
    const bool paper = m_debuggerFits || !paperOwner.empty();
    m_panels->setShown("paper", paper);
    m_ce158Printer->setVisible(printer);
    m_inspector->setVisible(m_debuggerFits || !(paper || printer));
}

void MainWindow::fitBar() {
    if (!m_shell || !m_faceplate || !centralWidget()) return;
    const QMargins margins = centralWidget()->layout()->contentsMargins();
    const double face = m_faceplate->artWidth();
    if (face > 0) m_shell->fitBar(face, centralWidget()->width() - margins.left() - margins.right());
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == centralWidget() && event->type() == QEvent::Resize) applyView();
    if (event->type() == QEvent::MouseButtonPress) disarmShiftTap(); // a Shift-click
    return QMainWindow::eventFilter(watched, event);
}

// ── Drag and drop, opening a file ──────────────────────────────────────

// One file, of a name and size the session would take; the drop itself
// goes through all of the session's checks, accepted and opened after the
// drop handler (library/qt6/FileOpening). More than one file is not taken.
void MainWindow::dragEnterEvent(QDragEnterEvent* event) { calcu::qt6::dragEnter(event, host()); }

void MainWindow::dragMoveEvent(QDragMoveEvent* event) { calcu::qt6::dragMove(event); }

void MainWindow::dropEvent(QDropEvent* event) {
    calcu::qt6::drop(event, this, [this](const QString& path) { openFile(path, calcu::opening::Way::Drop); });
}

void MainWindow::openFile(const QString& path, calcu::opening::Way way) {
    // The window raised for every way but the start; read up to the
    // way's limit (1 MiB but for the file dialogs); a refusal is the
    // session's notice already.
    calcu::qt6::openFile(*this, host(), path, way, m_controller->app().libraryParts().open().setup().maxBytes);
}

void MainWindow::onProgramOpened(const QString& name, const QByteArray& bytes, const QString& kind) {
    namespace sessions = calcu::sessions;
    if (m_programFlow) {
        qDebug().noquote() << "[open] a program flow runs; dropped" << name;
        return;
    }
    m_programFlow = true;
    bool code = kind == QStringLiteral("machine-code-program");
    std::optional<int> chosen;
    // A WAV with several files asks which one; the file
    // chosen goes to the flow of its own kind.
    const sessions::ReplyMessage r = m_controller->ask(
        cu1600::requests::kFileKind,
        sessions::Value::object({{"bytes", sessions::Value(sessions::Value::Bytes(bytes.begin(), bytes.end()))}}));
    const auto* k = sessions::payloadAs<cu1600::FileKindReply>(r.result);
    const std::optional<int> file = pickWavFile(tr("Open"), name, k);
    if (!file) {
        m_programFlow = false;
        return;
    }
    if (*file > 0) {
        chosen = *file;
        const auto n = static_cast<std::size_t>(*file - 1);
        if (n < k->tapeKinds.size()) code = k->tapeKinds[n].rfind("code-", 0) == 0;
    }
    if (code) loadMachineCode(name, bytes, chosen);
    else loadBasicProgram(name, bytes, chosen);
    m_programFlow = false;
}

