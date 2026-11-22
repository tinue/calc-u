#pragma once
#include "shell/Chrome.hpp"
#include <string_view>
#include <vector>

// Calc-U-59's look and layout values: one source for the Qt6 stylesheets,
// the Swift views (through the bridge's TIChrome) and the Web page (the
// `chrome` of `c59.shell-product`). Dark only, the apps' dark chrome.

namespace cu59 {

// The product's own colours (Chrome's `more`), by name.
namespace chrome {
inline constexpr std::string_view kDebugBackground = "debug-background";   // the debugger's column
inline constexpr std::string_view kDebugHeader = "debug-header";           // its tab row and headers
inline constexpr std::string_view kSection = "section";                    // a debugger section's body
inline constexpr std::string_view kTabSelected = "tab-selected";           // the selected debugger tab
inline constexpr std::string_view kPillOn = "pill-on";                     // a pill that is switched on (a plugged place)
inline constexpr std::string_view kDivider = "divider";                    // a line between bar groups
inline constexpr std::string_view kColumnLine = "column-line";             // the line between columns
inline constexpr std::string_view kOrange = "orange";
inline constexpr std::string_view kRed = "red";
inline constexpr std::string_view kYellow = "yellow";
inline constexpr std::string_view kBlue = "blue";
inline constexpr std::string_view kGreen = "green";
inline constexpr std::string_view kHover = "hover";                        // the address under the pointer
inline constexpr std::string_view kLogText = "log-text";
inline constexpr std::string_view kDialog = "dialog";                      // a dialog, a menu
inline constexpr std::string_view kDialogFooter = "dialog-footer";
inline constexpr std::string_view kField = "field";                        // a text field, a list
inline constexpr std::string_view kFieldBorder = "field-border";
inline constexpr std::string_view kListLine = "list-line";
inline constexpr std::string_view kAlternate = "alternate";                // every other row of a list
inline constexpr std::string_view kButton = "button";                      // a native button
inline constexpr std::string_view kHighlight = "highlight";                // the selection, the default button
inline constexpr std::string_view kScrim = "scrim";                        // behind a dialog
inline constexpr std::string_view kToast = "toast";                        // a toast, the status line
inline constexpr std::string_view kShadow = "shadow";
inline constexpr std::string_view kChevron = "chevron";                    // the portrait pages' arrows
inline constexpr std::string_view kInk = "ink";                            // the PC-100C's dots
} // namespace chrome

const calcu::shell::Chrome& tiChrome();
// Every name above, for the tests (calcu::shell::checkChrome).
std::vector<std::string_view> tiChromeNames();

} // namespace cu59
