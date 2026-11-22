#include "SharpChrome.hpp"

namespace cu1600 {

using calcu::shell::Colour;

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
        {std::string(chrome::kMenu), grey(246, 250)},
        {std::string(chrome::kMenuBorder), grey(0, 31)},
        {std::string(chrome::kMenuHover), Colour::rgb(0x007bff)},
        {std::string(chrome::kLampOff), grey(136)},
        {std::string(chrome::kLampOn), Colour::rgb(0x2ecc40)},
        {std::string(chrome::kHint), Colour::rgb(0xb45309)},
        {std::string(chrome::kKeyPressed), grey(255, 64)},
        {std::string(chrome::kScrim), grey(0, 31)},
        {std::string(chrome::kShadow), grey(0, 46)},
        {std::string(chrome::kModalShadow), grey(0, 71)},
        {std::string(chrome::kBannerBorder), Colour::rgb(0xe6d27a)},
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
    auto set = [&k](std::string_view name, Colour c) {
        for (auto& [n, v] : k.more)
            if (n == name) v = c;
    };
    set(chrome::kControl, grey(58));
    set(chrome::kControlHover, grey(69));
    set(chrome::kControlPressed, grey(80));
    set(chrome::kSeparator, grey(74));
    set(chrome::kMenu, Colour{44, 44, 46, 250});
    set(chrome::kMenuBorder, grey(255, 31));
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
    s.slotPickerEm = 9;   // the longest module ID ("CE-1600M"), whatever it shows
    // The view rule (shell::view): the panels' row below the wide
    // faceplate, at least this high; none beside it.
    s.panelsMinHeight = 200;
    s.panelsMinWidth = 0;
    return c;
}

} // namespace

const calcu::shell::Chrome& sharpChrome() {
    static const calcu::shell::Chrome c = make();
    return c;
}

std::vector<std::string_view> sharpChromeNames() {
    using namespace chrome;
    return {kControl, kControlHover, kControlPressed, kSeparator, kAccent, kAccentText, kMenu, kMenuBorder, kMenuHover,
            kLampOff, kLampOn, kHint, kKeyPressed, kScrim, kShadow, kModalShadow, kBannerBorder};
}

} // namespace cu1600
