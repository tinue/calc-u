#include "settings/Declaration.hpp"
#include <algorithm>
#include <charconv>
#include <system_error>
#include <set>
#include <utility>

namespace calcu::settings {

std::string lowerAscii(std::string_view s) {
    std::string out(s);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return out;
}

namespace {

bool isKebabCase(std::string_view name) {
    if (name.empty() || name.front() == '-' || name.back() == '-') return false;
    char prev = 0;
    for (char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!ok || (c == '-' && prev == '-')) return false;
        prev = c;
    }
    return true;
}

} // namespace

const KeyDecl* ProductDecl::find(std::string_view section, std::string_view key) const {
    const std::string s = lowerAscii(section), k = lowerAscii(key);
    for (const KeyDecl& d : keys)
        if (lowerAscii(d.section) == s && lowerAscii(d.key) == k) return &d;
    return nullptr;
}

bool ProductDecl::isModel(std::string_view name) const {
    return std::find(models.begin(), models.end(), name) != models.end();
}

namespace {

bool isAbsolutePath(std::string_view v) {
    if (v.empty()) return false;
    if (v[0] == '/' || v[0] == '\\') return true;                          // POSIX, \\server\share
    if (v == "~" || v.rfind("~/", 0) == 0 || v.rfind("~\\", 0) == 0) return true;  // the home folder
    return v.size() >= 3 && ((v[0] >= 'A' && v[0] <= 'Z') || (v[0] >= 'a' && v[0] <= 'z')) && v[1] == ':' &&
           (v[2] == '\\' || v[2] == '/');                                  // C:\ or C:/
}

std::optional<long> parseLong(std::string_view v) {
    long out = 0;
    const char* end = v.data() + v.size();
    auto [p, ec] = std::from_chars(v.data(), end, out);
    if (ec != std::errc() || p != end || v.empty()) return std::nullopt;
    return out;
}

KeyDecl key(std::string section, std::string name, ValueKind kind, std::string def, std::string help) {
    KeyDecl d;
    d.section = std::move(section);
    d.key = std::move(name);
    d.kind = kind;
    d.defaultValue = std::move(def);
    d.help = std::move(help);
    return d;
}

} // namespace

bool accepts(const ProductDecl& product, const KeyDecl& decl, std::string_view value) {
    switch (decl.kind) {
    case ValueKind::Text: return true;
    case ValueKind::Path: return value.empty() || isAbsolutePath(value);
    case ValueKind::Boolean: return value == "true" || value == "false";
    case ValueKind::Integer: {
        const auto n = parseLong(value);
        return n && (!decl.min || *n >= *decl.min) && (!decl.max || *n <= *decl.max);
    }
    case ValueKind::Choice:
        return std::find(decl.choices.begin(), decl.choices.end(), value) != decl.choices.end();
    case ValueKind::ModelOrLast: return value == "last" || product.isModel(value);
    case ValueKind::ModelName: return value.empty() || product.isModel(value);
    }
    return false;
}

std::vector<std::string> checkDeclaration(const ProductDecl& product) {
    std::vector<std::string> problems;
    std::set<std::pair<std::string, std::string>> seen;
    for (const KeyDecl& d : product.keys) {
        const std::string where = "[" + d.section + "] " + d.key;
        if (!isKebabCase(d.section)) problems.push_back(where + ": the section is not kebab-case");
        if (d.section == "preset") {
            if (!product.isModel(d.key)) problems.push_back(where + ": a [preset] key is a model of the product");
        } else if (!isKebabCase(d.key)) {
            problems.push_back(where + ": the key is not kebab-case");
        }
        if (!seen.insert({lowerAscii(d.section), lowerAscii(d.key)}).second)
            problems.push_back(where + ": declared twice");
        if (!d.defaultValue.empty() && !accepts(product, d, d.defaultValue))
            problems.push_back(where + ": the key does not accept its default");
        if (!d.firstValue.empty() && !accepts(product, d, d.firstValue))
            problems.push_back(where + ": the key does not accept its first value");
    }
    return problems;
}

namespace family {

KeyDecl startupModel() {
    return key("startup", "model", ValueKind::ModelOrLast, "last",
               "The model at start: last (the model used last) or a model's name.");
}
KeyDecl startupLastModel() {
    KeyDecl d = key("startup", "last-model", ValueKind::ModelName, "", "The model used last (written by the app).");
    d.writtenByApp = true;
    return d;
}
KeyDecl presetFor(const std::string& model, const std::string& firstPreset) {
    KeyDecl d = key("preset", model, ValueKind::Text, "", "The startup preset of the " + model + "; empty: none.");
    d.firstValue = firstPreset;
    return d;
}
KeyDecl saveFolder() {
    return key("storage", "save-folder", ValueKind::Path, "",
               "The save folder; empty: the product's folder in the home folder (Qt6 only).");
}
KeyDecl cassetteFolder() {
    return key("storage", "cassette-folder", ValueKind::Path, "", "The folder for cassettes; empty: Cassettes in the save folder.");
}
KeyDecl traceFolder() {
    return key("trace", "folder", ValueKind::Path, "", "The folder for traces; empty: the save folder.");
}
KeyDecl traceMaxFileSizeMb() {
    return key("trace", "max-file-size-mb", ValueKind::Integer, "50", "The largest trace file in MiB; zero or less: 50.");
}
KeyDecl traceLocation() {
    KeyDecl d = key("trace", "location", ValueKind::Choice, "app-support",
                    "Where traces go: app-support or icloud (Swift only).");
    d.choices = {"app-support", "icloud"};
    return d;
}
KeyDecl serialLinkFolder() {
    return key("serial", "link-folder", ValueKind::Path, "", "The folder for serial links; empty: the save folder.");
}
KeyDecl openFolder(const std::string& kind) {
    KeyDecl d = key("folders", kind, ValueKind::Text, "",
                    "Where the dialog for " + kind + " starts: the folder last used, or fixed:<folder>.");
    d.writtenByApp = true;
    return d;
}
KeyDecl windowNumber(const std::string& name, long defaultValue) {
    KeyDecl d = key("window", name, ValueKind::Integer, std::to_string(defaultValue),
                    "The window's " + name + " (written by the app).");
    d.writtenByApp = true;
    return d;
}

} // namespace family

} // namespace calcu::settings
