#pragma once
#include <QString>
#include <string>

#include "app/AppPaths.hpp"
#include "qt6/HostFolders.hpp"
#include "settings/HostDirs.hpp"

// Path resolution for bundled resources and the memory-module feature --
// the one place that tells the library where this host's folders are and
// where bundled resources (*.module.yaml templates AND ROM images, side by
// side) live. Bundled resources ship as loose files in the app's data folder
// (calcu::qt6::bundledDataDir), not Qt's .qrc embedding, since Core's
// scanMemoryCardDirectory() and the ROM lookup read a real directory. The places the app
// writes to come from the library (calcu::settings::Paths): the
// save folder defaults to ~/Calc-U-1600 on every platform (not
// QStandardPaths::AppDataLocation, no iCloud); the settings file,
// Calc-U-1600.ini, is in the platform's settings folder (Application
// Support on macOS, %APPDATA% on Windows, $XDG_CONFIG_HOME or ~/.config on
// Linux).
namespace AppPaths {

// The host folders and the settings file of this run (the Qt6 component
// set's; the settings base on macOS: calcu::qt6::macApplicationSupportFolder()):
// the file is opened at the first use, a problem goes to the log as
// "Settings: <problem>"; places() for the library's SettingsPart.
calcu::qt6::HostFolders& folders();

// Folders of its own for this run instead of the platform's (screenshot
// runs, `--shots`, main.cpp: a throwaway home and settings base, so the
// user's settings and saved cards/disks neither show in the pictures nor
// change). Call before anything reads a setting or a path.
void useHostDirs(const calcu::settings::HostDirs& dirs);
// The tests (CalcU1600QtTests): the bundled folders of their own, before the first use.
void useBundleDirs(const cu1600::BundleDirs& dirs);

// The cu1600::AppPaths every call below forwards to (app/AppPaths.hpp
// has the rules); built on first use from the bundled resources folder
// (calcu::qt6::bundledDataDir) and the host folders.
cu1600::AppPaths& paths();

// `path` the way every label shows one (cu1600::AppPaths::displayPath).
QString displayPath(const QString& path);

}  // namespace AppPaths
