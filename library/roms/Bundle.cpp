#include "roms/Bundle.hpp"
#include "files/Files.hpp"
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

// The bytes, or none with the system's reason in `problem` (if given).
std::optional<std::vector<std::uint8_t>> readEntry(const std::filesystem::directory_entry& e, std::string* problem) {
    std::error_code ec;
    if (!e.is_regular_file(ec)) {
        if (problem) *problem = ec ? ec.message() : "not a file";
        return std::nullopt;
    }
    return files::readWhole<std::vector<std::uint8_t>>(e.path(), problem);
}

} // namespace

DirectoryBundle::DirectoryBundle(std::filesystem::path folder) : folder_(std::move(folder)) {}

std::optional<std::vector<std::uint8_t>> DirectoryBundle::read(std::string_view fileName, std::string* problem) const {
    const auto e = entryNamed(folder_, fileName);
    if (!e) return std::nullopt;
    return readEntry(*e, problem);
}

std::string DirectoryBundle::where() const { return folder_.string(); }

} // namespace calcu::roms
