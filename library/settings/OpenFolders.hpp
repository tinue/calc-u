#pragma once
#include "settings/SettingsFile.hpp"
#include <string>
#include <string_view>

// The folder a file dialog starts in, per kind of file (LIB-9 AC-29 and
// Calc-U-59's [folders]; design D-2): one key per kind in [folders]. Its
// value is the folder last used, updated on every pick, or `fixed:<folder>`
// for a folder set in the settings dialog, which picks do not change.

namespace calcu::settings {

inline constexpr std::string_view kFixedPrefix = "fixed:";

// The fixed folder, else the folder last used, else `home`; `~` expanded.
std::string openStartFolder(const SettingsFile& settings, std::string_view kind, const std::string& home);
bool hasFixedOpenFolder(const SettingsFile& settings, std::string_view kind);
// After a pick: remembers `folder` unless a fixed folder is set.
SettingsFile::WriteResult rememberOpenFolder(SettingsFile& settings, std::string_view kind,
                                             const std::string& folder);
// Sets a fixed folder; an empty one goes back to the folder last used.
SettingsFile::WriteResult setFixedOpenFolder(SettingsFile& settings, std::string_view kind,
                                             const std::string& folder);

} // namespace calcu::settings
