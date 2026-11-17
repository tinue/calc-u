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
// Every setting the product keeps is declared once, in settingsDecl() (LIB-9
// AC-19): section, key, default and the values it accepts. The values live in
// the product's settings file, `Calc-U-59.ini`, read and written by the
// library (calcu::settings::SettingsFile); the frontend opens it at the place
// calcu::settings gives for its platform and passes its host folders. The
// Swift, Qt6 and Web frontends use the same declaration; a key a frontend has
// no use for is kept in the file and ignored (AC-25). No platform store
// (UserDefaults, QSettings, localStorage) holds a setting (AC-21); the one
// exception is the macOS grant for a chosen trace folder (AC-34). No setting
// holds the Solid State ROM module (AC-22).

namespace cu59::settings {

// The declaration of Calc-U-59's settings (AC-19). Lives as long as the program.
const calcu::settings::ProductDecl& settingsDecl();

// The model's name as the settings file writes it: "TI-59", "TI-58", "TI-58C".
const char* modelName(Model model);
std::optional<Model> modelNamed(std::string_view name);

using cu59::LedStyle;                                      // [display] led-style
enum class KeyboardFeedback { Off, Haptic, Click };       // [keyboard] feedback
enum class TraceLocation { AppSupport, ICloud, Folder };  // [trace] location (Swift, AC-24)

// The folders a file dialog starts in, one key each in [folders] (AC-19):
//   Presets     -- "Open Preset…": presets
//   AsmOverlays -- the debugger's ASM overlay picker: asm-overlays
//   Other       -- other files: other (declared; no dialog uses it yet, D-1)
enum class OpenFolder { Presets, AsmOverlays, Other };
const char* openFolderKind(OpenFolder folder);

// The Qt6 window as plain numbers in [window] (AC-35; design D-7): its place
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

    // [startup] model: a model, or none for `last` (also for a value that is
    // neither, AC-9).
    std::optional<Model> startupModelChoice() const;
    void setStartupModelChoice(std::optional<Model> model);
    // The model to start: the chosen one, else the last used, else the
    // TI-59 (AC-20, EC-1).
    Model startupModel() const;
    // [startup] last-model, written by the app at each model switch.
    std::optional<Model> lastUsedModel() const;
    void setLastUsedModel(Model model);

    // [preset] <model>: the startup preset; empty means none (AC-10).
    std::string startupPreset(Model model) const;

    // [trace] max-file-size-mb: missing, zero or negative is 50 (AC-11).
    int traceMaxFileSizeMB() const;
    void setTraceMaxFileSizeMB(int mb);
    // [trace] folder: the chosen trace folder (Qt6: empty = the save folder;
    // Swift: the folder of `location = folder`, AC-24).
    std::string traceFolder() const;
    void setTraceFolder(const std::string& dir);
    TraceLocation traceLocation() const;
    void setTraceLocation(TraceLocation location);

    // [storage] save-folder (Qt6 only): empty = ~/Calc-U-59 (AC-13).
    std::string saveFolderOverride() const;
    void setSaveFolderOverride(const std::string& dir);

    LedStyle ledStyle() const;
    void setLedStyle(LedStyle style);
    KeyboardFeedback keyboardFeedback() const;
    void setKeyboardFeedback(KeyboardFeedback feedback);

    // [folders] <kind> (design D-2): a fixed folder set in the settings, not
    // changed by each Open, or, when none is set, the folder last used.
    // openDir() is the fixed folder, empty when none is set; setOpenDir("")
    // goes back to "<last used>", starting from the folder that was fixed.
    std::string openDir(OpenFolder folder) const;
    void setOpenDir(OpenFolder folder, const std::string& dir);
    // The fixed folder, else the folder last used, else `home`.
    std::string openStartDir(OpenFolder folder, const std::string& home) const;
    // After a pick: the folder of `filePath` (absolute) unless a fixed one is set.
    void rememberOpenFile(OpenFolder folder, const std::string& filePath);

    WindowState windowState() const;
    void setWindowState(const WindowState& state);
    // [window] portrait-debug-page (Swift, iOS): the debugger as a third page.
    bool portraitDebugPage() const;
    void setPortraitDebugPage(bool on);

private:
    calcu::settings::SettingsFile& m_file;
};

} // namespace cu59::settings
