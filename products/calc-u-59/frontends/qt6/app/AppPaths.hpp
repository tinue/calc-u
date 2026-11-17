#pragma once
#include <QString>

#include "qt6/HostFolders.hpp"
#include "settings/HostDirs.hpp"

namespace calcu::settings {
class Paths;
class SettingsFile;
} // namespace calcu::settings

// Where the Qt app keeps files: the one place that tells the library where
// this host's folders are (LIB-9 AC-12). The host folders and the settings
// file are the Qt6 component set's (calcu::qt6::HostFolders); the places come
// from calcu::settings::Paths: the save folder ~/Calc-U-59 unless [storage]
// save-folder names another (cards, ti58c.memory.yaml; AC-13), traces in the save
// folder unless [trace] folder names another (AC-17); the settings file,
// Calc-U-59.ini, in the platform's settings folder (AC-2). The Swift app
// uses iCloud Documents instead (App/CardStorage.swift). Where "Open
// Preset…" starts is an open-dialog setting (AppSettings::openStartDir), not
// a storage folder. This file keeps the bundled examples' folder.
namespace AppPaths {

/// The host folders and the settings file of this run (on macOS the
/// settings base is ~/Library/Application Support: the build is not
/// sandboxed). The file is opened at the first use; a problem goes to the
/// log as "Settings: <problem>".
calcu::qt6::HostFolders& folders();

/// Folders of its own for this run instead of the platform's (a run that
/// must not touch the user's settings and cards, LIB-9 AC-18). Call before
/// anything reads a setting or a path.
void useHostDirs(const calcu::settings::HostDirs& dirs);

/// The settings file and the places of LIB-9 over it.
calcu::settings::SettingsFile& file();
const calcu::settings::Paths& places();

/// ~/Calc-U-59, created on demand.
QString defaultDir();

/// The save folder of LIB-9 (cards, ti58c.memory.yaml), created on demand;
/// the library's media part lists and writes the cards there (LIB-7 59).
QString saveDir();

/// The bundled examples (examples/bundle.yaml and the files it lists; LIB-6
/// AC-31): Resources/examples in the macOS bundle, resources/examples beside
/// the program elsewhere (frontends/qt6/CMakeLists.txt copies them).
QString bundledExamplesDir();
/// Another folder of bundled examples for this run (CalcU59QtTests: the
/// product's examples/); call before the window is made.
void useBundledExamplesDir(const QString& dir);

/// Binary traces: the trace folder override, else the save folder.
QString traceDir();

} // namespace AppPaths
