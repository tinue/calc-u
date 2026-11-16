#pragma once
#include "presets/ProductRules.hpp"
#include <string>
#include <string_view>
#include <variant>

// The text of `expect:` and `not-expect:` (LIB-6 design §2, D-18; spec
// AC-24, AC-66, AC-67): a literal text, a literal that begins with `/`
// written `\/`, or a regular expression between slashes (ECMAScript),
// compiled when the preset is read. A product refuses a text its comparison
// cannot take by ModelRules::checkExpect() (AC-68).

namespace calcu::presets {

// The text, or the sentence why it is refused (empty; an expression that
// is not valid).
std::variant<ExpectText, std::string> parseExpectText(std::string_view written);

// Whether one of the lines of `text` contains the text, or matches the
// expression somewhere: a comparison a product may use (AC-48).
bool foundInLines(std::string_view text, const ExpectText&);

} // namespace calcu::presets
