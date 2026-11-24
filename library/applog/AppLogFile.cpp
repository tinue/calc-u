#include "applog/AppLogFile.hpp"
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace calcu::applog {

namespace {
namespace fs = std::filesystem;

// UTF-8 text as a path (std::filesystem::path(std::string) would read it in
// the ANSI code page on Windows).
fs::path utf8Path(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

// <folder>/<stem>.1<ext>: Calc-U-59.log → Calc-U-59.1.log
fs::path olderOf(const fs::path& p) {
    fs::path out = p;
    out.replace_filename(p.stem().u8string() + u8".1" + p.extension().u8string());
    return out;
}
} // namespace

AppLogFile::AppLogFile(std::string path, std::string_view header, AppLogLevel level,
                       std::function<void(AppLogLevel, const std::string&)> mirror, std::uintmax_t rotateAt)
    : AppLog(level), path_(std::move(path)), mirror_(std::move(mirror)) {
    const fs::path p = utf8Path(path_);
    std::error_code ec;
    if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec);
    const std::uintmax_t size = fs::file_size(p, ec);
    if (!ec && size > rotateAt) {
        std::error_code ignored;
        fs::remove(olderOf(p), ignored);
        fs::rename(p, olderOf(p), ignored);
    }
    errno = 0;
    out_.open(p, std::ios::binary | std::ios::app);
    if (!out_) {
        const int e = errno;
        problem_ = "The app log " + path_ + " cannot be written" + (e ? std::string(": ") + std::strerror(e) : "") + ".";
    }
    append(AppLogLevel::Info, formatLine(std::chrono::system_clock::now(), AppLogLevel::Info, header));
}

void AppLogFile::put(AppLogLevel l, std::string_view text) {
    append(l, formatLine(std::chrono::system_clock::now(), l, text));
}

void AppLogFile::append(AppLogLevel l, const std::string& line) {
    if (out_) {
        out_ << line << '\n';
        out_.flush();
    }
    if (mirror_) mirror_(l, line);
}

} // namespace calcu::applog
