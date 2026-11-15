#include "formats/Png.hpp"
#include <array>
#include <cmath>

namespace calcu::formats {

namespace {

std::uint32_t crc32(const std::uint8_t* data, std::size_t len, std::uint32_t crc = 0) {
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[n] = c;
        }
        return t;
    }();
    crc = ~crc;
    for (std::size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void putBE32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(v >> shift));
}

void putChunk(std::vector<std::uint8_t>& out, const char type[4], const std::vector<std::uint8_t>& payload) {
    putBE32(out, static_cast<std::uint32_t>(payload.size()));
    const std::size_t typeAt = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    putBE32(out, crc32(out.data() + typeAt, payload.size() + 4));
}

constexpr int kLenBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                              35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr int kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr int kDistBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                               193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
                               6145, 8193, 12289, 16385, 24577};
constexpr int kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

} // namespace

std::uint32_t pixelsPerMetre(double dotsPerInch) {
    return static_cast<std::uint32_t>(std::lround(dotsPerInch / 0.0254));
}

PngWriter::PngWriter(int width, int height, std::uint32_t ppm) : width_(width), height_(height), ppm_(ppm) {
    zlib_ = {0x78, 0x01};
    bits(1, 1);   // BFINAL
    bits(1, 2);   // BTYPE 01: fixed Huffman
}

void PngWriter::bits(std::uint32_t value, int count) {
    for (int i = 0; i < count; ++i) {
        bitBuffer_ |= ((value >> i) & 1u) << bitCount_;
        if (++bitCount_ == 8) {
            zlib_.push_back(static_cast<std::uint8_t>(bitBuffer_));
            bitBuffer_ = 0;
            bitCount_ = 0;
        }
    }
}

void PngWriter::code(std::uint32_t value, int count) {
    for (int i = count - 1; i >= 0; --i) bits((value >> i) & 1u, 1);
}

void PngWriter::literal(int sym) {
    if (sym <= 143) code(static_cast<std::uint32_t>(0x30 + sym), 8);
    else if (sym <= 255) code(static_cast<std::uint32_t>(0x190 + (sym - 144)), 9);
    else if (sym <= 279) code(static_cast<std::uint32_t>(sym - 256), 7);
    else code(static_cast<std::uint32_t>(0xC0 + (sym - 280)), 8);
}

void PngWriter::match(int length, int distance) {
    int li = 28;
    while (kLenBase[li] > length) --li;
    literal(257 + li);
    bits(static_cast<std::uint32_t>(length - kLenBase[li]), kLenExtra[li]);
    int di = 29;
    while (kDistBase[di] > distance) --di;
    code(static_cast<std::uint32_t>(di), 5);
    bits(static_cast<std::uint32_t>(distance - kDistBase[di]), kDistExtra[di]);
}

// One scanline: its filter byte (None) and its pixels, matched against the
// bytes before it in the line (distance 1) and the line above.
void PngWriter::scanline(const std::uint8_t* rgb) {
    const std::size_t stride = static_cast<std::size_t>(width_) * 3 + 1;
    current_.assign(1, 0);
    current_.insert(current_.end(), rgb, rgb + stride - 1);
    for (std::uint8_t b : current_) {
        adlerA_ = (adlerA_ + b) % 65521u;
        adlerB_ = (adlerB_ + adlerA_) % 65521u;
    }
    const bool above = previous_.size() == stride && stride <= 32768;
    std::size_t i = 0;
    while (i < stride) {
        int bestLen = 0, bestDist = 0;
        if (i >= 1) {
            int len = 0;
            while (len < 258 && i + std::size_t(len) < stride && current_[i + std::size_t(len)] == current_[i + std::size_t(len) - 1])
                ++len;
            bestLen = len;
            bestDist = 1;
        }
        if (above) {
            int len = 0;
            while (len < 258 && i + std::size_t(len) < stride && current_[i + std::size_t(len)] == previous_[i + std::size_t(len)])
                ++len;
            if (len > bestLen) {
                bestLen = len;
                bestDist = static_cast<int>(stride);
            }
        }
        if (bestLen >= 3) {
            match(bestLen, bestDist);
            i += std::size_t(bestLen);
        } else {
            literal(current_[i]);
            ++i;
        }
    }
    previous_.swap(current_);
    ++rowsAdded_;
}

void PngWriter::addRows(const std::uint8_t* rgb, int rows) {
    const std::size_t row = static_cast<std::size_t>(width_) * 3;
    for (int r = 0; r < rows && rowsAdded_ < height_; ++r) scanline(rgb + row * std::size_t(r));
}

std::vector<std::uint8_t> PngWriter::finish() {
    if (rowsAdded_ < height_) {
        const std::vector<std::uint8_t> white(static_cast<std::size_t>(width_) * 3, 255);
        while (rowsAdded_ < height_) scanline(white.data());
    }
    literal(256);   // end of block
    if (bitCount_ > 0) {
        zlib_.push_back(static_cast<std::uint8_t>(bitBuffer_));
        bitBuffer_ = 0;
        bitCount_ = 0;
    }
    putBE32(zlib_, (adlerB_ << 16) | adlerA_);

    std::vector<std::uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<std::uint8_t> ihdr;
    putBE32(ihdr, static_cast<std::uint32_t>(width_));
    putBE32(ihdr, static_cast<std::uint32_t>(height_));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});   // 8 bits, RGB, deflate, filter 0, no interlace
    putChunk(out, "IHDR", ihdr);
    if (ppm_ > 0) {
        std::vector<std::uint8_t> phys;
        putBE32(phys, ppm_);
        putBE32(phys, ppm_);
        phys.push_back(1);   // the metre
        putChunk(out, "pHYs", phys);
    }
    putChunk(out, "IDAT", zlib_);
    putChunk(out, "IEND", {});
    zlib_.clear();
    return out;
}

} // namespace calcu::formats
