#pragma once
#include "settings/Declaration.hpp"
#include "settings/HostDirs.hpp"
#include <functional>
#include <optional>
#include <string>

// The one place of the settings file on each platform: a pure function of
// what the caller passes. The library reads no environment variable and calls
// no platform API; the frontend hands in a lookup and the folders it found.

namespace calcu::settings {

enum class HostOs { MacOS, IOS, Windows, Linux, Web };

using EnvLookup = std::function<std::optional<std::string>(const char* name)>;

// The settings base:
//   macOS, iOS  `passedFolder`, the app's Application Support folder
//               (inside the sandbox container for a sandboxed app), or
//               <home>/Library/Application Support when none is passed;
//   Windows     %APPDATA% (<home>\AppData\Roaming when it is not set);
//   Linux       $XDG_CONFIG_HOME, or <home>/.config when it is not set or empty;
//   Web         `passedFolder`, inside the file system the Web frontend passes.
std::string settingsBaseFor(HostOs os, const EnvLookup& env, const std::string& home,
                            const std::string& passedFolder);

// <settingsBase>/<Product>/<Product>.ini, the product's name as written.
// No other place is searched.
std::string settingsFilePath(const ProductDecl& product, const HostDirs& dirs);

// `base` and `name` joined with the separator `base` already uses ('\'
// for a path that has '\' and no '/', else '/').
std::string joinPath(const std::string& base, const std::string& name);

} // namespace calcu::settings
