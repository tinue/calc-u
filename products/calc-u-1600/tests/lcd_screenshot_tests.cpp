// Headless C++ tests for core/Display/LcdScreenshot, written as a PNG by
// the library's formats::PngWriter as the `screenshot` step does
// (app/SharpPresetRules). The PNG's IDAT is decoded with the system zlib
// (test-only dependency) to prove the deflate round-trips the exact pixels.
//
// Build & run: see tools/run_tests.sh

#include <zlib.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../core/Display/LcdScreenshot.hpp"
#include "TestHarness.hpp"
#include "formats/Png.hpp"

namespace {

uint32_t be32(const std::vector<uint8_t>& b, std::size_t at) {
    return (uint32_t(b[at]) << 24) | (uint32_t(b[at + 1]) << 16) | (uint32_t(b[at + 2]) << 8) | b[at + 3];
}

struct DecodedPng {
    bool ok = false;
    uint32_t width = 0, height = 0, ppmX = 0, ppmY = 0;
    std::vector<uint8_t> pixels;
};

// Walks the chunks (checking every CRC), then inflates IDAT with zlib and
// strips the per-row filter bytes (all must be 0 = None). RGB, 8 bits a
// channel; `pixels` is the grey of each pixel (R = G = B, else not ok).
DecodedPng decodePng(const std::vector<uint8_t>& png) {
    DecodedPng out;
    static const uint8_t kSig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (png.size() < 8 || !std::equal(kSig, kSig + 8, png.begin())) return out;
    std::vector<uint8_t> idat;
    std::size_t at = 8;
    bool sawEnd = false;
    while (at + 12 <= png.size()) {
        const uint32_t len = be32(png, at);
        if (at + 12 + len > png.size()) return out;
        const std::string type(png.begin() + at + 4, png.begin() + at + 8);
        if (::crc32(0, png.data() + at + 4, len + 4) != be32(png, at + 8 + len)) return out;
        const std::size_t data = at + 8;
        if (type == "IHDR") {
            out.width = be32(png, data);
            out.height = be32(png, data + 4);
            if (png[data + 8] != 8 || png[data + 9] != 2) return out;
        } else if (type == "pHYs") {
            out.ppmX = be32(png, data);
            out.ppmY = be32(png, data + 4);
        } else if (type == "IDAT") {
            idat.insert(idat.end(), png.begin() + data, png.begin() + data + len);
        } else if (type == "IEND") {
            sawEnd = true;
        }
        at += 12 + len;
    }
    if (!sawEnd) return out;
    const std::size_t stride = static_cast<std::size_t>(out.width) * 3 + 1;
    std::vector<uint8_t> raw(stride * out.height);
    uLongf rawLen = raw.size();
    if (uncompress(raw.data(), &rawLen, idat.data(), idat.size()) != Z_OK || rawLen != raw.size()) return out;
    for (uint32_t y = 0; y < out.height; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * stride;
        if (raw[row] != 0) return out;
        for (uint32_t x = 0; x < out.width; ++x) {
            const uint8_t* rgb = &raw[row + 1 + std::size_t(x) * 3];
            if (rgb[1] != rgb[0] || rgb[2] != rgb[0]) return out;
            out.pixels.push_back(rgb[0]);
        }
    }
    out.ok = true;
    return out;
}

// A grey image as the `screenshot` step writes it (app/SharpPresetRules).
std::vector<uint8_t> encodeGray(const std::vector<uint8_t>& grey, int width, int height, uint32_t ppm) {
    calcu::formats::PngWriter png(width, height, ppm);
    std::vector<uint8_t> row(static_cast<std::size_t>(width) * 3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x)
            row[std::size_t(x) * 3] = row[std::size_t(x) * 3 + 1] = row[std::size_t(x) * 3 + 2] =
                grey[std::size_t(y) * width + x];
        png.addRows(row.data(), 1);
    }
    return png.finish();
}

LcdBitmap checkerboard(int cols, int rows) {
    LcdBitmap b;
    b.cols = cols;
    b.rows = rows;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) b.pixels.push_back(((r + c) & 1) == 0);
    return b;
}

TEST(test_render_size_per_model) {
    const GrayImage p16 = renderLcdImage(checkerboard(156, 32), kPC1600ScreenMm);
    CHECK(p16.width == 2102 && p16.height == 425); // 89 x 18 mm at 600 DPI
    const GrayImage p15 = renderLcdImage(checkerboard(156, 7), kPC1500ScreenMm);
    CHECK(p15.width == 2457 && p15.height == 118); // 104 x 5 mm at 600 DPI
    CHECK(p16.pixelsPerMeter() == 23622);
}

TEST(test_render_off_is_white_and_dot_layout) {
    LcdBitmap off = checkerboard(156, 32);
    off.poweredOn = false;
    const GrayImage blank = renderLcdImage(off, kPC1600ScreenMm);
    bool allWhite = true;
    for (uint8_t v : blank.pixels) allWhite = allWhite && v == 255;
    CHECK(allWhite);

    // One lit dot at (col 10, row 5): black at its cell centre, white
    // between it and the unlit neighbour cell, white in that neighbour.
    LcdBitmap one;
    one.cols = 156;
    one.rows = 32;
    one.pixels.assign(156 * 32, false);
    one.pixels[5 * 156 + 10] = true;
    const GrayImage img = renderLcdImage(one, kPC1600ScreenMm);
    const double cw = double(img.width) / 156, ch = double(img.height) / 32;
    auto at = [&](double x, double y) { return img.pixels[std::size_t(int(y)) * img.width + std::size_t(int(x))]; };
    CHECK(at(10.5 * cw, 5.5 * ch) == 0);
    CHECK(at(11.0 * cw, 5.5 * ch) == 255); // cell boundary lies in the 10% gap
    CHECK(at(11.5 * cw, 5.5 * ch) == 255);
}

TEST(test_png_roundtrip) {
    const GrayImage img = renderLcdImage(checkerboard(156, 32), kPC1600ScreenMm);
    const std::vector<uint8_t> png = encodeGray(img.pixels, img.width, img.height, img.pixelsPerMeter());
    const DecodedPng d = decodePng(png);
    CHECK(d.ok);
    CHECK(d.width == 2102 && d.height == 425);
    CHECK(d.ppmX == 23622 && d.ppmY == 23622);
    CHECK(d.pixels == img.pixels);
    CHECK(png.size() < 200 * 1024); // run matches keep it far below the ~2.7 MB raw size

    // Odd sizes and noise-like data (long literal stretches, short runs).
    std::vector<uint8_t> noise(37 * 5);
    uint32_t x = 12345;
    for (auto& v : noise) { x = x * 1103515245u + 12345u; v = static_cast<uint8_t>(x >> 16); }
    const DecodedPng dn = decodePng(encodeGray(noise, 37, 5, 0));
    CHECK(dn.ok && dn.pixels == noise && dn.ppmX == 0);
}

} // namespace
