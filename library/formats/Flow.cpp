#include "formats/Flow.hpp"

namespace calcu::formats {

bool parseValue(std::string_view text, YamlNode* out, std::string* error) {
    const std::string s(text);
    if (s.find('\t') != std::string::npos) {
        *error = "a tab is not allowed";
        return false;
    }
    if (s.find('\n') != std::string::npos || s.find('\r') != std::string::npos) {
        *error = "a value is one line";
        return false;
    }
    *out = YamlNode{};
    if (!yaml_detail::parseFlow(s, out, error, 1)) {
        // The reader's messages carry a line number; a command has one line.
        const std::string prefix = "line 1: ";
        if (error->rfind(prefix, 0) == 0) error->erase(0, prefix.size());
        return false;
    }
    return true;
}

std::string scalarText(const YamlNode& n) {
    std::string out, error;
    if (!n.asString(&out, &error)) return n.scalar;
    return out;
}

} // namespace calcu::formats
