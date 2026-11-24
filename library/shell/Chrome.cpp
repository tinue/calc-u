#include "shell/Chrome.hpp"

#include <algorithm>
#include <cstdio>

namespace calcu::shell {

using sessions::Value;

std::string css(const Colour& c) {
    char out[48];
    if (c.a >= 255) std::snprintf(out, sizeof out, "#%02x%02x%02x", c.r, c.g, c.b);
    else std::snprintf(out, sizeof out, "rgba(%d, %d, %d, %.3g)", c.r, c.g, c.b, c.a / 255.0);
    return out;
}

std::vector<std::string_view> shellColourNames() {
    using namespace colour;
    return {kMenu, kMenuText, kMenuBorder, kMenuHover, kMenuHoverText, kLink, kScrim, kShadow, kAlert, kBanner, kBannerText,
            kBannerBorder, kPaper, kPaperText};
}

const Colour* ChromeColours::find(std::string_view name) const {
    for (const auto& [n, c] : more)
        if (n == name) return &c;
    return nullptr;
}

Colour ChromeColours::named(std::string_view name) const {
    const Colour* c = find(name);
    return c ? *c : Colour{255, 0, 255, 255};
}

void ChromeColours::setNamed(std::string_view name, Colour colour) {
    for (auto& [n, c] : more)
        if (n == name) c = colour;
}

double Chrome::textScale(double faceplateWidth) const {
    if (sizes.textScaleWidth <= 0) return 1;
    return std::clamp(faceplateWidth / sizes.textScaleWidth, 1.0, std::max(1.0, sizes.textScaleMax));
}

bool paperRowCompact(const ChromeSizes& s, double rowWidth) {
    return s.paperRowCompactWidth > 0 && rowWidth < s.paperRowCompactWidth;
}

namespace {

Value colours(const ChromeColours& c) {
    Value::Object o{{"window", Value(css(c.window))},
                    {"text", Value(css(c.text))},
                    {"text-dim", Value(css(c.textDim))},
                    {"header", Value(css(c.header))},
                    {"header-text", Value(css(c.headerText))},
                    {"button-bar", Value(css(c.buttonBar))},
                    {"pill", Value(css(c.pill))},
                    {"pill-hover", Value(css(c.pillHover))},
                    {"pill-pressed", Value(css(c.pillPressed))},
                    {"pill-off", Value(css(c.pillOff))},
                    {"pill-text", Value(css(c.pillText))},
                    {"pill-text-off", Value(css(c.pillTextOff))},
                    {"output", Value(css(c.output))},
                    {"output-text", Value(css(c.outputText))},
                    {"output-border", Value(css(c.outputBorder))},
                    {"plugged", Value(css(c.plugged))},
                    {"unplugged", Value(css(c.unplugged))}};
    for (const auto& [name, colour] : c.more) o.push_back({name, Value(css(colour))});
    return Value(std::move(o));
}

} // namespace

Value toValue(const Chrome& chrome) {
    const ChromeSizes& s = chrome.sizes;
    Value sizes = Value::object({{"panelHeaderHeight", s.panelHeaderHeight},
                                 {"panelButtonsHeight", s.panelButtonsHeight},
                                 {"panelInset", s.panelInset},
                                 {"pillHeight", s.pill.height},
                                 {"pillRadius", s.pill.radius},
                                 {"pillPaddingX", s.pill.paddingX},
                                 {"pillPaddingY", s.pill.paddingY},
                                 {"pillFontSize", s.pillFontSize},
                                 {"pillBold", s.pillBold},
                                 {"barHeight", s.barHeight},
                                 {"barGap", s.barGap},
                                 {"slotPickerEm", s.slotPickerEm},
                                 {"printerMinWidth", s.printerMinWidth},
                                 {"printerMaxWidth", s.printerMaxWidth},
                                 {"debuggerMinWidth", s.debuggerMinWidth},
                                 {"debuggerMinHeight", s.debuggerMinHeight},
                                 {"panelsMinWidth", s.panelsMinWidth},
                                 {"panelsMinHeight", s.panelsMinHeight},
                                 {"faceplateUsableWidth", s.faceplateUsableWidth},
                                 {"textScaleWidth", s.textScaleWidth},
                                 {"textScaleMax", s.textScaleMax},
                                 {"paperRowCompactWidth", s.paperRowCompactWidth},
                                 {"paperRowGap", s.paperRowGap},
                                 {"paperRowCompactGap", s.paperRowCompactGap}});
    Value::Object o{{"light", colours(chrome.light)}};
    if (chrome.dark) o.push_back({"dark", colours(*chrome.dark)});
    o.push_back({"sizes", std::move(sizes)});
    return Value(std::move(o));
}

std::vector<std::string> checkChrome(const Chrome& chrome, const std::vector<std::string_view>& names) {
    std::vector<std::string> problems;
    auto check = [&](const ChromeColours& c, const char* set) {
        for (auto name : names)
            if (!c.find(name)) problems.push_back(std::string(set) + " has no colour " + std::string(name));
    };
    check(chrome.light, "light");
    if (chrome.dark) {
        check(*chrome.dark, "dark");
        for (const auto& [name, colour] : chrome.light.more)
            if (!chrome.dark->find(name)) problems.push_back("dark has no colour " + name + " of light");
        for (const auto& [name, colour] : chrome.dark->more)
            if (!chrome.light.find(name)) problems.push_back("light has no colour " + name + " of dark");
    }
    return problems;
}

} // namespace calcu::shell
