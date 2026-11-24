#include "SharpChrome.hpp"

namespace cu1600 {

using calcu::shell::Colour;
namespace colour = calcu::shell::colour;

namespace {

Colour grey(int v, int a = 255) { return Colour{v, v, v, a}; }

calcu::shell::ChromeColours light() {
    calcu::shell::ChromeColours k;
    k.window = grey(255);
    k.text = Colour::rgb(0x1d1d1f);
    k.textDim = grey(0, 89);
    k.header = grey(204);
    k.headerText = grey(0, 140);
    k.buttonBar = grey(217);
    k.pill = grey(173);
    k.pillHover = grey(199);     // the pill 15 % lighter
    k.pillPressed = grey(133);   // 30 % darker
    k.pillOff = grey(199);
    k.pillText = grey(0, 204);
    k.pillTextOff = grey(0, 204);
    k.output = grey(247);
    k.outputText = grey(0, 204);
    k.outputBorder = grey(153);
    k.plugged = Colour::rgb(0xff3b30);
    k.unplugged = grey(128, 102);
    k.more = {
        {std::string(chrome::kControl), grey(235)},
        {std::string(chrome::kControlHover), grey(226)},
        {std::string(chrome::kControlPressed), grey(212)},
        {std::string(chrome::kSeparator), grey(200)},
        {std::string(chrome::kAccent), Colour::rgb(0x007bff)},
        {std::string(chrome::kAccentText), grey(255)},
        {std::string(colour::kMenu), grey(246, 250)},
        {std::string(colour::kMenuText), Colour::rgb(0x1d1d1f)},
        {std::string(colour::kMenuBorder), grey(0, 31)},
        {std::string(colour::kMenuHover), Colour::rgb(0x007bff)},
        {std::string(colour::kMenuHoverText), grey(255)},
        {std::string(colour::kLink), Colour::rgb(0x0058d0)},
        {std::string(chrome::kLampOff), grey(136)},
        {std::string(chrome::kLampOn), Colour::rgb(0x2ecc40)},
        {std::string(chrome::kKeyPressed), grey(255, 64)},
        {std::string(colour::kScrim), grey(0, 31)},
        {std::string(colour::kShadow), grey(0, 46)},
        {std::string(colour::kAlert), Colour::rgb(0xb00020)},
        {std::string(colour::kBanner), Colour::rgb(0xfff4c2)},
        {std::string(colour::kBannerText), Colour::rgb(0x5c4a00)},
        {std::string(colour::kBannerBorder), Colour::rgb(0xe6d27a)},
        {std::string(colour::kPaper), grey(255)},   // white whatever the appearance: the pens' inks
        {std::string(colour::kPaperText), Colour::rgb(0x1d1d1f)},
    };
    return k;
}

calcu::shell::ChromeColours dark() {
    calcu::shell::ChromeColours k = light();
    k.window = grey(30);
    k.text = grey(232);
    k.textDim = grey(255, 89);
    k.header = grey(60);
    k.headerText = grey(255, 150);
    k.buttonBar = grey(48);
    k.pill = grey(120);
    k.pillHover = grey(138);
    k.pillPressed = grey(92);
    k.pillOff = grey(66);
    k.pillText = grey(255, 220);
    k.pillTextOff = grey(255, 220);
    k.output = grey(32);
    k.outputText = grey(255, 210);
    k.outputBorder = grey(90);
    k.setNamed(chrome::kControl, grey(58));
    k.setNamed(chrome::kControlHover, grey(69));
    k.setNamed(chrome::kControlPressed, grey(80));
    k.setNamed(chrome::kSeparator, grey(74));
    k.setNamed(colour::kMenu, Colour{44, 44, 46, 250});
    k.setNamed(colour::kMenuText, grey(232));
    k.setNamed(colour::kMenuBorder, grey(255, 31));
    k.setNamed(colour::kLink, Colour::rgb(0x5aa9ff));
    k.setNamed(colour::kAlert, Colour::rgb(0xff6b6b));
    k.setNamed(colour::kBanner, Colour::rgb(0x4a3f10));
    k.setNamed(colour::kBannerText, Colour::rgb(0xffe9a0));
    return k;
}

calcu::shell::Chrome make() {
    calcu::shell::Chrome c;
    c.light = light();
    c.dark = dark();
    calcu::shell::ChromeSizes& s = c.sizes;
    // INSPECTOR, the paper panel and the CE-158 printer side by side.
    s.panelHeaderHeight = 28;
    s.panelButtonsHeight = 40;
    s.panelInset = 6;
    s.barHeight = 40;
    s.barGap = 6;
    s.barPills = true;
    s.slotPickerEm = 10.5;   // the longest module name ("superRAM r4"), whatever it shows
    // The view rule (shell::view): the panels' row below the wide
    // faceplate, at least this high; none beside it.
    s.panelsMinHeight = 200;
    s.panelsMinWidth = 0;
    // Upright, a phone's faceplate is too narrow for the keys (at most
    // 440 points), a tablet's is not (from 744): below this width, the
    // panels alone on mobile.
    s.faceplateUsableWidth = 600;
    // The inspector (the debugger) beside the paper in that row: the
    // window at least this wide; narrower, one of them at a time.
    s.debuggerMinWidth = 700;
    return c;
}

} // namespace

const calcu::shell::Chrome& sharpChrome() {
    static const calcu::shell::Chrome c = make();
    return c;
}

std::vector<std::string_view> sharpChromeNames() {
    using namespace chrome;
    std::vector<std::string_view> names{kControl, kControlHover, kControlPressed, kSeparator, kAccent, kAccentText,
                                        kLampOff, kLampOn, kKeyPressed};
    for (std::string_view n : calcu::shell::shellColourNames()) names.push_back(n);
    return names;
}

} // namespace cu1600
