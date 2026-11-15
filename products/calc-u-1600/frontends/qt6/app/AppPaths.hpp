#pragma once
#include <QString>
#include <string>

#include "app/AppPaths.hpp"
#include "settings/HostDirs.hpp"

// Path resolution for bundled resources and the memory-module feature --
// the one place that tells the library where this host's folders are and
// where bundled resources (*.card.yaml templates AND ROM images, side by
// side) live. Bundled resources ship as loose files next to the executable
// (not Qt's .qrc embedding, since Core's scanMemoryCardDirectory()/
// BundledRomCatalog read a real filesystem directory). The places the app
// writes to come from the library (calcu::settings::Paths, LIB-9): the
// save folder defaults to ~/Calc-U-1600 on every platform (not
// QStandardPaths::AppDataLocation, no iCloud); the settings file,
// Calc-U-1600.ini, is in the platform's settings folder (Application
// Support on macOS, %APPDATA% on Windows, $XDG_CONFIG_HOME or ~/.config on
// Linux).
namespace AppPaths {

// The host folders of this platform: home, and the settings base of LIB-9
// AC-2.
calcu::settings::HostDirs platformHostDirs();

// Folders of its own for this run instead of the platform's (screenshot
// runs, `--shots`, main.cpp: a throwaway home and settings base, so the
// user's settings and saved cards/disks neither show in the pictures nor
// change; LIB-9 AC-18). Call before anything reads a setting or a path.
void useHostDirs(const calcu::settings::HostDirs& dirs);

// The shared::AppPaths every call below forwards to (app/AppPaths.hpp
// has the rules); built on first use from the bundled resources folder
// next to the executable and the host folders.
shared::AppPaths& paths();

// ~/Calc-U-1600 (home dir, non-dotted), created if missing.
QString defaultInstanceDir();

// `path` the way every label shows one: under defaultInstanceDir() as
// "~/Calc-U-1600/...", else under the home folder as "~/...", else as is.
// A screenshot run's throwaway home thus reads like a fresh install's.
QString displayPath(const QString& path);

// AppSettings::instanceDirOverride() if non-empty, else
// defaultInstanceDir(). The single source of truth every other class calls
// to find "where battery-card instances live right now".
QString instanceDir();

// Cassette tapes (TapeManager): AppSettings::tapeDirOverride() if
// non-empty, else "Tapes" inside instanceDir(); created on demand. A tape
// named `tapeName` is "<tapeName>.wav" there (namedFileName()'s rules).
QString tapeDir();

// shared::AppPaths::traceDir() / serialLinkDir(): the Settings override,
// else instanceDir().
QString traceDir();
QString serialLinkDir();

}  // namespace AppPaths
