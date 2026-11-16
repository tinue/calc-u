#pragma once
#include <QDir>
#include <QSize>
#include <QString>

#include "app/Settings.hpp"

// The app's settings: key names, defaults and resolution live in
// app/Settings.hpp; the file itself is Calc-U-1600.ini, which AppPaths.cpp
// opens on first use at the place the library gives for this platform
// (LIB-9). This file keeps the QString-typed calls the widgets use.
namespace AppSettings {

// The settings over the product's settings file (defined in AppPaths.cpp).
shared::Settings& settings();

inline QString q(const std::string& s) { return QString::fromStdString(s); }
inline std::string s(const QString& q) { return q.toStdString(); }

inline QString instanceDirOverride() { return q(settings().instanceDirOverride()); }
inline void setInstanceDirOverride(const QString& dir) { settings().setInstanceDirOverride(s(dir)); }

inline QString tapeDirOverride() { return q(settings().tapeDirOverride()); }
inline void setTapeDirOverride(const QString& dir) { settings().setTapeDirOverride(s(dir)); }

using shared::OpenFolder;
inline QString openDir(OpenFolder folder) { return q(settings().openDir(folder)); }
inline void setOpenDir(OpenFolder folder, const QString& dir) { settings().setOpenDir(folder, s(dir)); }
inline void rememberOpenDir(OpenFolder folder, const QString& dir) { settings().rememberOpenDir(folder, s(dir)); }
inline void rememberOpenFile(OpenFolder folder, const QString& filePath) {
    settings().rememberOpenFile(folder, s(filePath));
}
inline QString openStartDir(OpenFolder folder) { return q(settings().openStartDir(folder, s(QDir::homePath()))); }

inline QString defaultPresetPath(const calcu::machine::ModelId& model) { return q(settings().defaultPresetPath(model)); }
inline void setDefaultPresetPath(const calcu::machine::ModelId& model, const QString& path) {
    settings().setDefaultPresetPath(model, s(path));
}

inline QString traceDirOverride() { return q(settings().traceDirOverride()); }
inline void setTraceDirOverride(const QString& dir) { settings().setTraceDirOverride(s(dir)); }
inline int traceMaxFileSizeMB() { return settings().traceMaxFileSizeMB(); }
inline void setTraceMaxFileSizeMB(int mb) { settings().setTraceMaxFileSizeMB(mb); }

inline bool dapEnabled() { return settings().dapEnabled(); }
inline void setDapEnabled(bool on) { settings().setDapEnabled(on); }
inline int dapPort() { return settings().dapPort(); }
inline void setDapPort(int port) { settings().setDapPort(port); }

inline QString serialLinkDirOverride() { return q(settings().serialLinkDirOverride()); }
inline void setSerialLinkDirOverride(const QString& dir) { settings().setSerialLinkDirOverride(s(dir)); }

inline QSize windowSize() { return QSize(settings().windowWidth(), settings().windowHeight()); }
inline void setWindowSize(const QSize& size) { settings().setWindowSize(size.width(), size.height()); }

inline QString startupModelPreference() { return q(settings().startupModelPreference()); }
inline void setStartupModelPreference(const QString& pref) { settings().setStartupModelPreference(s(pref)); }

}  // namespace AppSettings
