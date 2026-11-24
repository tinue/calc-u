#pragma once
#include <QString>

#include "app/Settings.hpp"

// The app's settings: key names, defaults and resolution live in
// app/Settings.hpp; the file itself is Calc-U-1600.ini, which AppPaths.cpp
// opens on first use at the place the library gives for this platform.
// This file keeps the QString-typed calls the widgets use; the settings
// dialog goes through the library's SettingsPart.
namespace AppSettings {

// The settings over the product's settings file (defined in AppPaths.cpp).
cu1600::Settings& settings();


inline QString startupModelPreference() { return QString::fromStdString(settings().startupModelPreference()); }
inline void setStartupModelPreference(const QString& pref) { settings().setStartupModelPreference(pref.toStdString()); }

}  // namespace AppSettings
