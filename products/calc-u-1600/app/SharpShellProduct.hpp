#pragma once
#include <string>

#include "machine/Ids.hpp"
#include "sessions/Value.hpp"
#include "shell/HitMap.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"

// What Calc-U-1600 gives the family shell (LIB-14 Part D, D.5, D-32): one
// source for the Qt6 and the Web frontend. shellProduct() is the data of
// ShellProduct; the Qt6 frontend adds the acts of its items (and Copy
// Screen), the page gets the same data through the request
// `c1600.shell-product` and adds its own JS acts. The faceplate's hit map
// and the LCD's layout come from the tables Qt6 draws with (app/KeyLayout,
// app/LcdLayout; D-31: `c1600.faceplate-map`).

namespace shared {

// The ids of the product's items (object names `item-<id>`).
inline constexpr const char* kItemLoadBasic = "load-basic-program";
inline constexpr const char* kItemLoadCode = "load-machine-code";
inline constexpr const char* kItemMountDirectory = "mount-directory";
inline constexpr const char* kItemUnmountDirectory = "unmount-directory";


inline constexpr const char* kHelpUrl = "https://calc-u.org/1600/";
// LIB-14 D-8: the distance a pointer in a gap still finds its key, in
// logical pixels.
inline constexpr double kFaceplateHitDistance = 24;

// `withHostFolders`: the Qt6 frontend's FileMedia items (Mount Directory…,
// Unmount); the Web has none (AC-35).
calcu::shell::ShellProduct shellProduct(std::string version, std::string build, bool withHostFolders);

// The hit map of the model's faceplate in the art's own pixels (the scale
// is the art's shown width over its width), the display area the LCD.
calcu::shell::HitMap faceplateHitMap(const calcu::machine::ModelId&);

// The settings dialog of Calc-U-1600 (AC-37, D-26): the family sections
// with the product's own Folders labels; in Qt6 also Serial ports (the link
// folder; not on Windows) and Debugger. The Web page shows General, Startup
// presets and Storage with the sentence on where saved media are kept.
calcu::shell::DialogSpec settingsSpec(bool qt6, bool serialPorts);
inline constexpr const char* kSavedMediaNote =
    "Saved media (modules, disks and tapes) are kept in this browser's storage for this page.";

// The page's forms (the replies of `c1600.shell-product`, `c1600.faceplate-map`).
calcu::sessions::Value shellProductValue(const calcu::shell::ShellProduct&);
calcu::sessions::Value dialogSpecValue(const calcu::shell::DialogSpec&);   // shellModel.js's form
// {width, height, distance, display: {x,y,w,h}, keys: [{name, x, y, w, h}],
// lcd: the colours and fractions of app/LcdLayout}.
calcu::sessions::Value faceplateMapValue(const calcu::machine::ModelId&);

} // namespace shared
