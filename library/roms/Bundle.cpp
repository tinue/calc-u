#include "roms/Bundle.hpp"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <system_error>

namespace calcu::roms {

namespace {

// The file of exactly that name in the folder, or none.
std::optional<std::filesystem::directory_entry> entryNamed(const std::filesystem::path& folder,
                                                           std::string_view fileName) {
    std::error_code ec;
    std::filesystem::directory_iterator it(folder, ec), end;
    for (; !ec && it != end; it.increment(ec))
        if (it->path().filename().string() == fileName) return *it;
    return std::nullopt;
}

// The bytes, or none with the system's reason in `problem`.
std::optional<std::vector<std::uint8_t>> readEntry(const std::filesystem::directory_entry& e, std::string& problem) {
    std::error_code ec;
    if (!e.is_regular_file(ec)) {
        problem = ec ? ec.message() : "not a file";
        return std::nullopt;
    }
    errno = 0;
    std::ifstream f(e.path(), std::ios::binary | std::ios::ate);
    auto failed = [&]() -> std::optional<std::vector<std::uint8_t>> {
        problem = errno ? std::strerror(errno) : "the system gives no reason";
        return std::nullopt;
    };
    if (!f) return failed();
    const std::streamoff size = f.tellg();
    if (size < 0) return failed();
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    f.seekg(0);
    if (size > 0 && !f.read(reinterpret_cast<char*>(bytes.data()), size)) return failed();
    return bytes;
}

} // namespace

DirectoryBundle::DirectoryBundle(std::filesystem::path folder) : folder_(std::move(folder)) {}

std::optional<std::vector<std::uint8_t>> DirectoryBundle::read(std::string_view fileName) const {
    const auto e = entryNamed(folder_, fileName);
    if (!e) return std::nullopt;
    std::string ignored;
    return readEntry(*e, ignored);
}

std::string DirectoryBundle::problem(std::string_view fileName) const {
    const auto e = entryNamed(folder_, fileName);
    if (!e) return {};
    std::string why;
    return readEntry(*e, why) ? std::string() : why;
}

std::string DirectoryBundle::where() const { return folder_.string(); }

} // namespace calcu::roms
