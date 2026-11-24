#include "qt6/AppLogSink.hpp"

#include <QDir>
#include <QString>
#include <QtGlobal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <optional>
#include <vector>

#include "qt6/HostInfo.hpp"
#include "settings/PlatformBase.hpp"
#include "shell/HostAppLog.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace calcu::qt6 {

namespace {

namespace cs = calcu::settings;

void mirror(applog::AppLogLevel, const std::string& line) {
#ifdef Q_OS_WIN
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(QString::fromStdString(line + "\n").utf16()));
#else
    std::fputs((line + "\n").c_str(), stderr);
#endif
}

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
    shell::hostAppLog().write(levelOf(type), msg.toStdString());
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
    shell::hostAppLog().error("The app ended: " + what);
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
    shell::installHostAppLog(platformPath(productName), applog::headerLine(productName, version, build, hostSupportRows()),
                             std::vector<std::string>(argv, argv + argc), &mirror);
    qInstallMessageHandler(qtMessage);
    std::set_terminate(terminated);
}

} // namespace calcu::qt6
