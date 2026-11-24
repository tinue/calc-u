#pragma once
#include <string>

// The app log of a Qt6 app, installed once for both products: the native
// hosts' log (shell::HostAppLog, read through shell::hostAppLog()) with the
// file at the platform's place (settings::appLogFilePathFor), every line
// also on standard error (Windows: the debugger output). Qt's own messages
// come in through a message handler (qDebug debug, qInfo info, qWarning
// warn, qCritical and qFatal error); an end through std::terminate writes
// an error line first.

namespace calcu::qt6 {

// Before QApplication, so Qt's first messages are kept. `argv` is read for
// `--app-log-level`; `version` and `build` go into the run's header line.
void installAppLog(const std::string& productName, const std::string& version, const std::string& build, int argc,
                   char** argv);

} // namespace calcu::qt6
