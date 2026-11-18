// The library's PNG writer: RGB, band by band,
// with pHYs. Read back by a small inflate of the one block kind it writes.
#include "TestHarness.hpp"
#include "formats/Png.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using calcu::formats::PngWriter;

namespace {

std::uint32_t be32(const std::vector<std::uint8_t>& b, std::size_t at) {
    return (std::uint32_t(b[at]) << 24) | (std::uint32_t(b[at + 1]) << 16) | (std::uint32_t(b[at + 2]) << 8) |
           std::uint32_t(b[at + 3]);
}

// Inflates a zlib stream of fixed-Huffman blocks (RFC 1951 §3.2.6).
std::optional<std::vector<std::uint8_t>> inflateFixed(const std::vector<std::uint8_t>& z) {
    std::size_t bit = 16;   // after the two header bytes
    auto get = [&](int n) {
        std::uint32_t v = 0;
        for (int i = 0; i < n; ++i, ++bit) v |= std::uint32_t((z[bit / 8] >> (bit % 8)) & 1u) << i;
        return v;
    };
    auto code = [&](int n) {
        std::uint32_t v = 0;
        for (int i = 0; i < n; ++i, ++bit) v = (v << 1) | ((z[bit / 8] >> (bit % 8)) & 1u);
        return v;
    };
    static const int lenBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const int lenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const int distBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                                     1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static const int distExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    std::vector<std::uint8_t> out;
    for (bool last = false; !last;) {
        last = get(1) != 0;
        if (get(2) != 1) return std::nullopt;
        for (;;) {
            std::uint32_t c = code(7);
            int sym;
            if (c <= 0x17) sym = int(c) + 256;
            else {
                c = (c << 1) | code(1);
                if (c >= 0x30 && c <= 0xBF) sym = int(c) - 0x30;
                else if (c >= 0xC0 && c <= 0xC7) sym = int(c) - 0xC0 + 280;
                else sym = int((c << 1) | code(1)) - 0x190 + 144;
            }
            if (sym < 256) out.push_back(std::uint8_t(sym));
            else if (sym == 256) break;
            else {
                const int li = sym - 257;
                const int len = lenBase[li] + int(get(lenExtra[li]));
                const int di = int(code(5));
                const int dist = distBase[di] + int(get(distExtra[di]));
                if (dist > int(out.size())) return std::nullopt;
                for (int k = 0; k < len; ++k) out.push_back(out[out.size() - std::size_t(dist)]);
            }
        }
    }
    return out;
}

} // namespace

TEST(png_rgb_bands_and_phys) {
    const int w = 5, h = 300;
    std::vector<std::uint8_t> pixels(std::size_t(w * h * 3));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            for (int k = 0; k < 3; ++k)
                pixels[std::size_t((y * w + x) * 3 + k)] = std::uint8_t(y < 100 ? 255 : (x * 40 + y + k * 7) & 0xFF);
    PngWriter png(w, h, calcu::formats::pixelsPerMetre(300));
    png.addRows(pixels.data(), 256);   // two bands
    png.addRows(pixels.data() + 256 * w * 3, h - 256);
    const std::vector<std::uint8_t> file = png.finish();
    CHECK(file.size() > 33);
    if (file.size() <= 33) return;
    CHECK_EQ(std::string(file.begin() + 12, file.begin() + 16), std::string("IHDR"));
    CHECK_EQ(be32(file, 16), std::uint32_t(w));
    CHECK_EQ(be32(file, 20), std::uint32_t(h));
    CHECK_EQ(int(file[24]), 8);   // 8 bits
    CHECK_EQ(int(file[25]), 2);   // RGB
    // pHYs: 300 dpi = 11811 pixels a metre, both axes.
    std::size_t at = 8;
    std::vector<std::uint8_t> idat;
    bool phys = false;
    while (at + 12 <= file.size()) {
        const std::uint32_t len = be32(file, at);
        const std::string type(file.begin() + std::ptrdiff_t(at + 4), file.begin() + std::ptrdiff_t(at + 8));
        if (type == "pHYs") {
            phys = true;
            CHECK_EQ(be32(file, at + 8), std::uint32_t(11811));
            CHECK_EQ(be32(file, at + 12), std::uint32_t(11811));
            CHECK_EQ(int(file[at + 16]), 1);
        }
        if (type == "IDAT") idat.insert(idat.end(), file.begin() + std::ptrdiff_t(at + 8), file.begin() + std::ptrdiff_t(at + 8 + len));
        at += 12 + len;
    }
    CHECK(phys);
    const auto raw = inflateFixed(idat);
    CHECK(raw.has_value());
    if (!raw) return;
    CHECK_EQ(raw->size(), std::size_t((w * 3 + 1) * h));
    bool same = raw->size() == std::size_t((w * 3 + 1) * h);
    for (int y = 0; same && y < h; ++y) {
        same = (*raw)[std::size_t(y * (w * 3 + 1))] == 0;   // filter None
        for (int i = 0; same && i < w * 3; ++i)
            same = (*raw)[std::size_t(y * (w * 3 + 1) + 1 + i)] == pixels[std::size_t(y * w * 3 + i)];
    }
    CHECK(same);
    // Rows not added are white; the same pixels give the same bytes.
    PngWriter blank(w, 2, 0), blank2(w, 2, 0);
    std::vector<std::uint8_t> white(std::size_t(w * 3), 255);
    blank2.addRows(white.data(), 1);
    CHECK(blank.finish() == blank2.finish());
}
