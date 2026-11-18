// Compiled with -ffp-contract=off (library/CMakeLists.txt; MSVC: /fp:precise,
// its default), so that the same marks give the same pixels on every
// platform.
#include "paper/Picture.hpp"
#include "formats/Png.hpp"
#include <algorithm>
#include <cmath>

namespace calcu::paper {

namespace {

constexpr int kBand = 256;

Coord halfSize(const PaperDescription& d, const Mark& m) {
    return std::holds_alternative<Dot>(m) ? d.dotDiameter / 2 : d.strokeWidth / 2;
}

double pixelsPerCoord(const PaperDescription& d, double dpi) {
    return dpi * d.unit.value() / 25.4 / static_cast<double>(kPerUnit);
}

int sideAt(Coord length, const PaperDescription& d, double dpi, bool roundDown) {
    const double px = static_cast<double>(length) * pixelsPerCoord(d, dpi);
    return std::max(1, static_cast<int>(roundDown ? std::floor(px) : std::llround(px)));
}

// Pixel space: x, y of the picture, 0 at its top left.
struct Pt {
    double x, y;
};

double distanceToSegment(Pt p, Pt a, Pt b) {
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    double t = 0;
    if (len2 > 0) t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0, 1.0);
    const double ex = p.x - (a.x + t * dx), ey = p.y - (a.y + t * dy);
    return std::sqrt(ex * ex + ey * ey);
}

double coverage(double reach, double distance) { return std::clamp(reach + 0.5 - distance, 0.0, 1.0); }

std::uint8_t level(double v) { return static_cast<std::uint8_t>(std::clamp<long>(std::lround(v), 0, 255)); }

struct Box {
    int x0, y0, x1, y1;   // inclusive
};

} // namespace

PictureSize pictureSize(const PaperDescription& d, const std::vector<Mark>& marks, Reach reach, bool empty) {
    PictureSize s;
    if (empty || d.width <= 0 || d.pictureDotsPerInch <= 0) return s;
    s.top = reach.from;
    s.bottom = reach.to;
    for (const Mark& m : marks) {
        const Coord h = halfSize(d, m);
        if (const auto* dot = std::get_if<Dot>(&m)) {
            s.top = std::min(s.top, dot->at.y - h);
            s.bottom = std::max(s.bottom, dot->at.y + h);
        } else {
            for (const Position& p : std::get<Stroke>(m).path) {
                s.top = std::min(s.top, p.y - h);
                s.bottom = std::max(s.bottom, p.y + h);
            }
        }
    }
    const Coord length = std::max<Coord>(s.bottom - s.top, 1);
    double dpi = d.pictureDotsPerInch;
    s.width = sideAt(d.width, d, dpi, false);
    s.height = sideAt(length, d, dpi, false);
    const auto over = [](int w, int h) {
        return w > kMaxPictureSide || h > kMaxPictureSide ||
               static_cast<std::int64_t>(w) * static_cast<std::int64_t>(h) > kMaxPicturePixels;
    };
    if (over(s.width, s.height)) {   // the highest resolution at which neither cap is passed
        const double perDpi = pixelsPerCoord(d, 1.0);   // pixels per Coord at 1 dpi
        const double w1 = static_cast<double>(d.width) * perDpi, h1 = static_cast<double>(length) * perDpi;
        dpi = std::min({dpi, kMaxPictureSide / std::max(w1, h1),
                        std::sqrt(static_cast<double>(kMaxPicturePixels) / (w1 * h1))});
        for (;;) {
            s.width = sideAt(d.width, d, dpi, true);
            s.height = sideAt(length, d, dpi, true);
            if (!over(s.width, s.height)) break;
            dpi *= 0.999999;
        }
    }
    s.dotsPerInch = dpi;
    return s;
}

PictureSize pictureSize(const Paper& p) { return pictureSize(p.description(), p.marks(), p.reach(), p.empty()); }

