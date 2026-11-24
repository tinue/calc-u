#include "settings/WindowPlace.hpp"
#include <charconv>
#include <string>
#include <system_error>

namespace calcu::settings {

namespace {

KeyDecl windowKey(std::string key, ValueKind kind, std::string defaultValue, std::string help) {
    KeyDecl d;
    d.section = "window";
    d.key = std::move(key);
    d.kind = kind;
    d.defaultValue = std::move(defaultValue);
    d.writtenByApp = true;
    d.help = std::move(help);
    return d;
}

std::string windowName(std::string_view prefix) {
    return prefix.empty() ? std::string("window") : std::string(prefix.substr(0, prefix.size() - 1)) + " window";
}

} // namespace

namespace family {

std::vector<KeyDecl> windowPlace(std::string_view prefix, long width, long height) {
    const std::string p(prefix), name = windowName(prefix);
    return {
        windowKey(p + "x", ValueKind::Integer, "", "The " + name + "'s left edge (written by the app)."),
        windowKey(p + "y", ValueKind::Integer, "", "The " + name + "'s top edge (written by the app)."),
        windowKey(p + "width", ValueKind::Integer, std::to_string(width), "The " + name + "'s width (written by the app)."),
        windowKey(p + "height", ValueKind::Integer, std::to_string(height), "The " + name + "'s height (written by the app)."),
    };
}

KeyDecl windowFlag(std::string_view key, std::string_view help) {
    return windowKey(std::string(key), ValueKind::Boolean, "false", std::string(help));
}

KeyDecl windowOptional(std::string_view key, std::string_view help) {
    return windowKey(std::string(key), ValueKind::Integer, "", std::string(help));
}

std::vector<KeyDecl> paperWindow() {
    std::vector<KeyDecl> keys{windowFlag(kPaperOwnWindowKey, "The printer's paper in a window of its own: true or false.")};
    for (KeyDecl& k : windowPlace(kPaperWindowPrefix, kPaperWindowWidth, kPaperWindowHeight))
        keys.push_back(std::move(k));
    return keys;
}

} // namespace family

std::optional<long> windowNumber(const SettingsFile& f, std::string_view key) {
    const std::string v = f.text("window", key);
    long out = 0;
    auto [p, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (v.empty() || ec != std::errc() || p != v.data() + v.size()) return std::nullopt;
    return out;
}

SettingsFile::WriteResult setWindowNumber(SettingsFile& f, std::string_view key, std::optional<long> v) {
    return f.set("window", key, v ? std::to_string(*v) : std::string());
}

WindowPlace windowPlace(const SettingsFile& f, std::string_view prefix) {
    const std::string p(prefix);
    WindowPlace w;
    w.x = windowNumber(f, p + "x");
    w.y = windowNumber(f, p + "y");
    // text() gives the declared default for a value that is not accepted.
    w.width = windowNumber(f, p + "width").value_or(0);
    w.height = windowNumber(f, p + "height").value_or(0);
    return w;
}

SettingsFile::WriteResult setWindowPlace(SettingsFile& f, std::string_view prefix, const WindowPlace& w) {
    const std::string p(prefix);
    SettingsFile::WriteResult r;
    auto set = [&](const std::string& key, std::optional<long> v) {
        SettingsFile::WriteResult one = setWindowNumber(f, key, v);
        if (!one.ok && r.ok) r = one;
    };
    set(p + "x", w.x);
    set(p + "y", w.y);
    set(p + "width", w.width);
    set(p + "height", w.height);
    return r;
}

} // namespace calcu::settings
