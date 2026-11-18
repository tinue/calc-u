#pragma once
#include "settings/SettingsFile.hpp"
#include <string>
#include <string_view>

// The folder a file dialog starts in, per kind of file: one key per kind in
// [folders]. Its value is the folder last used, updated on every pick, or
// `fixed:<folder>` for a folder set in the settings dialog, which picks do
// not change.

namespace calcu::settings {

inline constexpr std::string_view kFixedPrefix = "fixed:";

// The fixed folder, else the folder last used, else `home`; `~` expanded.
std::string openStartFolder(const SettingsFile& settings, std::string_view kind, const std::string& home);
bool hasFixedOpenFolder(const SettingsFile& settings, std::string_view kind);
// The fixed folder, or "" when none is set.
std::string fixedOpenFolder(const SettingsFile& settings, std::string_view kind);
// After a pick: remembers `folder` unless a fixed folder is set.
SettingsFile::WriteResult rememberOpenFolder(SettingsFile& settings, std::string_view kind,
                                             const std::string& folder);
// Sets a fixed folder. An empty one goes back to "the folder last used",
// which starts where the fixed folder was (one key holds both; the
// folders picked meanwhile are not kept).
SettingsFile::WriteResult setFixedOpenFolder(SettingsFile& settings, std::string_view kind,
                                             const std::string& folder);

} // namespace calcu::settings
