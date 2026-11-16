#pragma once
#include <optional>
#include <string>
#include <vector>

#include "machine/Ids.hpp"
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"

// ── Settings: Calc-U-1600's keys, and typed calls over the settings file ──
//
// Every setting the app keeps is declared once, in settingsDecl() (LIB-9
// AC-26): section, key, default and the values it accepts. The values live
// in the product's settings file, `Calc-U-1600.ini`, read and written by the
// library (calcu::settings::SettingsFile; the frontend opens it at the place
// calcu::settings gives for its platform). Qt6 and the Web frontend use the
// same declaration; a key a frontend has no use for is kept and ignored
// (AC-32). No platform store (QSettings, localStorage) holds a setting
// (AC-31).
namespace shared {

// The declaration of Calc-U-1600's settings (AC-26). Lives as long as the
// program.
const calcu::settings::ProductDecl& settingsDecl();

// The models in the catalogue's order. Their IDs ("PC-1500", "PC-1500A",
// "PC-1600") are also the names the settings file writes ([startup] model,
// last-model, the keys of [preset]); D-28 of LIB-14: no enum of its own.
const std::vector<calcu::machine::ModelId>& sharpModels();
// The model of that name, or none.
std::optional<calcu::machine::ModelId> sharpModel(const std::string& name);

// The folders the file-open dialogs start in, one per kind of file, each
// shown as its own row in Settings, and one key in [folders] (AC-29):
//   Samples   -- "Load Preset…" (and Settings' default-preset pickers): presets
//   Basic     -- "Load BASIC Program…" (.bas listings): basic-programs
//   Assembly  -- "Load Machine Code…" (.bin files): machine-code
//   HostDrive -- "Mount Directory…" (PC-1600 host drive S3:): host-drive
enum class OpenFolder { Samples, Basic, Assembly, HostDrive };
const char* openFolderKind(OpenFolder folder);

class Settings {
public:
    explicit Settings(calcu::settings::SettingsFile& file) : m_file(file) {}

    calcu::settings::SettingsFile& file() { return m_file; }
    const calcu::settings::SettingsFile& file() const { return m_file; }

    // [storage] save-folder -- empty means "use the ~/Calc-U-1600 default"
    // (AppPaths::instanceDir()); there is no AppData/Application Support
    // fallback in this design at all.
    std::string instanceDirOverride() const { return m_file.text("storage", "save-folder"); }
    void setInstanceDirOverride(const std::string& dir) { m_file.set("storage", "save-folder", dir); }

    // [storage] tape-folder -- the cassette tapes' folder; empty means
    // "Tapes" inside the save folder (AppPaths::tapeDir()).
    std::string tapeDirOverride() const { return m_file.text("storage", "tape-folder"); }
    void setTapeDirOverride(const std::string& dir) { m_file.set("storage", "tape-folder", dir); }

    // [folders] <kind> (design D-2): a fixed folder the dialog starts in,
    // set via Settings and not changed by each Open, or, when none is set,
    // the folder of the file last picked in that dialog (for HostDrive: the
    // directory last mounted).
    // openDir() is the fixed folder, empty when none is set; setOpenDir("")
    // -- Settings' Reset -- goes back to "<last used>", starting from the
    // folder that was fixed.
    std::string openDir(OpenFolder folder) const;
    void setOpenDir(OpenFolder folder, const std::string& dir);

    // The folder last picked in that dialog while no fixed folder is set;
    // empty when there is none. Stored absolute.
    std::string lastOpenDir(OpenFolder folder) const;
    void rememberOpenDir(OpenFolder folder, const std::string& dir);
    void rememberOpenFile(OpenFolder folder, const std::string& filePath);

    // The start directory for that dialog: the fixed openDir() if set, else
    // the last-used folder, else `homeDir`.
    std::string openStartDir(OpenFolder folder, const std::string& homeDir) const;

