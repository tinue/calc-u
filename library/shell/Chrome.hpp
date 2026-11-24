#pragma once
#include "sessions/Value.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// The look and the layout values of a product's GUIs, written once in the
// product's app/ (Calc-U-59's TIChrome, Calc-U-1600's SharpChrome) and read
// by every GUI: the Qt6 stylesheets, the Swift views through the bridge,
// the Web page as CSS custom properties on `.calcu-client` (the product's
// shell-product answer carries toValue()). Panels side by side line up: every
// panel header has `panelHeaderHeight`, every panel button bar
// `panelButtonsHeight`, and the buttons in them are pills of one shape.

namespace calcu::shell {

// An sRGB colour, each channel 0–255 (alpha too, as Qt's).
struct Colour {
    int r = 0, g = 0, b = 0, a = 255;

    // A grey of this whiteness (0–1), as SwiftUI's Color(white:).
    static constexpr Colour grey(double white, double alpha = 1) {
        const int v = static_cast<int>(white * 255 + 0.5);
        return Colour{v, v, v, static_cast<int>(alpha * 255 + 0.5)};
    }
    // 0xRRGGBB.
    static constexpr Colour rgb(std::uint32_t packed, double alpha = 1) {
        return Colour{static_cast<int>(packed >> 16 & 0xFF), static_cast<int>(packed >> 8 & 0xFF),
                      static_cast<int>(packed & 0xFF), static_cast<int>(alpha * 255 + 0.5)};
    }
    constexpr Colour withAlpha(double alpha) const { return Colour{r, g, b, static_cast<int>(alpha * 255 + 0.5)}; }
    friend constexpr bool operator==(const Colour&, const Colour&) = default;
};

// "#rrggbb" when opaque, else "rgba(r, g, b, a)" with a in 0–1.
std::string css(const Colour&);

// One set of the chrome's colours (a product has a light one, and a dark
// one where it follows the system's appearance).
struct ChromeColours {
    Colour window;                       // the columns' background
    Colour text, textDim;                // text on it; disabled text
    Colour header, headerText;           // a panel's header
    Colour buttonBar;                    // a panel's button bar and the control bar
    Colour pill, pillHover, pillPressed, pillOff, pillText, pillTextOff;   // pill buttons
    Colour output, outputText, outputBorder;   // a panel's body (the paper, the inspector's text)
    Colour plugged, unplugged;           // the dot of a place button: plugged in or not
    // The product's own colours by name (kebab-case: "debug-header"); the
    // Web's custom property is `--calcu-<name>`.
    std::vector<std::pair<std::string, Colour>> more;

