#pragma once
#include "Settings.hpp"

// The Qt app's settings: cu59::settings::Settings (app/Settings.hpp), which
// declares every key, its default and its rules, over the
// settings file. The values live in Calc-U-59.ini, which AppPaths opens at
// the platform's place (Application Support on macOS, %APPDATA% on Windows,
// $XDG_CONFIG_HOME or ~/.config on Linux). No platform store.
// The Swift-only keys ([trace] location, [keyboard] feedback,
// [window] portrait-debug-page) are kept in the file and ignored.
namespace AppSettings {

/// The product's settings over the settings file (AppPaths.cpp): the
/// window's place and size (MainWindow). The other keys are the library's
/// (Paths, the shell's dialogs and SettingsPart).
cu59::settings::Settings& settings();

} // namespace AppSettings
