// The picture of Copy and `paper png`, and the reference pictures that fix
// its pixels on every platform.
#include "PaperTestKit.hpp"
#include "TestHarness.hpp"
#include "formats/Png.hpp"
#include "paper/Picture.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace mt = calcu::machine::test;

namespace {

// A TD-20 paper: two lines and a feed, the second line's last row faint.
Paper td20Paper(FakePrinter& pr) {
    Paper p(mt::td20Description());
    p.bind(&pr);
    for (int line = 0; line < 2; ++line) {
        const Coord top = u(line * 11.0);
        pr.text(line ? "KEY 2" : "KEY 1", top);
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 8; ++c)
                if ((c + r + line) % 3 != 0)
                    pr.dot(u(2) + c * u(4.5) + u(0.5), top + u(r) + u(0.5), static_cast<std::uint8_t>(line == 1 && r == 4 ? 0 : 1));
        pr.feedTo(top + u(7));
        if (line == 0) {
            pr.text("", u(7));
            pr.feedTo(u(11));
        }
    }
    return p;
}

// A TP-4 paper: four strokes in the four colours, one a single position, one back above zero.
Paper tp4Paper(FakePrinter& pr) {
    Paper p(mt::tp4Description());
    p.bind(&pr);
    const Rgb pens[4] = {{0, 0, 0}, {0, 0, 255}, {0, 128, 0}, {255, 0, 0}};
    for (int k = 0; k < 4; ++k) {
        pr.down(pens[k], u(100 + 50 * k), u(0));
        if (k != 3)
            for (int s = 1; s < 4; ++s) pr.to(u(100 + 50 * k + 10 * s), u(5 * (s % 2) + k * 2));
        pr.up();
    }
    pr.feedTo(u(20));
    pr.down(pens[1], u(60), u(-6));
    pr.to(u(540), u(18));
    pr.up();
    return p;
}

void checkReference(const std::string& name, const std::vector<std::uint8_t>& png) {
    if (writePictures()) writeFixture(name, png);
    const bool same = png == fixtureBytes(name);
    CHECK(same);
    if (!same) std::fprintf(stderr, "  %s differs from the reference picture (CALCU_PAPER_PICTURES=1 writes it)\n", name.c_str());
}

Paper fromFixture(const PaperDescription& d, const std::string& name) {
    Paper p(d);
    FakePrinter pr(d);
    p.bind(&pr);
    const auto read = readMarksReply(fixtureLines(name));
    if (!read) return p;
    for (const Mark& m : read->marks) {
        if (const auto* dot = std::get_if<Dot>(&m)) {
            pr.dot(dot->at.x, dot->at.y, dot->darkness);
            continue;
        }
        const Stroke& s = std::get<Stroke>(m);
        pr.down(s.colour, s.path.front().x, s.path.front().y);
        for (std::size_t i = 1; i < s.path.size(); ++i) pr.to(s.path[i].x, s.path[i].y);
        pr.up();
    }
    pr.feedTo(read->reach.to);
    return p;
}

} // namespace

TEST(full_width_all_it_reaches_at_stated_resolution) {
    FakePrinter dp(mt::td20Description());
    Paper td = td20Paper(dp);
    const PictureSize s = pictureSize(td);
    // 40 units of 1/2 mm at 300 dpi: 236 pixels; from 0 to 18 units: 106.
    CHECK_EQ(s.width, 236);
    CHECK_EQ(s.height, 106);
    CHECK_EQ(s.dotsPerInch, 300.0);
    CHECK_EQ(s.top, Coord(0));
    CHECK_EQ(s.bottom, u(18));
    const auto info = pngInfo(picturePng(td));
    CHECK(info && info->width == 236 && info->height == 106 && info->colourType == 2);
    CHECK(info && info->pixelsPerMetre == calcu::formats::pixelsPerMetre(300));
    // TP-4: 600 units of 1/10 mm at 254 dpi: 600 pixels; from the stroke
    // above zero less half its width to the feed at 20.
    FakePrinter pp(mt::tp4Description());
    Paper tp = tp4Paper(pp);
    const PictureSize t = pictureSize(tp);
    CHECK_EQ(t.width, 600);
    CHECK_EQ(t.top, u(-7.5));
    CHECK_EQ(t.bottom, u(20));
    CHECK_EQ(t.height, 28);
    // An empty paper has no picture.
    Paper empty(mt::tp4Description());
    CHECK(picturePng(empty).empty());
    CHECK_EQ(pictureSize(empty).width, 0);
}

TEST(reference_pictures_of_the_test_printers) {
    FakePrinter dp(mt::td20Description());
    checkReference("td20.png", picturePng(td20Paper(dp)));
    FakePrinter pp(mt::tp4Description());
    checkReference("tp4.png", picturePng(tp4Paper(pp)));
}

TEST(long_paper_lowered_to_16000_pixels_never_cropped) {
    FakePrinter pr(mt::tp4Description());
    Paper p(mt::tp4Description());
    p.bind(&pr);
    pr.down(Rgb{}, u(100), u(0));
    pr.to(u(110), u(10));
    pr.up();
    pr.feedTo(u(20'000));   // 2 m: 20000 pixels at 254 dpi
    const PictureSize s = pictureSize(p);
    CHECK(s.height <= 16'000 && s.height >= 15'999);
    CHECK(s.width <= 16'000 && s.width >= 479);   // the full width, scaled with it
    CHECK(s.dotsPerInch < 254.0 && s.dotsPerInch > 203.0);
    CHECK(s.top <= p.reach().from && s.bottom >= p.reach().to);   // never cropped
}

TEST(wide_long_paper_lowered_to_64_megapixels) {
    // A TP-4 widened to 210 mm and drawn at 1200 dpi: 9921 pixels wide.
    PaperDescription wide = mt::tp4Description();
    wide.width = u(2'100);
    wide.printableWidth = u(2'000);
    wide.pictureDotsPerInch = 1200;
    FakePrinter pr(wide);
    Paper p(wide);
    p.bind(&pr);
    pr.down(Rgb{}, u(200), u(0));
    pr.to(u(300), u(10));
    pr.up();
    pr.feedTo(u(1'500));   // 150 mm: 9921 × 7087 pixels at 1200 dpi
    const PictureSize s = pictureSize(p);
    const std::int64_t area = std::int64_t(s.width) * s.height;
    CHECK(area <= kMaxPicturePixels);
    CHECK(area > kMaxPicturePixels - 2 * (s.width + s.height));   // the highest resolution that fits
    CHECK(s.width < 9921 && s.height <= 16'000);
    CHECK(s.dotsPerInch < 1200.0);
    CHECK(s.top <= 0 && s.bottom >= u(1'500));
}
