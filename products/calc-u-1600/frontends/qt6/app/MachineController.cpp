#include "MachineController.hpp"

#include <cstdlib>

#include <QDebug>
#include <QMessageBox>
#include <QtGlobal>

#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include "debug/DebugController.hpp"

MachineController::MachineController(QObject* parent) : QObject(parent) {
    shared::AppSession::Platform& platform = *this;
    m_app = std::make_unique<shared::AppSession>(AppSettings::settings(), AppPaths::paths(), platform);
    m_debug = std::make_unique<DebugController>(this);
    m_app->start();
    m_debug->refreshServer();
}

MachineController::~MachineController() {
    // The debugger goes first: its target refers to the live machine.
    m_debug.reset();
    m_app.reset();
}

// A file of the ROM full set is missing or bad (the start's lookup, LIB-12
// AC-5, AC-13; design D-8): no machine starts. Show the lookup's message --
// every file and its problem, the folder, that the full set is needed and
// how to fetch it -- then exit: an abort (qFatal) would be a silent crash
// for an app not launched from a terminal. std::exit() (not returning to
// unwind the call stack) is deliberate: this is reached mid-construction of
// MainWindow/MachineController, which aren't set up to unwind safely from
// here, and terminating the process reclaims everything the OS owns
// regardless.
void MachineController::romMissing(const calcu::roms::RomsMissing& missing) {
    QMessageBox::critical(nullptr, QObject::tr("Calc-U-1600 cannot start"),
                          QString::fromStdString(calcu::roms::describe(missing)));
    std::exit(1);
}

void MachineController::notice(shared::MachineSession::Notice notice, const std::string& detail) {
    const auto message = shared::MachineSession::noticeText(notice, detail);
    // A failed tape save is logged, not shown: it happens inside a rebuild.
    if (notice == shared::MachineSession::Notice::TapeSaveFailed) {
        qWarning("Tape: %s", detail.c_str());
        return;
    }
    QMessageBox::warning(nullptr, QString::fromStdString(message.title), QString::fromStdString(message.text));
}

void MachineController::machineAboutToChange() {
    if (m_debug) m_debug->machineAboutToChange();
}

void MachineController::machineReplaced() {
    if (m_debug) m_debug->machineReplaced();
}

void MachineController::bayError(shared::Bay bay, const std::string& text) {
    emit bayErrorMessage(bay, QString::fromStdString(text));
}

void MachineController::presetLog(const std::string& line) {
    qDebug().noquote() << "[preset]" << QString::fromStdString(line);
}

void MachineController::attachFailed(const std::string& device, const std::string& reason) {
    emit peripheralAttachFailed(QString::fromStdString(device), QString::fromStdString(reason));
}

bool MachineController::runDebuggerSlice(std::uint64_t cycles) {
    if (!m_debug || !m_debug->attached()) return false;
    m_debug->runSlice(cycles);
    return true;
}

void MachineController::refreshDebugServer() { m_debug->refreshServer(); }

bool MachineController::tapeEject(QString* error) {
    return withQtError(error, [this](std::string* err) { return session().tapeEject(err); });
}
