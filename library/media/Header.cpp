#include "media/Header.hpp"

namespace calcu::media {

std::string quotedScalar(const std::string& s) {
    const char q = s.find('"') == std::string::npos ? '"' : '\'';
    return std::string(1, q) + s + q;
}

std::string headerText(const Header& h) {
    std::string out = "# " + h.comment + "\n";
    out += "format: " + h.format + "\n";
    out += "format-version: " + std::to_string(h.formatVersion) + "\n";
    if (!h.nameKey.empty()) out += h.nameKey + ": " + quotedScalar(h.name) + "\n";
    if (!h.description.empty()) out += "description: " + quotedScalar(h.description) + "\n";
    if (h.isTemplate) out += "template: true\n";
    if (!h.savedAt.empty()) out += "saved: " + h.savedAt + "\n";
    return out;
}

} // namespace calcu::media
