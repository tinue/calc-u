#include "shell/HostSession.hpp"

#include "sessions/LibraryTopics.hpp"
#include "sessions/Listener.hpp"
#include "settings/PlatformBase.hpp"

namespace calcu::shell {

HostSettings::HostSettings(const settings::ProductDecl& decl, settings::HostDirs dirs,
                           std::optional<std::string> filePath, bool firstValues)
    : file_(settings::SettingsFile::open(decl, filePath ? *filePath : settings::settingsFilePath(decl, dirs), firstValues)),
      paths_(file_, std::move(dirs)) {}

namespace detail {

std::unique_ptr<SettingsPart> settingsPartOf(HostSessionSetup& setup) {
    if (!setup.settings) return nullptr;
    SettingsPartSetup s;
    s.file = &setup.settings->file();
    s.paths = &setup.settings->paths();
    // `[app-log] level` is followed at once, unless the run has its own level.
    s.changed = [changed = std::move(setup.settingsChanged), log = setup.appLog, fixed = setup.appLogLevel.has_value(),
                 file = s.file](std::string_view section, std::string_view key) {
        if (log && !fixed && section == "app-log" && key == "level")
            log->setLevel(applog::appLogLevelOfSetting(file->text("app-log", "level")));
        if (changed) changed(section, key);
    };
    s.alsoShown = std::move(setup.settingsAlsoShown);
    return std::make_unique<SettingsPart>(std::move(s));
}

namespace {
// What the session owns: the host's (the product's part among it), then
// SettingsPart, which goes first.
struct Owned {
    std::shared_ptr<void> host;
    std::unique_ptr<SettingsPart> settingsPart;
};
} // namespace

std::unique_ptr<sessions::AppSession> hostSessionOf(HostSessionSetup setup, std::vector<sessions::SessionPart*> parts,
                                                    std::unique_ptr<SettingsPart> settingsPart) {
    if (setup.appLog) {
        if (setup.appLogLevel) setup.appLog->setLevel(*setup.appLogLevel);
        else if (setup.settings)
            setup.appLog->setLevel(applog::appLogLevelOfSetting(setup.settings->file().text("app-log", "level")));
        if (setup.settings && !setup.settings->problem().empty())
            setup.appLog->warn("Settings: " + setup.settings->problem());
    }
    sessions::AppSessionSetup s;
    s.clock = setup.clock ? std::move(setup.clock) : pacing::WallClock(&pacing::steadyWallTime);
    s.hostTime = std::move(setup.hostTime);
    s.parts = std::move(parts);
    s.hosting = setup.hosting;
    s.jitterSeed = setup.jitterSeed;
    s.appLog = setup.appLog;
    auto owned = std::make_shared<Owned>();
    owned->host = std::move(setup.owned);
    owned->settingsPart = std::move(settingsPart);
    s.owned = std::move(owned);
    auto session = std::make_unique<sessions::AppSession>(std::move(s));
    if (setup.start) {
        sessions::Listener l;
        l.listen(*session);
        l.send(sessions::kStartCommand, sessions::Value::object({}));
    }
    return session;
}

} // namespace detail

} // namespace calcu::shell
