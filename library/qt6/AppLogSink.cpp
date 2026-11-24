#include "qt6/AppLogSink.hpp"

#include <QDir>
#include <QSysInfo>
#include <QtGlobal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>

#include "applog/AppLogFile.hpp"
#include "settings/PlatformBase.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace calcu::qt6 {

namespace {

namespace cs = calcu::settings;

// The file guarded by a lock; the level is the outer log's (write() leaves
// out what is above it), the file itself takes every line.
class GuardedAppLog final : public applog::AppLog {
public:
    GuardedAppLog(std::string path, const std::string& header, applog::AppLogLevel level)
        : AppLog(level), file_(std::move(path), header, applog::AppLogLevel::Debug, &mirror) {}
    const applog::AppLogFile& file() const { return file_; }

protected:
    void put(applog::AppLogLevel l, std::string_view text) override {
        const std::lock_guard<std::mutex> hold(lock_);
        file_.write(l, text);
    }

private:
    static void mirror(applog::AppLogLevel, const std::string& line) {
#ifdef Q_OS_WIN
        OutputDebugStringW(reinterpret_cast<const wchar_t*>(QString::fromStdString(line + "\n").utf16()));
#else
        std::fputs((line + "\n").c_str(), stderr);
#endif
    }
    std::mutex lock_;
    applog::AppLogFile file_;
};

std::unique_ptr<GuardedAppLog> installed;
std::optional<applog::AppLogLevel> runLevel;

applog::AppLogLevel levelOf(QtMsgType t) {
    switch (t) {
    case QtDebugMsg: return applog::AppLogLevel::Debug;
    case QtInfoMsg: return applog::AppLogLevel::Info;
    case QtWarningMsg: return applog::AppLogLevel::Warn;
    case QtCriticalMsg:
    case QtFatalMsg: return applog::AppLogLevel::Error;
    }
    return applog::AppLogLevel::Warn;
}

void qtMessage(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    appLog().write(levelOf(type), msg.toStdString());
    if (type == QtFatalMsg) std::abort();
}

void terminated() {
    std::string what = "an unknown exception";
    if (const std::exception_ptr e = std::current_exception()) {
        try {
            std::rethrow_exception(e);
        } catch (const std::exception& x) {
            what = x.what();
        } catch (...) {
        }
    } else {
        what = "no exception (std::terminate was called)";
    }
    appLog().error("The app ended: " + what);
    std::abort();
}

std::string platformPath(const std::string& productName) {
    const cs::EnvLookup env = [](const char* name) -> std::optional<std::string> {
        if (!qEnvironmentVariableIsSet(name)) return std::nullopt;
        return qEnvironmentVariable(name).toStdString();
    };
    const std::string home = QDir::homePath().toStdString();
#if defined(Q_OS_MACOS)
    return cs::appLogFilePathFor(cs::HostOs::MacOS, env, home, productName);
#elif defined(Q_OS_WIN)
    return cs::appLogFilePathFor(cs::HostOs::Windows, env, home, productName);
#else
    return cs::appLogFilePathFor(cs::HostOs::Linux, env, home, productName);
#endif
}

} // namespace

void installAppLog(const std::string& productName, const std::string& version, const std::string& build, int argc,
                   char** argv) {
    std::string badLevel;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "--app-log-level") == 0) {
            runLevel = applog::appLogLevelNamed(argv[i + 1]);
            if (!runLevel) badLevel = argv[i + 1];
        }
    const std::string header = productName + " " + version + " (" + build + ") " +
                               QSysInfo::prettyProductName().toStdString() + ", " +
                               QSysInfo::currentCpuArchitecture().toStdString();
    installed = std::make_unique<GuardedAppLog>(platformPath(productName), header,
                                                runLevel.value_or(applog::defaultAppLogLevel()));
    if (!installed->file().problem().empty()) std::fprintf(stderr, "%s\n", installed->file().problem().c_str());
    if (!badLevel.empty()) installed->warn("--app-log-level takes error, warn, info or debug, not " + badLevel + ".");
    qInstallMessageHandler(qtMessage);
    std::set_terminate(terminated);
}

applog::AppLog& appLog() { return installed ? static_cast<applog::AppLog&>(*installed) : applog::nullAppLog(); }

std::optional<applog::AppLogLevel> appLogLevelOfRun() { return runLevel; }

QString appLogFilePath() { return installed ? QString::fromStdString(installed->file().path()) : QString(); }

} // namespace calcu::qt6
