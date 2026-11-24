#pragma once
#include "files/Files.hpp"
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

// The Web's file cache: a folder in the browser's kept storage that holds
// every file the page uploads or picks but a preset it loads once, and the
// startup presets chosen with "Choose…", flat by name (a newer file of the
// same name replaces the older), so that a preset chosen later without its
// files still finds them. The native hosts have none: they read the files
// where they lie.

namespace calcu::files {

// Writes the bytes as `folder`/<plain name>; the path, or nullopt with the
// reason in *problem.
inline std::optional<std::filesystem::path> keepInCache(const std::filesystem::path& folder, std::string_view name,
                                                        const std::uint8_t* data, std::size_t size,
                                                        std::string* problem = nullptr) {
    const std::string plain = plainFileName(name);
    if (plain.empty() || plain == "." || plain == "..") {
        if (problem) *problem = "the file has no name";
        return std::nullopt;
    }
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    if (ec) {
        if (problem) *problem = ec.message();
        return std::nullopt;
    }
    const std::filesystem::path p = folder / toPath(plain);
    errno = 0;
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!out) {
        if (problem) *problem = osReason(errno);
        return std::nullopt;
    }
    return p;
}

// Whether `path` lies in `folder` (both as UTF-8 text, '/' between parts).
inline bool liesIn(std::string_view path, std::string_view folder) {
    if (folder.empty() || path.size() <= folder.size() || path.substr(0, folder.size()) != folder) return false;
    return folder.back() == '/' || path[folder.size()] == '/';
}

} // namespace calcu::files
