#pragma once
#include <cstdint>
#include <vector>

// The library's one PNG writer (LIB-8 design §2, formats/Png): RGB, 8 bits a
// channel, written band by band so that a large picture never needs its
// whole buffer (D-7), with a pHYs chunk for its resolution. Deflate as in
// Calc-U-1600's core/Display/PngWriter.hpp: one fixed-Huffman block whose
// only matches are runs at distance 1 and at one scanline, each scanline
// with filter None; extended from grey to RGB. Dependency-free and
// deterministic: the same pixels give the same bytes everywhere.

namespace calcu::formats {

class PngWriter {
public:
    // pixelsPerMetre 0: no pHYs chunk.
    PngWriter(int width, int height, std::uint32_t pixelsPerMetre);
    // `rows` scanlines of width × 3 bytes (R, G, B), top down.
    void addRows(const std::uint8_t* rgb, int rows);
    // The file; rows not added are white.
    std::vector<std::uint8_t> finish();

    int width() const { return width_; }
    int height() const { return height_; }

private:
    void bits(std::uint32_t value, int count);
    void code(std::uint32_t value, int count);
    void literal(int symbol);
    void match(int length, int distance);
    void scanline(const std::uint8_t* rgb);

    int width_, height_, rowsAdded_ = 0;
    std::uint32_t ppm_;
    std::vector<std::uint8_t> zlib_;
    std::uint32_t bitBuffer_ = 0;
    int bitCount_ = 0;
    std::uint32_t adlerA_ = 1, adlerB_ = 0;
    std::vector<std::uint8_t> previous_, current_;   // raw scanlines with their filter byte
};

// The pixels per metre of a resolution in dots per inch, rounded.
std::uint32_t pixelsPerMetre(double dotsPerInch);

} // namespace calcu::formats
