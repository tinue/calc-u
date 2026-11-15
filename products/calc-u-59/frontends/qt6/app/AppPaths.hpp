#pragma once
#include <QString>
#include <filesystem>

#include "settings/HostDirs.hpp"

// Where the Qt app keeps files: the one place that tells the library where
// this host's folders are (LIB-9 AC-12). The places come from
// calcu::settings::Paths: the save folder ~/Calc-U-59 unless [storage]
// save-folder names another (cards, ti58c.mem; AC-13), traces in the save
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

/// Cards and ti58c.mem: the save folder (adapter A-2 until M-54, M-55).
QString cardDir();
QString constantMemoryFile();

/// Binary traces: the trace folder override, else the save folder.
QString traceDir();

/// `path` as labels show it: "~/Calc-U-59/…" or "~/…" when under home.
QString displayPath(const QString& path);

/// A QString path as std::filesystem::path (wide characters on Windows).
std::filesystem::path toFsPath(const QString& path);

} // namespace AppPaths
