#include "qt6/HostFolders.hpp"

#include <QDir>
#include <QtGlobal>

#include "settings/PlatformBase.hpp"

namespace calcu::qt6 {

namespace cs = calcu::settings;

HostFolders::HostFolders(const cs::ProductDecl& decl, std::function<std::string()> macSettingsFolder)
    : decl_(decl), macSettingsFolder_(std::move(macSettingsFolder)) {}

HostFolders::~HostFolders() = default;

cs::HostDirs HostFolders::platformHostDirs() const {
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Qt6;
    dirs.home = QDir::homePath().toStdString();
    const cs::EnvLookup env = [](const char* name) -> std::optional<std::string> {
        if (!qEnvironmentVariableIsSet(name)) return std::nullopt;
        return qEnvironmentVariable(name).toStdString();
    };
#if defined(Q_OS_MACOS)
    dirs.settingsBase =
        cs::settingsBaseFor(cs::HostOs::MacOS, env, dirs.home, macSettingsFolder_ ? macSettingsFolder_() : std::string());
#elif defined(Q_OS_WIN)
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Windows, env, dirs.home, {});
#else
    dirs.settingsBase = cs::settingsBaseFor(cs::HostOs::Linux, env, dirs.home, {});
#endif
    return dirs;
}

const cs::HostDirs& HostFolders::hostDirs() {
    if (!dirs_) dirs_ = chosen_ ? *chosen_ : platformHostDirs();
    return *dirs_;
}

shell::HostSettings& HostFolders::settings() {
    if (!settings_) {
        settings_ = std::make_unique<shell::HostSettings>(decl_, hostDirs());
        if (!settings_->problem().empty()) qWarning("Settings: %s", settings_->problem().c_str());
    }
    return *settings_;
}

std::filesystem::path toFsPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

} // namespace calcu::qt6
