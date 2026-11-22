#pragma once
#include <QString>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "settings/Declaration.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/HostSession.hpp"

// Settings and paths on a Qt6 host: the host's folders and
// the product's settings file, written once for both products. Home is
// QDir::homePath(); the settings base comes from settings::settingsBaseFor
// with the platform's environment, on macOS from the folder the
// product passes (empty: ~/Library/Application Support). A run of its own
// (tests, screenshot runs) names its folders before the first use.
// The settings file is opened once, at the first use; a file that cannot
// be read goes to the log as "Settings: <problem>" and the app runs on the
// defaults. The library's settings places are over it. Each
// product keeps one HostFolders for its run (its AppPaths); on the GUI
// thread.

namespace calcu::qt6 {

class HostFolders {
public:
    // `macSettingsFolder`: asked once, on macOS only, when the platform's
    // folders are made; null or empty: ~/Library/Application Support.
    explicit HostFolders(const settings::ProductDecl& decl, std::function<std::string()> macSettingsFolder = {});
    ~HostFolders();
    HostFolders(const HostFolders&) = delete;
    HostFolders& operator=(const HostFolders&) = delete;

    // The host folders of this platform: home and the settings base.
    settings::HostDirs platformHostDirs() const;
    // Folders of its own for this run (the tests, the probes, the
    // screenshots): its first start is bare, without the keys' first
    // values. Before anything reads a setting or a path.
    void useHostDirs(const settings::HostDirs& dirs) { chosen_ = dirs; }
    // The folders of the run: the chosen ones, else the platform's (fixed at the first use).
    const settings::HostDirs& hostDirs();

    // The settings file and the places over it (opened at the first use).
    shell::HostSettings& settings();
    settings::SettingsFile& file() { return settings().file(); }
    const settings::Paths& places() { return settings().paths(); }

private:
    const settings::ProductDecl& decl_;
    std::function<std::string()> macSettingsFolder_;
    std::optional<settings::HostDirs> chosen_;
    std::optional<settings::HostDirs> dirs_;
    std::unique_ptr<shell::HostSettings> settings_;
};

// A QString path as std::filesystem::path (wide characters on Windows).
std::filesystem::path toFsPath(const QString& path);

} // namespace calcu::qt6
