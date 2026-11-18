#pragma once
#include "settings/HostDirs.hpp"
#include "settings/SettingsFile.hpp"
#include <optional>
#include <string>
#include <string_view>

// Every place a product uses, from the host directories the caller passes and
// the settings. No frontend and no other feature works out one of these
// places itself.
//
//   settings file  <settingsBase>/<Product>/<Product>.ini
//   save folder    Qt6: [storage] save-folder, else <home>/<Product>;
//                  Swift, Web, Given: the folder passed
//   cassettes      [storage] cassette-folder, else Cassettes/ in the save folder
//   traces         Qt6: [trace] folder, else the save folder; Swift: by
//                  [trace] location; Web: none, a download;
//                  Given: the folder passed
//   screenshots    the save folder; Web: none, a download; Given:
//                  the folder passed
//   serial links   Qt6: [serial] link-folder, else the save folder; else none
//   files          at the save folder's root (`ti58c.memory.yaml`)

namespace calcu::settings {

enum class Folder { Save, Cassettes, Traces, Screenshots, SerialLinks };

// A path value with a leading `~` expanded against `home`.
std::string expandHome(std::string_view value, const std::string& home);

class Paths {
public:
    // `settings` must outlive the Paths; its values are read at each call.
    Paths(const SettingsFile& settings, HostDirs dirs);

    std::string settingsFile() const;
    std::string saveFolder() const;
    std::string cassetteFolder() const;
    std::optional<std::string> traceFolder() const;
    std::optional<std::string> screenshotFolder() const;
    std::optional<std::string> serialLinkFolder() const;
    std::string saveFolderFile(std::string_view name) const;
    std::optional<std::string> folder(Folder which) const;

    struct Ensured { bool ok = false; std::string path, problem; };
    // Makes sure the folder exists and can be written, creating it when
    // missing. A folder that cannot be created or written is
    // reported, and no other folder is used instead. The traces on
    // Swift with [trace] location = icloud: refused while the host's
    // iCloudAvailable says no.
    Ensured ensure(Folder which) const;

    const HostDirs& hostDirs() const { return dirs_; }

private:
    const SettingsFile* settings_;
    HostDirs dirs_;
    std::optional<std::string> folderValue(std::string_view section, std::string_view key) const;
};

} // namespace calcu::settings
