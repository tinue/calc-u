#include "formats/Number.hpp"
#include <limits>

namespace calcu::formats {

std::optional<std::int64_t> parseNumber(std::string_view t) {
    while (!t.empty() && t.front() == ' ') t.remove_prefix(1);
    while (!t.empty() && t.back() == ' ') t.remove_suffix(1);
    int base = 10;
    bool negative = false;
    if (!t.empty() && (t.front() == '&' || t.front() == '$')) {
        base = 16;
        t.remove_prefix(1);
    } else if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
        base = 16;
        t.remove_prefix(2);
    } else if (!t.empty() && t.front() == '-') {
        negative = true;
        t.remove_prefix(1);
    }
    if (t.empty()) return std::nullopt;
    std::uint64_t v = 0;
    const std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    for (char c : t) {
        int d = -1;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (base == 16 && c >= 'A' && c <= 'F') d = c - 'A' + 10;
        if (d < 0) return std::nullopt;
        if (v > (limit - static_cast<std::uint64_t>(d)) / static_cast<std::uint64_t>(base)) return std::nullopt;
        v = v * static_cast<std::uint64_t>(base) + static_cast<std::uint64_t>(d);
    }
    const auto s = static_cast<std::int64_t>(v);
    return negative ? -s : s;
}

} // namespace calcu::formats
