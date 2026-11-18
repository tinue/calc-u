#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// A duration of a preset step or a harness command (docs/formats/preset.md
// §5.3): a number with a unit, `2s`, `1.5s`, `500ms`.
// The unit is required. Exact, in whole microseconds: no floating point, so
// two runs agree.

namespace calcu::formats {

// The duration in microseconds, or std::nullopt with `error` set: no unit,
// another unit, no number, a sign, finer than a microsecond, or too long.
std::optional<std::int64_t> parseDuration(std::string_view text, std::string* error);

} // namespace calcu::formats
