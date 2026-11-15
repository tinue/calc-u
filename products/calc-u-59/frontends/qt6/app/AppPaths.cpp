#include "AppPaths.hpp"
#include "AppSettings.hpp"
#include <QDir>
#include <QtGlobal>
#include <optional>

#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"

namespace AppPaths {

namespace cs = calcu::settings;

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

std::optional<cs::HostDirs>& chosenHostDirs() {
    static std::optional<cs::HostDirs> dirs;
    return dirs;
}

const cs::HostDirs& hostDirs() {
    static const cs::HostDirs dirs = chosenHostDirs() ? *chosenHostDirs() : platformHostDirs();
    return dirs;
}

// The settings file, read once at the first use (LIB-9 AC-5, AC-6). A file
// that cannot be read, or the first write that fails, goes to the log (EC-6,
// AC-7); the app then runs on the defaults.
cs::SettingsFile& settingsFile() {
    static cs::SettingsFile file = [] {
        const cs::ProductDecl& decl = cu59::settings::settingsDecl();
        cs::SettingsFile f = cs::SettingsFile::open(decl, cs::settingsFilePath(decl, hostDirs()));
        if (!f.problem().empty()) qWarning("Settings: %s", f.problem().c_str());
        return f;
    }();
    return file;
}

const cs::Paths& paths() {
    static const cs::Paths p(settingsFile(), hostDirs());
    return p;
}

// The folder, created when missing (EC-8). One that cannot be created or
// written is logged and still used: nothing goes to another folder (EC-9).
QString ensured(cs::Folder which) {
    const cs::Paths::Ensured e = paths().ensure(which);
    if (!e.ok) qWarning("%s", e.problem.c_str());
    return q(e.path.empty() ? paths().folder(which).value_or(std::string()) : e.path);
}

} // namespace

cs::HostDirs platformHostDirs() {
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Qt6;
    dirs.home = QDir::homePath().toStdString();
    const cs::EnvLookup env = [](const char* name) -> std::optional<std::string> {
        if (!qEnvironmentVariableIsSet(name)) return std::nullopt;
        return qEnvironmentVariable(name).toStdString();
    };
#if defined(Q_OS_MACOS)
    // The development build is not sandboxed: ~/Library/Application Support.
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::MacOS, env, dirs.home, {});
#elif defined(Q_OS_WIN)
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Windows, env, dirs.home, {});
#else
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Linux, env, dirs.home, {});
#endif
    return dirs;
}

void useHostDirs(const cs::HostDirs& dirs) { chosenHostDirs() = dirs; }

QString defaultDir() {
    const QString dir = q(cs::joinPath(hostDirs().home, cu59::settings::settingsDecl().productName));
    QDir().mkpath(dir);
    return dir;
}

QString cardDir() { return ensured(cs::Folder::Save); }

QString constantMemoryFile() { return q(paths().saveFolderFile("ti58c.mem")); }

QString traceDir() { return ensured(cs::Folder::Traces); }

QString displayPath(const QString& path) {
    const QString home = q(hostDirs().home);
    if (path == home || path.startsWith(home + QLatin1Char('/')))
        return QDir::toNativeSeparators(QStringLiteral("~") + path.mid(home.size()));
    return QDir::toNativeSeparators(path);
}

std::filesystem::path toFsPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

} // namespace AppPaths

namespace AppSettings {
cu59::settings::Settings& settings() {
    static cu59::settings::Settings s(AppPaths::settingsFile());
    return s;
}
} // namespace AppSettings
