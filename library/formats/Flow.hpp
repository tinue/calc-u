#pragma once
#include "formats/Yaml.hpp"
#include <string>
#include <string_view>

// The value of a preset step or of a harness command (LIB-11 design §3.2,
// D-3; AC-22): a scalar, a flow mapping or a flow sequence by
// docs/formats/README.md §1, on one line. `floppy { name: Disk2, side: 1 }`
// as a command takes the text after its name; the step `- floppy: { … }`
// the text after its colon; both read it here.

namespace calcu::formats {

// False with `error` set when the text is no value: an unterminated or
// malformed flow collection, a mapping entry without a colon, a tab, a
// line break. An empty text is the null value.
bool parseValue(std::string_view text, YamlNode* out, std::string* error);

// The text of a scalar node without its quotes (as YamlNode::asString).
std::string scalarText(const YamlNode&);

} // namespace calcu::formats
