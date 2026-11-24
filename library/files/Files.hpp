#pragma once
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Paths as UTF-8 text and whole files read at once (the area `files`: the
// standard library only, any area may include it).
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

// `name` without its folders: what follows its last '/' or '\'.
inline std::string plainFileName(std::string_view name) {
    const std::size_t slash = name.find_last_of("/\\");
    return std::string(slash == std::string_view::npos ? name : name.substr(slash + 1));
}

// The system's words for an errno value ("No such file or directory"):
// file streams report no reason of their own, the C library below them
// leaves it in errno.
inline std::string osReason(int err) {
    if (err == 0) return "the system gives no reason";
    return std::strerror(err);
}

// The whole file, or nullopt when it cannot be opened (missing, no access)
// or reading it fails; `problem`, when given, then takes the system's
// reason. Neither checks that the path is a regular file: a caller that
// refuses a folder asks std::filesystem::is_regular_file first.
template <class Whole>
std::optional<Whole> readWhole(const std::filesystem::path& p, std::string* problem) {
    errno = 0;
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        if (problem) *problem = osReason(errno);
        return std::nullopt;
    }
    Whole whole((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) {
        if (problem) *problem = osReason(errno);
        return std::nullopt;
    }
    return whole;
}

inline std::optional<std::string> readText(const std::filesystem::path& p, std::string* problem = nullptr) {
    return readWhole<std::string>(p, problem);
}

inline std::optional<std::vector<std::uint8_t>> readBytes(const std::filesystem::path& p,
                                                          std::string* problem = nullptr) {
    return readWhole<std::vector<std::uint8_t>>(p, problem);
}

} // namespace calcu::files
