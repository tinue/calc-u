#pragma once
#include <QSize>
#include <QString>

#include "app/Settings.hpp"

// The app's settings: key names, defaults and resolution live in
// app/Settings.hpp; the file itself is Calc-U-1600.ini, which AppPaths.cpp
// opens on first use at the place the library gives for this platform
// (LIB-9). This file keeps the QString-typed calls the widgets use; the
// settings dialog goes through LIB-14's SettingsPart.
namespace AppSettings {

// The settings over the product's settings file (defined in AppPaths.cpp).
cu1600::Settings& settings();

inline QSize windowSize() { return QSize(settings().windowWidth(), settings().windowHeight()); }
inline void setWindowSize(const QSize& size) { settings().setWindowSize(size.width(), size.height()); }

inline QString startupModelPreference() { return QString::fromStdString(settings().startupModelPreference()); }
inline void setStartupModelPreference(const QString& pref) { settings().setStartupModelPreference(pref.toStdString()); }

}  // namespace AppSettings
