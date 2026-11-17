#include "AppPaths.hpp"
#include "AppSettings.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

#include <optional>

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

std::optional<shared::BundleDirs>& chosenBundleDirs() {
    static std::optional<shared::BundleDirs> dirs;
    return dirs;
}

} // namespace

calcu::qt6::HostFolders& folders() {
    // LIB-9 AC-2: on macOS the settings base is the app's Application Support
    // folder as macOS gives it (inside the container of a sandboxed app).
    static calcu::qt6::HostFolders f(shared::settingsDecl(),
#if defined(Q_OS_MACOS)
                                     [] { return macApplicationSupportFolder(); }
#else
                                     {}
#endif
    );
    return f;
}

void useHostDirs(const calcu::settings::HostDirs& dirs) { folders().useHostDirs(dirs); }
void useBundleDirs(const shared::BundleDirs& dirs) { chosenBundleDirs() = dirs; }

shared::AppPaths& paths() {
    static shared::AppPaths p(AppSettings::settings(), folders().hostDirs(),
                              chosenBundleDirs() ? *chosenBundleDirs()
                                                 : shared::BundleDirs{bundledResourcesDir().toStdString(), {}});
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
    static shared::Settings s(AppPaths::folders().file());
    return s;
}
}  // namespace AppSettings
