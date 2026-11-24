#pragma once
#include "applog/AppLogLevel.hpp"
#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

// Where the app log goes. A host makes one (a file, the console, a test's
// memory) and hands it to everything that logs; write() leaves out a line
// above the log's level, so a caller never asks the level first. The level
// is atomic so a host may change it from another thread than the one that
// writes; a sink that is written from several threads guards put() itself.

namespace calcu::applog {

class AppLog {
public:
    explicit AppLog(AppLogLevel level = defaultAppLogLevel()) : level_(level) {}
    virtual ~AppLog() = default;
    AppLog(const AppLog&) = delete;
    AppLog& operator=(const AppLog&) = delete;

    void write(AppLogLevel at, std::string_view text) {
        if (static_cast<int>(at) <= static_cast<int>(level())) put(at, text);
    }
    void error(std::string_view text) { write(AppLogLevel::Error, text); }
    void warn(std::string_view text) { write(AppLogLevel::Warn, text); }
    void info(std::string_view text) { write(AppLogLevel::Info, text); }
    void debug(std::string_view text) { write(AppLogLevel::Debug, text); }

    AppLogLevel level() const { return level_.load(std::memory_order_relaxed); }
    void setLevel(AppLogLevel l) { level_.store(l, std::memory_order_relaxed); }

protected:
    virtual void put(AppLogLevel, std::string_view text) = 0;

private:
    std::atomic<AppLogLevel> level_;
};

// Takes nothing: for a session or a part that is given no log.
class NullAppLog final : public AppLog {
protected:
    void put(AppLogLevel, std::string_view) override {}
};

// Keeps every line it takes (the tests; a host that shows them later).
class MemoryAppLog final : public AppLog {
public:
    struct Line {
        AppLogLevel level;
        std::string text;
    };
    using AppLog::AppLog;
    const std::vector<Line>& lines() const { return lines_; }
    bool has(AppLogLevel l, std::string_view part) const {
        for (const Line& line : lines_)
            if (line.level == l && line.text.find(part) != std::string::npos) return true;
        return false;
    }
    void clear() { lines_.clear(); }

protected:
    void put(AppLogLevel l, std::string_view text) override { lines_.push_back(Line{l, std::string(text)}); }

private:
    std::vector<Line> lines_;
};

// The shared app log of a process that has none of its own: a NullAppLog.
AppLog& nullAppLog();

// One line as the files hold it: `2026-11-24T10:15:03.123 WARN  text`, in
// local time; the lines of a text with line breaks after the first are
// indented by two spaces, so every line of the file starts a record or
// continues one.
std::string formatLine(std::chrono::system_clock::time_point, AppLogLevel, std::string_view text);

} // namespace calcu::applog
