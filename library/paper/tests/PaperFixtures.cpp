#include "PaperFixtures.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace calcu::paper::test {

PaperDescription pc100cDescription() {
    PaperDescription d;
    d.unit = Millimetres{430, 1000};
    d.width = 1'290'700;   // 129.07 dot pitches (AC-24)
    d.printableLeft = 0;
    d.printableWidth = d.width;
    d.form = MarkForm::Dots;
    d.dotDiameter = 7'600;   // 0.38 of the pitch as radius
    for (int heat = 0; heat <= 20; ++heat) {
        const double alpha = heat >= 20 ? 1.0 : 0.05 + 0.95 * std::pow(heat / 20.0, 2.5);
        d.darknessGreys.push_back(static_cast<std::uint8_t>(std::lround(255.0 * (1.0 - alpha))));
    }
    d.pictureDotsPerInch = 600;
    d.textLayer = true;
    return d;
}

namespace {
PaperDescription plotter(Coord width, Coord left, Coord printable) {
    PaperDescription d;
    d.unit = Millimetres{190, 1920};
    d.width = width;
    d.printableLeft = left;
    d.printableWidth = printable;
    d.form = MarkForm::PenStrokes;
    d.strokeWidth = 35'649;   // one point, 25.4/72 mm
    d.pictureDotsPerInch = 1200;
    d.textLayer = false;
    return d;
}
} // namespace

PaperDescription ce150Description() { return plotter(5'658'947, 669'474, 4'320'000); }        // 56 mm, 42.75 drawn on
PaperDescription ce1600pDescription() { return plotter(21'221'053, 1'010'526, 19'200'000); }  // 210 mm, 190 drawn on

std::string fixturePath(const std::string& name) { return std::string(CALCU_PAPER_FIXTURES) + "/" + name; }

std::vector<std::string> fixtureLines(const std::string& name) {
    std::ifstream in(fixturePath(name));
    std::vector<std::string> out;
    for (std::string l; std::getline(in, l);) out.push_back(l);
    return out;
}

std::vector<std::uint8_t> fixtureBytes(const std::string& name) {
    std::ifstream in(fixturePath(name), std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool writePictures() {
    const char* e = std::getenv("CALCU_PAPER_PICTURES");
    return e && std::string(e) == "1";
}

void writeFixture(const std::string& name, const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(fixturePath(name), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::optional<PngInfo> pngInfo(const std::vector<std::uint8_t>& b) {
    static const std::uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (b.size() < 33 || !std::equal(sig, sig + 8, b.begin())) return std::nullopt;
    auto be32 = [&](std::size_t at) {
        return (std::uint32_t(b[at]) << 24) | (std::uint32_t(b[at + 1]) << 16) | (std::uint32_t(b[at + 2]) << 8) |
               std::uint32_t(b[at + 3]);
    };
    PngInfo info;
    std::size_t at = 8;
    while (at + 12 <= b.size()) {
        const std::uint32_t len = be32(at);
        const std::string type(b.begin() + std::ptrdiff_t(at + 4), b.begin() + std::ptrdiff_t(at + 8));
        if (type == "IHDR") {
            info.width = static_cast<int>(be32(at + 8));
            info.height = static_cast<int>(be32(at + 12));
            info.colourType = b[at + 17];
        } else if (type == "pHYs") {
            info.pixelsPerMetre = be32(at + 8);
        }
        at += 12 + len;
    }
    return info;
}

} // namespace calcu::paper::test
