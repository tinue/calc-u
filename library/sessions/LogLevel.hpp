#pragma once
#include <optional>
#include <string_view>

// How much the session's log tells: Off keeps what the user asked for and
// what went wrong; Info adds how a preset is carried out; Debug adds a
// product's own detail (Calc-U-59: every write to the registers).

namespace calcu::sessions {

enum class LogLevel { Off = 0, Info = 1, Debug = 2 };

std::string_view logLevelName(LogLevel);                 // "off", "info", "debug"
std::optional<LogLevel> logLevelNamed(std::string_view);

} // namespace calcu::sessions
