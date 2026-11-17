#pragma once
#include <QString>
#include <filesystem>

#include "settings/HostDirs.hpp"

// Where the Qt app keeps files: the one place that tells the library where
// this host's folders are (LIB-9 AC-12). The places come from
// calcu::settings::Paths: the save folder ~/Calc-U-59 unless [storage]
// save-folder names another (cards, ti58c.memory.yaml; AC-13), traces in the save
// folder unless [trace] folder names another (AC-17); the settings file,
// Calc-U-59.ini, in the platform's settings folder (AC-2). The Swift app
// uses iCloud Documents instead (App/CardStorage.swift). Where "Open
// Preset…" starts is an open-dialog setting (AppSettings::openStartDir), not
// a storage folder.
namespace AppPaths {

/// The host folders of this platform: home, and the settings base of LIB-9 AC-2.
calcu::settings::HostDirs platformHostDirs();

/// Folders of its own for this run instead of the platform's (a run that
/// must not touch the user's settings and cards, LIB-9 AC-18). Call before
/// anything reads a setting or a path.
void useHostDirs(const calcu::settings::HostDirs& dirs);

/// ~/Calc-U-59, created on demand.
QString defaultDir();

/// The save folder of LIB-9 (cards, ti58c.memory.yaml), created on demand;
/// the library's media part lists and writes the cards there (LIB-7 59).
QString saveDir();

/// The bundled examples (examples/bundle.yaml and the files it lists; LIB-6
/// AC-31): Resources/examples in the macOS bundle, resources/examples beside
/// the program elsewhere (frontends/qt6/CMakeLists.txt copies them).
QString bundledExamplesDir();

/// Binary traces: the trace folder override, else the save folder.
QString traceDir();

/// `path` as labels show it: "~/Calc-U-59/…" or "~/…" when under home.
QString displayPath(const QString& path);

/// A QString path as std::filesystem::path (wide characters on Windows).
std::filesystem::path toFsPath(const QString& path);

} // namespace AppPaths
