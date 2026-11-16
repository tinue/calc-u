#include "WebPageData.hpp"
#include "DebugText.hpp"
#include "KeyboardLayout.hpp"
#include "LedGeometry.hpp"
#include "MachineModelInfo.hpp"
#include "RomHeatmap.hpp"
#include "SevenSegment.hpp"

namespace cu59 {

using calcu::sessions::Value;

namespace {
Value rect(double x, double y, double w, double h) { return Value(Value::Array{Value(x), Value(y), Value(w), Value(h)}); }
} // namespace

Value webStaticData(const std::vector<ModuleMetadata>& moduleList) {
    Value::Array models;
    for (int raw = 0; raw <= 2; ++raw) {
        const auto& i = info(static_cast<Model>(raw));
        models.push_back(Value::object({{"model", raw}, {"name", std::string(i.displayName)}, {"hasCardReader", i.hasCardReader}}));
    }
    Value::Array modules;
    for (const auto& m : moduleList) modules.push_back(Value::object({{"id", m.id}, {"menuTitle", m.menuTitle}}));

    const auto& d = kDisplayRect;
    const auto& c = kCardRect;
    Value::Array corners;
    for (int side : {0, 1}) {
        const NRect r = cardCornerRect(side);
        corners.push_back(rect(r.x, r.y, r.w, r.h));
    }
    Value::Array keys;
    for (int row = 0; row < 9; ++row)
        for (int col = 0; col < 5; ++col) {
            const auto& k = kKeyRects[row][col];
            keys.push_back(rect(k.x, kKeyboardYStart + k.y * kKeyboardYScale, k.w, k.h * kKeyboardYScale));
        }
    Value layout = Value::object({{"display", rect(d.x, d.y, d.w, d.h)},
                                  {"card", rect(c.x, c.y, c.w, c.h)},
                                  {"cardCorners", Value(std::move(corners))},
                                  {"keys", Value(std::move(keys))}});

    Value::Array colors;   // [r, g, b] per heat step
    for (int i = 0; i < 6; ++i)
        colors.push_back(Value(Value::Array{Value(kHeatColors[i].r), Value(kHeatColors[i].g), Value(kHeatColors[i].b)}));
    Value heat = Value::object({{"cols", kHeatmapCols}, {"rows", kHeatmapRows}, {"colors", Value(std::move(colors))}});

    return Value::object({{"models", Value(std::move(models))},
                          {"modules", Value(std::move(modules))},
                          {"layout", std::move(layout)},
                          {"heatmap", std::move(heat)}});
}

Value webLedGeometry(double w, double h) {
    const LedStyleParams& p = ledStyleParams(LedStyle::Classic);
    Value::Array segments;
    for (const auto& seg : classicSegments(w, h)) {
        Value::Array centres;
        for (const auto& c : seg.centres) centres.push_back(Value(Value::Array{Value(c.x), Value(c.y)}));
        segments.push_back(Value::object({{"centres", Value(std::move(centres))}, {"radius", seg.radius}}));
    }
    const LedDot dot = decimalPoint(w, h);
    auto rgb = [](double r, double g, double b) { return Value(Value::Array{Value(r * 255), Value(g * 255), Value(b * 255)}); };
    return Value::object({{"segments", Value(std::move(segments))},
                          {"dp", Value::object({{"cx", dot.cx}, {"cy", dot.cy}, {"diameter", dot.diameter}})},
                          {"slant", p.slant},
                          {"active", rgb(p.activeR, p.activeG, p.activeB)},
                          {"inactive", rgb(p.inactiveR, p.inactiveG, p.inactiveB)},
                          {"inactiveAlpha", p.inactiveAlpha},
                          {"glowBlurPerRadius", p.glowBlurPerWidth},
                          {"glowOpacity", p.glowOpacity},
                          {"dpGlowBlurPerDiameter", kDecimalGlowBlurPerDiameter},
                          {"dpGlowOpacity", kDecimalGlowOpacity},
                          {"background", rgb(kLedBackgroundR, kLedBackgroundG, kLedBackgroundB)}});
}

// What the PWA shows in a window of this size (layout="auto"), as the iOS
// app does. A browser can't see a fold, so a half-open foldable counts as
// its whole window.
Value webMobileRules(double w, double h) {
    const auto layout = mobileLayout(w, h, false);
    const auto printer = mobilePrinterWidth(layout, w);
    return Value::object({{"paged", layout == MobileLayout::Paged},
                          {"debugger", layout == MobileLayout::CalcPrinterDebugger},
                          {"printerMin", printer.min},
                          {"printerMax", printer.max},
                          {"compactPrinterBar", compactPrinterBar(printer.max)},
                          {"barLabels", mobileBarLabels(w)}});
}

} // namespace cu59
