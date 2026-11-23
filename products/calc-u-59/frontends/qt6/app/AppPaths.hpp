#pragma once
#include <QString>

#include "qt6/HostFolders.hpp"
#include "settings/HostDirs.hpp"

namespace calcu::settings {
class Paths;
class SettingsFile;
} // namespace calcu::settings

// Where the Qt app keeps files: the one place that tells the library where
// this host's folders are. The host folders and the settings
// file are the Qt6 component set's (calcu::qt6::HostFolders); the places come
// from calcu::settings::Paths: the save folder ~/Calc-U-59 unless [storage]
// save-folder names another (cards, ti58c.memory.yaml), traces in the save
// folder unless [trace] folder names another; the settings file,
// Calc-U-59.ini, in the platform's settings folder. The Swift app
// uses iCloud Documents instead (App/CardStorage.swift). Where "Open
// Preset…" starts is an open-dialog setting ([folders], the shell's dialogs), not
// a storage folder. This file keeps the bundled examples' folder.
namespace AppPaths {

/// The host folders and the settings file of this run (on macOS the
/// settings base is ~/Library/Application Support: the build is not
/// sandboxed). The file is opened at the first use; a problem goes to the
/// log as "Settings: <problem>".
calcu::qt6::HostFolders& folders();

/// Folders of its own for this run instead of the platform's (a run that
/// must not touch the user's settings and cards). Call before
/// anything reads a setting or a path.
void useHostDirs(const calcu::settings::HostDirs& dirs);

/// The settings file and the places over it.
calcu::settings::SettingsFile& file();
const calcu::settings::Paths& places();

/// ~/Calc-U-59, created on demand.
QString defaultDir();

/// The save folder (cards, ti58c.memory.yaml), created on demand;
/// the library's media part lists and writes the cards there.
QString saveDir();

/// The bundled examples (examples/bundle.yaml and the files it lists):
/// examples/ in the app's data (the library's calcu::qt6::bundledDataDir:
/// Contents/Resources of the macOS bundle, share/calc-u-59 of a Linux
/// install, else resources beside the program).
QString bundledExamplesDir();
/// Another folder of bundled examples for this run (CalcU59QtTests: the
/// product's examples/); call before the window is made.
void useBundledExamplesDir(const QString& dir);

} // namespace AppPaths