    // A colour of `more`, or nullptr.
    const Colour* find(std::string_view name) const;
    // A colour of `more`; a missing one is magenta, so it shows (the
    // product's tests check every name it uses: checkChrome).
    Colour named(std::string_view name) const;
    // Gives a colour of `more` another value (a dark set over the light
    // one); a name not in `more` is not added.
    void setNamed(std::string_view name, Colour colour);
};

// The colours of `more` the component sets draw the shell with (the Web's
// menus, dialogs, notices and banner; the paper of every paper panel):
// every product's chrome has them, its tests check them (checkChrome with
// shellColourNames()).
namespace colour {
inline constexpr std::string_view kMenu = "menu";                    // a menu, the More popup, a dialog
inline constexpr std::string_view kMenuText = "menu-text";
inline constexpr std::string_view kMenuBorder = "menu-border";       // its border and separators
inline constexpr std::string_view kMenuHover = "menu-hover";         // the item under the pointer
inline constexpr std::string_view kMenuHoverText = "menu-hover-text";
inline constexpr std::string_view kScrim = "scrim";                  // behind a dialog
inline constexpr std::string_view kShadow = "shadow";                // a menu's, a dialog's, a pressed control's
inline constexpr std::string_view kLink = "link";                    // a link in a dialog
inline constexpr std::string_view kAlert = "alert";                  // a destructive item, a problem
inline constexpr std::string_view kBanner = "banner";                // the storage banner
inline constexpr std::string_view kBannerText = "banner-text";
inline constexpr std::string_view kBannerBorder = "banner-border";   // its edge towards the client
inline constexpr std::string_view kPaper = "paper";                  // a printer's paper
inline constexpr std::string_view kPaperText = "paper-text";         // its text view
} // namespace colour
std::vector<std::string_view> shellColourNames();

// The shape of a pill button, in logical pixels. Both products' pills.
struct PillShape {
    double height = 24, radius = 6, paddingX = 8, paddingY = 4;
};

struct ChromeSizes {
    double panelHeaderHeight = 28;      // every panel header
    double panelButtonsHeight = 40;     // every panel button bar
    double panelInset = 0;              // between a column's edge and its panel
    PillShape pill;
    double pillFontSize = 0;            // 0: the panel's font
    bool pillBold = false;
    double barHeight = 40, barGap = 6;  // the control bar
    bool barPills = false;              // its buttons are pills (else plain text)
    double slotPickerEm = 9;            // a module slot's picker, whatever it shows
    double printerMinWidth = 0, printerMaxWidth = 0;   // the printer column (desktop, embedded); 0: none
    double debuggerMinWidth = 0, debuggerMinHeight = 0;   // the debugger beside the printer; 0: none
    // The view rule's panels (shell::view): the least room they take beside
    // the faceplate (a width) and below it (a height); 0: no panels there.
    double panelsMinWidth = 0, panelsMinHeight = 0;
    // The least faceplate width whose keys a finger can use: a mobile
    // window turned against the faceplate's orientation shows the panels
    // alone below it (shell::view); 0: never.
    double faceplateUsableWidth = 0;
    // The bar's and the panels' text keeps its size next to the faceplate:
    // as designed at `textScaleWidth`, growing with a wider faceplate up to
    // `textScaleMax` times. 0: the text never scales.
    double textScaleWidth = 0, textScaleMax = 1;
    // The paper panel's control row is compact (the controls' short labels,
    // tighter spacing) below this width; 0: never. Its gap between the
    // buttons and at its ends, and the compact row's.
    double paperRowCompactWidth = 280;
    double paperRowGap = 12, paperRowCompactGap = 6;
};

// The paper panel's control row at this width: compact or not. Mirrored in
// shellModel.js (calcuShellPaperRowCompact): the page measures its panel.
bool paperRowCompact(const ChromeSizes&, double rowWidth);

struct Chrome {
    ChromeColours light;
    std::optional<ChromeColours> dark;   // none: `light` in either appearance
    ChromeSizes sizes;

    const ChromeColours& colours(bool darkAppearance) const { return darkAppearance && dark ? *dark : light; }
    // The text's scale for a faceplate this wide (1 without the rule).
    double textScale(double faceplateWidth) const;
};

// {light: {window: "#rrggbb", …, <more's names>}, dark: {…} (only when set),
//  sizes: {panelHeaderHeight, panelButtonsHeight, panelInset, pillHeight,
//  pillRadius, pillPaddingX, pillPaddingY, pillFontSize, pillBold,
//  barHeight, barGap, slotPickerEm, printerMinWidth, printerMaxWidth,
//  debuggerMinWidth, debuggerMinHeight, panelsMinWidth, panelsMinHeight,
//  faceplateUsableWidth, textScaleWidth, textScaleMax, paperRowCompactWidth, paperRowGap,
//  paperRowCompactGap}}; colour names in kebab-case, as the Web's
// custom properties (`--calcu-pill-text`).
sessions::Value toValue(const Chrome&);

// The problems of a product's chrome, for its tests: a name of `names`
// missing from `more` of a set, a set whose `more` names differ from the
// other's. Empty when sound.
std::vector<std::string> checkChrome(const Chrome&, const std::vector<std::string_view>& names);

} // namespace calcu::shell
