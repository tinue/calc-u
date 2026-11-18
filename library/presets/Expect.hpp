#pragma once
#include "presets/ProductRules.hpp"
#include <string>
#include <string_view>
#include <variant>

// The text of `expect:` and `not-expect:`: a literal text, a literal that
// begins with `/` written `\/`, or a regular expression between slashes
// (ECMAScript), compiled when the preset is read. A product refuses a text
// its comparison cannot take by ModelRules::checkExpect().

namespace calcu::presets {

// The text, or the sentence why it is refused (empty; an expression that
// is not valid).
std::variant<ExpectText, std::string> parseExpectText(std::string_view written);

// Whether one of the lines of `text` contains the text, or matches the
// expression somewhere: a comparison a product may use.
bool foundInLines(std::string_view text, const ExpectText&);

} // namespace calcu::presets
