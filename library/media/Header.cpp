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

bool readHeader(const formats::YamlNode& root, const HeaderRules& rules, Header* out, std::string* problem) {
    *out = Header{};
    out->formatVersion = 0;
    const std::string notThis = "not a " + rules.what + " file (expected 'format: " + rules.format + "')";
    const formats::YamlNode* format = root.find("format");   // nullptr when root is no mapping
    if (!format || !format->asString(&out->format, problem) || out->format != rules.format) {
        *problem = notThis;
        return false;
    }
    long version = 0;
    const bool versionRead =
        formats::requireFormatVersion(root, rules.formatVersion, rules.what.c_str(), problem, &version);
    out->formatVersion = static_cast<int>(version);
    if (!versionRead) return false;
    if (!rules.nameKey.empty()) {
        out->nameKey = rules.nameKey;
        const formats::YamlNode* name = root.find(rules.nameKey);
        if (!name) {
            *problem = "missing '" + rules.nameKey + "'";
            return false;
        }
        if (!name->asString(&out->name, problem)) return false;
        if (out->name.empty()) {
            *problem = "empty '" + rules.nameKey + "'";
            return false;
        }
    }
    if (const formats::YamlNode* d = root.find("description"); d && !d->asString(&out->description, problem)) return false;
    if (const formats::YamlNode* t = root.find("template"); t && !t->asBool(&out->isTemplate, problem)) return false;
    if (const formats::YamlNode* s = root.find("saved"); s && !s->asString(&out->savedAt, problem)) return false;
    return true;
}

} // namespace calcu::media
