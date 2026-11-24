#include "applog/AppLogLevel.hpp"

namespace calcu::applog {

std::string_view appLogLevelName(AppLogLevel l) {
    switch (l) {
    case AppLogLevel::Error: return "error";
    case AppLogLevel::Warn: return "warn";
    case AppLogLevel::Info: return "info";
    case AppLogLevel::Debug: return "debug";
    }
    return "warn";
}

std::optional<AppLogLevel> appLogLevelNamed(std::string_view n) {
    if (n == "error") return AppLogLevel::Error;
    if (n == "warn") return AppLogLevel::Warn;
    if (n == "info") return AppLogLevel::Info;
    if (n == "debug") return AppLogLevel::Debug;
    return std::nullopt;
}

AppLogLevel defaultAppLogLevel() {
#ifdef CALCU_DEV_BUILD
    return AppLogLevel::Info;
#else
    return AppLogLevel::Warn;
#endif
}

AppLogLevel appLogLevelOfSetting(std::string_view v) {
    const auto named = appLogLevelNamed(v);
    return named ? *named : defaultAppLogLevel();
}

} // namespace calcu::applog
