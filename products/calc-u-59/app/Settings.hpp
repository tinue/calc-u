#pragma once
#include "LedGeometry.hpp"
#include "MachineModelInfo.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include <optional>
#include <string>
#include <string_view>

// ── Settings: Calc-U-59's keys, and typed calls over the settings file ──────
//
// Every setting the product keeps is declared once, in settingsDecl():
// section, key, default and the values it accepts. The values live in
// the product's settings file, `Calc-U-59.ini`, read and written by the
// library (calcu::settings::SettingsFile); the frontend opens it at the place
// calcu::settings gives for its platform and passes its host folders. The
// Swift, Qt6 and Web frontends use the same declaration; a key a frontend has
// no use for is kept in the file and ignored. No platform store
// (UserDefaults, QSettings, localStorage) holds a setting; the one
// exception is the macOS grant for a chosen trace folder. No setting
// holds the Solid State ROM module.

namespace cu59::settings {

// The declaration of Calc-U-59's settings. Lives as long as the program.
const calcu::settings::ProductDecl& settingsDecl();

// The model's name as the settings file writes it: "TI-59", "TI-58", "TI-58C".
const char* modelName(const ModelId& model);
std::optional<ModelId> modelNamed(std::string_view name);

// The folders a file dialog starts in, one key each in [folders]:
//   Presets     -- "Load Preset…": presets
//   AsmOverlays -- the debugger's ASM overlay picker: asm-overlays
enum class OpenFolder { Presets, AsmOverlays };
const char* openFolderKind(OpenFolder folder);

// The Qt6 window as plain numbers in [window]: its place
// and size, and the widths of the three panels of the splitter. A value that
// is not set is empty: the window system places the window, the splitter
// splits by its own rules.
struct WindowState {
    std::optional<long> x, y;
    long width = 1280, height = 860;
    std::optional<long> calculatorWidth, printerWidth, debugWidth;
    bool operator==(const WindowState&) const = default;
};

class Settings {
public:
    explicit Settings(calcu::settings::SettingsFile& file) : m_file(file) {}

    calcu::settings::SettingsFile& file() { return m_file; }
    const calcu::settings::SettingsFile& file() const { return m_file; }

    // The other keys are read where they are used, through the settings file:
    // the startup keys by the library's start (settings::startupModel,
    // settings::startupPreset), the places by the library's Paths, [trace]
    // max-file-size-mb by the library's trace, [folders] by the shell's dialogs
    // (settings/OpenFolders), the rows of the settings dialog by the shell's
    // SettingsPart; [trace] location by Paths too (the Swift frontend's
    // iCloud gate is HostDirs::iCloudAvailable).

    // [window] (Qt6).
    WindowState windowState() const;
    void setWindowState(const WindowState& state);

private:
    calcu::settings::SettingsFile& m_file;
};

} // namespace cu59::settings
