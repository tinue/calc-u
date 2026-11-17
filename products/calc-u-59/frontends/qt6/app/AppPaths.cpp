#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"

namespace AppPaths {

namespace cs = calcu::settings;

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

// The folder, created when missing (EC-8). One that cannot be created or
// written is logged and still used: nothing goes to another folder (EC-9).
QString ensured(cs::Folder which) {
    const cs::Paths::Ensured e = places().ensure(which);
    if (!e.ok) qWarning("%s", e.problem.c_str());
    return q(e.path.empty() ? places().folder(which).value_or(std::string()) : e.path);
}

} // namespace

calcu::qt6::HostFolders& folders() {
    // The development build is not sandboxed: ~/Library/Application Support
    // on macOS (no folder of its own passed).
    static calcu::qt6::HostFolders f(cu59::settings::settingsDecl());
    return f;
}

void useHostDirs(const cs::HostDirs& dirs) { folders().useHostDirs(dirs); }

cs::SettingsFile& file() { return folders().file(); }
const cs::Paths& places() { return folders().places(); }

QString defaultDir() {
    const QString dir = q(cs::joinPath(folders().hostDirs().home, cu59::settings::settingsDecl().productName));
    QDir().mkpath(dir);
    return dir;
}

QString saveDir() { return ensured(cs::Folder::Save); }

namespace {
QString& examplesOverride() {
    static QString dir;
    return dir;
}
} // namespace

void useBundledExamplesDir(const QString& dir) { examplesOverride() = dir; }

QString bundledExamplesDir() {
    if (!examplesOverride().isEmpty()) return examplesOverride();
#if defined(Q_OS_MACOS)
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/examples");
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/resources/examples");
#endif
}

QString traceDir() { return ensured(cs::Folder::Traces); }

} // namespace AppPaths

namespace AppSettings {
cu59::settings::Settings& settings() {
    static cu59::settings::Settings s(AppPaths::file());
    return s;
}
} // namespace AppSettings
