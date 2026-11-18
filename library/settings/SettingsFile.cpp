#include "settings/SettingsFile.hpp"
#include "settings/AtomicFile.hpp"
#include <algorithm>
#include <charconv>
#include <system_error>

namespace calcu::settings {

namespace {

IniDocument defaultDocument(const ProductDecl& product) {
    ParseResult r = IniDocument::parse(SettingsFile::defaultText(product));
    return r.doc ? std::move(*r.doc) : IniDocument{};
}

// Read when the parsed file has the current version, else why not.
Outcome versionOutcome(const IniDocument& doc) {
    const auto v = doc.get("", kFormatVersionKey);
    if (!v) return Outcome::NoVersion;
    if (*v != std::to_string(kFormatVersion)) return Outcome::OtherVersion;
    return Outcome::Read;
}

std::optional<long> toLong(std::string_view v) {
    long out = 0;
    const char* end = v.data() + v.size();
    auto [p, ec] = std::from_chars(v.data(), end, out);
    if (v.empty() || ec != std::errc() || p != end) return std::nullopt;
    return out;
}

} // namespace

std::string SettingsFile::defaultText(const ProductDecl& product) {
    std::string out = std::string(kFormatVersionKey) + " = " + std::to_string(kFormatVersion) + "\n";
    std::vector<std::string> sections;
    for (const KeyDecl& k : product.keys)
        if (std::find(sections.begin(), sections.end(), k.section) == sections.end()) sections.push_back(k.section);
    for (const std::string& s : sections) {
        out += "\n[" + s + "]\n";
        for (const KeyDecl& k : product.keys) {
            if (k.section != s) continue;
            if (!k.help.empty()) out += "# " + k.help + "\n";
            out += k.defaultValue.empty() ? k.key + " =\n" : k.key + " = " + k.defaultValue + "\n";
        }
    }
    return out;
}

SettingsFile SettingsFile::defaultsOnly(const ProductDecl& product) {
    SettingsFile f;
    f.product_ = &product;
    f.outcome_ = Outcome::DefaultsOnly;
    f.doc_ = defaultDocument(product);
    return f;
}

SettingsFile SettingsFile::open(const ProductDecl& product, std::string path) {
    SettingsFile f;
    f.product_ = &product;
    f.path_ = std::move(path);
    f.doc_ = defaultDocument(product);
    const std::optional<std::string> bytes = readWholeFile(f.path_);
    if (!bytes) {
        if (fileExists(f.path_)) {
            f.outcome_ = Outcome::Unreadable;
            f.problem_ = f.path_ + " cannot be read; the settings have their defaults and are not saved in this run";
            return f;
        }
        // The first start: written at once, with every key at its default.
        f.outcome_ = Outcome::Created;
        f.writable_ = true;
        const FileWrite w = writeFileAtomically(f.path_, f.doc_.text());
        if (!w.ok) f.problem_ = w.problem;
        return f;
    }
    ParseResult parsed = IniDocument::parse(*bytes);
    if (!parsed.doc) {
        f.outcome_ = Outcome::Unreadable;
        f.problem_ = f.path_ + " is not a settings file (" + parsed.problem +
                     "); it is left as it is, the settings have their defaults and are not saved in this run";
        return f;
    }
    f.outcome_ = versionOutcome(*parsed.doc);
    if (f.outcome_ == Outcome::NoVersion) {
        f.problem_ = f.path_ + " has no " + kFormatVersionKey +
                     "; it is not read, the settings have their defaults and are not saved in this run";
        return f;
    }
    if (f.outcome_ == Outcome::OtherVersion) {
        f.problem_ = f.path_ + " has " + kFormatVersionKey + " " + parsed.doc->get("", kFormatVersionKey).value_or("") +
                     ", this release reads " + std::to_string(kFormatVersion) +
                     "; it is not read, the settings have their defaults and are not saved in this run";
        return f;
    }
    f.doc_ = std::move(*parsed.doc);
    f.writable_ = true;
    return f;
}

std::optional<std::string> SettingsFile::raw(std::string_view section, std::string_view key) const {
    return doc_.get(section, key);
}

std::string SettingsFile::text(std::string_view section, std::string_view key) const {
    const std::optional<std::string> value = doc_.get(section, key);
    const KeyDecl* decl = product_->find(section, key);
    if (!decl) return value.value_or("");
    if (value && accepts(*product_, *decl, *value)) return *value;
    return decl->defaultValue;
}

bool SettingsFile::boolean(std::string_view section, std::string_view key) const {
    return text(section, key) == "true";
}

long SettingsFile::integer(std::string_view section, std::string_view key) const {
    if (const auto n = toLong(text(section, key))) return *n;
    return 0;
}

SettingsFile::WriteResult SettingsFile::set(std::string_view section, std::string_view key, std::string_view value) {
    if (doc_.get(section, key) == std::optional<std::string>(std::string(value))) return {};
    doc_.set(section, key, value);
    if (outcome_ == Outcome::DefaultsOnly) return {};
    if (!writable_) return {false, "not saved: " + problem_};
    const std::pair<std::string, std::string> id{lowerAscii(section), lowerAscii(key)};
    if (std::find(pending_.begin(), pending_.end(), id) == pending_.end()) pending_.push_back(id);
    return writePending();
}

// Merge on write: the file on disk takes this run's pending changes and keeps
// everything else, also another instance's keys.
SettingsFile::WriteResult SettingsFile::writePending() {
    IniDocument disk = doc_;  // the file is gone: write it whole
    if (const std::optional<std::string> bytes = readWholeFile(path_)) {
        ParseResult parsed = IniDocument::parse(*bytes);
        if (!parsed.doc || versionOutcome(*parsed.doc) != Outcome::Read)
            return {false, "not saved: " + path_ + " was changed while the product runs and cannot be read now"};
        disk = std::move(*parsed.doc);
        for (const auto& [s, k] : pending_)
            if (const auto v = doc_.get(s, k)) disk.set(s, k, *v);
    }
    const FileWrite w = writeFileAtomically(path_, disk.text());
    if (!w.ok) return {false, w.problem};
    pending_.clear();
    return {};
}

} // namespace calcu::settings
