#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include <QDir>
#include <QtGlobal>

#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "qt6/BundledData.hpp"
#include "qt6/MacHost.hpp"

namespace AppPaths {

namespace cs = calcu::settings;

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

// The folder, created when missing. One that cannot be created or
// written is logged and still used: nothing goes to another folder.
QString ensured(cs::Folder which) {
    const cs::Paths::Ensured e = places().ensure(which);
    if (!e.ok) qWarning("%s", e.problem.c_str());
    return q(e.path.empty() ? places().folder(which).value_or(std::string()) : e.path);
}

} // namespace

calcu::qt6::HostFolders& folders() {
    // On macOS the settings base is the app's Application Support
    // folder as macOS gives it.
    static calcu::qt6::HostFolders f(cu59::settings::settingsDecl(),
#if defined(Q_OS_MACOS)
                                     [] { return calcu::qt6::macApplicationSupportFolder(); }
#else
                                     {}
#endif
    );
    return f;
}

void useHostDirs(const cs::HostDirs& dirs) { folders().useHostDirs(dirs); }

cs::SettingsFile& file() { return folders().file(); }
const cs::Paths& places() { return folders().places(); }

QString defaultDir() {
    const QString dir = q(cs::joinPath(folders().hostDirs().home, cu59::settings::settingsDecl().productName));
    // Still the folder a file dialog starts in; it opens where the system lets it.
    if (!QDir().mkpath(dir)) qWarning("The folder %s cannot be created.", qPrintable(dir));
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
    return calcu::qt6::bundledDataDir(QStringLiteral("calc-u-59")) + QStringLiteral("/examples");
}

} // namespace AppPaths

namespace AppSettings {
cu59::settings::Settings& settings() {
    static cu59::settings::Settings s(AppPaths::file());
    return s;
}
} // namespace AppSettings
