#include "settings/PlatformBase.hpp"

namespace calcu::settings {

std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    const char last = base.back();
    if (last == '/' || last == '\\') return base + name;
    const bool windows = base.find('\\') != std::string::npos && base.find('/') == std::string::npos;
    return base + (windows ? '\\' : '/') + name;
}

std::string settingsBaseFor(HostOs os, const EnvLookup& env, const std::string& home,
                            const std::string& passedFolder) {
    const auto lookup = [&](const char* name) -> std::optional<std::string> {
        if (!env) return std::nullopt;
        std::optional<std::string> v = env(name);
        if (v && v->empty()) return std::nullopt;
        return v;
    };
    switch (os) {
    case HostOs::MacOS:
    case HostOs::IOS:
        if (!passedFolder.empty()) return passedFolder;
        return joinPath(joinPath(home, "Library"), "Application Support");
    case HostOs::Windows:
        if (auto appData = lookup("APPDATA")) return *appData;
        return joinPath(joinPath(home, "AppData"), "Roaming");
    case HostOs::Linux:
        if (auto xdg = lookup("XDG_CONFIG_HOME")) return *xdg;
        return joinPath(home, ".config");
    case HostOs::Web:
        return passedFolder;
    }
    return passedFolder;
}

std::string appLogFilePathFor(HostOs os, const EnvLookup& env, const std::string& home, const std::string& productName,
                              const std::string& passedFolder) {
    const auto lookup = [&](const char* name) -> std::optional<std::string> {
        if (!env) return std::nullopt;
        std::optional<std::string> v = env(name);
        if (v && v->empty()) return std::nullopt;
        return v;
    };
    std::string folder;
    switch (os) {
    case HostOs::MacOS: folder = joinPath(joinPath(home, "Library"), "Logs"); break;
    case HostOs::IOS: folder = passedFolder; break;
    case HostOs::Windows:
        if (auto local = lookup("LOCALAPPDATA")) folder = *local;
        else folder = joinPath(joinPath(home, "AppData"), "Local");
        break;
    case HostOs::Linux:
        if (auto state = lookup("XDG_STATE_HOME")) folder = *state;
        else folder = joinPath(joinPath(home, ".local"), "state");
        break;
    case HostOs::Web: return {};
    }
    if (folder.empty()) return {};
    return joinPath(joinPath(folder, productName), productName + ".log");
}

std::string settingsFilePath(const ProductDecl& product, const HostDirs& dirs) {
    return joinPath(joinPath(dirs.settingsBase, product.productName), product.productName + ".ini");
}

} // namespace calcu::settings
