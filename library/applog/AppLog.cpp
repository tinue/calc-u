#include "applog/AppLog.hpp"
#include <cstdio>
#include <ctime>

namespace calcu::applog {

AppLog& nullAppLog() {
    static NullAppLog log;
    return log;
}

namespace {
std::tm localTm(std::time_t t) {
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
}

const char* paddedName(AppLogLevel l) {
    switch (l) {
    case AppLogLevel::Error: return "ERROR";
    case AppLogLevel::Warn: return "WARN ";
    case AppLogLevel::Info: return "INFO ";
    case AppLogLevel::Debug: return "DEBUG";
    }
    return "WARN ";
}
} // namespace

std::string formatLine(std::chrono::system_clock::time_point at, AppLogLevel level, std::string_view text) {
    using namespace std::chrono;
    const auto ms = duration_cast<milliseconds>(at.time_since_epoch()).count() % 1000;
    const std::tm tm = localTm(system_clock::to_time_t(at));
    char stamp[32];
    std::snprintf(stamp, sizeof stamp, "%04d-%02d-%02dT%02d:%02d:%02d.%03d", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms < 0 ? ms + 1000 : ms));
    std::string out = std::string(stamp) + ' ' + paddedName(level) + ' ';
    for (char c : text) {
        if (c == '\r') continue;
        out += c;
        if (c == '\n') out += "  ";
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '\n')) out.pop_back();
    return out;
}

std::string headerLine(std::string_view product, std::string_view version, std::string_view build,
                       const std::vector<std::pair<std::string, std::string>>& rows) {
    std::string out = std::string(product) + ' ' + std::string(version) + " (" + std::string(build) + ')';
    for (const auto& [label, value] : rows)
        if (!value.empty()) out += ", " + label + ": " + value;
    return out;
}

} // namespace calcu::applog
