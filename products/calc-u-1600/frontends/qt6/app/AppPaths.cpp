#include "AppPaths.hpp"
#include "AppSettings.hpp"

#include <QtGlobal>

#include <optional>

#include "settings/SettingsFile.hpp"
#include "shell/HostAppLog.hpp"
#include "qt6/BundledData.hpp"
#include "qt6/MacHost.hpp"

namespace AppPaths {

namespace {

// Where the bundled resources live -- *.module.yaml catalog AND ROM images
// (disjoint filename extensions, one shared directory) and examples/: the
// app's data (calcu::qt6::bundledDataDir). Read-only bundled content --
// never created if missing; a missing/unreadable directory is tolerated by
// scanMemoryCardDirectory() (an empty catalogue); the ROM lookup then names
// every file missing.
QString bundledResourcesDir() { return calcu::qt6::bundledDataDir(QStringLiteral("calc-u-1600")); }

QString q(const std::string& s) { return QString::fromStdString(s); }

std::optional<cu1600::BundleDirs>& chosenBundleDirs() {
    static std::optional<cu1600::BundleDirs> dirs;
    return dirs;
}

} // namespace

calcu::qt6::HostFolders& folders() {
    // On macOS the settings base is the app's Application Support
    // folder as macOS gives it (inside the container of a sandboxed app).
    static calcu::qt6::HostFolders f(cu1600::settingsDecl(),
#if defined(Q_OS_MACOS)
                                     [] { return calcu::qt6::macApplicationSupportFolder(); }
#else
                                     {}
#endif
    );
    return f;
}

void useHostDirs(const calcu::settings::HostDirs& dirs) { folders().useHostDirs(dirs); }
void useBundleDirs(const cu1600::BundleDirs& dirs) { chosenBundleDirs() = dirs; }

cu1600::AppPaths& paths() {
    static cu1600::AppPaths p = [] {
        cu1600::AppPaths made(AppSettings::settings(), folders().hostDirs(),
                              chosenBundleDirs() ? *chosenBundleDirs()
                                                 : cu1600::BundleDirs{bundledResourcesDir().toStdString(), {}});
        made.useAppLog(calcu::shell::hostAppLog());
        return made;
    }();
    return p;
}

QString defaultInstanceDir() { return q(paths().defaultInstanceDir()); }
QString displayPath(const QString& path) { return q(paths().displayPath(path.toStdString())); }
QString instanceDir() { return q(paths().instanceDir()); }
QString cassetteDir() { return q(paths().cassetteDir()); }
QString traceDir() { return q(paths().traceDir()); }
QString serialLinkDir() { return q(paths().serialLinkDir()); }
}  // namespace AppPaths

namespace AppSettings {
cu1600::Settings& settings() {
    static cu1600::Settings s(AppPaths::folders().file());
    return s;
}
}  // namespace AppSettings
