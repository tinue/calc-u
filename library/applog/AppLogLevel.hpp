#pragma once
#include <optional>
#include <string_view>

// The app log's levels (the area `applog`: the standard library only). The
// app log tells how the app ran; what the calculator ran is the session's
// `log`, which has levels of its own.
//   error  the app, a session or the Web worker ends or cannot start
//   warn   something failed and the app goes on; the line has the cause
//   info   what the app did for the user (saved, opened, started, ended)
//   debug  the app's own detail
// A log at a level keeps that level and the ones above it in this list.

namespace calcu::applog {

enum class AppLogLevel { Error = 0, Warn = 1, Info = 2, Debug = 3 };

std::string_view appLogLevelName(AppLogLevel);   // "error", "warn", "info", "debug"
std::optional<AppLogLevel> appLogLevelNamed(std::string_view);

// The level a log starts at: warn in a release build, info in a
// development build (CALCU_DEV_BUILD).
AppLogLevel defaultAppLogLevel();

// The level a setting names: a level's name, or `default` (and anything
// else) for defaultAppLogLevel().
AppLogLevel appLogLevelOfSetting(std::string_view);

} // namespace calcu::applog
