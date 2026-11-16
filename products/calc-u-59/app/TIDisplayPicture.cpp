#include "TIDisplayPicture.hpp"
#include "LedGeometry.hpp"
#include "SevenSegment.hpp"
#include "formats/Png.hpp"
#include <algorithm>
#include <cmath>
#include <variant>

namespace cu59 {

namespace {

constexpr int kCellW = 24, kCellH = 40, kCells = 12, kMargin = 8;

struct Canvas {
    int w, h;
    std::vector<std::uint8_t> rgb;
    Canvas(int width, int height) : w(width), h(height), rgb(static_cast<std::size_t>(width * height * 3)) {
        const auto r = static_cast<std::uint8_t>(std::lround(kLedBackgroundR * 255));
        const auto g = static_cast<std::uint8_t>(std::lround(kLedBackgroundG * 255));
        const auto b = static_cast<std::uint8_t>(std::lround(kLedBackgroundB * 255));
        for (std::size_t i = 0; i < rgb.size(); i += 3) rgb[i] = r, rgb[i + 1] = g, rgb[i + 2] = b;
    }
    // A filled disc, a pixel lit when its centre lies inside.
    void disc(double cx, double cy, double radius, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius))), x1 = std::min(w - 1, static_cast<int>(std::ceil(cx + radius)));
        const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius))), y1 = std::min(h - 1, static_cast<int>(std::ceil(cy + radius)));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x) {
                const double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
                if (dx * dx + dy * dy > radius * radius) continue;
                std::uint8_t* p = &rgb[static_cast<std::size_t>((y * w + x) * 3)];
                p[0] = r, p[1] = g, p[2] = b;
            }
    }
};

} // namespace

std::vector<std::uint8_t> displayPicture(const calcu::machine::DisplayOutput& o) {
    Canvas c(kCells * kCellW + 2 * kMargin, kCellH + 2 * kMargin);
    const LedStyleParams& style = ledStyleParams(LedStyle::Classic);
    const auto ar = static_cast<std::uint8_t>(std::lround(style.activeR * 255));
    const auto ag = static_cast<std::uint8_t>(std::lround(style.activeG * 255));
    const auto ab = static_cast<std::uint8_t>(std::lround(style.activeB * 255));
    const std::vector<DotSegment> segments = classicSegments(kCellW, kCellH);
    const LedDot dp = decimalPoint(kCellW, kCellH);
    const auto* frame = std::get_if<calcu::machine::SegmentFrame>(&o.frame);
    double calc = 0;
    for (const auto& i : o.indicators)
        if (i.on) calc = std::max(calc, static_cast<double>(i.brightness.value_or(1.0f)));
    for (int i = 0; i < kCells; ++i) {
        std::uint8_t glyph = 0, level = 0;
        bool point = false;
        if (frame && static_cast<std::size_t>(i) < frame->positions.size()) {
            const auto& p = frame->positions[static_cast<std::size_t>(i)];
            for (std::size_t s = 0; s < 7 && s < p.lit.size(); ++s)
                if (p.lit[s]) glyph = static_cast<std::uint8_t>(glyph | (1u << s));
            level = static_cast<std::uint8_t>(std::lround(std::clamp(static_cast<double>(p.brightness.value_or(1.0f)), 0.0, 1.0) * 255));
            point = p.lit.size() > 7 && p.lit[7] && level > 0;
        }
        const LedCell cell = ledCell(i, glyph, level, calc);
        const double ox = kMargin + (kCells - 1 - i) * kCellW, oy = kMargin;
        for (int s = 0; s < 7; ++s) {
            if (!((cell.segments >> s) & 1u) || cell.opacity <= 0) continue;
            const DotSegment& seg = segments[static_cast<std::size_t>(s)];
            for (const LedPoint& d : seg.centres) {
                // The classic shear: x' = x + slant × (h − y), bottom fixed.
                const double x = d.x + style.slant * (kCellH - d.y);
                c.disc(ox + x, oy + d.y, seg.radius, ar, ag, ab);
            }
        }
        if (point) c.disc(ox + dp.cx, oy + dp.cy, dp.diameter / 2, ar, ag, ab);
    }
    calcu::formats::PngWriter png(c.w, c.h, 0);
    png.addRows(c.rgb.data(), c.h);
    return png.finish();
}

} // namespace cu59
