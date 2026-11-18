#pragma once
#include "Files.hpp"
#include "TempFolder.hpp"
#include "settings/Declaration.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

// A product declared for the settings tests: `Test-Product`, models `Model-A`
// and `Model-B`, fallback `Model-B`, the family keys and one key of each
// other kind; the temporary folder and the file helpers of library/testing/.

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
            family::cassetteFolder(),
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

using TempDir = calcu::testing::TempFolder;

inline void writeText(const std::string& path, const std::string& text) { calcu::testing::writeText(path, text); }
using calcu::testing::readText;

inline bool exists(const std::string& path) { return std::filesystem::exists(path); }

} // namespace calcu::settings::test
