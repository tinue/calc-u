#pragma once
#include "machine/Ids.hpp"
#include "sessions/Value.hpp"
#include "shell/HitMap.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>

// What Calc-U-59 gives LIB-14's shell (LIB-14 design §10.6, D-32): one
// source for the Qt6 frontend, the Web page (through TIWebPart's request
// `c59.shell-product`) and, through the bridge, Swift (C59-8). The Qt6
// frontend adds the acts of the two card items; the page its own.

namespace cu59 {

inline constexpr const char* kHelpUrl = "https://calc-u.org/59/";
// C59-7's two card items (AC-14, AC-15), on the TI-59 only: they follow the
// media place `card-reader` (M17); C59-7's card controls stand for them on
// the bar (`onBar`, M14).
inline constexpr const char* kItemArmCard = "arm-card";
inline constexpr const char* kItemArmFreshCard = "arm-fresh-card";
inline constexpr const char* kCardReaderPlace = "card-reader";

// The settings dialog differs by frontend and layout (AC-30, design §10.6);
// the Swift frontend's on macOS and on iOS and iPadOS (C59-8 AC-7, design §3.4).
enum class ShellFrontend { Qt6, WebWide, WebPortrait, SwiftMac, SwiftIOS };

// The [window] key of the Swift frontend's iOS settings (C59-8 AC-7, OQ-4;
// LIB-9 OQ-5), which LIB-14's SettingsPart shows only when the frontend asks
// (SettingsPartSetup::alsoShown).
inline constexpr const char* kPortraitDebugPage = "window.portrait-debug-page";

// `version`, `build`: the Qt6 frontend's APP_VERSION and APP_BUILD_ID; the
// page's version on the Web (no build there).
calcu::shell::ShellProduct shellProduct(std::string version, std::string build);

// The sections of AC-30 for the frontend: General (Startup model and the
// line "Version <v> (<build>)", `versionLine`), Startup presets, then per
// frontend Storage, Folders, Tracing, Display, Keyboard. The Swift frontend
// (C59-8 AC-7): General, Startup presets, Display, on iOS Keyboard and
// Layout, then Tracing (where traces go, on macOS the folder, the size).
calcu::shell::DialogSpec settingsSpec(ShellFrontend, const std::string& versionLine);

// The faceplate's hit map (LIB-14 AC-24; D-41, D-44): in pixels of the
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
