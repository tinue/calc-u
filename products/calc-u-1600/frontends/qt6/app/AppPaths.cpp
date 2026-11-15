#include "AppPaths.hpp"
#include "AppSettings.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

#include <optional>

#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#ifdef __APPLE__
#include "MacAppSupport.h"
#endif

namespace AppPaths {

namespace {

// Where the bundled resources live -- *.card.yaml catalog AND ROM images
// (disjoint filename extensions, one shared directory) -- resolved
// relative to the running executable: macOS .app bundle's
// Contents/Resources (via applicationDirPath()'s Contents/MacOS +
// "../Resources"), or a "resources" folder next to the binary on
// Windows/Linux (matching windeployqt's and AppImage's flat-next-to-binary
// convention). Read-only bundled content -- never created if missing; a
// missing/unreadable directory is tolerated by scanMemoryCardDirectory()
// (an empty catalogue); the ROM lookup then names every file missing.
QString bundledResourcesDir() {
    const QString exeDir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
    // applicationDirPath() == .../Calc-U-1600.app/Contents/MacOS
    return QDir::cleanPath(QDir(exeDir).filePath(QStringLiteral("../Resources")));
#else
    // Windows: exeDir/resources (windeployqt lays extra files flat next
    // to the .exe by convention). Linux/AppImage: exeDir/resources --
    // AppRun launches the real binary from inside the AppDir, and a
    // "resources" dir placed next to it (see CMakeLists.txt's install()
    // rule) keeps this relative lookup correct whether running from a
    // raw build dir, an installed prefix, or a mounted AppImage.
    return QDir(exeDir).filePath(QStringLiteral("resources"));
#endif
}

QString q(const std::string& s) { return QString::fromStdString(s); }

std::optional<calcu::settings::HostDirs>& chosenHostDirs() {
    static std::optional<calcu::settings::HostDirs> dirs;
    return dirs;
}

const calcu::settings::HostDirs& hostDirs() {
    static const calcu::settings::HostDirs dirs = chosenHostDirs() ? *chosenHostDirs() : platformHostDirs();
    return dirs;
}

// The settings file, read once at the first use (LIB-9 AC-5, AC-6). A
// file that cannot be read, or the first write that fails, goes to the log
// (EC-6, AC-7); the app then runs on the defaults.
calcu::settings::SettingsFile& settingsFile() {
    static calcu::settings::SettingsFile file = [] {
        const calcu::settings::ProductDecl& decl = shared::settingsDecl();
        calcu::settings::SettingsFile f =
            calcu::settings::SettingsFile::open(decl, calcu::settings::settingsFilePath(decl, hostDirs()));
        if (!f.problem().empty()) qWarning("Settings: %s", f.problem().c_str());
        return f;
    }();
    return file;
}

} // namespace

calcu::settings::HostDirs platformHostDirs() {
    namespace cs = calcu::settings;
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Qt6;
    dirs.home = QDir::homePath().toStdString();
    const cs::EnvLookup env = [](const char* name) -> std::optional<std::string> {
        if (!qEnvironmentVariableIsSet(name)) return std::nullopt;
        return qEnvironmentVariable(name).toStdString();
    };
#if defined(Q_OS_MACOS)
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::MacOS, env, dirs.home, macApplicationSupportFolder());
#elif defined(Q_OS_WIN)
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Windows, env, dirs.home, {});
#else
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Linux, env, dirs.home, {});
#endif
    return dirs;
}

void useHostDirs(const calcu::settings::HostDirs& dirs) { chosenHostDirs() = dirs; }

shared::AppPaths& paths() {
    static shared::AppPaths p(AppSettings::settings(), hostDirs(),
                              shared::BundleDirs{bundledResourcesDir().toStdString(), {}});
    return p;
}

QString defaultInstanceDir() { return q(paths().defaultInstanceDir()); }
QString displayPath(const QString& path) { return q(paths().displayPath(path.toStdString())); }
QString instanceDir() { return q(paths().instanceDir()); }
QString tapeDir() { return q(paths().tapeDir()); }
QString traceDir() { return q(paths().traceDir()); }
QString serialLinkDir() { return q(paths().serialLinkDir()); }
}  // namespace AppPaths

namespace AppSettings {
shared::Settings& settings() {
    static shared::Settings s(AppPaths::settingsFile());
    return s;
}
}  // namespace AppSettings
