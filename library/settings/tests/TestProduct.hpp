#pragma once
#include "settings/Declaration.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

// A product declared for the settings tests (LIB-9 design §2): `Test-Product`,
// models `Model-A` and `Model-B`, fallback `Model-B`, the family keys and one
// key of each other kind. And a temporary folder that removes itself.

namespace calcu::settings::test {

inline KeyDecl testKey(std::string section, std::string key, ValueKind kind, std::string defaultValue,
                       std::vector<std::string> choices = {}) {
    KeyDecl d;
    d.section = std::move(section);
    d.key = std::move(key);
    d.kind = kind;
    d.defaultValue = std::move(defaultValue);
    d.choices = std::move(choices);
    return d;
}

inline const ProductDecl& testProduct() {
    static const ProductDecl decl = [] {
        ProductDecl d;
        d.productName = "Test-Product";
        d.models = {"Model-A", "Model-B"};
        d.fallbackModel = "Model-B";
        d.keys = {
            family::startupModel(),
            family::startupLastModel(),
            family::presetFor("Model-A"),
            family::presetFor("Model-B"),
            family::saveFolder(),
            family::tapeFolder(),
            family::traceFolder(),
            family::traceMaxFileSizeMb(),
            family::traceLocation(),
            family::serialLinkFolder(),
            family::openFolder("presets"),
            family::openFolder("programs"),
            family::windowNumber("width", 900),
        };
        KeyDecl flag = testKey("test", "flag", ValueKind::Boolean, "false");
        flag.help = "A boolean.";
        KeyDecl style = testKey("test", "style", ValueKind::Choice, "one", {"one", "two"});
        style.help = "A choice.";
        KeyDecl port = testKey("test", "port", ValueKind::Integer, "32168");
        port.min = 1024;
        port.max = 65535;
        port.help = "An integer in a range.";
        KeyDecl name = testKey("test", "name", ValueKind::Text, "plain");
        name.help = "A text.";
        d.keys.push_back(flag);
        d.keys.push_back(style);
        d.keys.push_back(port);
        d.keys.push_back(name);
        return d;
    }();
    return decl;
}

// A new, empty folder under the system's temporary folder, removed with
// everything in it when the object goes.
class TempDir {
public:
    TempDir() {
        static std::atomic<int> counter{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("calcu-settings-test-" + std::to_string(stamp) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::string str() const { return path_.generic_string(); }
    std::string operator/(const std::string& name) const { return (path_ / name).generic_string(); }

private:
    std::filesystem::path path_;
};

inline void writeText(const std::string& path, const std::string& text) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

inline std::string readText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

inline bool exists(const std::string& path) { return std::filesystem::exists(path); }

} // namespace calcu::settings::test
