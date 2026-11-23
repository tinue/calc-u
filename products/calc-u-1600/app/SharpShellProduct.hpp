#pragma once
#include <string>

#include "machine/Ids.hpp"
#include "sessions/Value.hpp"
#include "shell/HitMap.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"

// What Calc-U-1600 gives the family shell: one
// source for the Qt6 and the Web frontend. shellProduct() is the data of
// ShellProduct; the Qt6 frontend adds the acts of its items (and Copy
// Screen), the page gets the same data through the request
// `c1600.shell-product` and adds its own JS acts. The faceplate's hit map
// and the LCD's layout come from the tables Qt6 draws with (app/KeyLayout,
// app/LcdLayout; `c1600.faceplate-map`).

namespace cu1600 {

// The ids of the product's items (object names `item-<id>`).
inline constexpr const char* kItemLoadBasic = "load-basic-program";
inline constexpr const char* kItemLoadCode = "load-machine-code";
inline constexpr const char* kItemHostDrive = "host-drive";
inline constexpr const char* kItemUnmountHostDrive = "unmount-host-drive";
// Machine ▸ Trace (the trace file, CALCU1600_TRACE.bin) and Log Level ▸
// Off, Info, Debug (the session's `log-level`).
inline constexpr const char* kItemTrace = "trace";
inline constexpr const char* kItemLogLevel = "log-level";
inline constexpr const char* kItemLogOff = "log-off";
inline constexpr const char* kItemLogInfo = "log-info";
inline constexpr const char* kItemLogDebug = "log-debug";


inline constexpr const char* kHelpUrl = "https://calc-u.org/1600/";
// The distance a pointer in a gap still finds its key, in
// logical pixels.
inline constexpr double kFaceplateHitDistance = 24;

// `withHostFolders`: the Qt6 frontend's FileMedia items (Host Drive…,
// Unmount); `withTrace`: its Machine items Trace and Log Level ▸, which
// follow the topics `trace` and `log-level` and send their commands. The
// Web has neither: a page has no folder to mount, cannot reach the trace
// file and shows no log.
calcu::shell::ShellProduct shellProduct(std::string version, std::string build, bool withHostFolders, bool withTrace);

// The hit map of the model's faceplate in the art's own pixels (the scale
// is the art's shown width over its width), the display area the LCD.
calcu::shell::HitMap faceplateHitMap(const calcu::machine::ModelId&);

// The settings dialog of Calc-U-1600: the family sections
// with the product's own Folders labels; in Qt6 also Serial ports (the link
// folder; not on Windows) and Debugger. The Web page shows General, Startup
// presets and Storage with the sentence on where saved media are kept.
calcu::shell::DialogSpec settingsSpec(bool qt6, bool serialPorts);
inline constexpr const char* kSavedMediaNote =
    "Saved media (modules, disks and cassettes) are kept in this browser's storage for this page.";

// The page's form of the faceplate (the reply of `c1600.faceplate-map`):
// {model, width, height, then the library's form of the hit map (distance,
// display: {x,y,w,h}, keys: [{name, x, y, w, h}]; shell/ValueForms), lcd:
// the colours and fractions of app/LcdLayout}. The reply of
// `c1600.shell-product` is the library's forms of shellProduct() and
// settingsSpec().
calcu::sessions::Value faceplateMapValue(const calcu::machine::ModelId&);

} // namespace cu1600
