#pragma once
#include "Settings.hpp"
#include <QDir>
#include <QFileInfo>
#include <QString>

// The Qt app's settings: QString wrappers over cu59::settings::Settings
// (app/Settings.hpp), which declares every key, its default and its rules
// (LIB-9 AC-19). The values live in Calc-U-59.ini, which AppPaths opens at
// the platform's place (Application Support on macOS, %APPDATA% on Windows,
// $XDG_CONFIG_HOME or ~/.config on Linux; LIB-9 AC-2). No platform store (AC-21).
// The Swift-only keys ([trace] location, [keyboard] feedback,
// [window] portrait-debug-page) are kept in the file and ignored (AC-25).
namespace AppSettings {

/// The product's settings over the settings file (AppPaths.cpp).
cu59::settings::Settings& settings();

using OpenFolder = cu59::settings::OpenFolder;

/// [folders] <kind>: a fixed start folder set in Settings; empty means "<last used>".
inline QString openDir(OpenFolder folder) { return QString::fromStdString(settings().openDir(folder)); }
inline void setOpenDir(OpenFolder folder, const QString& dir) { settings().setOpenDir(folder, dir.toStdString()); }

/// After a pick: the folder of the file, unless a fixed folder is set.
inline void rememberOpenFile(OpenFolder folder, const QString& filePath) {
    settings().rememberOpenFile(folder, QFileInfo(filePath).absoluteFilePath().toStdString());
}

/// The start folder of that dialog: the fixed one, else the last used, else home.
inline QString openStartDir(OpenFolder folder) {
    return QString::fromStdString(settings().openStartDir(folder, QDir::homePath().toStdString()));
}

/// [storage] save-folder and [trace] folder: empty means the default (AppPaths).
inline QString saveFolderOverride() { return QString::fromStdString(settings().saveFolderOverride()); }
inline void setSaveFolderOverride(const QString& dir) { settings().setSaveFolderOverride(dir.toStdString()); }
inline QString traceFolderOverride() { return QString::fromStdString(settings().traceFolder()); }
inline void setTraceFolderOverride(const QString& dir) { settings().setTraceFolder(dir.toStdString()); }

} // namespace AppSettings
