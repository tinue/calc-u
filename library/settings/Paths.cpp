#include "settings/Paths.hpp"
#include "files/Files.hpp"
#include "settings/PlatformBase.hpp"
#include <filesystem>
#include <fstream>
#include <random>
#include <utility>

namespace calcu::settings {

namespace fs = std::filesystem;

std::string expandHome(std::string_view value, const std::string& home) {
    if (value == "~") return home;
    if (value.size() >= 2 && value[0] == '~' && (value[1] == '/' || value[1] == '\\'))
        return joinPath(home, std::string(value.substr(2)));
    return std::string(value);
}

Paths::Paths(const SettingsFile& settings, HostDirs dirs) : settings_(&settings), dirs_(std::move(dirs)) {}

std::optional<std::string> Paths::folderValue(std::string_view section, std::string_view key) const {
    if (!settings_->product().find(section, key)) return std::nullopt;
    const std::string v = settings_->text(section, key);
    if (v.empty()) return std::nullopt;
    return expandHome(v, dirs_.home);
}

std::string Paths::settingsFile() const { return settingsFilePath(settings_->product(), dirs_); }

std::string Paths::saveFolder() const {
    if (dirs_.frontend != FrontendKind::Qt6) return dirs_.saveFolder;
    if (auto over = folderValue("storage", "save-folder")) return *over;
    return joinPath(dirs_.home, settings_->product().productName);
}

std::string Paths::cassetteFolder() const {
    if (auto over = folderValue("storage", "cassette-folder")) return *over;
    return joinPath(saveFolder(), "Cassettes");
}

std::optional<std::string> Paths::traceFolder() const {
    switch (dirs_.frontend) {
    case FrontendKind::Web: return std::nullopt;
    case FrontendKind::Qt6:
        if (auto over = folderValue("trace", "folder")) return over;
        return saveFolder();
    case FrontendKind::Swift: {
        const std::string location = settings_->text("trace", "location");
        if (location == "icloud") return saveFolder();
        if (location == "folder") {
            if (!dirs_.chosenTraceFolder.empty()) return dirs_.chosenTraceFolder;
            if (auto plain = folderValue("trace", "folder")) return plain;
        }
        return dirs_.applicationSupport;
    }
    case FrontendKind::Given:
        if (dirs_.traceFolder.empty()) return std::nullopt;
        return dirs_.traceFolder;
    }
    return std::nullopt;
}

std::optional<std::string> Paths::screenshotFolder() const {
    if (dirs_.frontend == FrontendKind::Web) return std::nullopt;
    if (dirs_.frontend == FrontendKind::Given)
        return dirs_.screenshotFolder.empty() ? std::nullopt : std::optional<std::string>(dirs_.screenshotFolder);
    return saveFolder();
}

std::optional<std::string> Paths::serialLinkFolder() const {
    if (dirs_.frontend != FrontendKind::Qt6) return std::nullopt;
    if (auto over = folderValue("serial", "link-folder")) return over;
    return saveFolder();
}

std::string Paths::saveFolderFile(std::string_view name) const { return joinPath(saveFolder(), std::string(name)); }

std::optional<std::string> Paths::folder(Folder which) const {
    switch (which) {
    case Folder::Save: return saveFolder();
    case Folder::Cassettes: return cassetteFolder();
    case Folder::Traces: return traceFolder();
    case Folder::Screenshots: return screenshotFolder();
    case Folder::SerialLinks: return serialLinkFolder();
    }
    return std::nullopt;
}

Paths::Ensured Paths::ensure(Folder which) const {
    const std::optional<std::string> where = folder(which);
    if (!where || where->empty()) return {false, {}, "this frontend has no such folder"};
    const fs::path p = files::toPath(*where);
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec || !fs::is_directory(p, ec))
        return {false, *where, "cannot create the folder " + *where + (ec ? ": " + ec.message() : std::string())};
    // Written once to see that it can be.
    std::random_device rd;
    const fs::path probe = p / (".calcu-write-check-" + std::to_string(rd()));
    bool writable = false;
    {
        std::ofstream out(probe, std::ios::binary);
        writable = static_cast<bool>(out << "x");
    }
    fs::remove(probe, ec);
    if (!writable) return {false, *where, "cannot write to the folder " + *where};
    return {true, *where, {}};
}

} // namespace calcu::settings