std::vector<std::uint8_t> picturePng(const PaperDescription& d, const std::vector<Mark>& marks, Reach reach, bool empty) {
    const PictureSize size = pictureSize(d, marks, reach, empty);
    if (size.width == 0) return {};
    const double scale = pixelsPerCoord(d, size.dotsPerInch);
    const auto px = [&](Position p) {
        return Pt{static_cast<double>(p.x) * scale, static_cast<double>(p.y - size.top) * scale};
    };
    const int W = size.width, H = size.height;
    const int bands = (H + kBand - 1) / kBand;

    // Each mark's box in pixels, and the bands it touches, in the order made.
    std::vector<Box> boxes(marks.size());
    std::vector<std::vector<std::size_t>> inBand(static_cast<std::size_t>(bands));
    for (std::size_t i = 0; i < marks.size(); ++i) {
        const double h = static_cast<double>(halfSize(d, marks[i])) * scale + 1.0;
        double x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300;
        auto add = [&](Position p) {
            const Pt q = px(p);
            x0 = std::min(x0, q.x - h);
            y0 = std::min(y0, q.y - h);
            x1 = std::max(x1, q.x + h);
            y1 = std::max(y1, q.y + h);
        };
        if (const auto* dot = std::get_if<Dot>(&marks[i])) add(dot->at);
        else
            for (const Position& p : std::get<Stroke>(marks[i]).path) add(p);
        Box b{std::max(0, static_cast<int>(std::floor(x0))), std::max(0, static_cast<int>(std::floor(y0))),
              std::min(W - 1, static_cast<int>(std::floor(x1))), std::min(H - 1, static_cast<int>(std::floor(y1)))};
        boxes[i] = b;
        if (b.x0 > b.x1 || b.y0 > b.y1) continue;
        for (int band = b.y0 / kBand; band <= b.y1 / kBand; ++band) inBand[std::size_t(band)].push_back(i);
    }

    formats::PngWriter png(W, H, formats::pixelsPerMetre(size.dotsPerInch));
    std::vector<std::uint8_t> rgb;
    std::vector<double> alpha;
    for (int band = 0; band < bands; ++band) {
        const int top = band * kBand, rows = std::min(kBand, H - top);
        rgb.assign(static_cast<std::size_t>(W) * std::size_t(rows) * 3, 255);
        alpha.assign(static_cast<std::size_t>(W) * std::size_t(rows), 0.0);
        auto at = [&](int x, int y) { return (static_cast<std::size_t>(y - top) * std::size_t(W) + std::size_t(x)); };
        for (std::size_t i : inBand[std::size_t(band)]) {
            const Box b = boxes[i];
            const int y0 = std::max(b.y0, top), y1 = std::min(b.y1, top + rows - 1);
            if (const auto* dot = std::get_if<Dot>(&marks[i])) {
                // A disc of the dot's diameter in its grey; where dots overlap, the darker wins.
                const std::uint8_t grey = d.darknessGreys.empty()
                                              ? 0
                                              : d.darknessGreys[std::min<std::size_t>(dot->darkness, d.darknessGreys.size() - 1)];
                const Pt c = px(dot->at);
                const double r = static_cast<double>(d.dotDiameter) / 2.0 * scale;
                for (int y = y0; y <= y1; ++y)
                    for (int x = b.x0; x <= b.x1; ++x) {
                        const double dx = x + 0.5 - c.x, dy = y + 0.5 - c.y;
                        const double cov = coverage(r, std::sqrt(dx * dx + dy * dy));
                        if (cov <= 0) continue;
                        const std::uint8_t v = level(255.0 - cov * (255.0 - grey));
                        std::uint8_t* p = &rgb[at(x, y) * 3];
                        for (int k = 0; k < 3; ++k) p[k] = std::min(p[k], v);
                    }
                continue;
            }
            // A polyline of the stroke width with round ends and joins, in
            // its colour, over what is there; one coverage per pixel for the
            // whole stroke.
            const Stroke& s = std::get<Stroke>(marks[i]);
            const double hw = static_cast<double>(d.strokeWidth) / 2.0 * scale;
            for (int y = y0; y <= y1; ++y)
                for (int x = b.x0; x <= b.x1; ++x) alpha[at(x, y)] = 0.0;
            const std::size_t n = s.path.size();
            for (std::size_t k = 0; k < n; ++k) {
                const Pt a = px(s.path[k]), e = px(s.path[k + 1 < n ? k + 1 : k]);
                const int sx0 = std::max(b.x0, static_cast<int>(std::floor(std::min(a.x, e.x) - hw - 1)));
                const int sx1 = std::min(b.x1, static_cast<int>(std::floor(std::max(a.x, e.x) + hw + 1)));
                const int sy0 = std::max(y0, static_cast<int>(std::floor(std::min(a.y, e.y) - hw - 1)));
                const int sy1 = std::min(y1, static_cast<int>(std::floor(std::max(a.y, e.y) + hw + 1)));
                for (int y = sy0; y <= sy1; ++y)
                    for (int x = sx0; x <= sx1; ++x) {
                        const double cov = coverage(hw, distanceToSegment(Pt{x + 0.5, y + 0.5}, a, e));
                        double& al = alpha[at(x, y)];
                        al = std::max(al, cov);
                    }
                if (n > 1 && k + 2 >= n) break;   // the last segment was k → n-1
            }
            const double col[3] = {double(s.colour.r), double(s.colour.g), double(s.colour.b)};
            for (int y = y0; y <= y1; ++y)
                for (int x = b.x0; x <= b.x1; ++x) {
                    const double al = alpha[at(x, y)];
                    if (al <= 0) continue;
                    std::uint8_t* p = &rgb[at(x, y) * 3];
                    for (int k = 0; k < 3; ++k) p[k] = level(p[k] * (1.0 - al) + col[k] * al);
                }
        }
        png.addRows(rgb.data(), rows);
    }
    return png.finish();
}

std::vector<std::uint8_t> picturePng(const Paper& p) {
    return picturePng(p.description(), p.marks(), p.reach(), p.empty());
}

} // namespace calcu::paper
