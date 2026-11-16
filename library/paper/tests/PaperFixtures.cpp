#include "PaperFixtures.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace calcu::paper::test {

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
