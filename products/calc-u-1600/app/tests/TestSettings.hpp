#pragma once
#include <string>

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "Machine/SharpRoms.hpp"
#include "roms/Bundle.hpp"
#include "settings/HostDirs.hpp"
#include "settings/SettingsFile.hpp"

// Calc-U-1600's settings for a test: the product's declaration over a
// settings file that is never read or written (LIB-9 AC-18), so a test
// touches nothing of the user's. Not copyable: Settings holds the file.
struct TestSettings {
    calcu::settings::SettingsFile file = calcu::settings::SettingsFile::defaultsOnly(shared::settingsDecl());
    shared::Settings settings{file};

    TestSettings() = default;
    TestSettings(const TestSettings&) = delete;
    TestSettings& operator=(const TestSettings&) = delete;
};

// The host folders of a Qt6-like run on `home`, its settings inside it.
inline calcu::settings::HostDirs testHostDirs(const std::string& home) {
    calcu::settings::HostDirs dirs;
    dirs.frontend = calcu::settings::FrontendKind::Qt6;
    dirs.home = home;
    dirs.settingsBase = home + "/settings";
    return dirs;
}

// The full ROM set is in roms/ (the product directory): through the one
// lookup (LIB-12), looked up once; the tests that boot a machine skip without it.
inline bool fullRomSetPresent() {
    static const bool ok = sharp::lookUpRoms(calcu::roms::DirectoryBundle("roms")).ok();
    return ok;
}
