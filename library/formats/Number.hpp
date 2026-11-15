#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

// A number of the file formats (docs/formats/README.md §1; LIB-6 D-4): `&`,
// `0x` or `$` makes it hex, without one it is decimal; a leading `-` for a
// decimal. The whole value is the number: anything after it is an error.
// The module definition keeps YamlNode::asInt (decimal or `0x` only).

namespace calcu::formats {

// The value, or std::nullopt when the text is no number of that form or does
// not fit 64 bits.
std::optional<std::int64_t> parseNumber(std::string_view text);

} // namespace calcu::formats
