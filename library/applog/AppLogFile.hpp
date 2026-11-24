#pragma once
#include "applog/AppLog.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>

// The app log as a file: every line appended and flushed at once, so an
// end of the app loses none. At open, a file over `rotateAt` bytes becomes
// <name>.1.log (one such older file is kept) and the new run starts a fresh
// file; every run begins with its header line. A plain writer: it writes
// every line it is given; the host's log (shell::HostAppLog) leaves out what
// is above its level and holds the lock of a log written from several threads.

namespace calcu::applog {

class AppLogFile final {
public:
    static constexpr std::uintmax_t kRotateAt = 1024 * 1024;

    // `path`: UTF-8; its folders are made. `header`: the run's first line
    // (headerLine()), written at info.
    // `mirror`: also takes every line written (the platform's console); may be empty.
    AppLogFile(std::string path, std::string_view header,
               std::function<void(AppLogLevel, const std::string& line)> mirror = {},
               std::uintmax_t rotateAt = kRotateAt);
    AppLogFile(const AppLogFile&) = delete;
    AppLogFile& operator=(const AppLogFile&) = delete;

    void write(AppLogLevel, std::string_view text);

    const std::string& path() const { return path_; }
    // Why the file cannot be written ("" while it can): the lines then go
    // to the mirror only.
    const std::string& problem() const { return problem_; }

private:
    void append(AppLogLevel, const std::string& line);
    std::string path_;
    std::string problem_;
    std::ofstream out_;
    std::function<void(AppLogLevel, const std::string&)> mirror_;
};

} // namespace calcu::applog