    // [preset] <model> -- the preset applied whenever that model gets
    // selected; empty means none (the model just boots bare).
    std::string defaultPresetPath(const calcu::machine::ModelId& model) const { return m_file.text("preset", model.value); }
    void setDefaultPresetPath(const calcu::machine::ModelId& model, const std::string& path) {
        m_file.set("preset", model.value, path);
    }

    // [trace] folder -- empty means "write TRACE.bin under
    // AppPaths::instanceDir()".
    std::string traceDirOverride() const { return m_file.text("trace", "folder"); }
    void setTraceDirOverride(const std::string& dir) { m_file.set("trace", "folder", dir); }

    // [trace] max-file-size-mb -- the size the trace writer rolls TRACE.bin
    // over at, in megabytes; missing, zero, negative or not a number: 50
    // (LIB-9 AC-11).
    static constexpr int kDefaultTraceMaxFileSizeMB = 50;
    int traceMaxFileSizeMB() const { return static_cast<int>(m_file.integer("trace", "max-file-size-mb")); }
    void setTraceMaxFileSizeMB(int mb) { m_file.set("trace", "max-file-size-mb", std::to_string(mb)); }

    // [debug] dap-enabled (default off) and dap-port (default 32168,
    // anything outside 1024-65535 reads as the default, AC-28) -- the Debug
    // Adapter Protocol server VS Code attaches to. It listens on 127.0.0.1
    // only.
    static constexpr int kDefaultDapPort = 32168;
    bool dapEnabled() const { return m_file.boolean("debug", "dap-enabled"); }
    void setDapEnabled(bool on) { m_file.set("debug", "dap-enabled", on ? "true" : "false"); }
    int dapPort() const { return static_cast<int>(m_file.integer("debug", "dap-port")); }
    void setDapPort(int port) { m_file.set("debug", "dap-port", std::to_string(port)); }

    // [serial] link-folder -- directory the serial ports' stable symlinks
    // (`calcu1600-rs232c.serial`, ...) go in. Empty means
    // AppPaths::instanceDir().
    std::string serialLinkDirOverride() const { return m_file.text("serial", "link-folder"); }
    void setSerialLinkDirOverride(const std::string& dir) { m_file.set("serial", "link-folder", dir); }

    // [window] width / height -- last user-resized main-window size,
    // restored on the next launch (default 900x500, AC-30).
    static constexpr int kDefaultWindowWidth = 900;
    static constexpr int kDefaultWindowHeight = 500;
    int windowWidth() const { return static_cast<int>(m_file.integer("window", "width")); }
    int windowHeight() const { return static_cast<int>(m_file.integer("window", "height")); }
    void setWindowSize(int width, int height) {
        m_file.set("window", "width", std::to_string(width));
        m_file.set("window", "height", std::to_string(height));
    }

    // [startup] model -- which model to boot into: "last" (the default)
    // reuses lastUsedModel(), or one of "PC-1500"/"PC-1500A"/"PC-1600"
    // pins a model regardless of what was last selected. There is
    // deliberately no ROM-revision preference: the plain PC-1500 always
    // boots on ROM A04.
    std::string startupModelPreference() const { return m_file.text("startup", "model"); }
    void setStartupModelPreference(const std::string& pref) { m_file.set("startup", "model", pref); }

    // [startup] last-model -- the model last switched to. Only consulted
    // when startupModelPreference() is "last". None until a model was used.
    std::optional<calcu::machine::ModelId> lastUsedModel() const { return sharpModel(m_file.text("startup", "last-model")); }
    void setLastUsedModel(const calcu::machine::ModelId& model) { m_file.set("startup", "last-model", model.value); }

    // The model to boot into: the pinned preference if it names a model,
    // else the last used one, else the PC-1500A (AC-27, EC-1).
    calcu::machine::ModelId startupModel() const;

private:
    calcu::settings::SettingsFile& m_file;
};

} // namespace shared
