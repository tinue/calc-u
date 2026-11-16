#include "Operations.hpp"

#include <QEventLoop>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>
#include <QWidget>

#include "presets/PresetsPart.hpp"

namespace sessions = calcu::sessions;

namespace {
// How long an operation must run at full speed before "Loading…" appears
// (short ones finish without flashing it).
constexpr int kPopupDelayMs = 500;
} // namespace

Operations::Operations(QWidget* window, MachineController* machines, QObject* parent)
    : QObject(parent), m_window(window), m_machines(machines) {
    m_popupTimer = new QTimer(this);
    m_popupTimer->setSingleShot(true);
    connect(m_popupTimer, &QTimer::timeout, this, &Operations::showPopupIfLong);
    connect(m_machines, &MachineController::appBusyChanged, this, &Operations::onBusy);
    connect(m_machines, &MachineController::operationSpeedChanged, this, [this] {
        if (!m_machines->operationAtFullSpeed()) m_popup.reset();   // a stretch at regular speed: no "Loading…"
        else if (m_machines->busy() && !m_popupTimer->isActive() && !m_popup) m_popupTimer->start(kPopupDelayMs);
    });
}

Operations::~Operations() = default;

bool Operations::busy() const { return m_machines->busy(); }

void Operations::run(const QString& title, const sessions::CommandName& name, sessions::Value args, Done done) {
    m_title = title;
    m_machines->send(name, std::move(args), [this, title, done = std::move(done)](const sessions::ReplyMessage& r) {
        const QString reason = QString::fromStdString(r.reason);
        if (!r.done && !reason.isEmpty()) QMessageBox::warning(m_window, title, reason);
        if (done) done(r.done, reason);
    });
}

bool Operations::runAndWait(const QString& title, const sessions::CommandName& name, sessions::Value args,
                            QString* error) {
    m_title = title;
    bool over = false, ok = false;
    QString reason;
    QEventLoop loop;
    m_machines->send(name, std::move(args), [&](const sessions::ReplyMessage& r) {
        over = true;
        ok = r.done;
        reason = QString::fromStdString(r.reason);
        loop.quit();
    });
    if (m_machines->frozen()) m_machines->finishOperation();
    // The reply is handed over through the event loop, also when it came at once.
    if (!over) loop.exec();
    if (!ok) {
        if (error) *error = reason.isEmpty() ? tr("%1 failed.").arg(title) : reason;
        else if (!reason.isEmpty()) QMessageBox::warning(m_window, title, reason);
    }
    return ok;
}

void Operations::loadPreset(const QString& path) {
    // LIB-6 AC-54: a file in its folder, or a bundle path (`examples:…`).
    run(tr("Load Preset"), calcu::presets::kLoadPresetCommand, sessions::Value::object({{"ref", path.toStdString()}}));
}

void Operations::onBusy(bool busy) {
    if (busy) {
        m_window->setCursor(Qt::WaitCursor);
        m_popupTimer->start(kPopupDelayMs);
        return;
    }
    m_popupTimer->stop();
    m_popup.reset();
    m_window->unsetCursor();
}

void Operations::showPopupIfLong() {
    if (!m_machines->operationAtFullSpeed() || m_popup || m_machines->frozen()) return;
    m_popup = std::make_unique<QProgressDialog>(tr("Loading…"), QString(), 0, 0, m_window);
    m_popup->setWindowTitle(m_title);
    m_popup->setWindowModality(Qt::WindowModal);
    m_popup->setMinimumDuration(0);
    m_popup->show();
}
