#include "presets/Expect.hpp"

namespace calcu::presets {

std::variant<ExpectText, std::string> parseExpectText(std::string_view written) {
    ExpectText out;
    out.written = std::string(written);
    if (written.empty()) return std::string("The text to compare is empty.");
    if (written.size() >= 2 && written.front() == '/' && written.back() == '/') {
        out.text = std::string(written.substr(1, written.size() - 2));
        try {
            out.regex.emplace(out.text, std::regex::ECMAScript);
        } catch (const std::regex_error& e) {
            return "Not a valid regular expression: " + out.written + " (" + e.what() + ").";
        }
        return out;
    }
    if (written.size() >= 2 && written[0] == '\\' && written[1] == '/') out.text = "/" + std::string(written.substr(2));
    else out.text = std::string(written);
    return out;
}

bool foundInLines(std::string_view text, const ExpectText& e) {
    std::size_t at = 0;
    while (at <= text.size()) {
        std::size_t end = text.find('\n', at);
        if (end == std::string_view::npos) end = text.size();
        const std::string line(text.substr(at, end - at));
        if (e.regex ? std::regex_search(line, *e.regex) : line.find(e.text) != std::string::npos) return true;
        at = end + 1;
    }
    return false;
}

} // namespace calcu::presets
