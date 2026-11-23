#pragma once
#include "shell/Chrome.hpp"
#include <string_view>
#include <vector>

// Calc-U-1600's look and layout values: one source for the Qt6 stylesheets
// and the Web page (the `chrome` of `c1600.shell-product`). A light and a
// dark set, following the system's appearance; the docked panels' chrome
// (INSPECTOR, the paper panel, the CE-158 printer) and, for the Web, the
// window's own controls as the Qt6 app shows them on macOS.

namespace cu1600 {

// The product's own colours (Chrome's `more`), by name.
namespace chrome {
inline constexpr std::string_view kControl = "control";                  // a button, a picker
inline constexpr std::string_view kControlHover = "control-hover";
inline constexpr std::string_view kControlPressed = "control-pressed";
inline constexpr std::string_view kSeparator = "separator";
inline constexpr std::string_view kAccent = "accent";                    // a checked toggle
inline constexpr std::string_view kAccentText = "accent-text";
inline constexpr std::string_view kLampOff = "lamp-off";                 // a tape or disk lamp
inline constexpr std::string_view kLampOn = "lamp-on";
inline constexpr std::string_view kKeyPressed = "key-pressed";           // over a pressed key of the faceplate
inline constexpr std::string_view kBannerBorder = "banner-border";       // the storage banner's
} // namespace chrome

const calcu::shell::Chrome& sharpChrome();
// Every name above, for the tests (calcu::shell::checkChrome).
std::vector<std::string_view> sharpChromeNames();

} // namespace cu1600
