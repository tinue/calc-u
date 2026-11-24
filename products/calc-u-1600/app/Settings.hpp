#pragma once
#include <optional>
#include <string>
#include <vector>

#include "machine/Ids.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include "settings/WindowPlace.hpp"

// ── Settings: Calc-U-1600's keys, and typed calls over the settings file ──
//
// Every setting the app keeps is declared once, in settingsDecl():
// section, key, default and the values it accepts. The values live
// in the product's settings file, `Calc-U-1600.ini`, read and written by the
// library (calcu::settings::SettingsFile; the frontend opens it at the place
// calcu::settings gives for its platform). Qt6 and the Web frontend use the
// same declaration; a key a frontend has no use for is kept and ignored.
// No platform store (QSettings, localStorage) holds a setting.
namespace cu1600 {

// The declaration of Calc-U-1600's settings. Lives as long as the
// program.
const calcu::settings::ProductDecl& settingsDecl();

// The models in the catalogue's order. Their IDs ("PC-1500", "PC-1500A",
// "PC-1600") are also the names the settings file writes ([startup] model,
// last-model, the keys of [preset]); no enum of its own.
const std::vector<calcu::machine::ModelId>& sharpModels();
// The model of that name, or none.
std::optional<calcu::machine::ModelId> sharpModel(const std::string& name);

// The folders the file-open dialogs start in, one per kind of file, each
// shown as its own row in Settings, and one key in [folders] (the
// rule -- a fixed folder, else the one last used -- is the library's,
// settings/OpenFolders, which the shell's dialogs and SettingsPart use):
//   Samples   -- "Load Preset…" (and Settings' default-preset pickers): presets
//   Basic     -- "Load BASIC Program…" (.bas listings): basic-programs
//   Assembly  -- "Load Machine Code…" (.bin files): machine-code
//   HostDrive -- "Host Drive…" (PC-1600 host drive S3:): host-drive
enum class OpenFolder { Samples, Basic, Assembly, HostDrive };
const char* openFolderKind(OpenFolder folder);

class Settings {
public:
    explicit Settings(calcu::settings::SettingsFile& file) : m_file(file) {}

    calcu::settings::SettingsFile& file() { return m_file; }
    const calcu::settings::SettingsFile& file() const { return m_file; }

    // The family's keys ([storage], [trace], [serial], [preset], [folders])
    // and [debug] are read where they are used, through the settings file
    // (the library's Paths, the start, the trace, the DAP part).

    // [debug] dap-port: the Debug Adapter Protocol server VS Code attaches
    // to (127.0.0.1 only) listens here unless the value is outside
    // 1024-65535.
    static constexpr int kDefaultDapPort = 32168;

    // [window] x / y / width / height -- the main window's place and its
    // normal size, restored on the next launch (default 900x700: the panels
    // below the faceplate).
    static constexpr int kDefaultWindowWidth = 900;
    static constexpr int kDefaultWindowHeight = 700;

    // [startup] model -- which model to boot into: "last" (the default)
    // reuses lastUsedModel(), or one of "PC-1500"/"PC-1500A"/"PC-1600"
    // pins a model regardless of what was last selected. There is
    // deliberately no ROM-revision preference: the plain PC-1500 always
    // boots on ROM A04.
    std::string startupModelPreference() const { return m_file.text("startup", "model"); }
    void setStartupModelPreference(const std::string& pref) { m_file.set("startup", "model", pref); }

    // The model to boot into: the pinned preference if it names a model,
    // else [startup] last-model (the model last switched to), else the
    // PC-1500A.
    calcu::machine::ModelId startupModel() const;

private:
    calcu::settings::SettingsFile& m_file;
};

} // namespace cu1600
