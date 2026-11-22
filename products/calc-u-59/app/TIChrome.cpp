#include "TIChrome.hpp"
#include "KeyboardLayout.hpp"

namespace cu59 {

using calcu::shell::Colour;
namespace colour = calcu::shell::colour;

namespace {

calcu::shell::Chrome make() {
    calcu::shell::Chrome c;
    calcu::shell::ChromeColours& k = c.light;
    const Colour white{255, 255, 255, 255};
    k.window = Colour::rgb(0x1d1d1c);
    k.text = white;
    k.textDim = white.withAlpha(0.4);
    k.header = Colour::grey(0.08);
    k.headerText = white;
    k.buttonBar = Colour::grey(0.15);
    k.pill = Colour::grey(0.25);
    k.pillHover = Colour::grey(0.28);
    k.pillPressed = Colour::grey(0.18);
    k.pillOff = Colour::grey(0.15);
    k.pillText = white;
    k.pillTextOff = white.withAlpha(0.4);
    k.output = Colour{245, 240, 224, 255};   // the PC-100C's cream paper (0.96, 0.94, 0.88)
    k.outputText = Colour::rgb(0x1f1f1f);
    k.outputBorder = Colour::grey(0.7);
    k.plugged = Colour::rgb(0xff3b30);
    k.unplugged = Colour{128, 128, 128, 102};
    k.more = {
        {std::string(chrome::kDebugBackground), Colour::grey(0.10)},
        {std::string(chrome::kDebugHeader), Colour::grey(0.07)},
        {std::string(chrome::kSection), Colour::grey(0.13)},
        {std::string(chrome::kTabSelected), Colour::grey(0.20)},
        {std::string(chrome::kPillOn), Colour::rgb(0x5a5a5a)},
        {std::string(chrome::kDivider), white.withAlpha(0.25)},
        {std::string(chrome::kColumnLine), Colour::rgb(0x000000)},
        {std::string(chrome::kOrange), Colour::rgb(0xff9500)},
        {std::string(chrome::kRed), Colour::rgb(0xff3b30)},
        {std::string(chrome::kYellow), Colour::rgb(0xffcc00)},
        {std::string(chrome::kBlue), Colour::rgb(0x32ade6)},
        {std::string(chrome::kGreen), Colour::rgb(0x34c759)},
        {std::string(chrome::kHover), Colour{255, 214, 0, 217}},
        {std::string(chrome::kLogText), Colour::grey(0.85)},
        {std::string(chrome::kDialog), Colour::rgb(0x252525)},
        {std::string(chrome::kDialogFooter), Colour::rgb(0x222222)},
        {std::string(chrome::kField), Colour::rgb(0x1c1c1c)},
        {std::string(chrome::kFieldBorder), Colour::rgb(0x555555)},
        {std::string(chrome::kListLine), Colour::rgb(0x333333)},
        {std::string(chrome::kAlternate), Colour::rgb(0x2d2d2d)},
        {std::string(chrome::kButton), Colour::rgb(0x323232)},
        {std::string(chrome::kHighlight), Colour::rgb(0x0a84ff)},
        {std::string(colour::kScrim), Colour{0, 0, 0, 140}},
        {std::string(chrome::kToast), Colour{0, 0, 0, 191}},
        {std::string(colour::kShadow), Colour{0, 0, 0, 153}},
        // The Web's menus and dialogs, light on the dark chrome as iOS's.
        {std::string(colour::kMenu), Colour::rgb(0xffffff)},
        {std::string(colour::kMenuText), Colour::rgb(0x1d1d1f)},
        {std::string(colour::kMenuBorder), Colour::rgb(0xbbbbbb)},
        {std::string(colour::kMenuHover), Colour::rgb(0xe4ecf7)},
        {std::string(colour::kMenuHoverText), Colour::rgb(0x1d1d1f)},
        {std::string(colour::kAlert), Colour::rgb(0xb00020)},
        {std::string(colour::kBanner), Colour::rgb(0xfff4c2)},
        {std::string(colour::kBannerText), Colour::rgb(0x5c4a00)},
        {std::string(colour::kPaper), Colour{245, 240, 224, 255}},   // the PC-100C's cream paper
        {std::string(colour::kPaperText), Colour::rgb(0x1d1d1f)},
        {std::string(chrome::kChevron), Colour{60, 60, 60, 179}},
        {std::string(chrome::kInk), Colour::grey(0.12)},
        {std::string(chrome::kStepCurrent), Colour{153, 255, 153, 255}},
        {std::string(chrome::kHeatmapPc), Colour::rgb(0x00ff00)},
    };

    calcu::shell::ChromeSizes& s = c.sizes;
    // The printer's header and the debugger's tab row; the printer's
    // button bar, the debugger's and the control bar, all 50 high.
    s.panelHeaderHeight = 36;
    s.panelButtonsHeight = 50;
    s.panelInset = 8;
    s.pillFontSize = 11;
    s.pillBold = true;
    s.barHeight = 50;
    s.barGap = 10;
    s.slotPickerEm = 0;   // the module's ID, as wide as it is
    // The printer column beside the debugger on a desktop and embedded:
    // its buttons in one row below the paper.
    s.printerMinWidth = 240;
    s.printerMaxWidth = 400;
    // The view rule (shell::view): the printer column with its insets
    // beside the faceplate, else the pages; no panels below the tall
    // faceplate. The debugger joins it where the room beside takes both
    // and the window is tall enough for its sections (not a phone in
    // landscape).
    s.panelsMinWidth = 256;
    s.panelsMinHeight = 0;
    s.debuggerMinWidth = 220;
    s.debuggerMinHeight = 560;
    // The bar's and the paper panel's text as designed at 440 px, growing
    // with the faceplate up to twice that.
    s.textScaleWidth = 440;
    s.textScaleMax = 2;
    return c;
}

} // namespace

const calcu::shell::Chrome& tiChrome() {
    static const calcu::shell::Chrome c = make();
    return c;
}

std::vector<std::string_view> tiChromeNames() {
    using namespace chrome;
    std::vector<std::string_view> names{kDebugBackground, kDebugHeader, kSection, kTabSelected, kPillOn, kDivider, kColumnLine, kOrange, kRed,
            kYellow, kBlue, kGreen, kHover, kLogText, kDialog, kDialogFooter, kField, kFieldBorder, kListLine,
            kAlternate, kButton, kHighlight, kToast, kChevron, kInk, kStepCurrent, kHeatmapPc};
    for (std::string_view n : calcu::shell::shellColourNames()) names.push_back(n);
    return names;
}

calcu::shell::ViewInputs viewInputs(double width, double height, double extraWidth, bool mobile) {
    const calcu::shell::ChromeSizes& s = tiChrome().sizes;
    calcu::shell::ViewInputs in;
    in.width = width;
    in.height = height;
    in.faceplateAspect = kCanvasAspect;
    in.fixedWidth = 2 * s.panelInset + extraWidth;
    in.fixedHeight = s.barHeight + 2 * s.panelInset;
    in.panelsMinWidth = s.panelsMinWidth;
    in.panelsMinHeight = s.panelsMinHeight;
    in.debuggerMinWidth = s.debuggerMinWidth;
    in.debuggerMinHeight = s.debuggerMinHeight;
    in.mobile = mobile;
    return in;
}

} // namespace cu59
