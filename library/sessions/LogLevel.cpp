#include "sessions/LogLevel.hpp"

namespace calcu::sessions {

std::string_view logLevelName(LogLevel l) {
    switch (l) {
    case LogLevel::Off: return "off";
    case LogLevel::Info: return "info";
    case LogLevel::Debug: return "debug";
    }
    return "off";
}

std::optional<LogLevel> logLevelNamed(std::string_view n) {
    if (n == "off") return LogLevel::Off;
    if (n == "info") return LogLevel::Info;
    if (n == "debug") return LogLevel::Debug;
    return std::nullopt;
}

} // namespace calcu::sessions
