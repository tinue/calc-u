#include "formats/Duration.hpp"

namespace calcu::formats {

std::optional<std::int64_t> parseDuration(std::string_view text, std::string* error) {
    std::string_view t = text;
    while (!t.empty() && t.front() == ' ') t.remove_prefix(1);
    while (!t.empty() && t.back() == ' ') t.remove_suffix(1);
    const std::string written(t);
    std::int64_t scale = 0;   // microseconds per unit
    if (t.size() > 2 && t.substr(t.size() - 2) == "ms") {
        scale = 1'000;
        t.remove_suffix(2);
    } else if (t.size() > 1 && t.back() == 's') {
        scale = 1'000'000;
        t.remove_suffix(1);
    } else {
        *error = "A duration needs a unit, as in 2s, 1.5s or 500ms: " + written + ".";
        return std::nullopt;
    }
    std::int64_t whole = 0, fraction = 0, fractionScale = 1;
    bool digits = false, point = false;
    for (char c : t) {
        if (c == '.' && !point) {
            point = true;
            continue;
        }
        if (c < '0' || c > '9') {
            *error = "Not a duration: " + written + ".";
            return std::nullopt;
        }
        digits = true;
        if (!point) {
            if (whole > 1'000'000'000'000LL) {
                *error = "The duration is too long: " + written + ".";
                return std::nullopt;
            }
            whole = whole * 10 + (c - '0');
        } else {
            if (fractionScale >= scale) {
                *error = "The duration is finer than a microsecond: " + written + ".";
                return std::nullopt;
            }
            fraction = fraction * 10 + (c - '0');
            fractionScale *= 10;
        }
    }
    if (!digits) {
        *error = "Not a duration: " + written + ".";
        return std::nullopt;
    }
    return whole * scale + fraction * (scale / fractionScale);
}

} // namespace calcu::formats
