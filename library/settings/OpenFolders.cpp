#include "settings/OpenFolders.hpp"
#include "settings/Paths.hpp"

namespace calcu::settings {

bool hasFixedOpenFolder(const SettingsFile& settings, std::string_view kind) {
    return settings.text("folders", kind).rfind(kFixedPrefix, 0) == 0;
}

std::string fixedOpenFolder(const SettingsFile& settings, std::string_view kind) {
    if (!hasFixedOpenFolder(settings, kind)) return {};
    return settings.text("folders", kind).substr(kFixedPrefix.size());
}

std::string openStartFolder(const SettingsFile& settings, std::string_view kind, const std::string& home) {
    std::string v = settings.text("folders", kind);
    if (v.rfind(kFixedPrefix, 0) == 0) v.erase(0, kFixedPrefix.size());
    if (v.empty()) return home;
    return expandHome(v, home);
}

SettingsFile::WriteResult rememberOpenFolder(SettingsFile& settings, std::string_view kind, const std::string& folder) {
    if (hasFixedOpenFolder(settings, kind)) return {};
    return settings.set("folders", kind, folder);
}

SettingsFile::WriteResult setFixedOpenFolder(SettingsFile& settings, std::string_view kind, const std::string& folder) {
    return settings.set("folders", kind, folder.empty() ? fixedOpenFolder(settings, kind) : std::string(kFixedPrefix) + folder);
}

} // namespace calcu::settings
