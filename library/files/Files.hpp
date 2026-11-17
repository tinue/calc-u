#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Paths as UTF-8 text and whole files read at once (Part E simplification
// E-7; LIB-1 design §2a, the area `files`: the standard library only, any
// area may include it).
//
// The library and the products keep every path as a UTF-8 std::string (what
// QString::toStdString and a browser give). std::filesystem::path(std::string)
// reads the string in the ANSI code page on Windows, so they convert through
// these, via char8_t (C++20: u8path is deprecated, u8string() returns
// std::u8string).

namespace calcu::files {

inline std::filesystem::path toPath(std::string_view utf8) {
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

// The path as UTF-8 text with '/' between its parts on every host.
inline std::string fromPath(const std::filesystem::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

// The path as UTF-8 text in the host's own form ('\' on Windows).
inline std::string fromPathNative(const std::filesystem::path& p) {
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

// The whole file, or nullopt when it cannot be opened (missing, no access)
// or reading it fails. Neither checks that the path is a regular file: a
// caller that refuses a folder asks std::filesystem::is_regular_file first.
inline std::optional<std::string> readText(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return text;
}

inline std::optional<std::vector<std::uint8_t>> readBytes(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return bytes;
}

} // namespace calcu::files
