#pragma once
#include "settings/HostDirs.hpp"
#include "settings/SettingsFile.hpp"
#include <optional>
#include <string>
#include <string_view>

// Every place a product uses (LIB-9 AC-12–17, EC-3, EC-8, EC-9; design
// §3), from the host directories the caller passes and the settings. No
// frontend and no other feature works out one of these places itself.
//
//   settings file  <settingsBase>/<Product>/<Product>.ini (AC-2)
//   save folder    Qt6: [storage] save-folder, else <home>/<Product> (AC-13);
//                  Swift, Web: the folder passed (AC-14, AC-15)
//   tapes          [storage] tape-folder, else Tapes/ in the save folder (AC-16, EC-3)
//   traces         Qt6: [trace] folder, else the save folder; Swift: by
//                  [trace] location (AC-24); Web: none, a download (AC-17)
//   screenshots    the save folder; Web: none, a download (AC-17)
//   serial links   Qt6: [serial] link-folder, else the save folder; else none
//   files          at the save folder's root (`ti58c.memory.yaml`, AC-16)

namespace calcu::settings {

enum class Folder { Save, Tapes, Traces, Screenshots, SerialLinks };

// A path value with a leading `~` expanded against `home` (AC-33).
std::string expandHome(std::string_view value, const std::string& home);

class Paths {
public:
    // `settings` must outlive the Paths; its values are read at each call.
    Paths(const SettingsFile& settings, HostDirs dirs);

    std::string settingsFile() const;
    std::string saveFolder() const;
    std::string tapeFolder() const;
    std::optional<std::string> traceFolder() const;
    std::optional<std::string> screenshotFolder() const;
    std::optional<std::string> serialLinkFolder() const;
    std::string saveFolderFile(std::string_view name) const;
    std::optional<std::string> folder(Folder which) const;

    struct Ensured { bool ok = false; std::string path, problem; };
    // Makes sure the folder exists and can be written, creating it when
    // missing (EC-8). A folder that cannot be created or written is
    // reported, and no other folder is used instead (EC-9).
    Ensured ensure(Folder which) const;

    const HostDirs& hostDirs() const { return dirs_; }

private:
    const SettingsFile* settings_;
    HostDirs dirs_;
    std::optional<std::string> folderValue(std::string_view section, std::string_view key) const;
};

} // namespace calcu::settings
