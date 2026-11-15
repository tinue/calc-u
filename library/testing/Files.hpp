#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// Whole-file reads and writes of the tests (library/testing/): empty when
// the file cannot be read. For the library's tests and both products'.

namespace calcu::testing {

inline std::string readText(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

inline std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Writes `text` (any bytes), making the folders on the way; false when it could not.
inline bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::error_code ec;
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
    return static_cast<bool>(out);
}

inline bool writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    return writeText(path, std::string(bytes.begin(), bytes.end()));
}

} // namespace calcu::testing
