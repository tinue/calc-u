#include "roms/Bundle.hpp"
#include <fstream>
#include <cstddef>
#include <system_error>

namespace calcu::roms {

DirectoryBundle::DirectoryBundle(std::filesystem::path folder) : folder_(std::move(folder)) {}

std::optional<std::vector<std::uint8_t>> DirectoryBundle::read(std::string_view fileName) const {
    std::error_code ec;
    std::filesystem::directory_iterator it(folder_, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        if (it->path().filename().string() != fileName) continue;
        if (!it->is_regular_file(ec)) return std::nullopt;
        std::ifstream f(it->path(), std::ios::binary | std::ios::ate);
        if (!f) return std::nullopt;
        const std::streamoff size = f.tellg();
        if (size < 0) return std::nullopt;
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        f.seekg(0);
        if (size > 0 && !f.read(reinterpret_cast<char*>(bytes.data()), size)) return std::nullopt;
        return bytes;
    }
    return std::nullopt;
}

std::string DirectoryBundle::where() const { return folder_.string(); }

} // namespace calcu::roms
