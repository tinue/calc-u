#pragma once
#include "machine/Ids.hpp"
#include "sessions/Value.hpp"
#include "shell/HitMap.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>

// What Calc-U-59 gives the library's shell: one source for the Qt6
// frontend, the Web page (through TIWebPart's request `c59.shell-product`)
// and, through the bridge, Swift. The Qt6
// frontend adds the acts of the two card items; the page its own.

namespace cu59 {

inline constexpr const char* kHelpUrl = "https://calc-u.org/59/";
// The two card items, on the TI-59 only: they follow the media place
// `card-reader`; the card controls stand for them on the bar (`onBar`).
inline constexpr const char* kItemArmCard = "arm-card";
inline constexpr const char* kItemArmFreshCard = "arm-fresh-card";
inline constexpr const char* kCardReaderPlace = "card-reader";

// The settings dialog differs by frontend and layout; the Swift frontend's
// on macOS and on iOS and iPadOS.
enum class ShellFrontend { Qt6, WebWide, WebPortrait, SwiftMac, SwiftIOS };

// The [window] key of the Swift frontend's iOS settings, which the shell's
// SettingsPart shows only when the frontend asks
// (SettingsPartSetup::alsoShown).
inline constexpr const char* kPortraitDebugPage = "window.portrait-debug-page";

// `version`, `build`: the Qt6 frontend's APP_VERSION and APP_BUILD_ID; the
// page's version on the Web (no build there).
calcu::shell::ShellProduct shellProduct(std::string version, std::string build);

// The sections for the frontend: General (Startup model and the
// line "Version <v> (<build>)", `versionLine`), Startup presets, then per
// frontend Storage, Folders, Tracing, Display, Keyboard. The Swift frontend
// General, Startup presets, Display, on iOS Keyboard and
// Layout, then Tracing (where traces go, on macOS the folder, the size).
calcu::shell::DialogSpec settingsSpec(ShellFrontend, const std::string& versionLine);

// The faceplate's hit map: in pixels of the
// base image (2064 × 4075, KeyboardLayout.hpp's canvas), each key's widened
// rectangle (kHitExpandHorizontal, kHitExpandVertical; today's hit test),
// distance 0 (a gap is no key), the display area. Every TI model has the
// same 45 keys (the model only names whose map it is).
calcu::shell::HitMap faceplateHitMap(const calcu::machine::ModelId& model);
inline constexpr double kCanvasWidth = 2064.0, kCanvasHeight = 4075.0;

// `c59.faceplate-map {model}`: {map: the hit map's Value form, card: the
// card area {x, y, w, h}, canvas: {w, h}}, in the same units.
calcu::sessions::Value faceplateMapValue(const calcu::machine::ModelId& model);

} // namespace cu59
