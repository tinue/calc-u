#include "WebPageData.hpp"
#include "DebugText.hpp"
#include "KeyboardLayout.hpp"
#include "LedGeometry.hpp"
#include "MachineModelInfo.hpp"
#include "RomHeatmap.hpp"
#include "SevenSegment.hpp"

namespace cu59 {

using calcu::sessions::Value;

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
