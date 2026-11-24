#pragma once
#include <QString>
#include <optional>
#include <string>

#include "applog/AppLog.hpp"

// The app log of a Qt6 app, written once for both products: the file at the
// platform's place (settings::appLogFilePathFor), every line also on standard
// error (Windows: the debugger output). Qt's own messages come in through a
// message handler (qDebug debug, qInfo info, qWarning warn, qCritical and
// qFatal error); an end through std::terminate writes an error line first.
// The level is the run's `--app-log-level <level>` when given, else the
// session follows `[app-log] level` (shell::HostSessionSetup). A host
// adapter: written from any thread, so the file is guarded by a lock.

namespace calcu::qt6 {

// Before QApplication, so Qt's first messages are kept. `argv` is read for
// `--app-log-level`; `version` and `build` go into the run's header line.
void installAppLog(const std::string& productName, const std::string& version, const std::string& build, int argc,
                   char** argv);

// The installed app log (a log that takes nothing before installAppLog).
applog::AppLog& appLog();
// The level the command line fixed for this run, if it did.
std::optional<applog::AppLogLevel> appLogLevelOfRun();
// The app log's file ("" when none was installed).
QString appLogFilePath();

} // namespace calcu::qt6
