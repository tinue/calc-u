#pragma once
#include "applog/AppLog.hpp"
#include "applog/AppLogFile.hpp"
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The app log of a native app (Qt6, iOS), a host adapter: the file at the
// platform's place, every line also in the platform's console (the mirror).
// It is written from the GUI thread, the session thread and the host's own
// handlers, so this adapter at the session's edge holds a lock around the
// file. The level is the run's `--app-log-level <level>` when given, else
// the session follows `[app-log] level` (HostSessionSetup::appLogLevel);
// never built by the Web, whose page and worker log on their own.

namespace calcu::shell {

class HostAppLog final : public applog::AppLog {
public:
    using Mirror = std::function<void(applog::AppLogLevel, const std::string& line)>;

    // `args`: the launch arguments, read for `--app-log-level`; a bad value
    // is a warn line of its own. A file that cannot be written says so in
    // an error line to the mirror, which then takes every line alone.
    HostAppLog(std::string path, std::string_view header, const std::vector<std::string>& args, Mirror mirror);

    const std::string& path() const { return file_.path(); }
    // The level the command line fixed for this run, if it did.
    std::optional<applog::AppLogLevel> runLevel() const { return runLevel_; }

protected:
    void put(applog::AppLogLevel, std::string_view text) override;

private:
    HostAppLog(std::string path, std::string_view header, applog::RunAppLogLevel run, const Mirror& mirror);
    std::optional<applog::AppLogLevel> runLevel_;
    std::mutex lock_;
    applog::AppLogFile file_;
};

// The one app log of the process, made once at the app's start (a second
// call keeps the first), before anything logs.
void installHostAppLog(std::string path, std::string_view header, const std::vector<std::string>& args,
                       HostAppLog::Mirror mirror);
// The installed app log (a log that takes nothing before the install).
applog::AppLog& hostAppLog();
// The installed log's run level (none before the install).
std::optional<applog::AppLogLevel> hostAppLogLevelOfRun();
// The installed log's file ("" before the install).
std::string hostAppLogPath();

} // namespace calcu::shell
