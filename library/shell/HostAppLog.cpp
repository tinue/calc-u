#include "shell/HostAppLog.hpp"
#include <memory>

namespace calcu::shell {

HostAppLog::HostAppLog(std::string path, std::string_view header, const std::vector<std::string>& args,
                       Mirror mirror)
    : HostAppLog(std::move(path), header, applog::appLogLevelOfArguments(args), mirror) {}

HostAppLog::HostAppLog(std::string path, std::string_view header, applog::RunAppLogLevel run, const Mirror& mirror)
    : AppLog(run.level.value_or(applog::defaultAppLogLevel())), runLevel_(run.level),
      file_(std::move(path), header, mirror) {
    if (!file_.problem().empty() && mirror) mirror(applog::AppLogLevel::Error, file_.problem());
    if (!run.problem.empty()) warn(run.problem);
}

void HostAppLog::put(applog::AppLogLevel l, std::string_view text) {
    const std::lock_guard<std::mutex> hold(lock_);
    file_.write(l, text);
}

namespace {
std::unique_ptr<HostAppLog>& installed() {
    static std::unique_ptr<HostAppLog> log;
    return log;
}
} // namespace

void installHostAppLog(std::string path, std::string_view header, const std::vector<std::string>& args,
                       HostAppLog::Mirror mirror) {
    if (!installed()) installed() = std::make_unique<HostAppLog>(std::move(path), header, args, std::move(mirror));
}

applog::AppLog& hostAppLog() {
    return installed() ? static_cast<applog::AppLog&>(*installed()) : applog::nullAppLog();
}

std::optional<applog::AppLogLevel> hostAppLogLevelOfRun() {
    return installed() ? installed()->runLevel() : std::nullopt;
}

std::string hostAppLogPath() { return installed() ? installed()->path() : std::string(); }

} // namespace calcu::shell
